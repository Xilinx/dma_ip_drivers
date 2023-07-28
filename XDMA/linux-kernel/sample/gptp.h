#include "stdint-gcc.h"

#include "platform_config.h"

// All except peer delay messages. Forwardable
static const char PTP_MAC[] = { 0x01, 0x1b, 0x19, 0x00, 0x00, 0x00 };

// Peer delay messages. Not forwardable
// pdelay_req, pdelay_resp, pdelay_resp_fup
static const char PTP_PEER_MAC[] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };

// Statistics
struct gptp_statistics_result {
    int num;
    int min;
    int max;
    int max_abs;
    int mean;
    int rms;
    int stdev;
};

// Derived data types

struct PtpTimeInterval {
    union {
        uint64_t scaledNanoseconds;
        struct {
            uint64_t nanoseconds:48;
            uint16_t fraction;  // base 2. 0x01 = 2 ^ -16 ns
        } __attribute__((packed, scalar_storage_order("big-endian")));
    };
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpTimestamp {
    uint64_t seconds:48;
    uint32_t nanoseconds; // base 10. 0x00000001 = 1 ns
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpExtendedTimestamp {
    uint64_t seconds:48;
    uint64_t fractionalNanoseconds:48; // 0x10000 is 1 ns. 0x0000 0001 0000 = 1ns. Max 10^9 * 2^16
} __attribute__((packed, scalar_storage_order("big-endian")));

typedef uint8_t PtpClockIdentity[8];

struct PtpPortIdentity {
    PtpClockIdentity clockIdentity;
    uint16_t portNumber;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpClockQuality {
    uint8_t clockClass;
    uint8_t clockAccuracy;
    int16_t scaledLogVariance;
} __attribute__((packed, scalar_storage_order("big-endian")));

// General message format

// Message types

// Event types
#define PTP_MESSAGE_SYNC 0x0
#define PTP_MESSAGE_DELAY_REQ 0x1
#define PTP_MESSAGE_PDELAY_REQ 0x2
#define PTP_MESSAGE_PDELAY_RESP 0x3
// General types
#define PTP_MESSAGE_FOLLOW_UP 0x8
#define PTP_MESSAGE_DELAY_RESP 0x9
#define PTP_MESSAGE_PDELAY_RESP_FOLLOW_UP 0xA
#define PTP_MESSAGE_ANNOUNCE 0xB
#define PTP_MESSAGE_SIGNALING 0xC
#define PTP_MESSAGE_MANAGEMENT 0xD

// Control types
#define PTP_CONTROL_SYNC 0x0
#define PTP_CONTROL_DELAY_REQ 0x1
#define PTP_CONTROL_FOLLOW_UP 0x2
#define PTP_CONTROL_DELAY_RESP 0x3
#define PTP_CONTROL_MANAGEMENT 0x4
#define PTP_CONTROL_ALL_OTHER 0x5 // Not used and 0x6~ is reserved

// Flags
#define PTP_FLAG_LI_61 (1 << 0)
#define PTP_FLAG_LI_59 (1 << 1)
#define PTP_FLAG_UTC_REASONABLE (1 << 2)
#define PTP_FLAG_TIMESCALE (1 << 3)
#define PTP_FLAG_TIME_TRACEABLE (1 << 4)
#define PTP_FLAG_FREQUENCY_TRACEABLE (1 << 5)
#define PTP_FLAG_SYNCHRONIZATION_UNCERTAIN (1 << 6)

#define PTP_FLAG_ALTERNATE_MASTER (1 << 8)
#define PTP_FLAG_TWO_STEP (1 << 9)
#define PTP_FLAG_UNICAST (1 << 10)

#define PTP_FLAG_PROFILE_SPECIFIC_1 (1 << 13)
#define PTP_FLAG_PROFILE_SPECIFIC_2 (1 << 14)
#define PTP_FLAG_SECURITY (1 << 15)

struct PtpHeader {
    uint8_t transportSpecific:4;
    uint8_t messageType:4;
    uint8_t minorVersionPTP:4;
    uint8_t versionPTP:4;
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t minorSdoId;
    uint16_t flags;
    uint64_t correctionField; // ns * 2^16
    uint32_t messageTypeSpecific;
    struct PtpPortIdentity sourcePortIdentity;
    uint16_t sequenceId;
    uint8_t control;
    uint8_t logMeanMessageInterval;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpAnnounceHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp originTimestamp;
    int16_t currentUtcOffset;
    uint8_t _reserved1;
    uint8_t grandmasterPriority1;
    struct PtpClockQuality grandmasterClockQuality;
    uint8_t grandmasterPriority2;
    PtpClockIdentity grandmasterIdentity;
    uint16_t stepsRemoved;
    uint8_t timeSource;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpSyncHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp originTimestamp;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpFollowUpHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp preciseOriginTimestamp;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpDelayReqHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp originTimestamp;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpDelayRespHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp receiveTimestamp;
    struct PtpPortIdentity requestingPortIdentity;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpPdelayReqHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp originTimestamp;
    uint8_t _reserved1[10];
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpPdelayRespHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp requestReceiptTimestamp;
    struct PtpPortIdentity requestingPortIdentity;
} __attribute__((packed, scalar_storage_order("big-endian")));

struct PtpPdelayRespFollowUpHeader {
    struct PtpHeader hdr;
    struct PtpTimestamp responseOriginTimestamp;
    struct PtpPortIdentity requestingPortIdentity;
} __attribute__((packed, scalar_storage_order("big-endian")));

// Functions

/*
 * Initialize gPTP
 */
void gptp_init();

/*
 * Get current time in ns from sys clock
 *
 * To get rx/tx timestamp, Use gptp_get_{tx,rx}_timestamp instead of this.
 */
timestamp_t gptp_get_timestamp(sysclock_t sys_count);

/*
 * Get corrected TX timestamp
 *
 * @param timestamp_id Timestamp ID to get TX timestamp from
 */
timestamp_t gptp_get_tx_timestamp(int timestamp_id);

/*
 * Get corrected RX timestamp
 *
 * @param rx RX buffer to get RX timestamp from
 */
timestamp_t gptp_get_rx_timestamp(struct tsn_rx_buffer* rx);

/* 
 * Process gPTP packet and sync
 * 
 * @return Whether tx is set or not
 */
int process_gptp_packet(struct tsn_rx_buffer* rx);

/*
 * Make a gPTP announce packet
 *
 * @return ptp packet size (without ethernet header)
 */
int gptp_make_announce_packet(struct tsn_tx_buffer* tx);

/*
 * Make a gPTP Sync packet
 *
 * @return ptp packet size (without ethernet header)
 */
int gptp_make_sync_packet(struct tsn_tx_buffer* tx);

/*
 * Make a gPTP FollowUp packet
 *
 * This function must be called right after sync packet is sent
 *
 * @return ptp packet size (without ethernet header)
 */
int gptp_make_followup_packet(struct tsn_tx_buffer* tx);

/*
 * Make a gPTP PDelayReq packet
 *
 * @return ptp packet size (without ethernet header)
 */
int gptp_make_pdelay_req_packet(struct tsn_tx_buffer* tx);

/*
 * Get statistics value
 * @param struct gptp_statistics[3]
 * 0: offset
 * 1: freq
 * 2: pdelay
 */
bool gptp_get_statistics(struct gptp_statistics_result* stats);

/*
 * Reset statistics
 */
void gptp_reset_statistics();
