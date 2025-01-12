#include "SmartHomeServerClientWifi.h"
#include "ledstripe.h"
#include "utils.h"

void SmartHomeServerClientWifiClass::scheduleReconnect()
{
    currentState = SmartHomeServer_WIFI_CONNECT;
    currentStateChangedAt = millis();
}

bool SmartHomeServerClientWifiClass::hasMessage()
{
    return currentState == SmartHomeServer_MESSAGE_RECEIVED;
}

void SmartHomeServerClientWifiClass::markMessageConsumed()
{
    currentState = SmartHomeServer_READING_DATA;
    currentStateChangedAt = millis();
}

void SmartHomeServerClientWifiClass::scheduleNotify(bool asNotify)
{
    if (asNotify)
    {
        sendMode = SmartHomeServerClientWifi_SendMode_SEND_AS_NOTIFY;
    }
    else
    {
        sendMode = SmartHomeServerClientWifi_SendMode_SEND_AS_DATA_RESPONSE;
    }
};

void SmartHomeServerClientWifiClass::run()
{
    if (millis() - lastMsgFromServerAt > WIFI_RESET_IF_NO_MSG_FROM_SERVER_FOR)
    {
        Log.log("No msg from server, resetting wifi");
        currentState = SmartHomeServer_WIFI_CONNECT;
        currentStateChangedAt = millis();
        lastMsgFromServerAt = millis();
    }

    switch (currentState)
    {
    case SmartHomeServer_INIT:
    {

        // pinMode(NINA_RESETN, OUTPUT);
        // digitalWrite(NINA_RESETN, 1);
        // delay(100);
        // digitalWrite(NINA_RESETN, 0);
        // delay(100);

        if (WiFi.status() == WL_NO_MODULE)
        {
            Log.log("No wifi module detected");
            delay(10000);
            return;
        }

        static const char *expectedFirmeware = WIFI_FIRMWARE_LATEST_VERSION;
        static const char *actualFirmware = WiFi.firmwareVersion();

        if (strcmp(expectedFirmeware, actualFirmware) != 0)
        {
            char buf[1024];
            snprintf(buf, sizeof(buf), "Firmware upgrade needed: expected: %s, actual: %s", expectedFirmeware, actualFirmware);
            Log.log(buf);
            delay(10000);
            return;
        }

        currentState = SmartHomeServer_WIFI_CONNECT;
        currentStateChangedAt = millis();

        break;
    }
    case SmartHomeServer_WIFI_CONNECT:
    {
        WiFi.disconnect();
        currentState = SmartHomeServer_WIFI_CONNECTING;
        currentStateChangedAt = millis();
        break;
    }
    case SmartHomeServer_WIFI_CONNECTING:
        if (WiFi.begin(WIFI_SSID, WIFI_PASS) == WL_CONNECTED)
        {
            Log.log("Wifi Connected");

            // WiFi.localIP() hangs
            // IPAddress localIP = WiFi.localIP();
            // char buf[255];
            // sniprintf(buf, sizeof(buf), "SmartHomeServerClientWifi - wifi connected. Listening on: %u.%u.%u.%u:%u", localIP[0], localIP[1], localIP[2], localIP[3], LOCAL_UDP_PORT);
            // Log.log(buf);

            if (Udp.begin(LOCAL_UDP_PORT))
            {
                Log.log("Listening on UDP");
                currentState = SmartHomeServer_READING_DATA;
                currentStateChangedAt = millis();
            }
            else
            {
                Log.log("Udp.begin() - error");
            }
        }

        break;
    case SmartHomeServer_MESSAGE_RECEIVED:
    {
        Log.log("Handling request");
        uint8_t msgType = receivedMessage[0];

        switch (msgType)
        {
        case 0: // DATA_REQUEST
        {
            scheduleNotify(false);
            break;
        }
        case 2: // CMD_CEILING_LIGHT_ON
        {
            LedStripe.target_ceilingLight = true;

            Log.log("Sending...CMD_CEILING_LIGHT_ON-ACK");
            sendBuf[0] = 5;
            sendLen = 1;
            Udp.beginPacket(SmartHomeServer_IP, SmartHomeServer_PORT);
            Udp.write(sendBuf, sendLen);
            Udp.endPacket();
            break;
        }
        case 3: // CMD_CEILING_LIGHT_OFF
        {
            LedStripe.target_ceilingLight = false;

            Log.log("Sending...CMD_CEILING_LIGHT_OFF-ACK");
            sendBuf[0] = 5;
            sendLen = 1;
            Udp.beginPacket(SmartHomeServer_IP, SmartHomeServer_PORT);
            Udp.write(sendBuf, sendLen);
            Udp.endPacket();
            break;
        }
        case 4: // CMD_LEDSTRIPE
        {
            uint16_t requestedMilliVoltsCh0 = toUint16_t(receivedMessage, 1);
            uint16_t requestedMilliVoltsCh1 = toUint16_t(receivedMessage, 3);

            LedStripe.ch0_target = requestedMilliVoltsCh0;
            LedStripe.ch1_target = requestedMilliVoltsCh1;

            Log.log("Sending...CMD_LEDSTRIPE-ACK");
            sendBuf[0] = 5;
            sendLen = 1;
            Udp.beginPacket(SmartHomeServer_IP, SmartHomeServer_PORT);
            Udp.write(sendBuf, sendLen);
            Udp.endPacket();
            break;
        }

        default:
        {
            Log.log("Ignoring UDP msg");
            break;
        }
        }

        markMessageConsumed();

        break;
    }

    case SmartHomeServer_READING_DATA:
    {
        // handle inbound
        if (Udp.parsePacket())
        {
            IPAddress remoteIp = Udp.remoteIP();
            uint16_t remotePort = Udp.remotePort();
            char buf[255];
            sniprintf(buf, sizeof(buf), "Received msg from: %u.%u.%u.%u:%u", remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3], remotePort);
            Log.log(buf);

            receivedMessageLengh = Udp.read(receivedMessage, sizeof(receivedMessage));
            currentState = SmartHomeServer_MESSAGE_RECEIVED;
            currentStateChangedAt = millis();
        }

        // handle outbound
        switch (sendMode)
        {
        case SmartHomeServerClientWifi_SendMode_SEND_AS_DATA_RESPONSE:
        case SmartHomeServerClientWifi_SendMode_SEND_AS_NOTIFY:
        {
            sendBuf[0] = 1; // TYPE: DATA
            sendBuf[1] = LedStripe.currentCeilingLight();
            writeUint16(LedStripe.currentMilliVoltsCh0(), sendBuf, 2);
            writeUint16(LedStripe.currentMilliVoltsCh1(), sendBuf, 4);
            sendBuf[6] = sendMode == SmartHomeServerClientWifi_SendMode_SEND_AS_NOTIFY;
            sendLen = 7;

            // send it
            Udp.beginPacket(SmartHomeServer_IP, SmartHomeServer_PORT);
            Udp.write(sendBuf, sendLen);
            Udp.endPacket();

            sendMode = SmartHomeServerClientWifi_SendMode_NO_SEND;
            Log.log("Sending DATA response...done");

            break;
        }

        default:
            break;
        }

        break;
    }

    default:
        break;
    }
}

SmartHomeServerClientWifiClass SmartHomeServerClientWifi;
