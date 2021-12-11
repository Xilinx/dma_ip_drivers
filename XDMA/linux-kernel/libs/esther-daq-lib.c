/**
 * @file esther-daq-lib.c
 * @brief Header file for ESTHER  board functions
 * @author Bernardo Carvalho 
 * @date 01/10/2021
 *
 * @copyright Copyright 2016 - 2021 IPFN-Instituto Superior Tecnico, Portugal
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence, available in 23 official languages of
 * the European Union, at:
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl-text-11-12
 *
 * @warning Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basisO,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 *
 * @details
 * ESTHER PCI ADC  device driver
 * Definitions for the Linux Device Driver
 */

/*---------------------------------------------------------------------------*/
/*                        Standard heOader includes                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*-----------------------------------O----------------------------------------*/
#include "esther-daq-lib.h"

#define DEVICE_NAME_STR "/dev/fmc_xdma%d_c2h_0"
#define DEVICE_C2H_1_STR "/dev/fmc_xdma%d_c2h_1"
#define DEVICE_USER_STR "/dev/fmc_xdma%d_user"

#define MAP_SIZE (32*1024UL) 
#define MAP_MASK (MAP_SIZE - 1)

void save_to_disk2(int nrOfReads, short *acqData){
    FILE *stream_out;
    char file_name[40];
    char dir_name[40];
    char path_name[80];

    int numchan;
    int nrpk;
    time_t now;
    struct tm *t;

    // Build directory name based on current date and time
    now = time(NULL);
    t = localtime(&now);
    strftime(dir_name, sizeof(dir_name)-1, "%Y%m%d%H%M%S", t);

    printf("Saving data to disk ...\n");
    printf("\ndir_name: %s\n", dir_name);

    mkdir(dir_name,0777); // Create directory

    for (numchan = 0; numchan<32; numchan++) {
        // Create complete pathname for data file
        sprintf(file_name,"/ch%2.2d.txt", numchan+1);
        //printf("file_name: %s\n", file_name);
        strcpy(path_name, dir_name);
        strcat(path_name, file_name);
        //printf("path_name: %s\n", path_name);
        stream_out = fopen(path_name,"wt");
        // Write data of each channel  into a separate file (max 32)
        for (nrpk=0; nrpk < nrOfReads*N_AQS ; nrpk++) {

            fprintf(stream_out, "%.4hd ",acqData[(nrpk * NUM_CHAN)+numchan]);
            fprintf(stream_out, "\n");
        }
        printf("%d values for channel %d saved\n", nrOfReads * N_AQS, numchan+1);
    }
    

    fflush(stream_out);
    fclose(stream_out);
    printf("Saving data to disk finished\n");

}

int init_device_c2h_1(unsigned int dev_num){
    char devname[80];
    uint32_t control_reg = 0;
    int fd_dma;

   sprintf(devname, DEVICE_C2H_1_STR, dev_num);
/*
     * use O_TRUNC to indicate to the driver to flush the data up based on
     * EOP (end-of-packet), streaming mode only
     */
//  if (eop_flush)
//      fpga_fd = open(devname, O_RDWR | O_TRUNC);
//  else
    if ( (fd_dma= open(devname, O_RDWR )) == -1){
        printf("Device %s not opened.\n", devname );
        return -1;
    }
//    printf("Device %s opened.\n", device_u );
//    fflush(stdout);
    return fd_dma;
}
void * init_device(unsigned int dev_num, int chopped, unsigned int chopper_period, int *fd_bar0, int *fd_bar1){
    char devname[80];
    void *map_base = MAP_FAILED;
    uint32_t control_reg =0;
    int i;

   sprintf(devname, DEVICE_NAME_STR, dev_num);
/*
     * use O_TRUNC to indicate to the driver to flush the data up based on
     * EOP (end-of-packet), streaming mode only
     */
//  if (eop_flush)
//      fpga_fd = open(devname, O_RDWR | O_TRUNC);
//  else

    if ( (*fd_bar1= open(devname, O_RDWR )) == -1){
        printf("Device %s not opened.\n", devname );
        return map_base;
      //  return ((*void) -1);
    }
//    printf("Device %s opened.\n", device_u );
//    fflush(stdout);

    sprintf(devname, DEVICE_USER_STR, dev_num);
    if ((*fd_bar0 = open(devname, O_RDWR | O_SYNC)) == -1){
        printf("Device %s not opened.\n", devname );
        return map_base;
        //return (*void) -1;
    }
    /* map one page on BAR0 (FPGA registers )*/
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *fd_bar0, 0);
    if (map_base == MAP_FAILED){ //  (void *) -1
        close(*fd_bar0);
        close(*fd_bar1);
        return MAP_FAILED; //(*void) -1;
    }
    write_control_reg(map_base, control_reg);
    for (i=0; i < ADC_CHANNELS ; i++)
        write_fpga_reg(map_base, EO_REGS_OFF + i*sizeof(uint32_t), 0);
    for (i=0; i < INT_CHANNELS; i++)
        write_fpga_reg(map_base, WO_REGS_OFF + i*sizeof(uint32_t), 0);
    for (i=0; i < N_ILOCK_PARAMS ; i++)
        write_fpga_reg(map_base, ILCK_REGS_OFF + i*sizeof(uint32_t), 0);

    if (chopped > 0)   {      //Set the Chop off as default
        write_chopp_period(map_base, chopper_period);
        control_reg |= (1<<CHOP_EN_BIT);
    }
    control_reg |= (1<<ACQE_BIT);
    write_control_reg(map_base, control_reg);

    return map_base;
}

