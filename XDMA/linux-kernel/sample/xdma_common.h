#ifndef __XDMA_COMMON_H__
#define __XDMA_COMMON_H__

#define BUFFER_ALIGNMENT  (0x1000)
#define MAX_BUFFER_LENGTH (0x1000)
//#define MAX_BUFFER_LENGTH (0x10000 * 16)
#define NUMBER_OF_BUFFER  (4096)    // (2048)
#define NUMBER_OF_POOL_BUFFER (NUMBER_OF_BUFFER + 1)
#define NUMBER_OF_RESERVED_BUFFER (4)

typedef char *    BUF_POINTER;

#define EMPTY_ELEMENT (NULL)

typedef struct buffer_stack {
    BUF_POINTER elements[NUMBER_OF_POOL_BUFFER];
    int top;
    pthread_mutex_t mutex;
} buffer_stack_t;

typedef struct reserved_buffer_stack {
    BUF_POINTER elements[NUMBER_OF_RESERVED_BUFFER];
    int top;
    pthread_mutex_t mutex;
} reserved_buffer_stack_t;

#define NUMBER_OF_QUEUE  NUMBER_OF_BUFFER    // (2048)
typedef char *    QueueElement;
typedef struct circular_queue{
    QueueElement elements[NUMBER_OF_QUEUE];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
} CircularQueue_t;

typedef struct stats {
    unsigned long long  rxPackets;
    unsigned long long  rxBytes;
    unsigned long long  rxPps;
    unsigned long long  rxBps;
    unsigned long long  txPackets;
    unsigned long long  txBytes;
    unsigned long long  txPps;
    unsigned long long  txBps;
} stats_t;

#define MAX_DEVICE_NAME 20
#define TEST_DATA_FILE_NAME "./tests/data/datafile0_4K.bin"
#define TEST_DATA_SIZE 1024
enum {
    RUN_MODE_TSN,
    RUN_MODE_NORMAL,
    RUN_MODE_LOOPBACK,
    RUN_MODE_PERFORMANCE,

    RUN_MODE_CNT,
};

#define DEFAULT_RUN_MODE RUN_MODE_TSN

#define MAX_INPUT_FILE_NAME_SIZE 256
typedef struct rx_thread_arg {
    char devname[MAX_DEVICE_NAME];
    char fn[MAX_INPUT_FILE_NAME_SIZE];
    int mode;
    int size;
} rx_thread_arg_t;

typedef struct tx_thread_arg {
    char devname[MAX_DEVICE_NAME];
    char fn[MAX_INPUT_FILE_NAME_SIZE];
    int mode;
    int size;
} tx_thread_arg_t;

typedef struct stats_thread_arg {
    int mode;
} stats_thread_arg_t;

#define SEC  (1)
#define MIN  (60 * SEC)
#define HOUR (60 * MIN)
#define DAY  (24 * HOUR)

typedef struct _execTime
{
    int day;
    int hour;
    int min;
    int sec;
} execTime_t;

#define DEF_RX_DEVICE_NAME "/dev/xdma0_c2h_0"
#define DEF_TX_DEVICE_NAME "/dev/xdma0_h2c_0"

void* receiver_thread(void* arg);
void* sender_thread(void* arg);
void* stats_thread(void* arg);

#endif    // __XDMA_COMMON_H__
