#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#include <signal.h>
#include <sched.h>

#include <lib_menu.h>
#include <helper.h>
#include <log.h>
#include <version.h>

#include <error_define.h>

#include "../libxdma/api_xdma.h"
#include "platform_config.h"
#include "xdma_common.h"
#include "buffer_handler.h"

int watchStop = 1;
int rx_thread_run = 1;
int tx_thread_run = 1;
int stats_thread_run = 1;

int verbose = 0;

/************************** Variable Definitions *****************************/
struct reginfo reg_general[] = {
    {"TSN version", REG_TSN_VERSION},
    {"TSN Configuration", REG_TSN_CONFIG},
    {"TSN Control", REG_TSN_CONTROL},
    {"@sys count", REG_SYS_COUNT_HIGH},
    {"TEMAC status", REG_TEMAC_STATUS},
    {"", -1}
};

struct reginfo reg_rx[] = {
    {"rx packets", REG_RX_PACKETS},
    {"@rx bytes", REG_RX_BYTES_HIGH},
    {"rx drop packets", REG_RX_DROP_PACKETS},
    {"@rx drop bytes", REG_RX_DROP_BYTES_HIGH},
    {"rx input packet counter", REG_RX_INPUT_PACKET_COUNT},
    {"rx output packet counter", REG_RX_OUTPUT_PACKET_COUNT},
    {"rx buffer full drop packet count", REG_RX_BUFFER_FULL_DROP_PACKET_COUNT},
    {"RPPB FIFO status", REG_RPPB_FIFO_STATUS},
    {"RASB FIFO status", REG_RASB_FIFO_STATUS},
    {"MRIB debug", REG_MRIB_DEBUG},
    {"TEMAC rx statistics", REG_TEMAC_RX_STAT},
    {"", -1}
};

struct reginfo reg_tx[] = {
    {"tx packets", REG_TX_PACKETS},
    {"@tx bytes", REG_TX_BYTES_HIGH},
    {"tx drop packets", REG_TX_DROP_PACKETS},
    {"@tx drop bytes", REG_TX_DROP_BYTES_HIGH},
    {"tx timestamp count", REG_TX_TIMESTAMP_COUNT},
    {"@tx timestamp 1", REG_TX_TIMESTAMP1_HIGH},
    {"@tx timestamp 2", REG_TX_TIMESTAMP2_HIGH},
    {"@tx timestamp 3", REG_TX_TIMESTAMP3_HIGH},
    {"@tx timestamp 4", REG_TX_TIMESTAMP4_HIGH},
    {"tx input packet counter", REG_TX_INPUT_PACKET_COUNT},
    {"tx output packet counter", REG_TX_OUTPUT_PACKET_COUNT},
    {"tx buffer full drop packet count", REG_TX_BUFFER_FULL_DROP_PACKET_COUNT},
    {"TASB FIFO status", REG_TASB_FIFO_STATUS},
    {"TPPB FIFO status", REG_TPPB_FIFO_STATUS},
    {"MTIB debug", REG_MTIB_DEBUG},
    {"TEMAC tx statistics", REG_TEMAC_TX_STAT},
    {"", -1}
};

/*****************************************************************************/

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

    pthread_t tid1, tid2;
    rx_thread_arg_t    rx_arg;
    tx_thread_arg_t    tx_arg;
#ifdef PLATFORM_DEBUG
    pthread_t tid3;
    stats_thread_arg_t st_arg;
#endif

    if(initialize_buffer_allocation()) {
        return -1;
    }

    register_signal_handler();

    memset(&rx_arg, 0, sizeof(rx_thread_arg_t));
    memcpy(rx_arg.devname, DEF_RX_DEVICE_NAME, sizeof(DEF_RX_DEVICE_NAME));
    memcpy(rx_arg.fn, InputFileName, MAX_INPUT_FILE_NAME_SIZE);
    rx_arg.mode = mode;
    rx_arg.size = DataSize;
    pthread_create(&tid1, NULL, receiver_thread, (void *)&rx_arg);
#ifdef _MULTI_CPU_CORES_
    cpu_set_t cpuset1;
    CPU_ZERO(&cpuset1);
    CPU_SET(1, &cpuset1);
    pthread_setaffinity_np(tid1, sizeof(cpu_set_t), &cpuset1);
#endif
    sleep(1);

    memset(&tx_arg, 0, sizeof(tx_thread_arg_t));
    memcpy(tx_arg.devname, DEF_TX_DEVICE_NAME, sizeof(DEF_TX_DEVICE_NAME));
    memcpy(tx_arg.fn, InputFileName, MAX_INPUT_FILE_NAME_SIZE);
    tx_arg.mode = mode;
    tx_arg.size = DataSize;
    pthread_create(&tid2, NULL, sender_thread, (void *)&tx_arg);
#ifdef _MULTI_CPU_CORES_
    cpu_set_t cpuset2;
    CPU_ZERO(&cpuset2);
    CPU_SET(2, &cpuset2);
    pthread_setaffinity_np(tid2, sizeof(cpu_set_t), &cpuset2);
#endif
    sleep(1);

#ifdef PLATFORM_DEBUG
    memset(&st_arg, 0, sizeof(stats_thread_arg_t));
    st_arg.mode = 1;
    pthread_create(&tid3, NULL, stats_thread, (void *)&st_arg);
