/**
 * @file atcadaq.c
 * @brief Example application for the ATCA-MIMO-V2 boards
 * @author Bernardo Carvalho
 * @date 01/06/2021
 *
 * @copyright Copyright 2016 - 2021 IPFN-Instituto Superior Tecnico, Portugal
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence, available in 23 official languages of
 * the European Union, at:
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl-text-11-12
 *
 * @warning Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 *
 * @depends ATCA V2 PCI ADC xdma  device driver
 * @details
 */

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------- -----------------------------------------------*/
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE  deprecated
// #define _XOPEN_SOURCE 500
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>
#include <getopt.h>
//#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "dma_utils.c"
#include "esther-daq.h"
#include "esther-daq-lib.h"

   // int dmasize= 1048576*3; // 0x100000   maxx ~ 132k Samples
#define SIZE_DEFAULT (1048576*3) // 3Mbytes ~25 ms
#define TRIG_DEFAULT 0x7D0F830 // 9000,  -9000
#define MULT_DEFAULT 0x00030000
#define OFF_DEFAULT 0x00

//#define N_PACKETS 2  // size in full buffers (2MB) of data to save to disk. 2M=64 kSample =16 ms
//Global vars
//int run=1;
// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
/* Flag set by ‘--verbose’. */
static int verbose_flag;
static struct option long_options[] = {
    /*   NAME       ARGUMENT           FLAG  SHORTNAME */
        {"atrigger",    required_argument, NULL, 'a'},
        {"btrigger",    required_argument, NULL, 'b'},
        {"ctrigger",    required_argument, NULL, 'c'},
        {"device",      required_argument, NULL, 'd'},
        {"size",        required_argument, NULL, 's'},
        {"mult",        required_argument, NULL, 'm'},
        {"offset",      required_argument, NULL, 'o'},
        {"soft_trigger", no_argument,       NULL, 't'},
        {"user_trigger", no_argument,       NULL, 'u'},
        //{"delete",  required_argument, NULL, 0},
        {"verbose", no_argument        ,&verbose_flag, 1},
        //{"create",  required_argument, NULL, 'c'},
        {"file",    required_argument, NULL, 0},
        {NULL,      0,                 NULL, 0}
    };

void getkey()
{
    printf("Press Return to continue ...\n");
    getchar();
}
void check_count(short *acqData, int dmaSize){
    uint32_t * pCount = (uint32_t *) acqData;
    uint32_t lastCount, cnt, i;
    uint32_t firstMiss =0, misses =0;
    lastCount = pCount[3] +1;
    for (i=1; i< dmaSize/16; i++){
        cnt = pCount[i*4 + 3];
        if(cnt != lastCount){
            if(firstMiss==0)
                firstMiss = i;
            misses++;
            /*fprintf(stderr, "ce : %d %d %d\n", i, lastCount, cnt);*/
        }
        lastCount = cnt + 1;
    }
    if(misses != 0)
        fprintf(stderr, "First Miss %d, Misses:%d\n", firstMiss, misses);

}
void save_to_bin(short *acqData, int dmasize){
    FILE *stream_out;
    char path_name[80];
    //stream_out = fopen("data/out_fmc.bin","wb");
    sprintf(path_name,"data/out_fmc.bin");
    stream_out = fopen(path_name,"wb");
    fwrite(acqData, 1, dmasize, stream_out);
//    fflush(stream_out);
    fclose(stream_out);
}
void save_to_disk(int dmasize, short *acqData){
    FILE *stream_out;
    char file_name[40];
    char dir_name[40];
    char path_name[80];

    int numchan;
    int nsample;
    time_t now;
    struct tm *t;

    // Build directory name based on current date and time
    now = time(NULL);
    t = localtime(&now);
    strftime(dir_name, sizeof(dir_name)-1, "%Y%m%d%H%M%S", t);

    printf("Saving data to disk ...\n");
    printf("\ndir_name: %s\n", dir_name);

    mkdir(dir_name,0777); // Create directory

    for (numchan = 0; numchan < 4; numchan++) {
        // Create complete pathname for data file
        sprintf(file_name,"/ch%2.2d.txt", numchan+1);
        //printf("file_name: %s\n", file_name);
        strcpy(path_name, dir_name);
        strcat(path_name, file_name);
        //printf("path_name: %s\n", path_name);
        stream_out = fopen(path_name,"wt");
        // Write data of each channel  into a separate file (max 32)
        for (nsample=0; nsample < dmasize/4/2;  nsample++) {

            fprintf(stream_out, "%.4hd ",acqData[(nsample*4 )+ numchan]);
            fprintf(stream_out, "\n");
        }
		fflush(stream_out);
		fclose(stream_out);
//        printf("%d values for channel %d saved\n", nrOfReads*N_AQS, numchan+1);
    }


    printf("Saving data to disk finished\n");

}


int main(int argc, char *argv[])
{
    //int dev_num = 0;
    int i,  k , c;  					// loop iterators i
    int rc;
    void *map_base;//, *virt_addr;
    int fd_bar_0, fd_bar_1;   // file descriptor of device2
    short * acqData; 
    uint32_t control_reg;
    shapi_device_info * pDevice;
    shapi_module_info * pModule;
    uint32_t dly;
    float dlyF=0.0;

    int digit_optind = 0, swtrigger=0;
    int dopt=0;
    uint32_t sizeopt = SIZE_DEFAULT;
    uint32_t aopt = TRIG_DEFAULT, bopt = TRIG_DEFAULT, copt = TRIG_DEFAULT;
    uint32_t mopt = MULT_DEFAULT, offopt = OFF_DEFAULT;

    /*char *dopt = 0;*/
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "a:b:c:d:s:m:o:tu012",
                 long_options, &option_index)) != -1) {
        int this_option_optind = optind ? optind : 1;
        switch (c) {
        case 0:
            printf ("Goption %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;
        case '1':
        case '2':
            if (digit_optind != 0 && digit_optind != this_option_optind)
              printf ("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            /*printf ("option %c\n", c);*/
            break;
        case 'a':
            aopt = getopt_integer(optarg);
            /*printf ("option a 0x%08X\n", aopt);*/
            break;
        case 'b':
            bopt = getopt_integer(optarg);
            /*fprintf(stderr, "option b 0x%08X\n", bopt);*/
            break;
        case 'c':
            copt = getopt_integer(optarg);
            /*printf ("option c 0x%08X\n", copt);*/
            break;
        case 'd':
            dopt = atoi(optarg);
            /*printf ("option d %d\n", dopt);*/
            break;
        case 's':
            sizeopt = getopt_integer(optarg);
            /*printf ("option size '%d'\n", sizeopt);*/
            break;
        case 'm':
            mopt = getopt_integer(optarg);
            break;
        case 'o':
            offopt = getopt_integer(optarg);
            break;
        case 't':
            swtrigger = 1;
            break;
        case 'u':
            swtrigger = 2;
            break;
/*        case 'd':
            //printfLY ("option d with value '%s'\n", optarg);
            dopt = strdup(optarg);
            printf ("option d with value '%s'\n", dopt);
            break;
*/
        case '?':
            break;
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
        }
    }
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }

	posix_memalign((void **)&acqData, 4096 /*alignment */ , sizeopt + 4096);
	if (!acqData) {
		fprintf(stderr, "OOM %lu.\n", sizeopt + 4096);
		rc = -ENOMEM;
        exit(-1);
		/*goto out;*/
	}


    map_base=init_device(dopt, 0, 0, &fd_bar_0, &fd_bar_1);
    if (fd_bar_1   < 0)  {
        //if (map_base == MAP_FAILED){ //  (void *) -1
        fprintf (stderr,"Error: cannot open device %d \t", dopt);
        fprintf (stderr," errno = %i\n",errno);
        printf ("open error : %s\n", strerror(errno));
        free(acqData);
        exit(1);
    }
    /*printf("CR: 0x%0X\n", read_control_reg(map_base));*/
    pDevice = (shapi_device_info *) map_base;
    /*printf("SHAPI VER: 0x%X\n", pDevice->SHAPI_VERSION);*/
    pModule = (shapi_module_info *) (map_base +
            pDevice->SHAPI_FIRST_MODULE_ADDRESS);
    /*printf("SHAPI MOD VER: 0x%X\n", pModule->SHAPI_VERSION);*/
    printf("TS %d\n", read_fpga_reg(map_base,TIME_STAMP_REG_OFF));
    write_control_reg(map_base, 0);
    /*printf("TRIG0: 0x%0X\n",trigOff32 );*/
    write_fpga_reg(map_base, TRIG0_REG_OFF, aopt);
    /*printf("TRIG1: 0x%0X\n",trigOff32 );*/
    write_fpga_reg(map_base, TRIG1_REG_OFF, bopt);
    /*trigOff32 = ( (4000U << 16) & 0xFFFF0000) | (-9000 & 0xFFFF);*/
    write_fpga_reg(map_base, TRIG2_REG_OFF, copt);
    /*printf("TRIG2: 0x%0X\n", read_fpga_reg(map_base, TRIG2_REG_OFF));*/
    write_fpga_reg(map_base, PARAM_M_REG_OFF, mopt);
    /*write_fpga_reg(map_base, PARAM_M_REG_OFF, 0x00030000);*/
    printf("PMUL: 0x%0X\n", read_fpga_reg(map_base, PARAM_M_REG_OFF));
    write_fpga_reg(map_base, PARAM_OFF_REG_OFF, offopt);
    /*write_fpga_reg(map_base, PARAM_OFF_REG_OFF, 0x0A290001);*/
    /**
option size with value '3145728'
CR: 0x800000
SHAPI VER: 0x53480100
SHAPI MOD VER: 0x534D0100
TRIG2: 0xFA0DCD8
DLY 0xFFFF
PMUL: 0x30000
CR: 0x800000, mb:0x7f2a66add000
FPGA Status: 0x00000005
Software trigger mode active
CR: 0x1800000, mb:0x7f2a66add000
FPGA Status: 0x01630011
*/
    //acqData = (short *) malloc(sizeopt);
//    i = 0;
    control_reg = read_control_reg(map_base);
    control_reg |= (1<<ACQE_BIT);

    write_control_reg(map_base, control_reg);
    //loss_hits = 0;

        /*printf("CR: 0x%0X, mb:%p\n", read_control_reg(map_base), map_base);*/
    // ADC board is armed and waiting for trigger (hardware or software)
    if (swtrigger == 2) {  // option '-u'
        printf("FPGA Status: 0x%.8X\t", read_status_reg(map_base));
        fprintf(stderr, "User software trigger mode active\n");
        getkey(); // Press any key to start the acquisition
        // Start the dacq with a software trigger
        fpga_soft_trigger(map_base);
        //	    rc = ioctl(fd, PCIE_ADC_IOCG_STATUS, &tmp);
        printf("CR: 0x%0X, mb:%p\t", read_control_reg(map_base), map_base);
        printf("FPGA Status: 0x%.8X\n", read_status_reg(map_base));
        // Check status to see if everything is okay (optional)
    }
    else if (swtrigger == 1) {  // option '-u'
        printf("FPGA Status: 0x%.8X\n", read_status_reg(map_base));
        printf("Software trigger mode active\n");
        // Start the dacq with a software trigger
        fpga_soft_trigger(map_base);
        //	    rc = ioctl(fd, PCIE_ADC_IOCG_STATUS, &tmp);
        printf("CR: 0x%0X, mb:%p\n", read_control_reg(map_base), map_base);
        printf("FPGA Status: 0x%.8X\n", read_status_reg(map_base));
    }
    else
        printf("hardware trigger mode active\n");

    fflush(stdout);

    rc = 0;

    rc = read(fd_bar_1, acqData, sizeopt); // loop until there is something to read.
    usleep(10);
    dly = read_fpga_reg(map_base, PULSE_DLY_REG_OFF);
    write_control_reg(map_base, 0);  // stop board
    if (close(fd_bar_1)) {

        printf("Error closing : %s\n", strerror(errno));
    }
    // Check status to see if everything is okay (optional)
    /*rc = ioctl(fd, PCIE_ADC_IOCG_STATUS, &tmp);*/
    rc = read_status_reg(map_base);

//    save_to_disk(dmasize, acqData);
    save_to_bin(acqData, sizeopt);
    check_count(acqData, sizeopt);
    /*save_to_bin(sizeopt, acqData);*/

    printf("Streaming off - FPGA Status: 0x%.8X\n", rc );
    /*
     *printf("Del = 0x%08X, %d := %10.3f us\n", dly, (dly >>16), (dly>>16) /125.0);
     *printf("Delay: %10.3f us\n", (dly>>16) /125.0);
     */
    printf("Del = 0x%08X, %d, := %10.3f us\n", dly, dly , dly /125.0);
    printf("Delay: %d\n", dly);
    close(fd_bar_0);
    close(fd_bar_1);
    free(acqData);
    return 0;
    }
