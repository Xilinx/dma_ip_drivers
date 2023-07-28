#include <stddef.h>

#include "platform_config.h"
#include "tsn.h"
#include "api.h"
#include "gptp.h"

#include "util.h"

#define PRIO_QUEUE_COUNT (TSN_QUEUE_COUNT - 1)

#define CONNECTION_SPEED 100000000  // 100 Mbps FIXME
#define CBS_FRACTION_BITS 48

static struct tsn_tx_queues tx_queues;
static struct api_tas_schedules tas_schedules;
static struct api_cbs_configs cbs_configs;

static void get_qbv_status(timestamp_t timestamp, bool* qbv_gates_open);
static void get_qav_status(timestamp_t timestamp, bool* qav_enabled, int* qav_credits);


int tsn_init_queue() {
    for (int i = 0; i < TSN_QUEUE_COUNT; i++) {
        tx_queues.tx_queues[i].count = 0;
        tx_queues.tx_queues[i].head = 0;
        tx_queues.tx_queues[i].tail = 0;
    }
    return 0;
}

int tsn_init_configs() {
    // TAS config
    tas_schedules.gate_enabled = false; // XXX: Set this to true to enable TAS
    tas_schedules.config_change = false;
    tas_schedules.config_change_time.tv_sec = 0;
    tas_schedules.config_change_time.tv_nsec = 0;
    tas_schedules.supported_list_max = MAX_TAS_SCHEDULES;

    // Admin configs
    tas_schedules.admin_gate_states = 0xff;
    tas_schedules.admin_control_list_length = 2;
    tas_schedules.admin_base_time.tv_sec = 0;
    tas_schedules.admin_base_time.tv_nsec = 0;
    tas_schedules.admin_control_list[0].gates_bit = 0x00;
    tas_schedules.admin_control_list[0].duration_ns = 500 * 1000000;  // 500ms
    tas_schedules.admin_control_list[1].gates_bit = 0xff;
    tas_schedules.admin_control_list[1].duration_ns = 500 * 1000000;  // 500ms

    for (int i = 0; i < tas_schedules.admin_control_list_length; i += 1) {
        tas_schedules.admin_cycle_time_extention += tas_schedules.admin_control_list[i].duration_ns;
    }
    tas_schedules.admin_cycle_time = tas_schedules.admin_cycle_time_extention / 1000000000;
    tas_schedules.admin_cycle_time_extention = tas_schedules.admin_cycle_time_extention % 1000000000;

    // Copy admin to oper
    tas_schedules.oper_gate_states = tas_schedules.admin_gate_states;
    tas_schedules.oper_base_time = tas_schedules.admin_base_time;

    tas_schedules.oper_control_list_length = tas_schedules.admin_control_list_length;
    for (int i = 0; i < tas_schedules.oper_control_list_length; i += 1) {
        tas_schedules.oper_control_list[i] = tas_schedules.admin_control_list[i];
    }
    tas_schedules.oper_gate_states = tas_schedules.admin_gate_states;

    tas_schedules.oper_cycle_time = tas_schedules.admin_cycle_time;
    tas_schedules.oper_cycle_time_extention = tas_schedules.admin_cycle_time_extention;

    // CBS config
    for (int i = 0; i < TSN_QUEUE_COUNT; i++) {
        cbs_configs.configs[i].is_enabled = false;
    }

    return 0; // XXX: Remove this to set CBS on queue 0

    cbs_configs.configs[0].is_enabled = true;
    cbs_configs.configs[0].hicredit = 10;
    cbs_configs.configs[0].locredit = -100;
    cbs_configs.configs[0].sendslope = -90;
    cbs_configs.configs[0].idleslope = 10;

    cbs_configs.configs[0].current_credit = (int64_t)0 << CBS_FRACTION_BITS;
    cbs_configs.configs[0].last_update_time = gptp_get_timestamp(get_sys_count());

    return 0;
}

