/*
 *  esther-daq-app.cpp
 */
//#include <atomic>
//#include <csignal> /* sigaction */
#include <cstdio>  /* for printf */
#include <cstring> /* strerror*/
#include <getopt.h> /* getopt */
#include <iostream>
//#include <string>

#include "esther-daq-device.h"

namespace esther_app
{
    int numberOfAcquisitionRounds = 10;
    short lvlTrig[TRIGGER_LEVELS] = { 8194, -8194, 8194, -8194, 8194, -8194}; // 14 bits
    float paramDelay[TRIGGER_COEFFS] = {1.0, 0.0};  //-- Multuply / Contant delay
    float initHold = INIT_HOLD; // 25000000  (* 8ns) Initial Idle Time  = 0.2 s Max 4294967294/34 s 
    int deviceNumber = 0;
    //int digit_optind = 0, swtrigger=0;
    int swtrigger = 0;
    int dmaSize = DMA_SIZE_DEFAULT;
    bool softTrigger = false;
    std::string fileName = "data/out_fmc.bin";

    void writeBinaryFile(FILE* outputFile, uint8_t* dataBuffer, uint64_t size) {
        // Write all the channel data, in binary format
        fwrite(dataBuffer, 4096, size/4096, outputFile);
    }
    /*
    uint32_t getopt_integer(char *optarg)
    {
        int rc;
        uint32_t value;

        rc = sscanf(optarg, "0x%x", &value);
        if (rc <= 0)
            rc = sscanf(optarg, "%u", &value);
        printf("sscanf() = %d, value = 0x%lx\n", rc, value);

        return value;
    }
    */
    /*
       int readData(xdma::EstherDaq& device, FILE* outputFile) {
       const int numOfChannels = 32;
       uint8_t* buffer;
       return EXIT_SUCCESS;
       }
       */
    void printHelp() {
        printf("All parameters are optional. If parameter is not specified then the default values will be used.\n \
                -a [INT] - number of acquisition rounds\n \
                -d [INT] - device number \n \
                -s [INT] - dmaSize in kB\n \
                -f [name] - file to write\n \
                -a/A [SHORT] - neg/pos channel A level Trigger\n \
                -b/B [SHORT] - neg/pos channel B level Trigger\n \
                -c/C [SHORT] - neg/pos channel C level Trigger\n \
                -m [FLOAT] - multiply delay parameter\n \
                -s [FLOAT] - constant delay parameter in us\n \
                -i [FLOAT] - init holding time in s\n \
                -t - use software trigger\n \
                -h - print this help\n");
    }

    int processInputs(int argc, char* argv[])
    {
        int c;
        while ((c = getopt(argc, argv, "a:A:d:b:B:c:C:s:f:o:m:i:th")) != -1) {
            switch (c) {
                case 'd':
                    deviceNumber = atoi(optarg);
                    break;
                case 's':
                    dmaSize = 1024 * atoi(optarg);
                    break;
                case 'a':
                    lvlTrig[0] = atoi(optarg);
                    break;
                case 'A':
                    lvlTrig[1] = atoi(optarg);
                    break;
                case 'b':
                    lvlTrig[2] = atoi(optarg);
                    break;
                case 'B':
                    lvlTrig[3] = atoi(optarg);
                    break;
                case 'c':
                    lvlTrig[4] = atoi(optarg);
                    break;
                case 'C':
                    lvlTrig[5] = atoi(optarg);
                    break;
                case 'm':
                    paramDelay[0] = std::atof(optarg);
                    fprintf(stderr, "option m %f", paramDelay[0]);
                    break;
                case 'o':
                    paramDelay[1] =  std::atof(optarg);
                    fprintf(stderr, "option m %f", paramDelay[1]);
                    break;
                case 'i':
                    initHold =  std::atof(optarg);
                    fprintf(stderr, "option m %f", initHold);
                    break;
                case 'f':
                    fileName = strdup(optarg);
                    break;

                case 'h':
                    printHelp();
                    exit(EXIT_SUCCESS);
                default:
                    fprintf(stderr, "Unknown command '%s'.\n",  argv[0]);
                    abort();
            }
        }
        return EXIT_SUCCESS;
    }


    int runTest(int argc, char* argv[]) {
        uint8_t* buffer;
        processInputs(argc, argv);
        uint32_t lTrig[3];
        uint32_t uval;

        //buffer = (uint8_t*) malloc(dmaSize);
        xdma::EstherDaq device(deviceNumber);
        printf("device open\n");
        device.open();
        posix_memalign((void **) &buffer, 4096 /*alignment */ , dmaSize + 4096);
        if (!buffer) {
            fprintf(stderr, "Failed to alocate DMA memory %lu.\n", dmaSize + 4096);
            device.close();
            return EXIT_FAILURE;
        }
        device.read(buffer, dmaSize);
        device.close();
        printf("device close\n");
        /*
		device.setDelayParameters(lvlTrig, paramDelay, initHold);
        if(swtrigger)
            device.sendSoftwareTrigger();
        device.close();
*/
        // if (fileName) {
        /* create file to write data to */
        FILE* outputBinFile = fopen(fileName.c_str(), "wb");
        if (outputBinFile != NULL) {
            writeBinaryFile(outputBinFile, buffer, dmaSize);
            fclose(outputBinFile);
        }
        else {
            std::cerr << "Failed to open " <<  fileName << " output file: " << strerror(errno) << std::endl;
        }

        //}
        free(buffer);
        return EXIT_SUCCESS;
    }

} // namespace esther_app

int main(int argc, char* argv[]) {
    return esther_app::runTest(argc, argv);
}


//  vim: syntax=cpp:ts=4:sw=4:sts=4:sr:et:
