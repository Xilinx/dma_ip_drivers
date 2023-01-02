/*
 *  esther-daq-device.cpp
 */

#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#include  "esther-daq-device.h"

#define MAPSIZE  32768UL //  (32*1024UL)
#define MAP_MASK (MAPSIZE - 1)

#define XDMA_NODE_NAME  "fmc_xdma"
#define DEVICE_C2H_0_FMT "/dev/%s%d_c2h_0"
#define DEVICE_USER_FMT "/dev/%s%d_user"

// ---      Regiters addres offset in BAR0 space
#define SHAPI_MAGIC 0x53480100U // Magic Word/SHAPI Version
#define FW_VERSION_REG_OFF   0x10

#define TIME_STAMP_REG_OFF  0x14
#define SHAPI_DEV_CAP       0X24
#define SHAPI_DEV_STATUS    0X28
#define SHAPI_DEV_CONTROL   0X2C

#define STATUS_REG_OFF      0x80
#define CONTROL_REG_OFF     0x84
#define TRIG0_REG_OFF       0x88
#define TRIG1_REG_OFF       0x8C
#define TRIG2_REG_OFF       0x90
#define PARAM_M_REG_OFF     0x94
#define PARAM_OFF_REG_OFF   0x98
#define PULSE_DLY_REG_OFF   0x9C

//-- Device Control bits
// #define SHAPI_DEV_ENDIAN_BIT  0 //Endianness of DMA data words  (0:little , 1: Big Endian)
#define SHAPI_SOFT_RESET_BIT  30
#define SHAPI_FULL_RESET_BIT  31

// --- Bit positions in "Control Reg"
#define STREAM_EN_BIT   20 // Streaming enable
#define ACQ_EN_BIT      23
#define STRG_BIT        24 // Soft Trigger


namespace xdma {
    EstherDaq::EstherDaq(int devNumber) :
        deviceNumber(devNumber), mapBase(NULL), deviceHandle(0),
        deviceDmaHandle(0), dmaBuffer(NULL), isDeviceOpen(false) {
        }

    EstherDaq::~EstherDaq() {
        if (dmaBuffer)
            free(dmaBuffer);
        //sem_destroy(&readSemaphore);
    }

    int EstherDaq::open() {
        if(isDeviceOpen) {
            std::cerr << "Device already opened, closing device." << std::endl;
            close();
        }

        std::cout << "DeviceX opening." << std::endl;
        //        this->use32bits = use32bits;
        char devname[64];
        snprintf(devname, 64, DEVICE_USER_FMT, XDMA_NODE_NAME, deviceNumber);
#ifdef DEBUG
        printf("Openning Device name = %s, dgb:%d\n", devname, DEBUG);
#endif
        deviceHandle = ::open(devname, O_RDWR | O_SYNC);
        if (deviceHandle == -1) {
            std::cerr << "Device" << devname << " failed Open" << std::endl;
            return EXIT_FAILURE;
        }
        snprintf(devname, 64, DEVICE_C2H_0_FMT, XDMA_NODE_NAME, deviceNumber);
#ifdef DEBUG
        printf("Opening DMA Device name = %s\n", devname);
#endif
        deviceDmaHandle = ::open(devname, O_RDONLY);
        //map_base=atca_init_device(deviceNumber, chopperPeriod, &deviceHandle, &deviceDmaHandle);
        if (deviceDmaHandle == -1){
            ::close(deviceHandle);
#ifdef DEBUG
            fprintf(stderr, "Error Openning DMA Device name = %s\n", devname);
#endif
            deviceHandle = ::open(devname, O_RDWR | O_SYNC);
            return EXIT_FAILURE;
        }
        mapBase = (uint8_t *) ::mmap(0, MAPSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, deviceHandle, 0);
        if (mapBase == MAP_FAILED){
            ::close(deviceHandle);
            ::close(deviceDmaHandle);
            deviceHandle = -1;
#ifdef DEBUG
            fprintf(stderr, "Error Mapping Device name = %s\n", devname);
#endif
            return EXIT_FAILURE; //MAP_FAILED;
        }
        if(readFpgaReg(0) != SHAPI_MAGIC){
            fprintf(stderr, "Device is not SHAPI compatible, exiting.\nIs FW loaded?\n");
            ::close(deviceHandle);
            ::close(deviceDmaHandle);
            deviceHandle = -1;
#ifdef DEBUG
            fprintf(stderr, "Error SHAPI Reg. Device name = %s\n", devname);
#endif
            return EXIT_FAILURE;
        }

        uint32_t controlReg = 0;

        writeControlReg(controlReg);

        isDeviceOpen = true;
        //dmaReadThread = std::thread(&AtcaMimo32V2::dmaReadLoop, this);

        return EXIT_SUCCESS;
    }

    int EstherDaq::close()
    {
        if(!isDeviceOpen) {
            std::cerr << "Device is already Closed." << std::endl;
            return EXIT_FAILURE;
        }

        isDeviceOpen = false;
        //dmaReadLoopActive = false;
        // stopAcquisition();
        //if (dmaReadThread.joinable()) {
        //   dmaReadThread.join();
        //}

#ifdef DEBUG
        //fprintf(stderr, "Joined Threads.\n");
        if (readTimesFile != NULL)
            fclose(readTimesFile);
#endif

        // Clears whole control register which stops acquisition and chopper
        //writeControlReg(0);
        softReset();
        if (softTrigThread.joinable()) {
            softTrigThread.join();
        }

        if (dmaBuffer)
            free(dmaBuffer);
        //dmaBuffer = NULL;
        ::close(deviceHandle);
        ::close(deviceDmaHandle);
        mapBase = 0;
        deviceHandle = 0;
        deviceDmaHandle = 0;

        // Discart any remaining data

        return EXIT_SUCCESS;
    }