int write_use_data32(void *map_base, int use32bit){
    uint32_t control_reg = read_control_reg(map_base);
    if(use32bit)
        control_reg |= (1<<DMA_DATA32_BIT);
    else
        control_reg &= ~(1<<DMA_DATA32_BIT);
    return write_control_reg(map_base, control_reg);
}

int write_eo_offsets(void *map_base, struct atca_eo_config * pConfig){
    int i, rc;
    for (i=0; i < ADC_CHANNELS; i++)
        rc = write_fpga_reg(map_base,
                EO_REGS_OFF + i*sizeof(uint32_t), pConfig->offset[i]);
    return rc;
}
int write_wo_offsets(void *map_base, struct atca_wo_config * pConfig){
    int i, rc;
    for (i=0; i < INT_CHANNELS; i++)
        rc = write_fpga_reg(map_base,
                WO_REGS_OFF + i*sizeof(uint32_t), pConfig->offset[i]);
    return rc;
}
int write_ilck_params(void *map_base, union atca_ilck_params * pConfig){
    int i, rc;
    for (i=0; i < N_ILOCK_PARAMS ; i++)
        rc = write_fpga_reg(map_base,
                ILCK_REGS_OFF + i*sizeof(uint32_t), pConfig->val_uint[i]);
    return rc;
}
int write_chopp_period(void *map_base, uint16_t period){
    uint32_t val;
    val = period/2; // Duty cycle 50%
    val |= ( (int) period)<<16;
    write_fpga_reg(map_base, CHOPP_PERIOD_REG_OFF, val);
    return 0;
}
int write_control_reg(void *map_base, uint32_t val){
         write_fpga_reg(map_base, CONTROL_REG_OFF, val);
        return 0;
}
int write_fpga_reg(void *map_base, off_t target, uint32_t val){
         void *virt_addr = map_base + target;
         *((uint32_t *) virt_addr) = val;
        return 0;
}
void  fpga_soft_trigger(void * map_base ){
    uint32_t control_reg;
    control_reg=read_control_reg(map_base);
    //printf("CR: 0x%0X\n", control_reg);
    /*printf("CR: 0x%0X, mb:%p\n", control_reg, map_base);*/
    control_reg |= (1<<STRG_BIT);// 0x01004000; //
    write_control_reg(map_base, control_reg);
}
uint32_t  read_control_reg(void *map_base){
        return read_fpga_reg(map_base, CONTROL_REG_OFF);
}
uint32_t  read_fpga_reg(void *map_base, off_t target ){
        void *virt_addr = map_base + target;
        uint32_t val;
        val = *((uint32_t *) virt_addr);
        return val;
}
uint32_t  read_status_reg(void *map_base){
        uint32_t val;
        val= read_fpga_reg(map_base, STATUS_REG_OFF);
        return val;
}

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 40
/**
 * @brief prints a progress indicator
 * @param[in] percentage (0.0 to 1.0)
 * @return 
 * @details take from 
 * https://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
 *
 */
void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf(" %3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}
