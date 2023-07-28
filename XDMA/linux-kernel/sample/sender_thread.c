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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>


#include "error_define.h"
#include "receiver_thread.h"
#include "platform_config.h"

#include "ethernet.h"
#include "ip.h"
#include "ipv4.h"
#include "icmp.h"
#include "udp.h"
#include "arp.h"
#include "gptp.h"

#include "util.h"
#include "tsn.h"

#include "../libxdma/api_xdma.h"

/******************** Constant Definitions **********************************/
#define HW_ADDR_LEN 6
#define IP_ADDR_LEN 4

#define FCS_LEN 4

#define RESERVED_TX_COUNT 4

static const char* myMAC = "\x00\x11\x22\x33\x44\x55";

stats_t tx_stats;

char tx_devname[MAX_DEVICE_NAME];
int tx_fd;

extern int tx_thread_run;

void packet_dump(BUF_POINTER buffer, int length);
BUF_POINTER get_reserved_tx_buffer();

static int enqueue(struct tsn_tx_buffer* tx);
static struct tsn_tx_buffer* dequeue();

static int enqueue(struct tsn_tx_buffer* tx) {
#ifndef DISABLE_TSN_QUEUE
    uint16_t prio = tx->metadata.vlan_prio;
    int res = tsn_queue_enqueue(tx, prio);
    if (res == 0) {
        buffer_pool_free((BUF_POINTER)tx);
    }
    return res;
#else
    transmit_tsn_packet(tx);
    return 1;
#endif
}

static struct tsn_tx_buffer* dequeue() {
    timestamp_t now = gptp_get_timestamp(get_sys_count());
    int queue_index = tsn_select_queue(now);
    if (queue_index < 0) {
        return NULL;
    }

    return tsn_queue_dequeue(queue_index);
}

int transmit_tsn_packet(struct tsn_tx_buffer* packet) {

    uint64_t mem_len = sizeof(packet->metadata) + packet->metadata.frame_length;
    uint64_t bytes_tr;
    int status = 0;

    if (mem_len >= MAX_BUFFER_LENGTH) {
        printf("%s - %p length(%ld) is out of range(%d)\r\n", __func__, packet, mem_len, MAX_BUFFER_LENGTH);
        buffer_pool_free((BUF_POINTER)packet);
        return XST_FAILURE;
    }

    status = xdma_api_write_from_buffer_with_fd(tx_devname, tx_fd, (char *)packet,
                                       mem_len, &bytes_tr);

    tx_stats.txPackets++;
    tx_stats.txBytes += bytes_tr;

    buffer_pool_free((BUF_POINTER)packet);

    return status;
}

