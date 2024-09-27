#ifndef _SMARTHOME_CLIENT_H
#define _SMARTHOME_CLIENT_H

#include "RN2483-v2.h"
#include "logger.h"

struct SmartHomeServerMessageHeader
{
    bool receiveError;
    uint8_t type;
    uint8_t to;
    uint8_t from;
    unsigned long timestamp;
    unsigned long payloadLength;
};

enum SmartHomeServerClientState
{
    WAITING_FOR_LORA_GETTING_READY,
    WAITING_FOR_PONG,
    LISTENING,
    MESSAGE_RECEIVED
};

class SmartHomeServerClientClass
{
private:
    uint8_t loraAddr = 0;
    int headerSize = 11;
    uint8_t receivedBuffer[255];

    bool handleReceivedMessage();
    bool hasValidTimestamp();

public:
    SmartHomeServerClientClass();

    // internal status
    SmartHomeServerClientState currentState = WAITING_FOR_LORA_GETTING_READY;
    unsigned long currentStateChangedAt = millis();

    // RX
    SmartHomeServerMessageHeader receivedMessage;
    uint8_t receivedPayload[100];
    bool sendMessage(uint8_t type, unsigned char *payload, size_t payloadLength);
    void messageConsumed();

    // do work if possible - non-blocking functions - should be called in a loop to make progress
    void run();
};

extern SmartHomeServerClientClass SmartHomeServerClient;

#endif