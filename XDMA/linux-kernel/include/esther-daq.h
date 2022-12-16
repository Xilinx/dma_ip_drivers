/**
 * ATCA V2 PCI ADC  device driver
 * Definitions for the Linux Device Driver
 *
 * Copyright 2016 - 2019 IPFN-Instituto Superior Tecnico, Portugal
 * Creation Date  2016-02-10
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence, available in 23 official languages of
 * the European Union, at:
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl-text-11-12
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 *
 */
#ifndef _ESTHER_DAQ_H_
#define _ESTHER_DAQ_H_
/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
//
/* ltoh: little to host */
/* htol: little to host */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x)       (x)
#define ltohs(x)       (x)
#define htoll(x)       (x)
#define htols(x)       (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x)     __bswap_32(x)
#define ltohs(x)     __bswap_16(x)
#define htoll(x)     __bswap_32(x)
#define htols(x)     __bswap_16(x)
#endif


//#define DMA_BUFFS 8 // Number of DMA ch0 buffs
//#define PCK_N_SAMPLES 1024
#define ADC_CHANNELS    32
#define INT_CHANNELS    8
#define N_ILOCK_PARAMS  10    // Interlock algortithm parameters

//SHAPI PICMG Register Set
struct shapi_device_info {
    uint32_t SHAPI_VERSION;
    uint32_t SHAPI_FIRST_MODULE_ADDRESS;
    uint32_t SHAPI_HW_IDS;
    uint32_t SHAPI_FW_IDS;
    uint32_t SHAPI_FW_VERSION;
    uint32_t SHAPI_FW_TIMESTAMP;
    uint32_t SHAPI_FW_NAME[3];
    uint32_t SHAPI_DEVICE_CAP;
    uint32_t SHAPI_DEVICE_STATUS ;
    uint32_t SHAPI_DEVICE_CONTROL  ;
    uint32_t SHAPI_IRQ_MASK  ;
    uint32_t SHAPI_IRQ_FLAG  ;
    uint32_t SHAPI_IRQ_ACTIVE;
    uint32_t SHAPI_SCRATCH_REGISTER;
    char fw_name[13];
};
typedef struct shapi_device_info shapi_device_info;

// TODO: The list_head module_list is temporary commented out to make the code compile.
//       This is a temporary workaround for the missing libaio.h dependency.
struct shapi_module_info {
//    struct list_head module_list;
    uint32_t SHAPI_VERSION;
    uint32_t SHAPI_NEXT_MODULE_ADDRESS;
    uint32_t SHAPI_MODULE_FW_IDS ;
    uint32_t SHAPI_MODULE_VERSION  ;
    uint32_t SHAPI_MODULE_NAME[2] ;
    uint32_t SHAPI_MODULE_CAP;
    uint32_t SHAPI_MODULE_STATUS;
    uint32_t SHAPI_MODULE_CONTROL ;
    uint32_t SHAPI_IRQ_ID  ;
    uint32_t SHAPI_IRQ_FLAG_CLEAR  ;
    uint32_t SHAPI_IRQ_MASK  ;
    uint32_t SHAPI_IRQ_FLAG;
    uint32_t SHAPI_IRQ_ACTIVE;
    uint32_t reserved[2];
    uint32_t atca_status;
    uint32_t atca_control;
    uint32_t atca_chop_period;
    uint32_t atca_max_dma_size;
    uint32_t atca_max_tlp_size;
    uint32_t reserved1[27];
    uint32_t atca_eo_offsets[32];
    uint32_t atca_wo_offsets[10];
    uint32_t reserved2[6];
    uint32_t atca_ilck_params[10];
    char module_name[9];
    int module_num;
};

typedef struct shapi_module_info shapi_module_info;


/*      Regiters addres offset in BAR0 space */
#define FW_VERSION_REG_OFF      0x10
#define TIME_STAMP_REG_OFF      0x14
#define STATUS_REG_OFF          0x80
#define CONTROL_REG_OFF         0x84
#define TRIG0_REG_OFF           0x88
#define TRIG1_REG_OFF           0x8C
#define TRIG2_REG_OFF           0x90
#define PARAM_M_REG_OFF         0x94
#define PARAM_OFF_REG_OFF       0x98
#define IDELAY_OFF_REG_OFF      0x9C

#define PULSE_DLY_REG_OFF  0xC0

#define CHOPP_PERIOD_REG_OFF  0x88
#define EO_REGS_OFF      0x100
#define WO_REGS_OFF      0x180
#define ILCK_REGS_OFF    0x1C0

/*

 //#### CONTROL  REG BITS definitions ######//

#define DMA_DATA_32_BIT       15  // 0 : 16 bit , 1:32 Bit data

//`define FWUSTAR_BIT 19

 * Bit position in "Control Reg" */
#define DMA_0_ON_BIT 	        0
#define DMA_1_ON_BIT 	        1
#define ENDIAN_DMA_BIT     		8 //Endianness of DMA data words  (0:little , 1: Big Endian)
#define CHOP_EN_BIT          	10 // State of Chop, if equals 1 chop is ON if 0 it is OFF
#define CHOP_DEFAULT_BIT    	11 // Value of Chop case CHOP_STATE is 0
#define CHOP_RECONSTRUCT_BIT  	12 // State of Chop Recontruction, if equals 1 chop is ON if 0 it is OFF
#define DMA_DATA32_BIT       15  // 0 : 16 bit , 1:32 Bit data

#define STREAME_BIT     20 // Streaming enable
#define ACQE_BIT 		23
#define STRG_BIT 		24 // Soft Trigger
//#define DMAE_BIT 		27
//#define DMA_RST_BIT		28
//#define DMAiE_BIT 		30 // DMA end Interrup Enable
/*
struct atca_eo_config {
  int32_t offset[ADC_CHANNELS];
};

struct atca_wo_config {
  int32_t offset[INT_CHANNELS];
};

union atca_ilck_params {
  uint32_t val_uint[N_ILOCK_PARAMS];
  float val_f[N_ILOCK_PARAMS];
};
*/

// TODO : to be used.
#ifdef __BIG_ENDIAN_BTFLD
#define BTFLD(a, b) b, a
#else
#define BTFLD(a, b) a, b
#endif

#ifndef VM_RESERVED
#define VMEM_FLAGS (VM_IO | VM_DONTEXPAND | VM_DONTDUMP)
#else
#define VMEM_FLAGS (VM_IO | VM_RESERVED)
#endif

#undef PDEBUG /* undef it, just in case */
#ifdef ATCA_DEBUG
#ifdef __KERNEL__
/* This one if debugging is on, and kernel space */
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "atcav2: " fmt, ##args)
#else
/* This one for user space */
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#endif
#else
#define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#endif /* _ESTHER_DAQ_H_ */