static int process_packet(struct tsn_rx_buffer* rx) {

    uint8_t *buffer =(uint8_t *)rx;
    int tx_len;
    // Reuse RX buffer as TX
    struct tsn_tx_buffer* tx = (struct tsn_tx_buffer*)(buffer + sizeof(struct rx_metadata) - sizeof(struct tx_metadata));
    struct tx_metadata* tx_metadata = &tx->metadata;
    uint8_t* rx_frame = (uint8_t*)&rx->data;
    uint8_t* tx_frame = (uint8_t*)&tx->data;
    struct ethernet_header* rx_eth = (struct ethernet_header*)rx_frame;
    struct ethernet_header* tx_eth = (struct ethernet_header*)tx_frame;

    // make tx metadata
    // tx_metadata->vlan_tag = rx->metadata.vlan_tag;
    tx_metadata->timestamp_id = 0;
    tx_metadata->reserved = 0;

    // make ethernet frame
    memcpy(&(tx_eth->dmac), &(rx_eth->smac), 6);
    memcpy(&(tx_eth->smac), myMAC, 6);
    // tx_eth->type = rx_eth->type;

    tx_len = ETH_HLEN;

    // do arp, udp echo, etc.
    switch (rx_eth->type) {
#ifndef DISABLE_GPTP
    case ETH_TYPE_PTPv2:
        ;
        int len = process_gptp_packet(rx);
        if (len <= 0) { return XST_FAILURE; }
        tx_len += len;
        break;
#endif
    case ETH_TYPE_ARP: // arp
        ;
        struct arp_header* rx_arp = (struct arp_header*)ETH_PAYLOAD(rx_frame);
        struct arp_header* tx_arp = (struct arp_header*)ETH_PAYLOAD(tx_frame);
        if (rx_arp->opcode != ARP_OPCODE_ARP_REQUEST) { return XST_FAILURE; }

        // make arp packet
        // tx_arp->hw_type = rx_arp->hw_type;
        // tx_arp->proto_type = rx_arp->proto_type;
        // tx_arp->hw_size = rx_arp->hw_size;
        // tx_arp->proto_size = rx_arp->proto_size;
        tx_arp->opcode = ARP_OPCODE_ARP_REPLY;
        memcpy(tx_arp->target_hw, rx_arp->sender_hw, HW_ADDR_LEN);
        memcpy(tx_arp->sender_hw, myMAC, HW_ADDR_LEN);
        uint8_t sender_proto[4];
        memcpy(sender_proto, rx_arp->sender_proto, IP_ADDR_LEN);
        memcpy(tx_arp->sender_proto, rx_arp->target_proto, IP_ADDR_LEN);
        memcpy(tx_arp->target_proto, sender_proto, IP_ADDR_LEN);

        tx_len += ARP_HLEN;
        break;
    case ETH_TYPE_IPv4: // ip
        ;
        struct ipv4_header* rx_ipv4 = (struct ipv4_header*)ETH_PAYLOAD(rx_frame);
        struct ipv4_header* tx_ipv4 = (struct ipv4_header*)ETH_PAYLOAD(tx_frame);

        uint32_t src;

        // Fill IPv4 header
        // memcpy(tx_ipv4, rx_ipv4, IPv4_HLEN(rx_ipv4));
        src = rx_ipv4->dst;
        tx_ipv4->dst = rx_ipv4->src;
        tx_ipv4->src = src;
        tx_len += IPv4_HLEN(rx_ipv4);

        if (rx_ipv4->proto == IP_PROTO_ICMP) {
            struct icmp_header* rx_icmp = (struct icmp_header*)IPv4_PAYLOAD(rx_ipv4);

            if (rx_icmp->type != ICMP_TYPE_ECHO_REQUEST) { return XST_FAILURE; }

            struct icmp_header* tx_icmp = (struct icmp_header*)IPv4_PAYLOAD(tx_ipv4);
            unsigned long icmp_len = IPv4_BODY_LEN(rx_ipv4);

            // Fill ICMP header and body
            // memcpy(tx_icmp, rx_icmp, icmp_len);
            tx_icmp->type = ICMP_TYPE_ECHO_REPLY;
            icmp_checksum(tx_icmp, icmp_len);
            tx_len += icmp_len;

        } else if (rx_ipv4->proto == IP_PROTO_UDP){
            struct udp_header* rx_udp = (struct udp_header*)IPv4_PAYLOAD(rx_ipv4);
            if (rx_udp->dstport != 7) { return XST_FAILURE; }

            struct udp_header* tx_udp = IPv4_PAYLOAD(tx_ipv4);

            // Fill UDP header
            // memcpy(tx_udp, rx_udp, rx_udp->length);
            uint16_t srcport;
            srcport = rx_udp->dstport;
            tx_udp->dstport = rx_udp->srcport;
            tx_udp->srcport = srcport;
            tx_udp->checksum = 0;
            tx_len += rx_udp->length; // UDP.length contains header length
        } else {
            return XST_FAILURE;
        }
        break;
    default:
        printf_debug("Unknown type: %04x\n", rx_eth->type);
        return XST_FAILURE;
    }
    tx_metadata->frame_length = tx_len;
    if (enqueue(tx) > 0) {
        return XST_SUCCESS;
    }
    return XST_FAILURE;
}

