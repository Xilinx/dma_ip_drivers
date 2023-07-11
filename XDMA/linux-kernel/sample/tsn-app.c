#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <signal.h>
#include <sched.h>

#include <lib_menu.h>
#include <helper.h>
#include <log.h>
#include <version.h>

#include "../libxdma/api_xdma.h"
#include "xdma_common.h"
#include "buffer_handler.h"

int watchStop = 1;
int rx_thread_run = 1;
int tx_thread_run = 1;
int stats_thread_run = 1;

int verbose = 0;

void xdma_signal_handler(int sig) {

    printf("\nXDMA-APP is exiting, cause (%d)!!\n", sig);
    tx_thread_run = 0;
    sleep(1);
    rx_thread_run = 0;
    sleep(1);
    stats_thread_run = 0;
    sleep(1);
    exit(0);
}

void signal_stop_handler() {

    if(watchStop) {
        xdma_signal_handler(2);
    } else {
        watchStop = 1;
    }
}

void register_signal_handler() {

    signal(SIGINT,  signal_stop_handler);
    signal(SIGKILL, xdma_signal_handler);
    signal(SIGQUIT, xdma_signal_handler);
    signal(SIGTERM, xdma_signal_handler);
    signal(SIGTSTP, xdma_signal_handler);
    signal(SIGHUP,  xdma_signal_handler);
    signal(SIGABRT, xdma_signal_handler);
}

int tsn_app(int mode, int DataSize, char *InputFileName) {

    pthread_t tid1, tid2, tid3;
    rx_thread_arg_t    rx_arg;
    tx_thread_arg_t    tx_arg;
    stats_thread_arg_t st_arg;

    register_signal_handler();

    initialize_buffer_allocation();

    memset(&rx_arg, 0, sizeof(rx_thread_arg_t));
    memcpy(rx_arg.devname, DEF_RX_DEVICE_NAME, sizeof(DEF_RX_DEVICE_NAME));
    memcpy(rx_arg.fn, InputFileName, MAX_INPUT_FILE_NAME_SIZE);
    rx_arg.mode = mode;
    rx_arg.size = DataSize;
    pthread_create(&tid1, NULL, receiver_thread, (void *)&rx_arg);
    cpu_set_t cpuset1;
    CPU_ZERO(&cpuset1);
    CPU_SET(1, &cpuset1);
    pthread_setaffinity_np(tid1, sizeof(cpu_set_t), &cpuset1);

    memset(&tx_arg, 0, sizeof(tx_thread_arg_t));
    memcpy(tx_arg.devname, DEF_TX_DEVICE_NAME, sizeof(DEF_TX_DEVICE_NAME));
    memcpy(tx_arg.fn, InputFileName, MAX_INPUT_FILE_NAME_SIZE);
    tx_arg.mode = mode;
    tx_arg.size = DataSize;
    pthread_create(&tid2, NULL, sender_thread, (void *)&tx_arg);
    cpu_set_t cpuset2;
    CPU_ZERO(&cpuset2);
    CPU_SET(2, &cpuset2);
    pthread_setaffinity_np(tid2, sizeof(cpu_set_t), &cpuset2);

    memset(&st_arg, 0, sizeof(stats_thread_arg_t));
    st_arg.mode = 1;
    pthread_create(&tid3, NULL, stats_thread, (void *)&st_arg);
    cpu_set_t cpuset3;
    CPU_ZERO(&cpuset3);
    CPU_SET(3, &cpuset3);
    pthread_setaffinity_np(tid3, sizeof(cpu_set_t), &cpuset3);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    buffer_release();

    return 0;
}

int process_main_runCmd(int argc, const char *argv[], menu_command_t *menu_tbl);

menu_command_t  mainCommand_tbl[] = {
    { "run",     EXECUTION_ATTR,   process_main_runCmd, \
        "   run -m <mode> -f <file name> -s <size>", \
        "   Run tsn test application with data szie in mode\n"
        "            <mode> default value: 0 (0: normal, 1: loopback-integrity check, 2: performance)\n"
        "       <file name> default value: ./tests/data/datafile0_4K.bin(Binary file for test)\n"
        "            <size> default value: 1024 (64 ~ 4096)"},
    { 0,           EXECUTION_ATTR,   NULL, " ", " "}
};

#define MAIN_RUN_OPTION_STRING  "m:s:f:hv"
int process_main_runCmd(int argc, const char *argv[],
                            menu_command_t *menu_tbl) {
    int mode  = DEFAULT_RUN_MODE;
    int DataSize = TEST_DATA_SIZE;
    char InputFileName[256] = TEST_DATA_FILE_NAME;
    int argflag;

    while ((argflag = getopt(argc, (char **)argv,
                             MAIN_RUN_OPTION_STRING)) != -1) {
        switch (argflag) {
        case 'm':
            if (str2int(optarg, &mode) != 0) {
                printf("Invalid parameter given or out of range for '-m'.");
                return -1;
            }
            if ((mode < 0) || (mode > 2)) {
                printf("mode %d is out of range.", mode);
                return -1;
            }
            break;
        case 's':
            if (str2int(optarg, &DataSize) != 0) {
                printf("Invalid parameter given or out of range for '-s'.");
                return -1;
            }
            if ((DataSize < 64) || (DataSize > 4096)) {
                printf("DataSize %d is out of range.", DataSize);
                return -1;
            }
            break;
        case 'f':
            memset(InputFileName, 0, 256);
            strcpy(InputFileName, optarg);
            break;
        case 'v':
            log_level_set(++verbose);
            if (verbose == 2) {
                /* add version info to debug output */
                lprintf(LOG_DEBUG, "%s\n", VERSION_STRING);
            }
            break;

        case 'h':
            process_manCmd(argc, argv, menu_tbl, ECHO);
            return 0;
        }
    }

    return tsn_app(mode, DataSize, InputFileName);
}

int command_parser(int argc, char ** argv) {
    char **pav = NULL;
    int  id;

    for(id=0; id<argc; id++) {
        debug_printf("argv[%d] : %s", id, argv[id]);
    }

    pav = argv;

    return lookup_cmd_tbl(argc, (const char **)pav, mainCommand_tbl, ECHO);
}

int main(int argc, char *argv[]) {
    int id;
    int rc=0, t_argc;
    char **pav = NULL;

    for(id=0; id<argc; id++) {
        debug_printf("argv[%d] : %s", id, argv[id]);
    }

    t_argc = argc, pav = argv;
    pav++, t_argc--;

    rc = command_parser(t_argc, pav);

    return rc;
}

