#include "stdint-gcc.h"

#define IPv4_HLEN(ipv4_p) (ipv4_p->hdr_len * 4)
#define IPv4_BODY_LEN(ipv4_p) (ipv4_p->len - IPv4_HLEN(ipv4_p))
#define IPv4_PAYLOAD(ipv4_p) (void*)((uint8_t*)ipv4_p + IPv4_HLEN(ipv4_p))

struct ipv4_header {
    // Comment that the bit fields are nibble-swapped by compiler.
    // And if set big-endian attribute to this entire ipv4 structure, Compiler refuses to compile.
    struct {
        uint8_t version:4;
        uint8_t hdr_len:4;
        uint8_t dscp:6;
        uint8_t ecn:2;
    } __attribute__((packed, scalar_storage_order("big-endian")));
    uint16_t len;
    uint16_t id;
    uint16_t flags:3;
    uint16_t frag_offset:13;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
    uint8_t opt[0];
} __attribute__((packed, scalar_storage_order("big-endian")));