static void periodic_process_ptp()
{
#ifdef DISABLE_GPTP
    return;
#endif
    if (1) {
        struct gptp_statistics_result stats[3];
        gptp_get_statistics(stats);

        struct gptp_statistics_result* offset = &stats[0];
        struct gptp_statistics_result* freq = &stats[1];
        struct gptp_statistics_result* delay = &stats[2];

        timestamp_t now = gptp_get_timestamp(get_sys_count());

        if (offset->num + freq->num + delay->num > 0) {
            printf_debug(
                "[%d.%03d]: "
                "rms %4d max %4d "
                "freq %+6d +/- %3d "
                "delay %5d +/- %3d\n",
                (int)(now / 1e9), (int)(now / 1e6) % 1000,
                offset->rms, offset->max_abs,
                freq->mean, freq->stdev,
                delay->mean, delay->stdev);
        }

        gptp_reset_statistics();
    }


    uint32_t size;
    struct tsn_tx_buffer* buffer;

    buffer = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (buffer == NULL) {
        printf_debug("Cannot get buffer for pdelay_req\n");
        return;
    }
    size = gptp_make_pdelay_req_packet(buffer);
    if (size == 0) {
        buffer_pool_free((BUF_POINTER)buffer);
        return;
    }
    enqueue(buffer);

    // gPTP Master part

    // Send announce packet
    buffer = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (buffer == NULL) {
        printf_debug("Cannot get buffer for announce\n");
        return;
    }
    size = gptp_make_announce_packet(buffer);
    if (size == 0) {
        buffer_pool_free((BUF_POINTER)buffer);
        return; // I am not a master
    }
    enqueue(buffer);

    // Send Sync packet
    buffer = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (buffer == NULL) {
        printf_debug("Cannot get buffer for sync\n");
        return;
    }
    size = gptp_make_sync_packet(buffer);
    transmit_tsn_packet(buffer);

    // Send Followup packet
    buffer = (struct tsn_tx_buffer*)get_reserved_tx_buffer();
    if (buffer == NULL) {
        printf_debug("Cannot get buffer for fup\n");
        return;
    }
    size = gptp_make_followup_packet(buffer);
    enqueue(buffer);
}

void sender_in_tsn_mode(char* devname, int fd, uint64_t size) {

    QueueElement buffer = NULL;
    double elapsedTime;
    struct timeval previousTime;
    struct timeval currentTime;
    int received_packet_count;
    int index;

    gettimeofday(&previousTime, NULL);
    while (tx_thread_run) {
        gettimeofday(&currentTime, NULL);
        elapsedTime = (currentTime.tv_sec - previousTime.tv_sec) + (currentTime.tv_usec - previousTime.tv_usec) / 1000000.0;
        if (elapsedTime >= 1.0) {
            periodic_process_ptp();
            memcpy(&previousTime, &currentTime, sizeof(struct timeval));
        }

        // Process RX
        received_packet_count = getQueueCount();

        if (received_packet_count > 0) {
            for(index=0; index<received_packet_count; index++) {
                buffer = NULL;
                buffer = xbuffer_dequeue();
                if(buffer == NULL) {
                    continue;
                }
                process_packet((struct tsn_rx_buffer*)buffer);
            }
        }
#ifndef DISABLE_TSN_QUEUE
        // Process TX
        for (int i = 0; i < 20; i += 1) {
            struct tsn_tx_buffer* tx_buffer = dequeue();
            if (tx_buffer == NULL) {
                break;
            }
            transmit_tsn_packet(tx_buffer);
        }
#endif
    }
}

void sender_in_normal_mode(char* devname, int fd, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;

    while (tx_thread_run) {
        buffer = NULL;
        buffer = xbuffer_dequeue();

        if(buffer == NULL) {
            continue;
        }

        if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                       size, &bytes_tr)) {
            continue;
        }

        tx_stats.txPackets++;
        tx_stats.txBytes += bytes_tr;

        buffer_pool_free((BUF_POINTER)buffer);
    }
}