    int EstherDaq::setDelayParameters(short trigLevels[TRIGGER_LEVELS],
            float delayCoefficients[TRIGGER_COEFFS], float initialHold) {
        if(!isDeviceOpen) {
            return EXIT_FAILURE;
        }
        for (int i =0; i< TRIGGER_LEVELS; i++){
            if(trigLevels[i] > 8194)
                trigLevels[i] = 8194;
            if( trigLevels[i] < -8194)
                trigLevels[i] = -8194;
        }
        int32_t val1 = trigLevels[0];
        val1 &= 0x0000FFFF;
        int32_t val2 = trigLevels[1];
        val1 |= 0xFFFF0000 & (val2 << 16);
        writeFpgaReg(TRIG0_REG_OFF, val1);
        val1 = trigLevels[2];
        val1 &= 0x0000FFFF;
        val2 = trigLevels[3];
        val1 |= 0xFFFF0000 & (val2 << 16);
        writeFpgaReg(TRIG1_REG_OFF, val1);
        val1 = trigLevels[4];
        val1 &= 0x0000FFFF;
        val2 = trigLevels[5];
        val1 |= 0xFFFF0000 & (val2 << 16);
        writeFpgaReg(TRIG2_REG_OFF, val1);

        int32_t valP = (int32_t) (delayCoefficients[0] * 65536);  // 1 << 16
        writeFpgaReg(PARAM_M_REG_OFF, valP);
        valP = (int32_t) (delayCoefficients[1] * 125 * 65536);  // delay in us
        printf("PARAM OFF: %0.3g,  0x%.8X, %d \n", delayCoefficients[1], valP, valP);
        writeFpgaReg(PARAM_OFF_REG_OFF, valP);
		
		
        return EXIT_SUCCESS;
    }

    int EstherDaq::read(uint8_t* buffer, uint32_t readSize) {
        if(!isDeviceOpen) {
            return EXIT_FAILURE;
        }
        int rc  = ::read(deviceDmaHandle, buffer, readSize);

        writeControlReg(0);
        //return EXIT_SUCCESS;
        return rc;
    }

    uint32_t EstherDaq::getTimeOfFlight() {
        if(!isDeviceOpen) {
            return EXIT_FAILURE;
        }
        return readFpgaReg(PULSE_DLY_REG_OFF);;
    }

    int EstherDaq::sendSoftwareTrigger() {
        if(!isDeviceOpen) {
            return EXIT_FAILURE;
        }
        softTrigThread = std::thread(&EstherDaq::softTrigDelayed, this);
        return EXIT_SUCCESS;
    }

    void* EstherDaq::softTrigDelayed(void* device)
    {
        EstherDaq* dev = (EstherDaq*) device;
        //printf("FPGA Status: 0x%.8X\n", read_status_reg(m_base));
        //printf("Software trigger mode active\n");
        sleep(1);
        dev->softTrig();
            //printf("FPGA Status: 0x%.8X\n", read_status_reg(m_base));
            //ret1 = 0;
            //pthread_exit(&ret1);
        return 0;
    }

    int EstherDaq::readFirmwareVersion(uint32_t* version)
    {
        if(!isDeviceOpen) {
            return EXIT_FAILURE;
        }
        uint32_t ver =  EstherDaq::readFpgaReg(FW_VERSION_REG_OFF);
        /* check for outdated FW version */
        if (ver < CURRENT_FW_VERSION){
            fprintf(stderr, "The FPGA FW 0x%08X in the device %d is no longer compatible with this SW\n",
                    ver, deviceNumber);
            return EXIT_FAILURE;
            return -1;
        }
        /* check for future major/minor version */
        else if (ver >= (CURRENT_FW_VERSION + 0x00010000)){
            fprintf(stderr, "The FPGA FW 0x%08X in the device %d is newer than this SW\n", 
                    ver, deviceNumber);

            return EXIT_FAILURE;
        }

        *version = ver;

        return EXIT_SUCCESS;
    }


    // private:

    void EstherDaq::softTrig() {
        uint32_t controlReg = readControlReg();
        controlReg |= (1 << STRG_BIT);
        writeControlReg(controlReg);
    }

    void EstherDaq::softReset() {
        uint32_t deviceReg = (1 << SHAPI_SOFT_RESET_BIT);
        writeFpgaReg(SHAPI_DEV_CONTROL, deviceReg);
        usleep(1000);
        // No need to re-write old deviceReg, as it will be reset.

    }

    void EstherDaq::writeFpgaReg(off_t target, uint32_t val) {
        uint8_t* virt_addr = mapBase + target;
        *((uint32_t*) virt_addr) = val;
    }
    void EstherDaq::writeControlReg(uint32_t val) {
        writeFpgaReg(CONTROL_REG_OFF, val);
    }
    uint32_t EstherDaq::readFpgaReg(off_t target) {
        uint8_t* virt_addr = mapBase + target;
        return *((uint32_t*) virt_addr);
    }
    uint32_t EstherDaq::readControlReg() {
        uint8_t* virt_addr = mapBase + CONTROL_REG_OFF;
        return *((uint32_t*) virt_addr);
    }


} // namespace xdma

//  vim: syntax=cpp ts=4 sw=4 sts=4 sr et


