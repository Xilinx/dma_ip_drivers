/****************************************************************************************
 *
 * QDMA Maverick driver. 
 *
 * Handle the spirent specific overhead
 *
 * Date: Fri 7th June 2024
 * Initial Blame: Matt Philpott
 *
 * Notes:
 * Essentially the bits from rogue.c that deal with the overhead hdr
 *
 */

#ifndef __QDMA_SPIRENT__
#define __QDMA_SPIRENT__

#include <rte_sp_mbuf.h>
#include "qdma.h"

#define STATIC static 
#define INLINE inline

/****************************************************************************************
 *
 * STC RX cut-through overhead
 *
 */

typedef struct ct_rxover
{
    volatile uint32_t reserved:8;
    volatile uint32_t flags:8;
    volatile uint32_t block_len:16;
    volatile uint32_t ts_lo;
    volatile uint32_t ts_hi:16;
    volatile uint32_t len:16;
} __attribute__ ((__packed__)) ct_rxover_t ;

#define RX_OH_FCSERR      0x01
#define RX_OH_CMP         0x02
#define RX_OH_SIG         0x04
#define RX_OH_BCOM_QUAL   0x08
#define RX_OH_CAP_QUAL    0x10
#define RX_OH_PRBSERR     0x20

/* return errors */
#define RX_OH_RUNT        0x40
#define RX_OH_MEM_ERR     0x80

/****************************************************************************************
 * STC Tx cut-through overhead
 *
 */
typedef struct ct_txover
{
    volatile uint16_t frame_length;
    volatile uint16_t block_length;
    volatile unsigned seq: 12;
    volatile unsigned spare1: 2;
    volatile unsigned hwfcs: 1;
    volatile unsigned TSEnA: 1;
    volatile unsigned pfc: 4;
    volatile unsigned prelen: 6;
    volatile unsigned hwts: 4;
    volatile unsigned spare2: 2;
} __attribute__ ((__packed__)) ct_txover_t ;


STATIC INLINE int qdma_spirent_tx_oh(struct rte_mbuf *tx_pkt, struct qdma_pkt_stats *stats) 
{
    int len;
    ct_txover_t *hdr = NULL; // Keep compiler happy

    rte_pktmbuf_dump(stdout, tx_pkt, tx_pkt->data_len);

    len = rte_pktmbuf_pkt_len(tx_pkt);
    hdr = (ct_txover_t *)rte_pktmbuf_prepend (tx_pkt, sizeof(ct_txover_t));
    if(hdr == NULL) {
        stats->FCDiscards++;
        return 1;
    }
    memset((void *)hdr, 0, sizeof(ct_txover_t));

    int seq =0;
    int TsEn = 0;
    
    rte_sp_mbuf_ts_t *ts = rte_sp_mbuf_dyn_ts(tx_pkt);
    TsEn = ts->ts.en;
    seq = ts->ts.seq;
    ts->ts.en = 0;

    // Append FCS and clear
    hdr->hwfcs = 1;
    *((uint32_t *) rte_pktmbuf_append (tx_pkt, RTE_ETHER_CRC_LEN)) = 0;

    // Timestamp sequence number
    if(TsEn)
    {
        //DB_ERR("Insert timestamp seq %u\n", seq);
        hdr->TSEnA = 1;
        hdr->seq = seq;
    }
    
    hdr->frame_length = len - sizeof(ct_txover_t) ;
    hdr->block_length = hdr->frame_length + sizeof(ct_txover_t);

    return 0;
}


STATIC INLINE int qdma_spirent_rx_oh(struct rte_mbuf *rx_pkt, struct qdma_pkt_stats *stats)  
{
    ct_rxover_t     *hdr;
    int             len;
    uint32_t        flags = 0;

    hdr = (ct_rxover_t *)(((uint8_t *)rx_pkt->buf_addr) + rx_pkt->data_off);

    len = hdr->len;
    flags = hdr->flags;

    /*
     * Check for runts
     */
    if(unlikely(len < 64))
    {
        PMD_DRV_LOG(ERR, "Runt frame (len %d)\n", len);
        stats->runts++;
        return 1;
    }

    if(flags & RX_OH_FCSERR)
    {
        PMD_DRV_LOG(ERR, "FCS Error (len %d)(flags 0x%x)\n", len, flags);
        stats->CRCErrors++;
        return 1;
    }

    len += (sizeof(ct_rxover_t) - 4);

    // set the mbuf length
    if (NULL == rte_pktmbuf_append(rx_pkt, (uint16_t)len)) {
        stats->FCDiscards++;
        return 1;
    }

    // strip the phx overhead
    rte_pktmbuf_adj(rx_pkt, sizeof(ct_rxover_t));

    // passing hw timestamp value to upper layers
    rte_sp_mbuf_dyn_ts(rx_pkt)->timestamp = ((uint64_t)hdr->ts_hi << 32) | hdr->ts_lo;
   
    return 0;
}

#endif /* __QDMA_SPIRENT__ */
