#ifndef _SMARTHOME_CLIENT_H
#define _SMARTHOME_CLIENT_H

#include "RN2483.h"
#include "flash.h"

struct InboundPacketHeader
{
    bool receiveError;
    uint8_t type;
    uint8_t to;
    uint8_t from;
    unsigned long timestamp;
    unsigned long payloadLength;
};

struct FirmwareInfoResponse
{
    bool receiveError;
    unsigned long totalLength;
    unsigned long crc32;
};

class SmartHomeServerClientClass
{
private:
    uint8_t loraAddr = 0;

    bool sendMessage(uint8_t type, unsigned char *payload, size_t payloadLength);
    bool hasValidTimestamp(InboundPacketHeader inboundPacketHeader);
    InboundPacketHeader receiveMessage(uint8_t *payloadBuffer, size_t payloadBufferLength, const unsigned long timeout);

    FirmwareInfoResponse getFirmwareInfo();

public:
    SmartHomeServerClientClass();

    int headerSize = 11;

    bool ping();
    InboundPacketHeader receivePong();
    void setLoraAddr(uint8_t addr);

    bool sendAck(uint8_t type);

    bool sendData(
        int temp,
        bool heaterOn,
        uint8_t firmwareVersion,
        uint8_t temperatureError);

    InboundPacketHeader receiveRequest(uint8_t *payloadBuffer, size_t payloadBufferLength);
    void upgradeFirmware(FirmwareInfoResponse firmwareInfoResponse);
};

extern SmartHomeServerClientClass SmartHomeServerClient;

#endif