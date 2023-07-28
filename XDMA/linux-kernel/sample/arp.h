#include "stdint-gcc.h"

#define ARP_HLEN (sizeof(struct arp_header))

enum arp_opcode {
    ARP_OPCODE_ARP_REQUEST = 1,
    ARP_OPCODE_ARP_REPLY = 2,
    ARP_OPCODE_RARP_REQUEST = 3,
    ARP_OPCODE_RARP_REPLY = 4,
    ARP_OPCODE_DRARP_REQUEST = 5,
    ARP_OPCODE_DRARP_REPLY = 6,
    ARP_OPCODE_DRARP_ERROR = 7,
    ARP_OPCODE_INARP_REQUEST = 8,
    ARP_OPCODE_INARP_REPLY = 9,
};

struct arp_header { // Only for ethernet+ipv4
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t sender_hw[6];
    uint8_t sender_proto[4];
    uint8_t target_hw[6];
    uint8_t target_proto[4];
} __attribute__((packed, scalar_storage_order("big-endian")));