#ifdef _MULTI_CPU_CORES_
    cpu_set_t cpuset3;
    CPU_ZERO(&cpuset3);
    CPU_SET(3, &cpuset3);
    pthread_setaffinity_np(tid3, sizeof(cpu_set_t), &cpuset3);
#endif
#endif

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
#ifdef PLATFORM_DEBUG
    pthread_join(tid3, NULL);
#endif

    buffer_release();

    return 0;
}

int process_main_runCmd(int argc, const char *argv[], menu_command_t *menu_tbl);
int process_main_showCmd(int argc, const char *argv[], menu_command_t *menu_tbl);
int process_main_setCmd(int argc, const char *argv[], menu_command_t *menu_tbl);


menu_command_t  mainCommand_tbl[] = {
    { "run",  EXECUTION_ATTR,   process_main_runCmd, \
        "   run -m <mode> -f <file name> -s <size>", \
        "   Run tsn test application with data szie in mode\n"
        "            <mode> default value: 0 (0: tsn, 1: normal, 2: loopback-integrity check, 3: performance)\n"
        "       <file name> default value: ./tests/data/datafile0_4K.bin(Binary file for test)\n"
        "            <size> default value: 1024 (64 ~ 4096)"},
    {"show",  EXECUTION_ATTR, process_main_showCmd, \
        "   show register [gen, rx, tx, h2c, c2h, irq, con, h2cs, c2hs, com, msix]\n", \
        "   Show XDMA resource"},
    {"set",   EXECUTION_ATTR, process_main_setCmd, \
        "   set register [gen, rx, tx, h2c, c2h, irq, con, h2cs, c2hs, com, msix] <addr(Hex)> <data(Hex)>\n", \
        "   set XDMA resource"},
    { 0,           EXECUTION_ATTR,   NULL, " ", " "}
};

