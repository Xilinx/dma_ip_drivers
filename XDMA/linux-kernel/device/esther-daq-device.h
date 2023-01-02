/*
 *  esther-daq-device.h
 */


#ifndef ESTHER_DAQ_DEVICE_H_
#define ESTHER_DAQ_DEVICE_H_
#include <atomic>
#include <mutex>
//#include <semaphore.h>
#include <string>
#include <thread>


#define DEBUG 1

#ifdef DEBUG
#include <time.h>
#endif

#define DMA_SIZE_DEFAULT (4096 * 1024)  // in bytes
#define INIT_HOLD (25 * 1000 * 1000)    // 32'd25_000_000;  (* 8ns) Initial HOLDING Time  = 0.2 s Max 4294967294/34 s 
#define TRIGGER_CHANNELS 3
#define TRIGGER_LEVELS (TRIGGER_CHANNELS * 2)
#define TRIGGER_COEFFS   2

#define CURRENT_FW_VERSION  0x01010001     // MAJOR/MINOR Version numbers

namespace xdma {

    /**
     * @brief ESTHER DAQ xdma device driver wrapper. Provides controllables that provide configuration and sampling functionality.
     */
    class EstherDaq {

        public:
            /**
             * @brief Construct the AtcaMimo32V2 object and initialize its internal data buffer.
             * @param bufferSize of the buffer to read to. Size is in MB.
             * @param dmaBufferSize of the buffer to read to. Size is in kB.
             *
             */
            EstherDaq(int devNumber);

            /**
             * @brief Clear buffer and destruct the AtcaMimo32V2 object.
             */
            virtual ~EstherDaq();
            /**
             * @brief Open the connection to the device, configure chopper and start acquisition via soft trigger.
             * @param chopped Set to true to enable chopper.
             * @param channelMask of channels to be returned in buffer. For channels not selected the 0 value will be used. 
             *                    Note: channel mask is inverted where LEAST significant bit represents first channel 0.
             * @return EXIT_SUCCESS (0) if successful, EXIT_FAILURE (1) otherwise
             */
            int open();
            //bool use32bits, uint16_t chopperPeriod, uint32_t channelMask);

            int readFirmwareVersion(uint32_t* version);
			/**
             * @brief Set interlock parameters on the board.
             * @param arrray 6 trigLevels to be used by the board.
             * @param array 2 delayCoefficients to set on the board.
             * @param initHold  
             * @return EXIT_SUCCESS (0) if successful, EXIT_FAILURE (1) otherwise
             */
            int setDelayParameters(short trigLevels[TRIGGER_LEVELS],
            float delayCoefficients[TRIGGER_COEFFS], float initialHold);

            /**
             * @brief Send software trigger to start acquisition. Device should be armed before sending the trigger.
             */
            int sendSoftwareTrigger();

            /**
             * @brief Gets delay value in 8ns units of the CH 0-> CH 1 pulse ToF
             *      It must be called befeore device close
             */
            uint32_t getTimeOfFlight();

            /**
             * @brief Clears all registers which stops the acquisition and chopper and then close the connection to the device.
             * @return EXIT_SUCCESS (0) if successful, EXIT_FAILURE (1) otherwise
             */
            int close();

            /**
             * @brief Read the data from the device. Device will first acquire all the samples and only then copy values to
             *        given buffer. Buffer has to be sufficiently large.
             * @param buffer to read to. Buffer allocation has to be big enough to accomodate 16 or 32 bit acquisition.
             * @return EXIT_SUCCESS (0) if successful, EXIT_FAILURE (1) otherwise
             */
            //int read(uint8_t* buffer, uint32_t nSamples, uint32_t timeoutMiliseconds);
            int read(uint8_t* buffer, uint32_t readSize);

            /**
             * @brief Add number of miliseconds to the given time.
             * @param a Time structure to add time to.
             * @param ms Miliseconds to add to current time.
             */
            static inline void timespec_add_ms(struct timespec *a, uint64_t ms) {
                uint64_t sum = a->tv_nsec + ms * 1000000L;  // Convert miliseconds to nanoseconds
                while (sum >= 1000000000L) { // Nanoseconds per second
                    a->tv_sec ++;
                    sum -= 1000000000L; // Nanoseconds per second
                }
                a->tv_nsec = sum;
            }

            int deviceNumber;
            uint8_t* mapBase;
            int deviceHandle;
            int deviceDmaHandle;

            //uint8_t* readBuffer;
//            uint64_t readBufferSize;    /** Size of the whole DMA buffer */
//            uint64_t readBufferOffset;  /** Offset of first unread source data */

//            size_t  dmaSize;            /** Size of the XDMA buffer used in ::read()*/
            uint8_t* dmaBuffer = NULL;  /** real XDMA buffer  **/

            //bool isDeviceOpen;
            std::atomic_bool isDeviceOpen;
            std::thread softTrigThread;
            // std::mutex readMutex;
            //std::mutex fpgaMutex;
            //sem_t readSemaphore;
            void writeFpgaReg(off_t target, uint32_t val);
            void writeControlReg(uint32_t val);
            uint32_t readFpgaReg(off_t target);
            uint32_t readControlReg();
            void softReset();
            void softTrig();
            static void* softTrigDelayed(void* device);

            uint64_t counter;

#ifdef DEBUG
            static struct timespec diff(struct timespec start, struct timespec end);
            struct timespec readTime;
            FILE* readTimesFile;
#endif

    };
}




#endif // ESTHER_DAQ_DEVICE_H_
//  vim: syntax=cpp ts=4 sw=4 sts=4 sr et


