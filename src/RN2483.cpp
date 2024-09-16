#include "RN2483.h"
#include "logger.h"
#include "utils.h"

RN2483Class::RN2483Class() {}

void RN2483Class::setup()
{
    int bytesRead;
    char buf[100];
    char buf2[100];

    // @  0ms power on
    // @ 99ms TX LOW->HIGH
    // @257ms transmits its version number
    // (but Seeeduino XIAO bootloader consumes 2000ms anyway)

    // setup UART to RN2483A module
    while (1)
    {
        Log.log("Setting up communication to RN2483...");
        Serial1.end();

        // reset D2
        pinMode(2, OUTPUT);
        digitalWrite(2, LOW);
        delay(50);
        digitalWrite(2, HIGH);
        Serial1.begin(57600);

        // read its version number
        Log.log("Reset done, waiting for version string...");
        bytesRead = readResponse(buf, sizeof(buf), 2000);
        snprintf(buf2, sizeof(buf2), "Reset done, waiting for version string...got %u : %s", bytesRead, buf);
        Log.log(buf2);
        if (bytesRead > 0)
        {
            if (strncmp("RN2483", buf, 6) == 0)
            {
                break;
            }
        }
    }
    Log.log("Setting up communication to RN2483...Done");
}

bool RN2483Class::consumeAndLog()
{
    uint8_t hexString[1024];
    int bytesRead = 0;
    while (Serial1.available())
    {
        hexString[bytesRead++] = Serial1.read();
    }
    if (bytesRead > 0)
    {
        hexString[bytesRead] = 0;
        char buf[1024];
        snprintf(buf, sizeof(buf), "sinked text: %s", hexString);
        Log.log(buf);
        return true;
    }

    return false;
}

int RN2483Class::sendCommandRaw(const char cmd[], char *receiveBuf, size_t length)
{
    consumeAndLog();

    // send cmd
    Serial1.println(cmd);

    int bytesReceived = readResponse(receiveBuf, length, 1000);

    char logBuf[1024];
    if (bytesReceived > 0)
    {
        snprintf(logBuf, sizeof(logBuf), "LoRa cmd: %s -> %u: %s", cmd, bytesReceived, receiveBuf);
        Log.log(logBuf);
    }
    else
    {
        snprintf(logBuf, sizeof(logBuf), "LoRa cmd: %s -> %u", cmd, bytesReceived);
        Log.log(logBuf);
    }

    return bytesReceived;
}

int RN2483Class::readResponse(char *receiveBuf, size_t length, const unsigned long timeout)
{
    // read response
    unsigned long start = millis();
    int bytesRead = 0;
    int read = 0;
    do
    {
        read = Serial1.read();
        if (read >= 0)
        {
            receiveBuf[bytesRead] = read;
            bytesRead++;
            if (read == '\n')
            {
                break;
            }
        }
    } while (millis() - start < timeout);

    if (bytesRead == 0)
    {
        Log.log("readResponse() - nothing received");
        return -1;
    }

    // find string end
    int pos = -1;
    for (int i = 0; i < bytesRead; i++)
    {
        if (receiveBuf[i] == '\n' || receiveBuf[i] == '\r')
        {
            pos = i;
            break;
        }
    }

    if (pos == -1)
    {
        Log.log("Could not find LF");
        return -1;
    }

    // terminate string
    receiveBuf[pos] = 0;

    return pos;
}

bool RN2483Class::transmitData(uint8_t *plainText, size_t plainTextLen)
{

    if (plainTextLen > maxEncryptedDataSize)
    {
        Log.log("Payload length too large");
        return false;
    }

    uint8_t data[plainTextLen + CryptUtil.encryptionOverhead];
    if (!CryptUtil.encrypt(
            plainText,
            plainTextLen,
            data,
            plainTextLen + CryptUtil.encryptionOverhead))
    {
        return false;
    }

    return transmitDataRaw(data, plainTextLen + CryptUtil.encryptionOverhead);
}