#define MAIN_RUN_OPTION_STRING  "m:s:f:hv"
int process_main_runCmd(int argc, const char *argv[],
                            menu_command_t *menu_tbl) {
    int mode  = DEFAULT_RUN_MODE;
    int DataSize = MAX_BUFFER_LENGTH;
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
            if ((mode < 0) || (mode >= RUN_MODE_CNT)) {
                printf("mode %d is out of range.", mode);
                return -1;
            }
            break;
        case 's':
            if (str2int(optarg, &DataSize) != 0) {
                printf("Invalid parameter given or out of range for '-s'.");
                return -1;
            }
            if ((DataSize < 64) || (DataSize > MAX_BUFFER_LENGTH)) {
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

int32_t fn_show_register_genArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_rxArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_txArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_h2cArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_c2hArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_irqArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_configArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_h2c_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_c2h_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_common_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_show_register_msix_vectorArgument(int32_t argc, const char *argv[]);

argument_list_t  showRegisterArgument_tbl[] = {
        {"gen",  fn_show_register_genArgument},
        {"rx",   fn_show_register_rxArgument},
        {"tx",   fn_show_register_txArgument},
        {"h2c",  fn_show_register_h2cArgument},
        {"c2h",  fn_show_register_c2hArgument},
        {"irq",  fn_show_register_irqArgument},
        {"con",  fn_show_register_configArgument},
        {"h2cs", fn_show_register_h2c_sgdmaArgument},
        {"c2hs", fn_show_register_c2h_sgdmaArgument},
        {"com",  fn_show_register_common_sgdmaArgument},
        {"msix", fn_show_register_msix_vectorArgument},
        {0,     NULL}
    };

int fn_show_registerArgument(int argc, const char *argv[]);
argument_list_t  showArgument_tbl[] = {
        {"register", fn_show_registerArgument},
        {0,          NULL}
    };

int32_t fn_set_register_genArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_h2cArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_c2hArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_irqArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_configArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_h2c_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_c2h_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_common_sgdmaArgument(int32_t argc, const char *argv[]);
int32_t fn_set_register_msix_vectorArgument(int32_t argc, const char *argv[]);

argument_list_t  setRegisterArgument_tbl[] = {
        {"gen",  fn_set_register_genArgument},
        {"rx",   fn_set_register_genArgument},
        {"tx",   fn_set_register_genArgument},
        {"h2c",  fn_set_register_h2cArgument},
        {"c2h",  fn_set_register_c2hArgument},
        {"irq",  fn_set_register_irqArgument},
        {"con",  fn_set_register_configArgument},
        {"h2cs", fn_set_register_h2c_sgdmaArgument},
        {"c2hs", fn_set_register_c2h_sgdmaArgument},
        {"com",  fn_set_register_common_sgdmaArgument},
        {"msix", fn_set_register_msix_vectorArgument},
        {0,     NULL}
    };

int fn_set_registerArgument(int argc, const char *argv[]);
argument_list_t  setArgument_tbl[] = {
        {"register", fn_set_registerArgument},
        {0,          NULL}
    };

#define XDMA_REGISTER_DEV    "/dev/xdma0_user"

int set_register(int offset, uint32_t val) {

    return xdma_api_wr_register(XDMA_REGISTER_DEV, offset, 'w', val);
}

uint32_t get_register(int offset) {

    uint32_t read_val = 0;
    xdma_api_rd_register(XDMA_REGISTER_DEV, offset, 'w', &read_val);

    return read_val;
}

void dump_reginfo(struct reginfo* reginfo) {

    for (int i = 0; reginfo[i].offset >= 0; i++) {
        if (reginfo[i].name[0] == '@') {
            uint64_t ll = get_register(reginfo[i].offset);
            ll = (ll << 32) | get_register(reginfo[i].offset + 4);
            printf("%s : 0x%lx\n", &(reginfo[i].name[1]), ll);
        } else {
            printf("%s : 0x%x\n", reginfo[i].name, (unsigned int)get_register(reginfo[i].offset));
        }
    }
}


#define XDMA_ENGINE_REGISTER_DEV    "/dev/xdma0_control"
uint32_t get_xdma_engine_register(int offset) {

    uint32_t read_val = 0;
    xdma_api_rd_register(XDMA_ENGINE_REGISTER_DEV, offset, 'w', &read_val);

    return read_val;
}

void xdma_reginfo(struct reginfo* reginfo) {

    for (int i = 0; reginfo[i].offset >= 0; i++) {
        if (reginfo[i].name[0] == '@') {
            uint64_t ll = get_xdma_engine_register(reginfo[i].offset);
            ll = (ll << 32) | get_xdma_engine_register(reginfo[i].offset + 4);
            printf("%s : 0x%lx\n", &(reginfo[i].name[1]), ll);
        } else {
            printf("%s : 0x%x\n", reginfo[i].name, (unsigned int)get_xdma_engine_register(reginfo[i].offset));
        }
    }
}

#define H2C_CHANNEL_IDENTIFIER_0x00                   (0X00)
#define H2C_CHANNEL_CONTROL_0X04                      (0X04)
#define H2C_CHANNEL_CONTROL_0X08                      (0X08)
#define H2C_CHANNEL_CONTROL_0X0C                      (0X0C)
#define H2C_CHANNEL_STATUS_0X40                       (0X40)
#define H2C_CHANNEL_STATUS_0X44                       (0X44)
#define H2C_CHANNEL_COMPLETED_DESCRIPTOR_COUNT_0X48   (0X48)
#define H2C_CHANNEL_ALIGNMENTS_0X4C                   (0X4C)
#define H2C_POLL_MODE_LOW_WRITE_BACK_ADDRESS_0X88     (0X88)
#define H2C_POLL_MODE_HIGH_WRITE_BACK_ADDRESS_0X8C    (0X8C)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X90        (0X90)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X94        (0X94)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X98        (0X98)
#define H2C_CHANNEL_PERFORMANCE_MONITOR_CONTROL_0XC0  (0XC0)
#define H2C_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC4      (0XC4)
#define H2C_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC8      (0XC8)
#define H2C_CHANNEL_PERFORMANCE_DATA_COUNT_0XCC       (0XCC)
#define H2C_CHANNEL_PERFORMANCE_DATA_COUNT_0XD0       (0XD0)

struct reginfo reg_h2c[] = {
    {"H2C Channel Identifier (0x00)",                  H2C_CHANNEL_IDENTIFIER_0x00},
    {"H2C Channel Control (0x04)",                     H2C_CHANNEL_CONTROL_0X04},
    {"H2C Channel Control (0x08)",                     H2C_CHANNEL_CONTROL_0X08},
    {"H2C Channel Control (0x0C)",                     H2C_CHANNEL_CONTROL_0X0C},
    {"H2C Channel Status (0x40)",                      H2C_CHANNEL_STATUS_0X40},
    {"H2C Channel Status (0x44)",                      H2C_CHANNEL_STATUS_0X44},
    {"H2C Channel Completed Descriptor Count (0x48)",  H2C_CHANNEL_COMPLETED_DESCRIPTOR_COUNT_0X48},
    {"H2C Channel Alignments (0x4C)",                  H2C_CHANNEL_ALIGNMENTS_0X4C},
    {"H2C Poll Mode Low Write Back Address (0x88)",    H2C_POLL_MODE_LOW_WRITE_BACK_ADDRESS_0X88},
    {"H2C Poll Mode High Write Back Address (0x8C)",   H2C_POLL_MODE_HIGH_WRITE_BACK_ADDRESS_0X8C},
    {"H2C Channel Interrupt Enable Mask (0x90)",       H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X90},
    {"H2C Channel Interrupt Enable Mask (0x94)",       H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X94},
    {"H2C Channel Interrupt Enable Mask (0x98)",       H2C_CHANNEL_INTERRUPT_ENABLE_MASK_0X98},
    {"H2C Channel Performance Monitor Control (0xC0)", H2C_CHANNEL_PERFORMANCE_MONITOR_CONTROL_0XC0},
    {"H2C Channel Performance Cycle Count (0xC4)",     H2C_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC4},
    {"H2C Channel Performance Cycle Count (0xC8)",     H2C_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC8},
    {"H2C Channel Performance Data Count (0xCC)",      H2C_CHANNEL_PERFORMANCE_DATA_COUNT_0XCC},
    {"H2C Channel Performance Data Count (0xD0)",      H2C_CHANNEL_PERFORMANCE_DATA_COUNT_0XD0},
    {"", -1}
};

#define    C2H_CHANNEL_IDENTIFIER_0X00                  (0x1000 + 0x00)
#define    C2H_CHANNEL_CONTROL_0X04                     (0x1000 + 0x04)
#define    C2H_CHANNEL_CONTROL_0X08                     (0x1000 + 0x08)
#define    C2H_CHANNEL_CONTROL_0X0C                     (0x1000 + 0x0C)
#define    C2H_CHANNEL_STATUS_0X40                      (0x1000 + 0x40)
#define    C2H_CHANNEL_STATUS_0X44                      (0x1000 + 0x44)
#define    C2H_CHANNEL_COMPLETED_DESCRIPTOR_COUNT_0X48  (0x1000 + 0x48)
#define    C2H_CHANNEL_ALIGNMENTS_0X4C                  (0x1000 + 0x4C)
#define    C2H_POLL_MODE_LOW_WRITE_BACK_ADDRESS_0X88    (0x1000 + 0x88)
#define    C2H_POLL_MODE_HIGH_WRITE_BACK_ADDRESS_0X8C   (0x1000 + 0x8C)
#define    C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X90       (0x1000 + 0x90)
#define    C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X94       (0x1000 + 0x94)
#define    C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X98       (0x1000 + 0x98)
#define    C2H_CHANNEL_PERFORMANCE_MONITOR_CONTROL_0XC0 (0x1000 + 0xC0)
#define    C2H_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC4     (0x1000 + 0xC4)
#define    C2H_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC8     (0x1000 + 0xC8)
#define    C2H_CHANNEL_PERFORMANCE_DATA_COUNT_0XCC      (0x1000 + 0xCC)
#define    C2H_CHANNEL_PERFORMANCE_DATA_COUNT_0XD0      (0x1000 + 0xD0)

struct reginfo reg_c2h[] = {
    {"C2H Channel Identifier (0x00)",                  C2H_CHANNEL_IDENTIFIER_0X00},
    {"C2H Channel Control (0x04)",                     C2H_CHANNEL_CONTROL_0X04},
    {"C2H Channel Control (0x08)",                     C2H_CHANNEL_CONTROL_0X08},
    {"C2H Channel Control (0x0C)",                     C2H_CHANNEL_CONTROL_0X0C},
    {"C2H Channel Status (0x40)",                      C2H_CHANNEL_STATUS_0X40},
    {"C2H Channel Status (0x44)",                      C2H_CHANNEL_STATUS_0X44},
    {"C2H Channel Completed Descriptor Count (0x48)",  C2H_CHANNEL_COMPLETED_DESCRIPTOR_COUNT_0X48},
    {"C2H Channel Alignments (0x4C)",                  C2H_CHANNEL_ALIGNMENTS_0X4C},
    {"C2H Poll Mode Low Write Back Address (0x88)",    C2H_POLL_MODE_LOW_WRITE_BACK_ADDRESS_0X88},
    {"C2H Poll Mode High Write Back Address (0x8C)",   C2H_POLL_MODE_HIGH_WRITE_BACK_ADDRESS_0X8C},
    {"C2H Channel Interrupt Enable Mask (0x90)",       C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X90},
    {"C2H Channel Interrupt Enable Mask (0x94)",       C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X94},
    {"C2H Channel Interrupt Enable Mask (0x98)",       C2H_CHANNEL_INTERRUPT_ENABLE_MASK_0X98},
    {"C2H Channel Performance Monitor Control (0xC0)", C2H_CHANNEL_PERFORMANCE_MONITOR_CONTROL_0XC0},
    {"C2H Channel Performance Cycle Count (0xC4)",     C2H_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC4},
    {"C2H Channel Performance Cycle Count (0xC8)",     C2H_CHANNEL_PERFORMANCE_CYCLE_COUNT_0XC8},
    {"C2H Channel Performance Data Count (0xCC)",      C2H_CHANNEL_PERFORMANCE_DATA_COUNT_0XCC},
    {"C2H Channel Performance Data Count (0xD0)",      C2H_CHANNEL_PERFORMANCE_DATA_COUNT_0XD0},
    {"", -1}
};

#define IRQ_BLOCK_IDENTIFIER_0X00                    (0x2000 + 0x00)
#define IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X04    (0x2000 + 0x04)
#define IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X08    (0x2000 + 0x08)
#define IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X0C    (0x2000 + 0x0C)
#define IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X10 (0x2000 + 0x10)
#define IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X14 (0x2000 + 0x14)
#define IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X18 (0x2000 + 0x18)
#define IRQ_BLOCK_USER_INTERRUPT_REQUEST_0X40        (0x2000 + 0x40)
#define IRQ_BLOCK_CHANNEL_INTERRUPT_REQUEST_0X44     (0x2000 + 0x44)
#define IRQ_BLOCK_USER_INTERRUPT_PENDING_0X48        (0x2000 + 0x48)
#define IRQ_BLOCK_CHANNEL_INTERRUPT_PENDING_0X4C     (0x2000 + 0x4C)
#define IRQ_BLOCK_USER_VECTOR_NUMBER_0X80            (0x2000 + 0x80)
#define IRQ_BLOCK_USER_VECTOR_NUMBER_0X84            (0x2000 + 0x84)
#define IRQ_BLOCK_USER_VECTOR_NUMBER_0X88            (0x2000 + 0x88)
#define IRQ_BLOCK_USER_VECTOR_NUMBER_0X8C            (0x2000 + 0x8C)
#define IRQ_BLOCK_CHANNEL_VECTOR_NUMBER_0XA0         (0x2000 + 0xA0)
#define IRQ_BLOCK_CHANNEL_VECTOR_NUMBER_0XA4         (0x2000 + 0xA4)

struct reginfo reg_irq[] = {
    {"IRQ Block Identifier (0x00)",                    IRQ_BLOCK_IDENTIFIER_0X00},
    {"IRQ Block User Interrupt Enable Mask (0x04)",    IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X04},
    {"IRQ Block User Interrupt Enable Mask (0x08)",    IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X08},
    {"IRQ Block User Interrupt Enable Mask (0x0C)",    IRQ_BLOCK_USER_INTERRUPT_ENABLE_MASK_0X0C},
    {"IRQ Block Channel Interrupt Enable Mask (0x10)", IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X10},
    {"IRQ Block Channel Interrupt Enable Mask (0x14)", IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X14},
    {"IRQ Block Channel Interrupt Enable Mask (0x18)", IRQ_BLOCK_CHANNEL_INTERRUPT_ENABLE_MASK_0X18},
    {"IRQ Block User Interrupt Request (0x40)",        IRQ_BLOCK_USER_INTERRUPT_REQUEST_0X40},
    {"IRQ Block Channel Interrupt Request (0x44)",     IRQ_BLOCK_CHANNEL_INTERRUPT_REQUEST_0X44},
    {"IRQ Block User Interrupt Pending (0x48)",        IRQ_BLOCK_USER_INTERRUPT_PENDING_0X48},
    {"IRQ Block Channel Interrupt Pending (0x4C)",     IRQ_BLOCK_CHANNEL_INTERRUPT_PENDING_0X4C},
    {"IRQ Block User Vector Number (0x80)",            IRQ_BLOCK_USER_VECTOR_NUMBER_0X80},
    {"IRQ Block User Vector Number (0x84)",            IRQ_BLOCK_USER_VECTOR_NUMBER_0X84},
    {"IRQ Block User Vector Number (0x88)",            IRQ_BLOCK_USER_VECTOR_NUMBER_0X88},
    {"IRQ Block User Vector Number (0x8C)",            IRQ_BLOCK_USER_VECTOR_NUMBER_0X8C},
    {"IRQ Block Channel Vector Number (0xA0)",         IRQ_BLOCK_CHANNEL_VECTOR_NUMBER_0XA0},
    {"IRQ Block Channel Vector Number (0xA4)",         IRQ_BLOCK_CHANNEL_VECTOR_NUMBER_0XA4},
    {"", -1}
};


#define CONFIG_BLOCK_IDENTIFIER_0X00                 (0x3000 + 0x00)
#define CONFIG_BLOCK_BUSDEV_0X04                     (0x3000 + 0x04)
#define CONFIG_BLOCK_PCIE_MAX_PAYLOAD_SIZE_0X08      (0x3000 + 0x08)
#define CONFIG_BLOCK_PCIE_MAX_READ_REQUEST_SIZE_0X0C (0x3000 + 0x0C)
#define CONFIG_BLOCK_SYSTEM_ID_0X10                  (0x3000 + 0x10)
#define CONFIG_BLOCK_MSI_ENABLE_0X14                 (0x3000 + 0x14)
#define CONFIG_BLOCK_PCIE_DATA_WIDTH_0X18            (0x3000 + 0x18)
#define CONFIG_PCIE_CONTROL_0X1C                     (0x3000 + 0x1C)
#define CONFIG_AXI_USER_MAX_PAYLOAD_SIZE_0X40        (0x3000 + 0x40)
#define CONFIG_AXI_USER_MAX_READ_REQUEST_SIZE_0X44   (0x3000 + 0x44)
#define CONFIG_WRITE_FLUSH_TIMEOUT_0X60              (0x3000 + 0x60)

struct reginfo reg_config[] = {
    {"Config Block Identifier (0x00)",                 CONFIG_BLOCK_IDENTIFIER_0X00},
    {"Config Block BusDev (0x04)",                     CONFIG_BLOCK_BUSDEV_0X04},
    {"Config Block PCIE Max Payload Size (0x08)",      CONFIG_BLOCK_PCIE_MAX_PAYLOAD_SIZE_0X08},
    {"Config Block PCIE Max Read Request Size (0x0C)", CONFIG_BLOCK_PCIE_MAX_READ_REQUEST_SIZE_0X0C},
    {"Config Block System ID (0x10)",                  CONFIG_BLOCK_SYSTEM_ID_0X10},
    {"Config Block MSI Enable (0x14)",                 CONFIG_BLOCK_MSI_ENABLE_0X14},
    {"Config Block PCIE Data Width (0x18)",            CONFIG_BLOCK_PCIE_DATA_WIDTH_0X18},
    {"Config PCIE Control (0x1C)",                     CONFIG_PCIE_CONTROL_0X1C},
    {"Config AXI User Max Payload Size (0x40)",        CONFIG_AXI_USER_MAX_PAYLOAD_SIZE_0X40},
    {"Config AXI User Max Read Request Size (0x44)",   CONFIG_AXI_USER_MAX_READ_REQUEST_SIZE_0X44},
    {"Config Write Flush Timeout (0x60)",              CONFIG_WRITE_FLUSH_TIMEOUT_0X60},
    {"", -1}
};

#define H2C_SGDMA_IDENTIFIER_0X00              (0x4000 + 0x00)
#define H2C_SGDMA_DESCRIPTOR_LOW_ADDRESS_0X80  (0x4000 + 0x80)
#define H2C_SGDMA_DESCRIPTOR_HIGH_ADDRESS_0X84 (0x4000 + 0x84)
#define H2C_SGDMA_DESCRIPTOR_ADJACENT_0X88     (0x4000 + 0x88)
#define H2C_SGDMA_DESCRIPTOR_CREDITS_0X8C      (0x4000 + 0x8C)

struct reginfo reg_h2c_sgdma[] = {
    {"H2C SGDMA Identifier (0x00)",              H2C_SGDMA_IDENTIFIER_0X00},
    {"H2C SGDMA Descriptor Low Address (0x80)",  H2C_SGDMA_DESCRIPTOR_LOW_ADDRESS_0X80},
    {"H2C SGDMA Descriptor High Address (0x84)", H2C_SGDMA_DESCRIPTOR_HIGH_ADDRESS_0X84},
    {"H2C SGDMA Descriptor Adjacent (0x88)",     H2C_SGDMA_DESCRIPTOR_ADJACENT_0X88},
    {"H2C SGDMA Descriptor Credits (0x8C)",      H2C_SGDMA_DESCRIPTOR_CREDITS_0X8C},
    {"", -1}
};

#define C2H_SGDMA_IDENTIFIER_0X00              (0x5000 + 0x00)
#define C2H_SGDMA_DESCRIPTOR_LOW_ADDRESS_0X80  (0x5000 + 0x80)
#define C2H_SGDMA_DESCRIPTOR_HIGH_ADDRESS_0X84 (0x5000 + 0x84)
#define C2H_SGDMA_DESCRIPTOR_ADJACENT_0X88     (0x5000 + 0x88)
#define C2H_SGDMA_DESCRIPTOR_CREDITS_0X8C      (0x5000 + 0x8C)
  
struct reginfo reg_c2h_sgdma[] = {
    {"C2H SGDMA Identifier (0x00)",              C2H_SGDMA_IDENTIFIER_0X00},
    {"C2H SGDMA Descriptor Low Address (0x80)",  C2H_SGDMA_DESCRIPTOR_LOW_ADDRESS_0X80},
    {"C2H SGDMA Descriptor High Address (0x84)", C2H_SGDMA_DESCRIPTOR_HIGH_ADDRESS_0X84},
    {"C2H SGDMA Descriptor Adjacent (0x88)",     C2H_SGDMA_DESCRIPTOR_ADJACENT_0X88},
    {"C2H SGDMA Descriptor Credits (0x8C)",      C2H_SGDMA_DESCRIPTOR_CREDITS_0X8C},
    {"", -1}
};


#define SGDMA_IDENTIFIER_REGISTERS_0X00          (0x6000 + 0x00)
#define SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X10   (0x6000 + 0x10)
#define SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X14   (0x6000 + 0x14)
#define SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X18   (0x6000 + 0x18)
#define SGDMA_DESCRIPTOR_CREDIT_MODE_ENABLE_0X20 (0x6000 + 0x20)
#define SG_DESCRIPTOR_MODE_ENABLE_REGISTER_0X24  (0x6000 + 0x24)
#define SG_DESCRIPTOR_MODE_ENABLE_REGISTER_0X28  (0x6000 + 0x28)

struct reginfo reg_common_sgdma[] = {
    {"SGDMA Identifier Registers (0x00)",          SGDMA_IDENTIFIER_REGISTERS_0X00},
    {"SGDMA Descriptor Control Register (0x10)",   SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X10},
    {"SGDMA Descriptor Control Register (0x14)",   SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X14},
    {"SGDMA Descriptor Control Register (0x18)",   SGDMA_DESCRIPTOR_CONTROL_REGISTER_0X18},
    {"SGDMA Descriptor Credit Mode Enable (0x20)", SGDMA_DESCRIPTOR_CREDIT_MODE_ENABLE_0X20},
    {"SG Descriptor Mode Enable Register (0x24)",  SG_DESCRIPTOR_MODE_ENABLE_REGISTER_0X24},
    {"SG Descriptor Mode Enable Register (0x28)",  SG_DESCRIPTOR_MODE_ENABLE_REGISTER_0X28},
    {"", -1}
};

#define MSI_X_VECTOR0_MESSAGE_LOWER_ADDRESS  (0x8000 + 0x00)
#define MSI_X_VECTOR0_MESSAGE_UPPER_ADDRESS  (0x8000 + 0x04)
#define MSI_X_VECTOR0_MESSAGE_DATA           (0x8000 + 0x08)
#define MSI_X_VECTOR0_CONTROL                (0x8000 + 0x0C)
#define MSI_X_VECTOR31_MESSAGE_LOWER_ADDRESS (0x8000 + 0x1F0)
#define MSI_X_VECTOR31_MESSAGE_UPPER_ADDRESS (0x8000 + 0x1F4)
#define MSI_X_VECTOR31_MESSAGE_DATA          (0x8000 + 0x1F8)
#define MSI_X_VECTOR31_CONTROL               (0x8000 + 0x1FC)
#define MSI_X_PENDING_BIT_ARRAY              (0x8000 + 0xFE0)

struct reginfo reg_msix_vector[] = {
    {"MSI-X vector0 message lower address.",  MSI_X_VECTOR0_MESSAGE_LOWER_ADDRESS},
    {"MSI-X vector0 message upper address.",  MSI_X_VECTOR0_MESSAGE_UPPER_ADDRESS},
    {"MSI-X vector0 message data.",           MSI_X_VECTOR0_MESSAGE_DATA},
    {"MSI-X vector0 control.",                MSI_X_VECTOR0_CONTROL},
    {"MSI-X vector31 message lower address.", MSI_X_VECTOR31_MESSAGE_LOWER_ADDRESS},
    {"MSI-X vector31 message upper address.", MSI_X_VECTOR31_MESSAGE_UPPER_ADDRESS},
    {"MSI-X vector31 message data.",          MSI_X_VECTOR31_MESSAGE_DATA},
    {"MSI-X vector31 control.",               MSI_X_VECTOR31_CONTROL},
    {"MSI-X Pending Bit Array",               MSI_X_PENDING_BIT_ARRAY},
    {"", -1}
};


void dump_registers(int dumpflag, int on) {
    printf("==== Register Dump[%d] Start ====\n", on);
    if (dumpflag & DUMPREG_GENERAL) dump_reginfo(reg_general);
    if (dumpflag & DUMPREG_RX) dump_reginfo(reg_rx);
    if (dumpflag & DUMPREG_TX) dump_reginfo(reg_tx);
    if (dumpflag & XDMA_REG_H2C) xdma_reginfo(reg_h2c);
    if (dumpflag & XDMA_REG_C2H) xdma_reginfo(reg_c2h);
    if (dumpflag & XDMA_REG_IRQ) xdma_reginfo(reg_irq);
    if (dumpflag & XDMA_REG_CON) xdma_reginfo(reg_config);
    if (dumpflag & XDMA_REG_H2CS) xdma_reginfo(reg_h2c_sgdma);
    if (dumpflag & XDMA_REG_C2HS) xdma_reginfo(reg_c2h_sgdma);
    if (dumpflag & XDMA_REG_SCOM) xdma_reginfo(reg_common_sgdma);
    if (dumpflag & XDMA_REG_MSIX) xdma_reginfo(reg_msix_vector);
    printf("==== Register Dump[%d] End ====\n", on);
}

sysclock_t get_sys_count() {

    return ((uint64_t)get_register(REG_SYS_COUNT_HIGH) << 32) | get_register(REG_SYS_COUNT_LOW);
}

sysclock_t get_tx_timestamp(int timestamp_id) {

    switch (timestamp_id) {
    case 1:
        return ((uint64_t)get_register(REG_TX_TIMESTAMP1_HIGH) << 32) | get_register(REG_TX_TIMESTAMP1_LOW);
    case 2:
        return ((uint64_t)get_register(REG_TX_TIMESTAMP2_HIGH) << 32) | get_register(REG_TX_TIMESTAMP2_LOW);
    case 3:
        return ((uint64_t)get_register(REG_TX_TIMESTAMP3_HIGH) << 32) | get_register(REG_TX_TIMESTAMP3_LOW);
    case 4:
        return ((uint64_t)get_register(REG_TX_TIMESTAMP4_HIGH) << 32) | get_register(REG_TX_TIMESTAMP4_LOW);
    default:
        return 0;
    }
}

int32_t fn_show_register_genArgument(int32_t argc, const char *argv[]) {

    dump_registers(DUMPREG_GENERAL, 1);
    return 0;
}

int32_t fn_show_register_rxArgument(int32_t argc, const char *argv[]) {

    dump_registers(DUMPREG_RX, 1);
    return 0;
}

int32_t fn_show_register_txArgument(int32_t argc, const char *argv[]) {

    dump_registers(DUMPREG_TX, 1);
    return 0;
}

int32_t fn_show_register_h2cArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_H2C, 1);
    return 0;
}

