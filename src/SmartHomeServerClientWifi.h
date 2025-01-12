#ifndef _SMARTHOME_CLIENT_WIFI_H
#define _SMARTHOME_CLIENT_WIFI_H

#include "logger.h"
#include <WiFiNINA.h>
#include "secrets.h"
#include "config.h"
#include <SPI.h>

enum SmartHomeServerClientWifiState
{
    SmartHomeServer_INIT,
    SmartHomeServer_WIFI_CONNECT,
    SmartHomeServer_WIFI_CONNECTING,
    SmartHomeServer_READING_DATA,
    SmartHomeServer_MESSAGE_RECEIVED,
};

enum SmartHomeServerClientWifi_SendMode
{
    SmartHomeServerClientWifi_SendMode_NO_SEND,
    SmartHomeServerClientWifi_SendMode_SEND_AS_NOTIFY,
    SmartHomeServerClientWifi_SendMode_SEND_AS_DATA_RESPONSE,
};

class SmartHomeServerClientWifiClass
{
private:
    WiFiUDP Udp;
    SmartHomeServerClientWifiState currentState = SmartHomeServer_INIT;
    unsigned long currentStateChangedAt = millis();
    SmartHomeServerClientWifi_SendMode sendMode = SmartHomeServerClientWifi_SendMode_NO_SEND;
    uint8_t sendBuf[1000];
    size_t sendLen = 0;

    unsigned long lastMsgFromServerAt = millis();

public:
    uint8_t receivedMessage[255];
    size_t receivedMessageLengh;

    void scheduleReconnect();
    void run();
    bool hasMessage();
    void markMessageConsumed();
    void scheduleNotify(bool withAck);
};

extern SmartHomeServerClientWifiClass SmartHomeServerClientWifi;

#endif