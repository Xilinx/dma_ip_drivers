#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>


#include "platform_config.h"
#include "packet_handler.h"

// #define DEBUG_PRINT
#include "util.h"

// #define DEBUG_GPTP
#ifdef DEBUG_GPTP
#define printf_gptp(...) printf(__VA_ARGS__)
#else
#define printf_gptp(...) {}
#endif

#include "ethernet.h"
#include "gptp.h"

#define TICKS_SCALE 8 // 8ns tick / 125MHz
#define TICKS_PER_SEC (1000000000 / TICKS_SCALE)
#define SERVER_DECAY_TIMEOUT_TICK (1 * TICKS_PER_SEC)  // 1s

#define TIMESTAMP_ID_GPTP_RESP 1
#define TIMESTAMP_ID_GPTP_REQ 2

#define SYNTONIZE_SIZE 10

//XXX: These should be removed or changed
static const char myMAC[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
static PtpClockIdentity myClockId = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
static const uint8_t myPriority1 = 248;
static const uint8_t myPriority2 = 248;

// As a Master
static struct {
    uint16_t sequenceId;
} my_info = {
    .sequenceId = 0,
};

// As a slave
static struct {
    struct PtpPortIdentity portIdentity;
    uint8_t priority1;
    uint8_t priority2;
    sysclock_t last_sync;
    uint16_t sequenceId;
} current_gm;

static int has_master = 0;

static struct {
    uint16_t master_sequenceId;
    uint16_t slave_sequenceId;
    // These are ns
    timestamp_t t1; // Remote TX timestamp of the sync message
    timestamp_t t2; // Local RX timestamp of the sync message
    timestamp_t t3; // Local TX timestamp of the follow-up message
    timestamp_t t4; // Remote RX timestamp of the follow-up message

    uint64_t c1; // pdelay_resp.correction
    uint64_t c2; // pdelay_resp_fup.correction

    timestamp_t sync_tx; // fup.originTimestamp
    timestamp_t sync_rx; // sync rx timestamp
    timestamp_t sync_correction; // fup.correction

    // Calc results
    int64_t link_delay;
} current_status;

struct statistics {
    int num;
    double min;
    double max;
    double mean;
    double sum_sqr;
    double sum_diff_sqr;
} stats_offset, stats_freq, stats_delay;

// Lower resolution - increases throughput but increases RMS
// Higher resolution - decreases throughput but decreases RMS
// And IDK why > 0x200000 causes freezing the system
#define TIMESTAMP_MAG_RESOLUTION (0x200000)
static int64_t timestamp_mag_int = TICKS_SCALE;
static int64_t timestamp_mag_fraction = 0;
static uint64_t timestamp_offset = 0;

static int process_sync(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx);
static int process_followup(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx);
static int process_pdelay_req(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx);
static int process_pdelay_resp(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx);
static int process_pdelay_resp_fup(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx);

static void set_eth(struct ethernet_header* eth, const char* dmac);

static bool is_decayed(sysclock_t time);
static int is_new_master(struct PtpAnnounceHeader* hdr, sysclock_t time);
static int is_current_master(struct PtpHeader* hdr);
static void timestamp_to_ptp_time(struct PtpTimestamp* ptp_time, const timestamp_t timestamp);
static timestamp_t ptp_time_to_ns(struct PtpTimestamp* ptp_time);
static timestamp_t ptp_correction_to_ns(uint64_t correction);

static inline void adjust_timestamp_mag(double new_mag);
static void adjust_clock();
static void calculate_link_delay();
static void syntonize(uint64_t t1_sysclock, uint64_t t2_ns);

static void debug_timestamp(const char* name, timestamp_t timestamp);
static void debug_sysclock(const char* name, sysclock_t timestamp);
static void debug_ns64(const char* name, int64_t value);

static void add_statistics(struct statistics* stats, double value);
static bool get_statistics(struct gptp_statistics_result* result, struct statistics* stats);

void gptp_init() {
    // Use some random value;
    sysclock_t now = get_sys_count();

    myClockId[0] = myMAC[0];
    myClockId[1] = myMAC[1];
    myClockId[2] = myMAC[2];
    myClockId[3] = 0xff;
    myClockId[4] = 0xff;
    myClockId[5] = myMAC[3];
    myClockId[6] = myMAC[4];
    myClockId[7] = myMAC[5];

    memcpy(&current_gm.portIdentity.clockIdentity, myClockId, sizeof(myClockId));
    current_gm.portIdentity.portNumber = 1;
    current_gm.priority1 = myPriority1;
    current_gm.priority2 = myPriority2;
    current_gm.last_sync = now;
}

int process_gptp_packet(struct tsn_rx_buffer* rx) {
    struct tsn_tx_buffer* tx;
    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpHeader* rx_ptp = (struct PtpHeader*)ETH_PAYLOAD(rx_eth);

    switch (rx_ptp->messageType) {
    case PTP_MESSAGE_SYNC:
        printf_gptp("PTP_SYNC\n");
        return process_sync(rx, tx);
    case PTP_MESSAGE_FOLLOW_UP:
        printf_gptp("PTP_FOLLOW_UP\n");
        return process_followup(rx, tx);
    case PTP_MESSAGE_DELAY_REQ:
        printf_gptp("PTP_DELAY_REQ\n");
        // Ignore
        return 0;
    case PTP_MESSAGE_DELAY_RESP:
        printf_gptp("PTP_DELAY_RESP\n");
        // Ignore
        return 0;
    case PTP_MESSAGE_PDELAY_REQ:
        printf_gptp("PTP_PDELAY_REQ\n");
        return process_pdelay_req(rx, tx);
    case PTP_MESSAGE_PDELAY_RESP:
        printf_gptp("PTP_PDELAY_RESP\n");
        return process_pdelay_resp(rx, tx);
    case PTP_MESSAGE_PDELAY_RESP_FOLLOW_UP:
        printf_gptp("PTP_PDELAY_RESP_FOLLOW_UP\n");
        return process_pdelay_resp_fup(rx, tx);
    case PTP_MESSAGE_ANNOUNCE:
        printf_gptp("PTP_ANNOUNCE\n");

        // Check if new master have higher priority than me, if so, become slave
        if (!is_new_master((struct PtpAnnounceHeader*)rx_ptp, rx->metadata.timestamp)) {
            break;
        }
        has_master = 1;
        memcpy(&current_gm.portIdentity, &rx_ptp->sourcePortIdentity, sizeof(struct PtpPortIdentity));
        current_gm.priority1 = ((struct PtpAnnounceHeader*)rx_ptp)->grandmasterPriority1;
        current_gm.priority2 = ((struct PtpAnnounceHeader*)rx_ptp)->grandmasterPriority2;
        current_gm.last_sync = rx->metadata.timestamp;

        printf_gptp("New grand master found: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                rx_ptp->sourcePortIdentity.clockIdentity[0],
                rx_ptp->sourcePortIdentity.clockIdentity[1],
                rx_ptp->sourcePortIdentity.clockIdentity[2],
                rx_ptp->sourcePortIdentity.clockIdentity[3],
                rx_ptp->sourcePortIdentity.clockIdentity[4],
                rx_ptp->sourcePortIdentity.clockIdentity[5],
                rx_ptp->sourcePortIdentity.clockIdentity[6],
                rx_ptp->sourcePortIdentity.clockIdentity[7]);

        printf_gptp("Master priority: %d %d\n", current_gm.priority1, current_gm.priority2);
        debug_timestamp("Timestamp", current_gm.last_sync);
        return 0;
    default:
        printf_gptp("PTP_UNKNOWN %04x\n", rx_ptp->messageType);
        return 0;
    }

    return 0;
}

static int process_sync(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx) {
    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpSyncHeader* rx_ptp = (struct PtpSyncHeader*)ETH_PAYLOAD(rx_eth);
    // Don't use tx buffer

    if (!is_current_master(&rx_ptp->hdr)) {
        return 0;
    }
    current_gm.last_sync = rx->metadata.timestamp;

    current_status.sync_rx = gptp_get_rx_timestamp(rx);
    current_status.master_sequenceId = rx_ptp->hdr.sequenceId;

    return 0;
}

static int process_followup(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx) {
    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpFollowUpHeader* rx_ptp = (struct PtpFollowUpHeader*)ETH_PAYLOAD(rx_eth);
    // Don't use tx buffer

    if (!is_current_master(&rx_ptp->hdr) || rx_ptp->hdr.sequenceId != current_status.master_sequenceId) {
        return 0;
    }

    current_gm.last_sync = rx->metadata.timestamp;

    current_status.sync_tx = ptp_time_to_ns(&rx_ptp->preciseOriginTimestamp);
    current_status.sync_correction = ptp_correction_to_ns(rx_ptp->hdr.correctionField);

    adjust_clock();

    return 0;
}

static int process_pdelay_req(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx) {
    // Note: This function sends two packets.
    // So we use bare transmit_tsn_packet and return 0.
    // FIXME whenever

    tx = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (tx == NULL) {
        printf_gptp("Cannot get buffer to send PTP_PDELAY_RESP\n");
        return 0;
    }

    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpPdelayReqHeader* rx_ptp = (struct PtpPdelayReqHeader*)ETH_PAYLOAD(rx_eth);
    struct ethernet_header* tx_eth = (struct ethernet_header*)&tx->data;
    struct PtpPdelayRespHeader* tx_ptp = (struct PtpPdelayRespHeader*)ETH_PAYLOAD(tx_eth);

    set_eth(tx_eth, PTP_PEER_MAC);

    memcpy(&tx_ptp->hdr, &rx_ptp->hdr, sizeof(tx_ptp->hdr));
    tx_ptp->hdr.minorVersionPTP = 0x1;
    tx_ptp->hdr.messageType = PTP_MESSAGE_PDELAY_RESP;
    tx_ptp->hdr.messageLength = sizeof(*tx_ptp);
    tx_ptp->hdr.flags = PTP_FLAG_TWO_STEP;
    tx_ptp->hdr.correctionField = 0;
    tx_ptp->hdr.messageTypeSpecific = 0;
    memcpy(&tx_ptp->hdr.sourcePortIdentity.clockIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->hdr.sourcePortIdentity.portNumber = 1; // I have only one port
    // tx_ptp->hdr.sequenceId = rx_ptp->hdr.sequenceId;
    tx_ptp->hdr.control = PTP_CONTROL_ALL_OTHER;
    tx_ptp->hdr.logMeanMessageInterval = 127;

    timestamp_to_ptp_time(
        &tx_ptp->requestReceiptTimestamp,
        gptp_get_rx_timestamp(rx)
    );

    memcpy(&tx_ptp->requestingPortIdentity, &rx_ptp->hdr.sourcePortIdentity, sizeof(tx_ptp->requestingPortIdentity));

    tx->metadata.frame_length = sizeof(*tx_eth) + sizeof(*tx_ptp);
    tx->metadata.timestamp_id = TIMESTAMP_ID_GPTP_RESP;
    tx->metadata.vlan_tag = 0;

    transmit_tsn_packet(tx);
    timestamp_t pdelay_resp_tx_timestamp = gptp_get_tx_timestamp(TIMESTAMP_ID_GPTP_RESP);

    // Send pdelay_resp_fup packet

    struct tsn_tx_buffer* fup = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (fup == NULL) {
        printf_gptp("No tx buffer available to send pdelay_resp_fup\n");
        return 0;
    }

    struct ethernet_header* fup_eth = (struct ethernet_header*)&tx->data;
    struct PtpPdelayRespFollowUpHeader* fup_ptp = (struct PtpPdelayRespFollowUpHeader*)ETH_PAYLOAD(tx_eth);

    memcpy(fup_eth, tx_eth, sizeof(*fup_eth) + sizeof(*fup_ptp));

    fup_ptp->hdr.messageType = PTP_MESSAGE_PDELAY_RESP_FOLLOW_UP;
    fup_ptp->hdr.messageLength = sizeof(*fup_ptp);
    fup_ptp->hdr.flags = 0;
    fup_ptp->hdr.correctionField = 0;
    fup_ptp->hdr.messageTypeSpecific = 0;
    // fup_ptp->hdr.control = PTP_CONTROL_ALL_OTHER;
    // fup_ptp->hdr.logMeanMessageInterval = 127;

    timestamp_to_ptp_time(
        &fup_ptp->responseOriginTimestamp,
        pdelay_resp_tx_timestamp
    );

    // Already copied from pdelay_resp
    // memcpy(&fup_ptp->requestingPortIdentity, &rx_ptp->hdr.sourcePortIdentity, sizeof(fup_ptp->requestingPortIdentity));

    fup->metadata.frame_length = sizeof(*fup_eth) + sizeof(*fup_ptp);
    fup->metadata.timestamp_id = 0;
    fup->metadata.vlan_tag = 0;

    transmit_tsn_packet(fup);

    // As we send packets manually, return 0;
    return 0;
}

static int process_pdelay_resp(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx) {
    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpPdelayRespHeader* rx_ptp = (struct PtpPdelayRespHeader*)ETH_PAYLOAD(rx_eth);
    // Don't use tx buffer

    // Check valid response
    if (memcmp(rx_ptp->requestingPortIdentity.clockIdentity, myClockId, sizeof(PtpClockIdentity)) != 0 || rx_ptp->requestingPortIdentity.portNumber != 1) {
        printf_gptp("Invalid PdelayResp: clock id\n");
        return 0;
    }
    if (rx_ptp->hdr.sequenceId != my_info.sequenceId) {
        printf_gptp("Invalid PdelayResp: sequence id\n");
        return 0;
    }

    // Get previous pdelay req timestamp
    sysclock_t tx_ts_for_syntonize = get_tx_timestamp(TIMESTAMP_ID_GPTP_REQ);

    timestamp_t req_tx_timestamp = gptp_get_tx_timestamp(TIMESTAMP_ID_GPTP_REQ);
    timestamp_t resp_rx_timestamp = gptp_get_rx_timestamp(rx);

    current_status.t1 = req_tx_timestamp;
    current_status.t4 = resp_rx_timestamp;

    current_status.t2 = ptp_time_to_ns(&rx_ptp->requestReceiptTimestamp);

    current_status.c1 = ptp_correction_to_ns(rx_ptp->hdr.correctionField);

    if ((rx_ptp->hdr.flags & PTP_FLAG_TWO_STEP) != 0) {
        // Two step
        // Leave t3 for later, pdelay_resp_fup
    } else {
        // One step
        current_status.t2 = 0;
        current_status.t3 = 0;

        // TODO: implement
    }

    // Syntonize MUST be done AFTER saving current_status.something
    syntonize(tx_ts_for_syntonize, current_status.t2);

    return 0;
}

static int process_pdelay_resp_fup(struct tsn_rx_buffer* rx, struct tsn_tx_buffer* tx) {
    struct ethernet_header* rx_eth = (struct ethernet_header*)&rx->data;
    struct PtpPdelayRespFollowUpHeader* rx_ptp = (struct PtpPdelayRespFollowUpHeader*)ETH_PAYLOAD(rx_eth);
    // Don't use tx buffer

    // Check valid response
    if (memcmp(rx_ptp->requestingPortIdentity.clockIdentity, myClockId, sizeof(PtpClockIdentity)) != 0 || rx_ptp->requestingPortIdentity.portNumber != 1) {
        printf_gptp("Invalid PdelayRespFup: clock id\n");
        return 0;
    }
    if (rx_ptp->hdr.sequenceId != my_info.sequenceId) {
        printf_gptp("Invalid PdelayRespFup: sequence id\n");
        return 0;
    }

    current_status.t3 = ptp_time_to_ns(&rx_ptp->responseOriginTimestamp);
    current_status.c2 = ptp_correction_to_ns(rx_ptp->hdr.correctionField);

    // calculate link delay
    calculate_link_delay();
    return 0;
}

static void set_eth(struct ethernet_header* eth, const char* ptp_mac) {
    memcpy(eth->dmac, ptp_mac, 6);
    memcpy(eth->smac, myMAC, 6);
    eth->type = ETH_TYPE_PTPv2;
}

static bool is_decayed(sysclock_t time) {
    return (has_master == 0) || ((current_gm.last_sync + SERVER_DECAY_TIMEOUT_TICK) < time);
}

static int is_new_master(struct PtpAnnounceHeader* hdr, sysclock_t time) {
    if (is_decayed(time)) {
        current_gm.priority1 = myPriority1;
        current_gm.priority2 = myPriority2;
        memcpy(&current_gm.portIdentity.clockIdentity, myClockId, sizeof(myClockId));
        current_gm.portIdentity.portNumber = 1;
    }

    if (memcmp(&current_gm.portIdentity, &hdr->hdr.sourcePortIdentity, sizeof(current_gm.portIdentity)) == 0) {
        return 0;
    }
    if (current_gm.priority1 < hdr->grandmasterPriority1) {
        return 0;
    } else if (current_gm.priority1 > hdr->grandmasterPriority1) {
        return 1;
    }

    if (current_gm.priority2 < hdr->grandmasterPriority2) {
        return 0;
    } else if (current_gm.priority2 > hdr->grandmasterPriority2) {
        return 1;
    }

    // All checkable values are equal. Use our custom algorithm to tie breaker

    // Sequence ID
    if (hdr->hdr.sequenceId > current_gm.sequenceId) {
        return 1;
    } else if (hdr->hdr.sequenceId < current_gm.sequenceId) {
        return 0;
    }

    // Cannot determine which is best. Just use Clock ID
    if (memcmp(&current_gm.portIdentity, &hdr->hdr.sourcePortIdentity.clockIdentity, sizeof(current_gm.portIdentity)) < 0) {
        return 1;
    } else {
        return 0;
    }
}

int gptp_make_announce_packet(struct tsn_tx_buffer* tx) {
    sysclock_t now = get_sys_count();
    if (is_decayed(now)) {
        has_master = 0;
    } else {
        // Has master, Don't send announce
        return 0;
    }

    struct ethernet_header* tx_eth = (struct ethernet_header*)&tx->data;
    struct PtpAnnounceHeader* tx_ptp = (struct PtpAnnounceHeader*)ETH_PAYLOAD(tx_eth);

    set_eth(tx_eth, PTP_PEER_MAC);

    // Many of these are copied from linuxptp capture
    tx_ptp->hdr.transportSpecific = 0x01; // gPTP
    tx_ptp->hdr.messageType = PTP_MESSAGE_ANNOUNCE;
    tx_ptp->hdr.minorVersionPTP = 1;
    tx_ptp->hdr.versionPTP = 2;
    tx_ptp->hdr.messageLength = sizeof(struct PtpAnnounceHeader);
    tx_ptp->hdr.domainNumber = 0;
    tx_ptp->hdr.flags = PTP_FLAG_TIMESCALE;
    tx_ptp->hdr.correctionField = 0;
    memcpy(&tx_ptp->hdr.sourcePortIdentity.clockIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->hdr.sourcePortIdentity.portNumber = 1;
    tx_ptp->hdr.sequenceId = ++my_info.sequenceId;
    tx_ptp->hdr.control = PTP_CONTROL_ALL_OTHER;
    tx_ptp->hdr.logMeanMessageInterval = 0;
    tx_ptp->originTimestamp.seconds = 0;
    tx_ptp->originTimestamp.nanoseconds = 0;
    tx_ptp->currentUtcOffset = 37; // Will never change after Y2023
    tx_ptp->grandmasterPriority1 = myPriority1;
    tx_ptp->grandmasterClockQuality.clockClass = 248;
    tx_ptp->grandmasterClockQuality.clockAccuracy = 0xFE; // Unknown
    tx_ptp->grandmasterClockQuality.scaledLogVariance = 0xFFFF;
    tx_ptp->grandmasterPriority2 = myPriority2;
    memcpy(&tx_ptp->grandmasterIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->stepsRemoved = 0;
    tx_ptp->timeSource = 0xA0; // Internal oscillator

    tx->metadata.frame_length = sizeof(struct ethernet_header) + sizeof(struct PtpAnnounceHeader);
    tx->metadata.timestamp_id = 0;

    // Update master info
    current_gm.sequenceId = my_info.sequenceId;

    tx->metadata.frame_length = sizeof(*tx_eth) + sizeof(*tx_ptp);

    return sizeof(struct PtpAnnounceHeader);
}

int gptp_make_sync_packet(struct tsn_tx_buffer* tx) {
    sysclock_t now = get_sys_count();
    if (is_decayed(now)) {
        has_master = 0;
    } else {
        // Has master, Don't send announce
        return 0;
    }

    struct ethernet_header* tx_eth = (struct ethernet_header*)&tx->data;
    struct PtpSyncHeader* tx_ptp = (struct PtpSyncHeader*)ETH_PAYLOAD(tx_eth);

    set_eth(tx_eth, PTP_PEER_MAC);

    // Many of these are copied from linuxptp capture
    tx_ptp->hdr.transportSpecific = 0x01;
    tx_ptp->hdr.messageType = PTP_MESSAGE_SYNC;
    tx_ptp->hdr.minorVersionPTP = 1;
    tx_ptp->hdr.versionPTP = 2;
    tx_ptp->hdr.messageLength = sizeof(struct PtpSyncHeader);
    tx_ptp->hdr.domainNumber = 0;
    tx_ptp->hdr.flags = PTP_FLAG_TWO_STEP; // PTP_TWO_STEP
    tx_ptp->hdr.correctionField = 0;
    memcpy(&tx_ptp->hdr.sourcePortIdentity.clockIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->hdr.sourcePortIdentity.portNumber = 1;
    tx_ptp->hdr.sequenceId = my_info.sequenceId;
    tx_ptp->hdr.control = PTP_CONTROL_SYNC;
    tx_ptp->hdr.logMeanMessageInterval = -3;

    tx_ptp->originTimestamp.seconds = 0;
    tx_ptp->originTimestamp.nanoseconds = 0;

    tx->metadata.frame_length = sizeof(*tx_eth) + sizeof(*tx_ptp);
    tx->metadata.timestamp_id = TIMESTAMP_ID_GPTP_RESP;

    return sizeof(struct PtpSyncHeader);
}

int gptp_make_followup_packet(struct tsn_tx_buffer* tx) {
    struct ethernet_header* tx_eth = (struct ethernet_header*)&tx->data;
    struct PtpFollowUpHeader* tx_ptp = (struct PtpFollowUpHeader*)ETH_PAYLOAD(tx_eth);

    set_eth(tx_eth, PTP_PEER_MAC);

    // Many of these are copied from linuxptp capture
    tx_ptp->hdr.transportSpecific = 0x01;
    tx_ptp->hdr.messageType = PTP_MESSAGE_FOLLOW_UP;
    tx_ptp->hdr.minorVersionPTP = 1;
    tx_ptp->hdr.versionPTP = 2;
    tx_ptp->hdr.messageLength = sizeof(*tx_ptp);
    tx_ptp->hdr.domainNumber = 0;
    tx_ptp->hdr.flags = 0;
    tx_ptp->hdr.correctionField = 0;
    memcpy(&tx_ptp->hdr.sourcePortIdentity.clockIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->hdr.sourcePortIdentity.portNumber = 1;
    tx_ptp->hdr.sequenceId = my_info.sequenceId;
    tx_ptp->hdr.control = PTP_CONTROL_FOLLOW_UP;
    tx_ptp->hdr.logMeanMessageInterval = -3;

    // Get TX timestamp from sync packet
    timestamp_t sync_time = gptp_get_tx_timestamp(TIMESTAMP_ID_GPTP_RESP);
    tx_ptp->preciseOriginTimestamp.seconds = sync_time / 1000000000;
    tx_ptp->preciseOriginTimestamp.nanoseconds = sync_time % 1000000000;

    tx->metadata.frame_length = sizeof(*tx_eth) + sizeof(*tx_ptp);
    tx->metadata.timestamp_id = 0;

    return sizeof(*tx_ptp);
}

int gptp_make_pdelay_req_packet(struct tsn_tx_buffer* tx) {
    struct ethernet_header* tx_eth = (struct ethernet_header*)&tx->data;
    struct PtpPdelayReqHeader* tx_ptp = (struct PtpPdelayReqHeader*)ETH_PAYLOAD(tx_eth);

    set_eth(tx_eth, PTP_PEER_MAC);

    // Many of these are copied from linuxptp capture
    tx_ptp->hdr.transportSpecific = 0x1; // gPTP
    tx_ptp->hdr.messageType = PTP_MESSAGE_PDELAY_REQ;
    tx_ptp->hdr.minorVersionPTP = 0x0;
    tx_ptp->hdr.versionPTP = 0x2;
    tx_ptp->hdr.messageLength = sizeof(*tx_ptp);
    tx_ptp->hdr.domainNumber = 0;
    tx_ptp->hdr.flags = 0;
    tx_ptp->hdr.correctionField = 0;
    tx_ptp->hdr.messageTypeSpecific = 0;
    memcpy(&tx_ptp->hdr.sourcePortIdentity.clockIdentity, &myClockId, sizeof(PtpClockIdentity));
    tx_ptp->hdr.sourcePortIdentity.portNumber = 1;
    tx_ptp->hdr.sequenceId = ++my_info.sequenceId;
    tx_ptp->hdr.control = PTP_CONTROL_ALL_OTHER;
    tx_ptp->hdr.logMeanMessageInterval = 0;

    tx->metadata.frame_length = sizeof(*tx_eth) + sizeof(*tx_ptp);
    tx->metadata.timestamp_id = TIMESTAMP_ID_GPTP_REQ;

    return sizeof(*tx_ptp);
}

static int is_current_master(struct PtpHeader* hdr) {
    if (has_master == 0) {
        return 0;
    }
    if (memcmp(&current_gm.portIdentity, &hdr->sourcePortIdentity, sizeof(current_gm.portIdentity)) != 0) {
        return 0;
    }
    return 1;
}

static void timestamp_to_ptp_time(struct PtpTimestamp* ptp_time, const timestamp_t timestamp) {
    ptp_time->seconds = timestamp / 1000000000;
    ptp_time->nanoseconds = timestamp % 1000000000;
}

static timestamp_t ptp_time_to_ns(struct PtpTimestamp* ptp_time) {
    return ((uint64_t)ptp_time->seconds * 1000000000) + ptp_time->nanoseconds;
}

static timestamp_t ptp_correction_to_ns(uint64_t correction) {
    return correction >> 16;
}

timestamp_t gptp_get_timestamp(uint64_t sys_count) {
    // return (uint64_t)(timestamp_mag * sys_count) + timestamp_offset;
    timestamp_t timestamp = timestamp_mag_int * sys_count;
    timestamp += (timestamp_mag_fraction * sys_count) / TIMESTAMP_MAG_RESOLUTION;
    timestamp += timestamp_offset;

    return timestamp;
}

timestamp_t gptp_get_tx_timestamp(int timestamp_id) {
    // Since Real TX timestamp is later than our TX timestamp, we need to adjust it by plus
    sysclock_t ts = get_tx_timestamp(timestamp_id);
    return gptp_get_timestamp(ts) + TX_ADJUST_NS;
}

timestamp_t gptp_get_rx_timestamp(struct tsn_rx_buffer* rx) {
    // Since Real RX timestamp is earlier than our RX timestamp, we need to adjust it by minus
    sysclock_t ts = rx->metadata.timestamp;
    return gptp_get_timestamp(ts) - RX_ADJUST_NS;
}

static inline void adjust_timestamp_mag(double new_mag) {
    uint64_t sys_count = get_sys_count();
    timestamp_t current_timestamp = gptp_get_timestamp(sys_count);
    timestamp_mag_int = new_mag;
    timestamp_mag_fraction = (new_mag - timestamp_mag_int) * TIMESTAMP_MAG_RESOLUTION;
    timestamp_offset += current_timestamp - gptp_get_timestamp(sys_count);
}

static void adjust_clock() {
    int64_t t1 = current_status.sync_tx;
    int64_t t2 = current_status.sync_rx;
    int64_t l1 = current_status.link_delay;
    int64_t c1 = current_status.sync_correction;

    int64_t t1c = t1 + c1;

    int64_t delta = (t2 - t1c) - l1;

    timestamp_offset -= delta;
    debug_ns64("delta", delta);

    add_statistics(&stats_offset, delta);

    uint64_t current_ts = gptp_get_timestamp(get_sys_count());
    debug_timestamp("Current time", current_ts);
}

static void calculate_link_delay() {
    uint64_t t1 = current_status.t1;
    uint64_t t2 = current_status.t2;
    uint64_t t3 = current_status.t3;
    uint64_t t4 = current_status.t4;

    uint64_t c1 = current_status.c1;
    uint64_t c2 = current_status.c2;

    uint64_t t3c = t3 + c1 + c2;

    debug_timestamp("t1", t1);
    debug_timestamp("t2", t2);
    debug_timestamp("t3", t3);
    debug_timestamp("t4", t4);
    debug_timestamp("c1", c1);
    debug_timestamp("c2", c2);

    int64_t link_delay = ((t2 - t1) + (t4 - t3c)) / 2;
    printf_gptp("Link delay: %lld\n", link_delay);

    add_statistics(&stats_delay, link_delay);
}

static void syntonize(uint64_t t1_sysclock, uint64_t t2_ns) {
    // Statistics
    static sysclock_t t1[SYNTONIZE_SIZE];
    static timestamp_t t2[SYNTONIZE_SIZE];
    static int index = 0;

    // Save the timestamps
    t1[index] = t1_sysclock;
    t2[index] = t2_ns;
    index = (index + 1) % SYNTONIZE_SIZE;

    // Syntonize if we have enough timestamps
    int oldest_index = index;
    int newest_index = (oldest_index + SYNTONIZE_SIZE - 1) % SYNTONIZE_SIZE;

    uint64_t t1_oldest = t1[oldest_index];
    uint64_t t2_oldest = t2[oldest_index];
    uint64_t t1_newest = t1[newest_index];
    uint64_t t2_newest = t2[newest_index];

    // Checks filled enough
    if (t1_oldest == 0) {
        return;
    }

    uint64_t t1_diff = t1_newest - t1_oldest;
    uint64_t t2_diff = t2_newest - t2_oldest;

    double neighbor_ratio = (double)t2_diff / t1_diff;
    double scaled_ratio = neighbor_ratio / TICKS_SCALE;
    printf_gptp("Neighbor ratio: %.6f\n", scaled_ratio);

    add_statistics(&stats_freq, (1.0 - scaled_ratio) * 1e9);

    // Safty check
    if (scaled_ratio > 0.9 && scaled_ratio < 1.1) {
        printf_gptp("Adjusting timestamp mag!!\n");
        adjust_timestamp_mag(neighbor_ratio);
    }
}

static void debug_timestamp(const char* name, timestamp_t timestamp) {
#ifdef DEBUG_GPTP
    uint32_t seconds = timestamp / 1000000000;
    uint32_t nanoseconds = timestamp % 1000000000;
    printf_gptp("%s: %u.%09u\n", name, seconds, nanoseconds);
#endif
}

static void debug_sysclock(const char* name, sysclock_t clock) {
#ifdef DEBUG_GPTP
    uint32_t upper = clock >> 32;
    // uint32_t lower = clock & ((1 << 32) - 1);
    uint32_t lower = clock & 0xFFFFFFFF;
    printf_gptp("%s: %08lx%08lx\n", name, upper, lower);
#endif
}

static void debug_ns64(const char* name, int64_t value) {
#ifdef DEBUG_GPTP
    int32_t upper = value / 1000000000;
    int32_t lower = value % 1000000000;
    char sign = '+';
    if (lower < 0) {
        upper *= -1;
        lower *= -1;
        sign = '-';
    }
    printf_gptp("%s: %c%d.%09d\n", name, sign, upper, lower);
#endif
}

bool gptp_get_statistics(struct gptp_statistics_result* stats) {
    struct gptp_statistics_result* offset = &stats[0];
    struct gptp_statistics_result* freq = &stats[1];
    struct gptp_statistics_result* delay = &stats[2];

    int res = 0;
    res += !get_statistics(offset, &stats_offset);
    res += !get_statistics(freq, &stats_freq);
    res += !get_statistics(delay, &stats_delay);

    return res == 0;
}

static void add_statistics(struct statistics* stats, double value) {
    double old_mean = stats->mean;

    if (stats->num == 0 || stats->max < value) {
        stats->max = value;
    }
    if (stats->num == 0 || stats->min > value) {
        stats->min = value;
    }

    stats->num += 1;
    stats->mean = old_mean + (value - old_mean) / stats->num;
    stats->sum_sqr += value * value;
    stats->sum_diff_sqr += (value - old_mean) * (value - stats->mean);
}

static bool get_statistics(struct gptp_statistics_result* result, struct statistics* stats) {
    result->num = stats->num;
    result->min = stats->min;
    result->max = stats->max;
    result->max_abs = stats->max > -stats->min ? stats->max : -stats->min;
    result->mean = stats->mean;
    result->rms = sqrt(stats->sum_sqr / stats->num);
    result->stdev = sqrt(stats->sum_diff_sqr / stats->num);

    return true;
}

void gptp_reset_statistics() {
    memset(&stats_offset, 0, sizeof(stats_offset));
    memset(&stats_freq, 0, sizeof(stats_offset));
    memset(&stats_delay, 0, sizeof(stats_offset));
}