int32_t fn_show_register_c2hArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_C2H, 1);
    return 0;
}

int32_t fn_show_register_irqArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_IRQ, 1);
    return 0;
}

int32_t fn_show_register_configArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_CON, 1);
    return 0;
}

int32_t fn_show_register_h2c_sgdmaArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_H2CS, 1);
    return 0;
}

int32_t fn_show_register_c2h_sgdmaArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_C2HS, 1);
    return 0;
}

int32_t fn_show_register_common_sgdmaArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_SCOM, 1);
    return 0;
}

int32_t fn_show_register_msix_vectorArgument(int32_t argc, const char *argv[]) {

    dump_registers(XDMA_REG_MSIX, 1);
    return 0;
}

int fn_show_registerArgument(int argc, const char *argv[]) {

    if(argc <= 0) {
        printf("%s needs a parameter\r\n", __func__);
        return ERR_PARAMETER_MISSED;
    }

    for (int index = 0; showRegisterArgument_tbl[index].name; index++) {
        if (!strcmp(argv[0], showRegisterArgument_tbl[index].name)) {
            showRegisterArgument_tbl[index].fP(argc, argv);
            return 0;
        }
    }

    return ERR_INVALID_PARAMETER;
}

int32_t char_to_hex(char c) {
    if ((c >= '0') && (c <= '9')) {
        return (c - 48);
    } else if ((c >= 'A') && (c <= 'F')) {
        return (c - 55);
    } else if ((c >= 'a') && (c <= 'f')) {
        return (c - 87);
    } else {
        return -1;
    }
}