bool RN2483Class::transmitDataRaw(uint8_t *cipherTextWithIv, size_t length)
{

    char hexString[maxEncryptedDataSize * 2 + 1]; // max 255 bytes (1 bytes takes 2 chars + 1 for \0)
    toHex(cipherTextWithIv, length, hexString);

    char cmdString[sizeof(hexString) + 10];
    snprintf(cmdString, sizeof(cmdString), "radio tx %s", hexString);

    char receiveBuf[100];

    // send cmd and check for "ok"
    int bytesReceived = sendCommandRaw(cmdString, receiveBuf, 100);
    if (bytesReceived > 0)
    {
        if (strncmp("ok", receiveBuf, 2) != 0)
        {
            char buf[100];
            snprintf(buf, 100, "Unexpected response: %s", receiveBuf);
            Log.log(buf);
            return false;
        }
    }
    else
    {
        Log.log("No valid response received after radio tx cmd");
        return false;
    }

    // receive "radio_tx_ok"
    bytesReceived = readResponse(receiveBuf, 100, 1000);
    if (bytesReceived > 0)
    {
        if (strncmp("radio_tx_ok", receiveBuf, 11) != 0)
        {
            char buf[100];
            snprintf(buf, 100, "Unexpected response: %s", receiveBuf);
            Log.log(buf);
            return false;
        }
    }
    else
    {
        Log.log("No valid response received while waiting for radio_tx_ok");
        return false;
    }

    return true;
}

int RN2483Class::receiveData(uint8_t *receiveBuf, const size_t receiveBufLength, const unsigned long timeout)
{
    if (receiveBufLength < maxDataSize)
    {
        Log.log("receiveData() - receiveBufLength too small");
        return -1;
    }

    uint8_t bufferEncrypted[maxEncryptedDataSize];
    int bytesReceived = receiveDataRaw(bufferEncrypted, maxEncryptedDataSize, timeout);
    if (bytesReceived < CryptUtil.encryptionOverhead)
    {
        Log.log("receiveData() - too few (or none) bytes received");
        return -1;
    }

    int plainTextSize = CryptUtil.decrypt(
        bufferEncrypted,
        bytesReceived,
        receiveBuf);

    if (plainTextSize < 0)
    {
        Log.log("Could not decrypt");
        return -1;
    }

    return plainTextSize;
}

int RN2483Class::receiveDataRaw(uint8_t *receiveBuf, const size_t receiveBufLength, const unsigned long timeout)
{
    const unsigned long startTime = millis();
    char buffer[(maxEncryptedDataSize * 2) + 20 + 1];
    if (receiveBufLength < maxEncryptedDataSize)
    {
        Log.log("receiveDataRaw() - receiveBufLength too small");
        return -1;
    }

    while (millis() - startTime < timeout)
    {
        int bytesReceived = sendCommandRaw("radio rx 0", buffer, sizeof(buffer));
        if (bytesReceived <= 0)
        {
            return -1;
        }
        if (strncmp(buffer, "ok", 2) != 0)
        {
            continue;
        }

        unsigned long duration = millis() - startTime;
        if (duration > timeout)
        {
            return -1;
        }

        char buf[1000];
        snprintf(buf, sizeof(buf), "About to listen for incoming RF... timeout: %lu", timeout - duration);
        Log.log(buf);
        bytesReceived = readResponse(buffer, sizeof(buffer), timeout - duration);
        if (bytesReceived <= 0)
        {
            continue;
        }

        snprintf(buf, sizeof(buf), "Received: %s", buffer);
        Log.log(buf);

        if (strncmp(buffer, "radio_rx ", 9) != 0)
            continue;

        // find the start pos of the hex string after 'radio_rx '
        int hexStartPos = 7;
        while (buffer[++hexStartPos] == ' ')
            ;

        // decode hex string
        bytesReceived = fromHex(buffer + hexStartPos, receiveBuf, receiveBufLength);
        if (bytesReceived < 0)
        {
            snprintf(buf, sizeof(buf), "Could parse hex string: %s", buffer + hexStartPos);
            Log.log(buf);
            continue;
        }

        return bytesReceived;
    }

    Log.log("Timeout while receiving a message");
    sendCommandRaw("radio rxstop", buffer, sizeof(buffer));
    return -1;
}

RN2483Class RN2483;