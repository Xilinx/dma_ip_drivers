/**
 * @file esther-daq-lib.h
 * @brief Header file for ATCA V2 MIMO board functions
 * @author Bernardo Carvalho 
 * @date 01/06/2021
 *
 * @copyright Copyright 2016 - 2021 IPFN-Instituto Superior Tecnico, Portugal
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence, available in 23 official languages of
 * the Eatcauropean Union, at:
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl-text-11-12
 *
 * @warning Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 *
 * @details
 *  PCI ADC  device driver
 * Definitions for the Linux Device Driver
 */

#ifndef _ESTHER_DAQ_LIB_H_
#define _ESTHER_DAQ_LIB_H_
/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "esther-daq.h"

//#define DMA_SIZE 2097152 // 2MB 4194304 // 4Mb 
//#define DMA_SIZE  4194304 // 4Mb 
//#define DMA_SIZE  0x003FF000 // 4190208 B
//#define NUM_CHAN 32      // nr of 16bit data fields per sample. Can be 32 channels or 2 counters and 30 channels, etc.
//#define TIMERVALUE 1000000
//#define N_AQS DMA_SIZE/(NUM_CHAN*2)  // nr samples per buffer (IRQ mode)

/**
 * @brief Writes the 
 * @param[in] 
 * @param[in] 
 * @retur o if OK
 */
//void save_to_disk2(int nrOfReads, short *acqData);

//int init_device_c2h_1(unsigned int dev_num);

/**
 * @brief reset ATCA board and prepare acquisition
 * @param[in] dev_num Board Number
 * @param[in] chopped = 1 if cHopped modules 
 * @param[in] chopper_period Chopper Period ( Duty Cycle is set to 50 %). 
 *              Should be even
 * @param[out] fd0 pointer to file descriptor on BAR0 (Fpga regs)
 * @param[out] fd1 pointer to file descriptor on BAR1 (DMA reads)
 * @return virtual address of BAR1 to call other API functions
 */
void *init_device(unsigned int dev_num, int chopped, unsigned int chopper_period, int *fd_bar0, int *fd_bar1);

/**
 * @brief Switch between 32 and 16 bit acquisition
 * @param[in] map_base virtual address return from mmap function
 * @param[in] use32bit True if using 32 bit acquisition, false for 16 bit acquisition
 * @return 0 if OK
 */
int write_use_data32(void *map_base, int use32bit);

/**
 * @brief Send a software to the configured board 
 * @param[in] map_base virtual address return from mmap function
 */
void  fpga_soft_trigger(void *map_base );


/**
 * @brief Reads a 32 bit value on the control_reg register in BAR0 space
 * @param[in] map_base virtual address return from mmap function
 * @return Control Reg value
 */
uint32_t  read_control_reg(void *map_base);

/**
 * @brief Writes a 32 bit value on the control_reg register in BAR0 space
 * @param[in] map_base virtual address return from mmap function
 * @param[in] val 32-bit value to write
 * @return 0 if OK
 */
int write_control_reg(void *map_base, uint32_t val);

/**
 * @brief Writes a 32 bit value on a register in BAR0 space
 * @param[in] map_base virtual address return from mmap function
 * @param[in] target offset address (must be multiple of 4)
 * @param[in] val 32-bit value to write
 * @return 0 if OK
 */

int write_fpga_reg(void *map_base, off_t target, uint32_t val);

/**
 * @brief Reads a 32 bit value from a register in BAR0 space
 * @param[in] map_base virtual address return from mmap function
 * @param[in] target offset address (must be multiple of 4)
 * @param[in] value to write
 * @return 0 if OK
 */
uint32_t read_fpga_reg(void *map_base, off_t target);

/**
 * @brief Reads a 32 bit value from the control_reg register in BAR0 space
 * @param[in] map_base virtual address return from mmap function
 * @param[in] target offset address (must be multiple of 4)
 * @return read value
 */
uint32_t read_status_reg(void *map_base);

/**
 * @brief prints a progress indicator Bar
 * @param[in] percentage (0.0 to 1.0)
 * @return 
 */
void printProgress(double percentage);

/*---------------------------------------------------------------------------*/
#endif /* _ESTHER_DAQ_LIB_H_ */