int32_t str_to_hex(char *str, int32_t *n) {
    int32_t i;
    int32_t len;
    int32_t v = 0;

    len = strlen(str);
    for (i = 0; i < len; i++) {
        if (!isxdigit(str[i])) {
            return -1;
        }
        v = v * 16 + char_to_hex(str[i]);
    }

    *n = v;
    return 0;
}

int fn_set_register_genArgument(int argc, const char *argv[]) {

    int32_t addr = 0x0010; // Scratch Register
    int32_t value = 0x95302342;

    if(argc > 0) {
        if (str_to_hex((char *)argv[0], &addr) != 0) {
            printf("Invalid parameter: %s\r\n", argv[0]);
            return ERR_INVALID_PARAMETER;
        }
        if(argc > 1) {
            if (str_to_hex((char *)argv[1], &value) != 0) {
                printf("Invalid parameter: %s\r\n", argv[1]);
                return ERR_INVALID_PARAMETER;
            }
        }
    }

    if(addr % 4) {
        printf("The address value(0x%08x) does not align with 4-byte alignment.\r\n", addr);
        return ERR_INVALID_PARAMETER;
    }

    set_register(addr, value);
    printf("address(%08x): %08x\n", addr, get_register(addr));

    return 0;
}

#define XDMA_CONTROL_REGISTER_DEV    "/dev/xdma0_control"

