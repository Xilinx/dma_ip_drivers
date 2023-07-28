#include "stdint-gcc.h"

#define UDP_HLEN (sizeof(struct udp_header))
#define UDP_PAYLOAD(udp_p) (void*)((uint8_t*)udp_p + UDP_HLEN)

struct udp_header {
    uint16_t srcport;
    uint16_t dstport;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed, scalar_storage_order("big-endian")));
