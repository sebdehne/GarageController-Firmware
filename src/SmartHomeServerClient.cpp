#include "SmartHomeServerClient.h"
#include "logger.h"
#include "utils.h"
#include "time.h"
#include "RN2483-v2.h"

SmartHomeServerClientClass::SmartHomeServerClientClass() {}

void SmartHomeServerClientClass::messageIgnored()
{
    if (currentState == MESSAGE_RECEIVED)
    {
        currentState = LISTENING;
        currentStateChangedAt = millis();
    }
}
bool SmartHomeServerClientClass::hasValidTimestamp()
{
    DateTime dateTime = Time.readTime();

    long delta = dateTime.secondsSince2000 - receivedMessage.timestamp;

    if (delta < 30 && delta > -30)
    {
        return true;
    }
    Serial.println(dateTime.secondsSince2000);
    Serial.println(receivedMessage.timestamp);
    return false;
}
bool SmartHomeServerClientClass::handleReceivedMessage()
{
    receivedMessage.receiveError = true;
    bool decrypted = CryptUtil.decrypt(
        RN2483V2.receivedData,
        RN2483V2.receivedDataLength,
        receivedBuffer);
    RN2483V2.dataConsumed();

    if (!decrypted)
    {
        Log.log("decrypt failed");
        return false;
    }

    receivedMessage.to = receivedBuffer[0];
    receivedMessage.from = receivedBuffer[1];
    receivedMessage.type = receivedBuffer[2];
    receivedMessage.timestamp = toUInt(receivedBuffer, 3);
    receivedMessage.payloadLength = toUInt(receivedBuffer, 7);
    receivedMessage.receiveError = false;
    for (int i = 0; i < receivedMessage.payloadLength; i++)
    {
        receivedPayload[i] = receivedBuffer[headerSize + i];
    }
    return true;
}

bool SmartHomeServerClientClass::scheduleSendMessage(unsigned long waitBeforeSending, uint8_t type, unsigned char *payload, size_t payloadLength)
{
    if (payloadLength + headerSize + CryptUtil.encryptionOverhead > sizeof(scheduledOutboundMessagePayload))
    {
        Log.log("Payload too large");
        return false;
    }

    DateTime time = Time.readTime();

    uint8_t data[payloadLength + headerSize];
    uint8_t i = 0;
    data[i++] = 1;        // to
    data[i++] = loraAddr; // from
    data[i++] = type;
    writeUint32(time.secondsSince2000, data, i++); // 3-6
    i = i + 3;
    writeUint32(payloadLength, data, i++); // 7-10 - payloadsize
    i = i + 3;

    for (int pi = 0; pi < payloadLength; pi++)
    {
        data[i + pi] = payload[pi];
    }

    if (!CryptUtil.encrypt(
            data,
            headerSize + payloadLength,
            scheduledOutboundMessagePayload,
            sizeof(scheduledOutboundMessagePayload)))
    {
        Log.log("Encryption failed");
        return false;
    }

    scheduledOutboundMessagePayloadLength = headerSize + payloadLength + CryptUtil.encryptionOverhead;
    scheduledOutboundMessageWaittime = waitBeforeSending;

    currentState = RESPONSE_SCHEDULED;
    currentStateChangedAt = millis();

    Log.log("SmartHomeServerClientClass.messageConsumedScheduleResponse() - Message scheudled");
    return true;
}

void SmartHomeServerClientClass::run()
{
    RN2483V2.run();

    switch (currentState)
    {
    case WAITING_FOR_LORA_GETTING_READY:
        if (RN2483V2.readyToTx())
        {
            uint8_t data[headerSize + 1 + 16];
            DateTime time = Time.readTime();
            data[0] = 1;                                 // to addr
            data[1] = 0;                                 // from addr
            data[2] = 0;                                 // setup request
            writeUint32(time.secondsSince2000, data, 3); // 3-6
            writeUint32(17, data, 7);                    // 7-10 - payloadsize
            data[11] = FIRMWARE_VERSION;
            writeSerial16Bytes(data, 12);

            uint8_t dataEncryped[sizeof(data) + CryptUtil.encryptionOverhead];
            if (!CryptUtil.encrypt(
                    data,
                    sizeof(data),
                    dataEncryped,
                    sizeof(dataEncryped)))
            {
                return;
            }
            RN2483V2.scheduleTx(dataEncryped, sizeof(dataEncryped));
            Log.log("Sendt PING/Setup");
            currentState = WAITING_FOR_PONG;
            currentStateChangedAt = millis();
        }
        break;

    case WAITING_FOR_PONG:
        if (millis() - currentStateChangedAt > 5000)
        {
            Log.log("PING retry");
            currentState = WAITING_FOR_LORA_GETTING_READY;
            currentStateChangedAt = millis();
        }
        else if (RN2483V2.currentState == LISTENING_WAITING_FOR_DATA_CONSUME)
        {
            if (handleReceivedMessage())
            {
                if (receivedMessage.from == 1 && receivedMessage.payloadLength == 17 && receivedBuffer[headerSize + 0] == FIRMWARE_VERSION)
                {
                    loraAddr = receivedMessage.to;
                    Log.log("Received PONG");
                    Time.setTime(receivedMessage.timestamp);
                    currentState = LISTENING;
                    currentStateChangedAt = millis();
                }
                else
                {
                    char buf[350];
                    snprintf(buf, 350, "Invalid message received, from: %u payloadLength: %u", receivedMessage.from, receivedMessage.payloadLength);
                    Log.log(buf);
                }
            }
        }
        break;

    case LISTENING:
        if (RN2483V2.currentState == LISTENING_WAITING_FOR_DATA_CONSUME)
        {
            if (handleReceivedMessage())
            {
                if (receivedMessage.to != loraAddr)
                {
                    Log.log("Msg not for me");
                }
                else if (!hasValidTimestamp())
                {
                    Log.log("Invalid timestamp");
                }
                else
                {
                    // local clock is drifting too much - need to update each time
                    Time.setTime(receivedMessage.timestamp);
                    currentState = MESSAGE_RECEIVED;
                    currentStateChangedAt = millis();
                }
            }
        }
        break;
    case MESSAGE_RECEIVED:
        break;
    case RESPONSE_SCHEDULED:
        if (millis() - currentStateChangedAt >= scheduledOutboundMessageWaittime)
        {
            RN2483V2.scheduleTx(scheduledOutboundMessagePayload, scheduledOutboundMessagePayloadLength);
            Log.log("Message delivered to RN2483V2");
            currentState = LISTENING;
            currentStateChangedAt = millis();
        } else {
            Log.log("Wait time not passed yet");
        }
        break;

    default:
        break;
    }
}

SmartHomeServerClientClass SmartHomeServerClient;