/*
*
*/

#ifndef __ERROR_DEFINE_H__
#define __ERROR_DEFINE_H__

#define XST_SUCCESS            (0)
#define XST_FAILURE            (1)

#define ERR_TCP_PORT_FILE            -2
#define ERR_TX_SERVER_NOT_RESPONSE   -3
#define ERR_NO_DATA_OR_FAIL_TO_READ  -4
#define ERR_WAIT_TIME_OUT            -5
#define ERR_FAIL_TO_FILE_OPEN        -6
#define ERR_FAIL_TO_MEMORY_ALLOC     -7
#define ERR_FAIL_TO_PCAP_FILE_OPEN   -8
#define ERR_FAIL_TO_HANDLE_PCAP_FN   -9
#define ERR_DATA_CHANNEL_IS_IN_USE   -10

#define ERR_FAIL_TO_PARSE_COMMAND    -11
#define ERR_PARAMETER_MISSED         -12
#define ERR_INVALID_PARAMETER        -13
#define ERR_CHANNEL_ALREADY_OPEN     -14
#define ERR_CHANNEL_ALREADY_CLOSE    -15
#define ERR_FAIL_TO_CHANNEL_OPEN     -16
#define ERR_NOT_SUPPORTED_COMMAND    -17
#define ERR_FAIL_PROTECT_TRAFFIC     -18
#define ERR_FAIL_GET_ROOM_E_CNT      -19
#define ERR_FAIL_GET_CONTAINER       -20
#define ERR_FAIL_TO_WRITE_DEVICE     -21
#define ERR_FAIL_TO_RST_BUF_IDX      -22
#define ERR_INVALID_DATA_FORMAT      -23
#define ERR_PACKET_SIZE_LONG         -24
#define ERR_DATA_IS_NULL             -25
#define ERR_FAIL_OPEN_CTRL_CHAN      -26
#define ERR_FILE_IS_NOT_DEFINED      -27
#define ERR_FAIL_TO_FREE_ROOM        -28
#define ERR_FAIL_TO_FREE_INDEX       -29
#define ERR_FAIL_TO_READ_DEVICE      -30
#define ERR_FAIL_READ_REGISTER       -31
#define ERR_NOT_ALIGN_64BITS         -32
#define ERR_FAIL_WRITE_REGISTER      -33
#define ERR_NOT_ALIGNED              -34
#define ERR_OUT_OF_ADDR_RANGE        -35
#define ERR_DEVICE_NOT_OPEN          -36
#define ERR_OUT_OF_DATA_RANGE        -37
#define ERR_OUT_OF_AUTHORITY         -38
#define ERR_FAIL_TO_USER_INFO        -39
#define ERR_NOT_AVAILABLE_TX         -40

#define FAIL_TO_READ_QDR_MEMORY      -41
#define FAIL_TO_WRITE_QDR_MEMORY     -42
#define ERR_UNDER_QDR_MEMORY_TEST    -43
#define ERR_FAIL_WRITE_EEPROM        -44
#define ERR_FAIL_READ_EEPROM         -45

#define ERR_FAIL_TO_SELECT_SOCKET    -51
#define ERR_FAIL_TO_CONNECT_SOCKET   -52
#define ERR_FAIL_TO_SEND_MSG_SOCKET  -53
#define ERR_FAIL_TO_OPEN_SOCKET      -54
#define ERR_FAIL_TO_BIND_SOCKET      -55
#define ERR_FAIL_TO_LISTEN_SOCKET    -56
#define ERR_FAIL_TO_ACCEPT_SOCKET    -57

#define ERR_ALREADY_TRANSMITTING     -61
#define ERR_ALREADY_STOPPED          -62

#define ERR_UNKNOWN_ETH_TYPE         -71

#endif // __ERROR_DEFINE_H__
