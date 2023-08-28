/*
* TSNv1 XDMA :
* -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "xdma_common.h"

#define MAX_COUNTERS            29

char* counter_name[MAX_COUNTERS] = {
    "rxPackets",
    "rxBytes",
    "rxErrors",
    "rxNoBuffer",
    "rxPps",
    "rxBps",
    "txPackets",
    "txBytes",
    "txFiltered",
    "txErrors",
    "txPps",
    "txBps",
    NULL,
};

enum {
    COUNTERS_RXPACKETS,
    COUNTERS_RXBYTES,
    COUNTERS_RXERRORS,
    COUNTERS_RXNOBUF,
    COUNTERS_RXPPS,
    COUNTERS_RXBPS,
    COUNTERS_TXPACKETS,
    COUNTERS_TXBYTES,
    COUNTERS_TXFILTERED,
    COUNTERS_TXERRORS,
    COUNTERS_TXPPS,
    COUNTERS_TXBPS,

    COUNTERS_CNT,
};

int printCounters[] =
{
    COUNTERS_RXPACKETS,
    COUNTERS_RXBYTES,
    COUNTERS_RXPPS,
    COUNTERS_RXBPS,

    COUNTERS_TXPACKETS,
    COUNTERS_TXBYTES,
    COUNTERS_TXPPS,
    COUNTERS_TXBPS,

    0xffff,
};

extern int stats_thread_run;

time_t  start_time;
time_t  stopTime;

extern stats_t rx_stats;
extern stats_t tx_stats;

stats_t cs;     /* total stats */
stats_t os;     /* total stats */
unsigned long long  currTv;
unsigned long long  lastTv;

void calculate_stats()
{
    unsigned long long  usec = currTv - lastTv;

    cs.rxPackets = rx_stats.rxPackets;
    cs.rxBytes = rx_stats.rxBytes;
    cs.txPackets = tx_stats.txPackets;
    cs.txBytes = tx_stats.txBytes;
    cs.rxPps = ((rx_stats.rxPackets - os.rxPackets) * 1000000) / usec;
    cs.rxBps = ((rx_stats.rxBytes - os.rxBytes) * 8000000) / usec;
    cs.txPps = ((tx_stats.txPackets - os.txPackets) * 1000000) / usec;
    cs.txBps = ((tx_stats.txBytes - os.txBytes) * 8000000) / usec;
    memcpy(&os, &cs, sizeof(stats_t));
}

void print_counter() {

    printf("%20s", counter_name[COUNTERS_RXPACKETS]);
    printf("%16llu\n", rx_stats.rxPackets);
    printf("%20s", counter_name[COUNTERS_RXBYTES]);
    printf("%16llu\n", rx_stats.rxBytes);
    printf("%20s", counter_name[COUNTERS_RXERRORS]);
    printf("%16llu\n", rx_stats.rxErrors);
    printf("%20s", counter_name[COUNTERS_RXNOBUF]);
    printf("%16llu\n", rx_stats.rxNoBuffer);
    printf("%20s", counter_name[COUNTERS_RXPPS]);
    printf("%16llu\n", cs.rxPps);
    printf("%20s", counter_name[COUNTERS_RXBPS]);
    printf("%16llu\n", cs.rxBps);
    printf("%20s", counter_name[COUNTERS_TXPACKETS]);
    printf("%16llu\n", tx_stats.txPackets);
    printf("%20s", counter_name[COUNTERS_TXBYTES]);
    printf("%16llu\n", tx_stats.txBytes);
    printf("%20s", counter_name[COUNTERS_TXFILTERED]);
    printf("%16llu\n", tx_stats.txFiltered);
    printf("%20s", counter_name[COUNTERS_TXERRORS]);
    printf("%16llu\n", tx_stats.txErrors);
    printf("%20s", counter_name[COUNTERS_TXPPS]);
    printf("%16llu\n", cs.txPps);
    printf("%20s", counter_name[COUNTERS_TXBPS]);
    printf("%16llu\n", cs.txBps);
}

void CalExecTimeInfo(int seed, execTime_t *info) {

    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    int tmp;

    day = seed / DAY;
    tmp = seed % DAY;
    if (tmp) {
        seed = tmp;
        hour = seed / HOUR;
        tmp = seed % HOUR;
        if (tmp) {
            seed = tmp;
            min = seed / MIN;
            sec = seed % MIN;
        }
    }

    info->day = day;
    info->hour = hour;
    info->min = min;
    info->sec = sec;
}

void print_stats() {

    int totalRunTime;
    time_t cur_time;
    struct timeval  tv;
    execTime_t runTimeInfo;

    int systemRet = system("clear");
    if(systemRet == -1){
    }

    gettimeofday(&tv, NULL);
    currTv = tv.tv_sec * 1000000 + tv.tv_usec;

    calculate_stats();

    time(&cur_time);
    totalRunTime = cur_time - start_time;
    CalExecTimeInfo(totalRunTime, &runTimeInfo);

    print_counter();
    printf("%20s %u sec (%d-%d:%d:%d)\n", "Running",
           (unsigned int)totalRunTime,
           runTimeInfo.day, runTimeInfo.hour, runTimeInfo.min, runTimeInfo.sec);

    lastTv = currTv;
}

void* stats_thread(void* arg) {

    stats_thread_arg_t* p_arg = (stats_thread_arg_t*)arg;
    struct timeval previousTime, currentTime;
    double elapsedTime;
    struct timeval  tv;

    printf(">>> %s(mode: %d)\n", __func__, p_arg->mode);

    time(&start_time);
    gettimeofday(&tv, NULL);
    lastTv = tv.tv_sec * 1000000 + tv.tv_usec;

    gettimeofday(&previousTime, NULL);

    while (stats_thread_run) {
        gettimeofday(&currentTime, NULL); 
        elapsedTime = (currentTime.tv_sec - previousTime.tv_sec) + (currentTime.tv_usec - previousTime.tv_usec) / 1000000.0;

        if (elapsedTime >= 1.0) {
            print_stats();
			memcpy(&previousTime, &currentTime, sizeof(struct timeval));
        }
    }

    printf("<<< %s\n", __func__);

    return NULL;
}
