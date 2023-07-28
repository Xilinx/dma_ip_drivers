
#ifndef __TSN_H__
#define __TSN_H__

#include <stdint.h>

#include "api.h"

#define TSN_QUEUE_SIZE 40


// Ring queue
struct tsn_tx_queue {
  int count;  // For faster check
  int head;  // Enqueue
  int tail;  // Dequeue
  struct tsn_tx_buffer* packets[TSN_QUEUE_SIZE];
};

struct tsn_tx_queues {
  struct tsn_tx_queue tx_queues[TSN_QUEUE_COUNT];
};

/**
 * Initializes the TSN queue.
 * Init 8 queues, TAS entries, CBS entries, etc.
 * @return 0 on success, -1 on failure.
 */
int tsn_init_queue();

/**
 * Initializes the TSN Configs. Only for testing
 * @return 0 on success, -1 on failure.
 */
int tsn_init_configs();

/**
 * Selects the queue to use for the next packet.
 * @param tx_buffer The TSN TX buffer.
 * @param timestamp The timestamp now.
 * @return The queue index. -1 if no queue is available.
 */
int tsn_select_queue(timestamp_t timestamp);

/**
 * Gets the next TX time for the given queue.
 * Next TX time will be the time which any of queue satisfying these:
 * - The queue must have at lease one packet
 * - Qbv slot is open for the queue if Qbv is enabled
 * - Qav credit >= 0 if CBS is enabled
 * XXX: Must be checked manually if new packet is inserted into the queues.
 * XXX: This function would be complicated.
 * @param timestamp The timestamp now.
 * @return The next TX time
 */
timestamp_t tsn_get_next_tx_time(timestamp_t timestamp);

/**
 * Peek the packet from the given queue.
 * @param queue_index The queue index. 0..7
 * @return The packet. NULL if the queue is empty.
 */
struct tsn_tx_buffer* tsn_queue_peek(int queue_index);

/**
 * Dequeue the packet from the given queue.
 * @param queue_index The queue index. 0..7
 * @return The packet. NULL if the queue is empty.
 */
struct tsn_tx_buffer* tsn_queue_dequeue(int queue_index);

/**
 * Enqueue the packet to the given queue.
 * @param tx_buffer The TSN TX buffer.
 * @param queue_index The queue index. 0..7
 * @return 1 if success (packet is enqueued), 0 if failed (queue is full).
 */
int tsn_queue_enqueue(struct tsn_tx_buffer* tx_buffer, int queue_index);

#endif // __TSN_H__
