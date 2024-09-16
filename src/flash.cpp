#include "flash.h"

FlashUtilsClass::FlashUtilsClass()
{
}

extern "C"
{
    // these functions must be in RAM (.data) and NOT inlined
    // as they erase and copy the sketch data in flash

    __attribute__((long_call, noinline, section(".data#"))) void flashWaitForReady()
    {
        while (!NVMCTRL->INTFLAG.bit.READY)
            ;
    }

    __attribute__((long_call, noinline, section(".data#"))) static void flashErase(int address, int length, int pageSize)
    {
        int rowSize = pageSize * 4;
        for (int i = 0; i < length; i += rowSize)
        {
            NVMCTRL->ADDR.reg = ((uint32_t)(address + i)) / 2;
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;

            flashWaitForReady();
        }
    }

    __attribute__((long_call, noinline, section(".data#"))) static void flashCopyAndReset(int dest, int src, int length, int pageSize)
    {
        uint32_t *d = (uint32_t *)dest;
        uint32_t *s = (uint32_t *)src;

        flashErase(dest, length, pageSize);

        for (int i = 0; i < length; i += 4)
        {
            *d++ = *s++;

            flashWaitForReady();
        }

        NVIC_SystemReset();
    }
}

void FlashUtilsClass::init()
{

    // setup whole rows because earsing happens at row-level
    addrFirmware = FIRMWARE_START_ADDR;
    addrTempFirmware = addrFirmware + FIRMWARE_SIZE + (rowSize - (FIRMWARE_SIZE % rowSize));
    addrUserdata = addrTempFirmware + FIRMWARE_SIZE + (rowSize - (FIRMWARE_SIZE % rowSize));
    writeBufferAddr = (uint32_t *)writeBuffer;

#ifdef DEBUG
    char buf[1000];
    snprintf(buf, sizeof(buf), "Flash debug: addrFirmware=%lu, addrTempFirmware=%lu, addrUserdata=%lu",
             addrFirmware,
             addrTempFirmware,
             addrUserdata);
    Log.log(buf);
#endif
}

void FlashUtilsClass::prepareWritingUserdata()
{
    writeAddr = (uint32_t *)addrUserdata;
    writeBufferIndex = 0;

    // enable auto page writes
    NVMCTRL->CTRLB.bit.MANW = 0;
    // erase rows
    flashErase(
        addrUserdata,
        USERDATA_SIZE + (rowSize - (USERDATA_SIZE % rowSize)),
        pageSize);
}

void FlashUtilsClass::prepareWritingFirmware()
{
    writeAddr = (uint32_t *)addrTempFirmware;
    writeBufferIndex = 0;

    // enable auto page writes
    NVMCTRL->CTRLB.bit.MANW = 0;
    // erase rows
    flashErase(
        addrTempFirmware,
        addrUserdata - addrTempFirmware,
        pageSize);
}

void FlashUtilsClass::write(uint8_t b)
{
    writeBuffer[writeBufferIndex++] = b;

    if (writeBufferIndex == 4)
    {
        writeBufferIndex = 0;
        *writeAddr = *writeBufferAddr;
        writeAddr++;
        flashWaitForReady();
    }
}

void FlashUtilsClass::finishWriting()
{
    while ((int)writeAddr % pageSize || writeBufferIndex % 4)
    {
        write(0xff);
    }
}

uint8_t FlashUtilsClass::readUserdata(unsigned long offset)
{
    uint8_t *ptr;
    ptr = (uint8_t *)(addrUserdata + offset);
    return *ptr;
}

unsigned long FlashUtilsClass::applyFirmwareAndReset(unsigned long firmwareLength, unsigned long expectedCrc32)
{
    CRC32 crc32;

    crc32.reset();
    uint8_t *addr = (uint8_t *)addrTempFirmware;
    for (unsigned long i = 0; i < firmwareLength; i++)
    {
        crc32.update(*addr);
        addr++;
    }
    unsigned long firmwareChecksum = crc32.finalize();
    if (firmwareChecksum != expectedCrc32)
    {
        Log.log("Invalid CRC32 check");
        return firmwareChecksum;
    }

    // disable interrupts, as vector table will be erase during flash sequence
    noInterrupts();

    flashCopyAndReset(addrFirmware, addrTempFirmware, FIRMWARE_SIZE, pageSize);
    return 0;
}

FlashUtilsClass FlashUtils;
