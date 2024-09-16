#ifndef _H_FLASH
#define _H_FLASH

#include <Arduino.h>
#include "logger.h"
#include <CRC32.h>

#define FIRMWARE_START_ADDR 0x2000
#define FIRMWARE_TEMP_START_ADDR 0x2000

class FlashUtilsClass
{
private:
    const unsigned long pageSize = 64; // read from NVMCTRL->PARAM.bit.PSZ
    const unsigned long rowSize = pageSize * 4;

    unsigned long addrFirmware;
    unsigned long addrTempFirmware;
    unsigned long addrUserdata;

    uint8_t writeBuffer[4];
    uint8_t writeBufferIndex;
    uint32_t * writeBufferAddr;

    uint32_t *writeAddr;

public:
    FlashUtilsClass();

    void init();

    void prepareWritingFirmware();
    unsigned long applyFirmwareAndReset(unsigned long firmwareLength, unsigned long crc32);

    void write(uint8_t b);
    void finishWriting();

    void prepareWritingUserdata();

    uint8_t readUserdata(unsigned long offset);
};

extern FlashUtilsClass FlashUtils;

#endif