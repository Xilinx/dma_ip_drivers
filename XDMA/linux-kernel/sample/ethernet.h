#include "stdint-gcc.h"

#define ETH_HLEN (sizeof(struct ethernet_header))
#define ETH_PAYLOAD(ethernet_p) (void*)((uint8_t*)ethernet_p + ETH_HLEN)

enum ethernet_type {
    ETH_TYPE_ARP = 0x0806,
    ETH_TYPE_IPv4 = 0x0800,
    ETH_TYPE_IPv6 = 0x86DD,
    ETH_TYPE_VLAN = 0x8100,
    ETH_TYPE_PTPv2 = 0x88F7,
};

struct ethernet_header {
    uint8_t dmac[6];
    uint8_t smac[6];
    uint16_t type;
} __attribute__((packed, scalar_storage_order("big-endian")));