int tsn_select_queue(timestamp_t timestamp) {
    // TODO: Make it faster
    // Check which queue is open (qbv)
    bool qbv_gates_open[TSN_QUEUE_COUNT] = {true,};
    get_qbv_status(timestamp, qbv_gates_open);  // TODO: implement

    bool qav_enabled[TSN_QUEUE_COUNT];
    int qav_credits[TSN_QUEUE_COUNT];
    get_qav_status(timestamp, qav_enabled, qav_credits);  // TODO: implement

    // Check which queue has enough token (qav)
    // Check which queue has the highest priority and the queue is not empty
    // Weighted round robin from Qbv opened unshaped queues if Qav queue not selected
    int selected_queue = -1;
    int highest_token = -1;
    for (int i = TSN_QUEUE_COUNT - 1; i >= 0; i -= 1) {
        if (!qbv_gates_open[i]) continue;
        if (qav_enabled[i]) {
            if (qav_credits[i] < 0) {
                continue;
            };
            int token = qav_credits[i];
            if (token > highest_token) {
                highest_token = token;
                selected_queue = i;
            }
        }
    }

    if (selected_queue >= 0) {
        return selected_queue;
    }

    // Weighted round robin
    // On this implementation, Use classical WRR algorithm.
    // And the queue N has the weight N.
    // The queue 0 is the BE queue. So we don't check the BE queue here.
    static int wrr_current = PRIO_QUEUE_COUNT - 1;  // highest queue first, 0 means queue 1
    static int wrr_count = 0;
    for (int i = 0; i < PRIO_QUEUE_COUNT; i++) {
        int index = (wrr_current - i) % PRIO_QUEUE_COUNT + 1;
        if (qbv_gates_open[index] == false) continue;
        if (tx_queues.tx_queues[index].count == 0) continue;

        // Select this queue.
        wrr_count += 1;
        if (wrr_count >= index) {
            // Move onto next queue
            wrr_current = (index - 1) % PRIO_QUEUE_COUNT;
        }

        return index;
    }

    // Nothing selected from WRR
    // Return BE queue or -1 if BE queue is empty
    if (qbv_gates_open[0] == true && qav_enabled[0] == false && tx_queues.tx_queues[0].count > 0) {
        // printf_debug("open: %d, count: %d\n", qbv_gates_open[0] & 1, tx_queues.tx_queues[0].count);
        return 0;
    } else if (tx_queues.tx_queues[0].count > 0 && qbv_gates_open[0] == false) {
        printf_debug("Closed but count: %d\n", tx_queues.tx_queues[0].count);
    }

    return -1;
}

timestamp_t tsn_get_next_tx_time(timestamp_t timestamp) {
    // TODO: implement
    // * Gets the next TX time for the given queue.
    // * Next TX time will be the time which any of queue satisfying these:
    // * - The queue must have at lease one packet
    // * - Qbv slot is open for the queue if Qbv is enabled
    // * - Qav credit >= 0 if CBS is enabled
    return 0;
}

struct tsn_tx_buffer* tsn_queue_peek(int queue_index) {
    if (queue_index < 0 || queue_index >= TSN_QUEUE_COUNT)
        return NULL;
    if (tx_queues.tx_queues[queue_index].count == 0)
        return NULL;

    struct tsn_tx_queue* queue = &tx_queues.tx_queues[queue_index];
    int index = queue->tail;
    return queue->packets[index];
}

struct tsn_tx_buffer* tsn_queue_dequeue(int queue_index) {
    struct tsn_tx_buffer* result = tsn_queue_peek(queue_index);
    if (result == NULL)
        return NULL;

    // TODO: thread safety
    struct tsn_tx_queue* queue = &tx_queues.tx_queues[queue_index];
    int index = queue->tail;
    queue->tail = (index + 1) % TSN_QUEUE_SIZE;
    queue->count -= 1;
    // printf_debug("Dequeue %d, count: %d\n", queue_index, queue->count);

    // if Qav enabled queue, update credit based on the packet size
    if (cbs_configs.configs[queue_index].is_enabled) {
        printf_debug("Dequeueing from Qav enabled queue %d\n", queue_index);
        int packet_size_bits = result->metadata.frame_length * 8;
        int sendslope = cbs_configs.configs[queue_index].sendslope;
        int hicredit = cbs_configs.configs[queue_index].hicredit;
        int locredit = cbs_configs.configs[queue_index].locredit;
        int64_t current_credit = cbs_configs.configs[queue_index].current_credit;

        int64_t delta_time_ns = packet_size_bits * 1000000000 / CONNECTION_SPEED;
        int64_t delta_credit = (delta_time_ns * sendslope) * (((int64_t)1 << CBS_FRACTION_BITS) / 1000000000);
        printf_debug("Delta is %lld, credit = %016llx\n", delta_credit, current_credit);

        current_credit += delta_credit;
        if (current_credit > (int64_t)hicredit << CBS_FRACTION_BITS) {
            current_credit = (int64_t)hicredit << CBS_FRACTION_BITS;
        } else if (current_credit < (int64_t)locredit << CBS_FRACTION_BITS) {
            current_credit = (int64_t)locredit << CBS_FRACTION_BITS;
        }

        cbs_configs.configs[queue_index].current_credit = current_credit;
        cbs_configs.configs[queue_index].last_update_time += delta_time_ns;
    }