int set_xdma_register(int offset, uint32_t val) {

    return xdma_api_wr_register(XDMA_CONTROL_REGISTER_DEV, offset, 'w', val);
}

uint32_t get_xdma_register(int offset) {

    uint32_t read_val = 0;
    xdma_api_rd_register(XDMA_CONTROL_REGISTER_DEV, offset, 'w', &read_val);

    return read_val;
}

int fn_set_xdma_registerArgument(int argc, const char *argv[], int offset) {

    int32_t addr = 0x0; // Scratch Register
    int32_t value = 0x0;

    if(argc > 0) {
        if (str_to_hex((char *)argv[0], &addr) != 0) {
            printf("Invalid parameter: %s\r\n", argv[0]);
            return ERR_INVALID_PARAMETER;
        }
        if(argc > 1) {
            if (str_to_hex((char *)argv[1], &value) != 0) {
                printf("Invalid parameter: %s\r\n", argv[1]);
                return ERR_INVALID_PARAMETER;
            }
        } else {
            printf("value are missed.\r\n");
            return ERR_PARAMETER_MISSED;
        }
    } else {
        printf("address & value are missed.\r\n");
        return ERR_PARAMETER_MISSED;
    }

    if(addr % 4) {
        printf("The address value(0x%08x) does not align with 4-byte alignment.\r\n", addr);
        return ERR_INVALID_PARAMETER;
    }

    set_xdma_register(addr + offset, value);
    printf("address(%08x): %08x\n", addr + offset, get_xdma_register(addr + offset));

    return 0;
}

