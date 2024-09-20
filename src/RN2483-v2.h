#ifndef _RN2483_H
#define _RN2483_H

#include <Arduino.h>
#include "crypto.h"

enum ExecutionState
    {
        INIT_COOLDOWN,
        INIT,
        SETUP_WAITING_VERSION,
        SETUP_WAITING_ACK_MAC_PAUSE,
        SETUP_WAITING_ACK_SET_PWR,
        SETUP_WAITING_ACK_SET_SF,
        LISTENING_RX_SENT_WAITING_FOR_OK,
        LISTENING_WAITING_FOR_DATA,
        LISTENING_RXSTOP_SENT_WAITING_FOR_OK,
        LISTENING_WAITING_FOR_DATA_CONSUME, // not listening
        SENDING_TX_SENDT_WAITING_FOR_OK,
        SENDING_WAITING_FOR_TRANSMISSION
    };

    enum TxStatus
    {
        SCHEDULED,
        SENDING,
        SUCCESS,
        FAILED
    };

class RN2483V2Class
{
private:
    bool read();
    void scheduleReset();
    void stopListening();
    void startListening();
    void calcNextState();
    bool currentStateTimeout(unsigned long timeoutInMs);

    char rxBuffer[300]; // maxDataSize + 9 for ("radio_tx ")
    size_t rxBufferBytesRead = 0;
    uint8_t txBuffer[255]; // maxDataSize
    size_t txBufferBytesToWrite = 0;

public:
    RN2483V2Class();

    size_t maxDataSize = 255;

    // state handling
    ExecutionState currentState = INIT;
    unsigned long currentStateChangedAt = millis();

    // tx handling
    bool readyToTx();
    TxStatus lastTxStatus = SUCCESS;
    void scheduleTx(uint8_t *data, size_t dataLen);

    // rx handling
    uint8_t receivedData[255]; // maxDataSize
    size_t receivedDataLength = 0;
    void dataConsumed();

    // do work if possible - non-blocking functions - should be called in a loop to make progress
    void run();
};

extern RN2483V2Class RN2483V2;

#endif