    return result;
}

int tsn_queue_enqueue(struct tsn_tx_buffer* tx_buffer, int queue_index) {
    printf_debug("Enqueue: %d\n", queue_index);
    if (queue_index < 0 || queue_index >= TSN_QUEUE_COUNT) {
        printf_debug("Invalid queue index\n");
        return 0;
    }
    if (tx_queues.tx_queues[queue_index].count == TSN_QUEUE_SIZE) {
        printf_debug("Queue %d is full\n", queue_index);
        return 0;
    }

    // TODO: thread safety
    struct tsn_tx_queue* queue = &tx_queues.tx_queues[queue_index];
    int index = queue->head;
    queue->packets[index] = tx_buffer;
    queue->head = (index + 1) % TSN_QUEUE_SIZE;
    queue->count += 1;

    printf_debug("Enqueue: %d, count: %d\n", queue_index, tx_queues.tx_queues[queue_index].count);

    return 1;
}

static void get_qbv_status(timestamp_t timestamp, bool* qbv_gates_open) {

    if (tas_schedules.gate_enabled == false) {
        // Qbv is disabled
        // qbv_gates_open is already all true
        return;
    }

    // FIXME: Consider config change

    // MOD the time
    uint64_t cycle_time = tas_schedules.oper_cycle_time * 1000000000ULL + tas_schedules.oper_cycle_time_extention;
    uint64_t mod_time = timestamp % cycle_time;

    // TODO: Could make table for faster loop?

    // Find the active schedule
    for (int index = 0; index < tas_schedules.oper_control_list_length; index++) {
        struct api_tas_entry* schedule = &(tas_schedules.oper_control_list[index]);
        uint64_t gate_open_duration_ns = schedule->duration_ns;
        if (mod_time >= gate_open_duration_ns) {
            // This schedule is not active, Move onto next schedule
            mod_time -= gate_open_duration_ns;
            continue;
        }

        // This schedule is active
        for (int i = 0; i < TSN_QUEUE_COUNT; i++) {
            qbv_gates_open[i] = (schedule->gates_bit >> i) & 1;
        }
        printf_debug("Gates status: %d %d %d %d %d %d %d %d\n",
                schedule->gates_bit >> 7 & 1,
                schedule->gates_bit >> 6 & 1,
                schedule->gates_bit >> 5 & 1,
                schedule->gates_bit >> 4 & 1,
                schedule->gates_bit >> 3 & 1,
                schedule->gates_bit >> 2 & 1,
                schedule->gates_bit >> 1 & 1,
                schedule->gates_bit >> 0 & 1);
        return;
    }

    printf_debug("Error: No active schedule found\n");
}

static void get_qav_status(timestamp_t timestamp, bool* qav_enabled, int* qav_credits) {
    // TODO: To speed up, Could skip calculating credit on closed queue (Qbv)
    // But not implement that now.

    // To avoid loss, lazy update the credit before it reaches >= 0
    // But return the credit as it is

    for (int i = 0; i < TSN_QUEUE_COUNT; i++) {
        qav_enabled[i] = cbs_configs.configs[i].is_enabled;
        if (cbs_configs.configs[i].is_enabled == false) {
            continue;
        }

        int64_t credit = cbs_configs.configs[i].current_credit;
        if (timestamp < cbs_configs.configs[i].last_update_time) {
            // Skip calculating due to sending is ongoing
            printf_debug("Skip %d = %llx (%d)\n", i, credit, credit >> 48);
        } else {
            int64_t time_delta = timestamp - cbs_configs.configs[i].last_update_time;
            int64_t delta = ((int64_t)cbs_configs.configs[i].idleslope) * time_delta * ((1LL << CBS_FRACTION_BITS) / 1000000000);

            credit += delta;
            if (credit > (int64_t)cbs_configs.configs[i].hicredit << CBS_FRACTION_BITS) {
                credit = (int64_t)cbs_configs.configs[i].hicredit << CBS_FRACTION_BITS;
            } else if (credit < (int64_t)cbs_configs.configs[i].locredit << CBS_FRACTION_BITS) {
                credit = (int64_t)cbs_configs.configs[i].locredit << CBS_FRACTION_BITS;
            }

            if (credit >= 0) {
                // Update the credit and last update time
                cbs_configs.configs[i].last_update_time = timestamp;
                cbs_configs.configs[i].current_credit = credit;
            }
        }

        qav_credits[i] = credit >> CBS_FRACTION_BITS;
    }
}
