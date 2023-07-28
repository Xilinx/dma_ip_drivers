#include "stdint-gcc.h"

#define ICMP_HLEN (sizeof(struct icmp_header))
#define ICMP_PAYLOAD(icmp_p) (void*)((uint8_t*)icmp_p + ICMP_HLEN)

enum icmp_type {
    ICMP_TYPE_ECHO_REPLY = 0,
    ICMP_TYPE_ECHO_REQUEST = 8,
};

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

void icmp_checksum(struct icmp_header* icmp, uint16_t len);