int fn_set_register_msix_vectorArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x8000);
}

int fn_set_register_common_sgdmaArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x6000);
}

int fn_set_register_c2h_sgdmaArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x5000);
}

int fn_set_register_h2c_sgdmaArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x4000);
}

int fn_set_register_configArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x3000);
}

int fn_set_register_irqArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x2000);
}

int fn_set_register_c2hArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x1000);
}

int fn_set_register_h2cArgument(int argc, const char *argv[]) {
    return fn_set_xdma_registerArgument(argc, argv, 0x0000);
}

int fn_set_registerArgument(int argc, const char *argv[]) {

    if(argc <= 0) {
        printf("%s needs a parameter\r\n", __func__);
        return ERR_PARAMETER_MISSED;
    }

    for (int index = 0; setRegisterArgument_tbl[index].name; index++) {
        if (!strcmp(argv[0], setRegisterArgument_tbl[index].name)) {
            argv++, argc--;
            setRegisterArgument_tbl[index].fP(argc, argv);
            return 0;
        }
    }

    return ERR_INVALID_PARAMETER;
}

int process_main_showCmd(int argc, const char *argv[], 
                             menu_command_t *menu_tbl) {

    if(argc <= 1) {
        print_argumentWarningMessage(argc, argv, menu_tbl, NO_ECHO);
        return ERR_PARAMETER_MISSED;
    }
    argv++, argc--;
    for (int index = 0; showArgument_tbl[index].name; index++)
        if (!strcmp(argv[0], showArgument_tbl[index].name)) {
            argv++, argc--;
            showArgument_tbl[index].fP(argc, argv);
            return 0;
        }

    return ERR_INVALID_PARAMETER;
}

int process_main_setCmd(int argc, const char *argv[], 
                             menu_command_t *menu_tbl) {

    if(argc <= 3) {
        print_argumentWarningMessage(argc, argv, menu_tbl, NO_ECHO);
        return ERR_PARAMETER_MISSED;
    }
    argv++, argc--;
    for (int index = 0; setArgument_tbl[index].name; index++)
        if (!strcmp(argv[0], setArgument_tbl[index].name)) {
            argv++, argc--;
            setArgument_tbl[index].fP(argc, argv);
            return 0;
        }

    return ERR_INVALID_PARAMETER;
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
    int t_argc;
    char **pav = NULL;

    for(id=0; id<argc; id++) {
        debug_printf("argv[%d] : %s\n", id, argv[id]);
    }
    debug_printf("\n");

    t_argc = argc, pav = argv;
    pav++, t_argc--;

    return command_parser(t_argc, pav);
}