void sender_in_loopback_mode(char* devname, int fd, char *fn, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;
    int infile_fd = -1;
    ssize_t rc;

    infile_fd = open(fn, O_RDONLY);
    if (infile_fd < 0) {
        printf("Unable to open input file %s, %d.\n", fn, infile_fd);
        return;
    }

    while (tx_thread_run) {
        buffer = NULL;
        buffer = xbuffer_dequeue();

        if(buffer == NULL) {
            continue;
        }

        lseek(infile_fd, 0, SEEK_SET);
        rc = read_to_buffer(fn, infile_fd, buffer, size, 0);
        if (rc < 0 || rc < size) {
            printf("%s - Error, read_to_buffer: size - %ld, rc - %ld.\n", __func__, size, rc);
            close(infile_fd);
            return;
        }

        if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                       size, &bytes_tr)) {
            printf("%s - Error, xdma_api_write_from_buffer_with_fd.\n", __func__);
            continue;
        }

        tx_stats.txPackets++;
        tx_stats.txBytes += bytes_tr;

        buffer_pool_free((BUF_POINTER)buffer);
    }
    close(infile_fd);
}

void sender_in_performance_mode(char* devname, int fd, char *fn, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;
    int infile_fd = -1;
    ssize_t rc;
    struct timeval previousTime, currentTime;
    double elapsedTime;

    infile_fd = open(fn, O_RDONLY);
    if (infile_fd < 0) {
        printf("Unable to open input file %s, %d.\n", fn, infile_fd);
        return;
    }

    buffer = (QueueElement)xdma_api_get_buffer(size);
    if(buffer == NULL) {
        close(infile_fd);
        return;
    }

    rc = read_to_buffer(fn, infile_fd, buffer, size, 0);
    if (rc < 0 || rc < size) {
        close(infile_fd);
        free(buffer);
        return;
    }

    gettimeofday(&previousTime, NULL);
    while (tx_thread_run) {
        gettimeofday(&currentTime, NULL);
        elapsedTime = (currentTime.tv_sec - previousTime.tv_sec) + (currentTime.tv_usec - previousTime.tv_usec) / 1000000.0;
        if (elapsedTime >= 0.00001) {
            if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                           size, &bytes_tr)) {
                continue;
            }

            tx_stats.txPackets++;
            tx_stats.txBytes += bytes_tr;
            gettimeofday(&previousTime, NULL);
        }
    }

    close(infile_fd);
    free(buffer);
}

void* sender_thread(void* arg) {

    tx_thread_arg_t* p_arg = (tx_thread_arg_t*)arg;

    printf(">>> %s(devname: %s, fn: %s, mode: %d, size: %d)\n", 
               __func__, p_arg->devname, p_arg->fn, 
               p_arg->mode, p_arg->size);

    memset(tx_devname, 0, MAX_DEVICE_NAME);
    memcpy(tx_devname, p_arg->devname, MAX_DEVICE_NAME);

    if(xdma_api_dev_open(p_arg->devname, 0 /* eop_flush */, &tx_fd)) {
        printf("FAILURE: Could not open %s. Make sure xdma device driver is loaded and you have access rights (maybe use sudo?).\n", p_arg->devname);
        printf("<<< %s\n", __func__);
        return NULL;
    }

    initialize_statistics(&tx_stats);

    switch(p_arg->mode) {
    case RUN_MODE_TSN:
        sender_in_tsn_mode(p_arg->devname, tx_fd, p_arg->size);
    break;
    case RUN_MODE_NORMAL:
        sender_in_normal_mode(p_arg->devname, tx_fd, p_arg->size);
    break;
    case RUN_MODE_LOOPBACK:
        sender_in_performance_mode(p_arg->devname, tx_fd, p_arg->fn, p_arg->size);
    break;
    case RUN_MODE_PERFORMANCE:
        sender_in_performance_mode(p_arg->devname, tx_fd, p_arg->fn, p_arg->size);
    break;
    default:
        printf("%s - Unknown mode(%d)\n", __func__, p_arg->mode);
    break;
    }

    close(tx_fd);
    printf("<<< %s()\n", __func__);

    return NULL;
}

