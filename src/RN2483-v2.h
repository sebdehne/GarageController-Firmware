#ifndef _RN2483_H
#define _RN2483_H

#include <Arduino.h>
#include "crypto.h"

class RN2483Class
{
private:
    bool transmitDataRaw(uint8_t *cipherTextWithIv, size_t length);
    int receiveDataRaw(uint8_t *receiveBuf, size_t receiveBufLength, const unsigned long timeout);

public:
    RN2483Class();

    enum ExecutionState
    {
        IDLE,
        LISTENING,
        HIGH
    };

    ExecutionState currentState = IDLE;

    size_t maxEncryptedDataSize = 255;
    size_t maxDataSize = maxEncryptedDataSize - CryptUtil.encryptionOverhead;

    void setup();
    int readResponse(char *receiveBuf, size_t length, const unsigned long timeout);
    int sendCommandRaw(const char cmd[], char *receiveBuf, size_t length);
    bool consumeAndLog();

    bool transmitData(uint8_t *plainText, size_t plainTextLen);
    int receiveData(uint8_t *receiveBuf, const size_t receiveBufLength, const unsigned long timeout);
};

extern RN2483Class RN2483;

#endif