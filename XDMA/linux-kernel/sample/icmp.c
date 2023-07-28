/*
 * icmp.c
 *
 *  Created on: Apr 24, 2023
 *      Author: pooky
 */

#include "icmp.h"

void icmp_checksum(struct icmp_header* icmp, uint16_t len) {
    icmp->checksum = 0;
    int32_t checksum = 0;
    uint16_t* ptr = (uint16_t*)icmp;
    while (len > 1) {
        checksum += *ptr++;
        len -= 2;
        if (checksum >> 16 > 0) {
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }

    if (len > 0) {
        checksum += *(uint8_t*)ptr << 8;
        if (checksum >> 16 > 0) {
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }

    // Finalise
    checksum = (checksum >> 16) + (checksum & 0xFFFF);
    checksum = (checksum >> 16) + (checksum & 0xFFFF);

    icmp->checksum = ~checksum;
}

