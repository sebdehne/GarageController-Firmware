#include "RN2483-v2.h"
#include "logger.h"
#include "utils.h"

RN2483V2Class::RN2483V2Class() {}

void RN2483V2Class::scheduleTx(uint8_t *data, size_t dataLen)
{
    if (dataLen > maxDataSize)
    {
        Log.log("RN2483V2.scheduleTx() - cannot send more than 255 bytes");
        return;
    }
    if (lastTxStatus == SENDING || lastTxStatus == SCHEDULED)
    {
        Log.log("RN2483V2.scheduleTx() - there is already data scheduled - skipped sending");
        return;
    }
    int i = 0;
    size_t dataLenCopy = dataLen;
    while (dataLenCopy-- > 0)
    {
        txBuffer[i++] = *data++;
    }
    txBufferBytesToWrite = dataLen;
    lastTxStatus = SCHEDULED;
}
void RN2483V2Class::scheduleReset()
{
    currentState = INIT_COOLDOWN;
    currentStateChangedAt = millis();
}
bool RN2483V2Class::readyToTx()
{
    return txBufferBytesToWrite == 0 && currentState != INIT &&
           currentState != SETUP_WAITING_VERSION &&
           currentState != SETUP_WAITING_ACK_MAC_PAUSE &&
           currentState != SETUP_WAITING_ACK_SET_PWR &&
           currentState != SETUP_WAITING_ACK_SET_SF;
}
void RN2483V2Class::dataConsumed()
{
    receivedDataLength = 0;
}
void RN2483V2Class::stopListening()
{
    Serial1.println("radio rxstop");
    currentState = LISTENING_RXSTOP_SENT_WAITING_FOR_OK;
    currentStateChangedAt = millis();
    // prepare reading
    rxBufferBytesRead = 0;
}
void RN2483V2Class::startListening()
{
    Log.log("RN2483V2.startListening()");
    Serial1.println("radio rx 0");
    // move to next state
    currentState = LISTENING_RX_SENT_WAITING_FOR_OK;
    currentStateChangedAt = millis();
    // prepare reading
    rxBufferBytesRead = 0;
}
void RN2483V2Class::calcNextState()
{
    if (receivedDataLength > 0)
    {
        if (currentState != LISTENING_WAITING_FOR_DATA_CONSUME)
        {
            currentState = LISTENING_WAITING_FOR_DATA_CONSUME;
            currentStateChangedAt = millis();
        }
    }
    else if (lastTxStatus == SCHEDULED)
    {
        Log.log("RN2483V2.calcNextState() - sending now");
        // send now
        char hexString[maxDataSize * 2 + 1]; // max 255 bytes (1 bytes takes 2 chars + 1 for \0)
        toHex(txBuffer, txBufferBytesToWrite, hexString);

        char cmdString[sizeof(hexString) + 10];
        snprintf(cmdString, sizeof(cmdString), "radio tx %s", hexString);

        Serial1.println(cmdString);

        txBufferBytesToWrite = 0;
        lastTxStatus = SENDING;
        currentState = SENDING_TX_SENDT_WAITING_FOR_OK;
        currentStateChangedAt = millis();
    }
    else
    {
        // back to listening
        startListening();
    }
}

bool RN2483V2Class::currentStateTimeout(unsigned long timeoutInMs)
{
    unsigned long now = millis();
    unsigned long elapsed = now - currentStateChangedAt;
    return elapsed > timeoutInMs;
}

bool RN2483V2Class::read()
{
    int read;
    while (Serial1.available())
    {
        if (rxBufferBytesRead >= size_t(rxBuffer))
        {
            Log.log("RN2483V2.read() - Buffer is full - resetting");
            currentState = INIT;
            rxBufferBytesRead = 0;
            return false;
        }
        read = Serial1.read();
        if (read < 0) // no data available - give up
            break;

        if (read != '\r') // ignore \r
        {
            rxBuffer[rxBufferBytesRead++] = read;
        }

        if (read == '\n')
        {
            // end found
            rxBuffer[rxBufferBytesRead] = 0;
            return true;
        }
    }

    return false;
}

void RN2483V2Class::run()
{

    switch (currentState)
    {
    case INIT_COOLDOWN:
        if (millis() - currentStateChangedAt > 5000)
        {
            currentState = INIT;
            currentStateChangedAt = millis();
            rxBufferBytesRead = 0;
        }
        break;
    case INIT:
        Log.log("RN2483V2 - Setting up communication to RN2483...");
        Serial1.end();
        Serial1.begin(57600);
        Serial1.setTimeout(0);

        // reset D2
        pinMode(2, OUTPUT);
        digitalWrite(2, LOW);
        delay(50);
        digitalWrite(2, HIGH);

        // read its version number
        Log.log("RN2483V2.INIT - Reset done, waiting for version string...");

        // move to next state
        currentState = SETUP_WAITING_VERSION;
        currentStateChangedAt = millis();
        // prepare reading
        rxBufferBytesRead = 0;
        break;
    case SETUP_WAITING_VERSION:
        if (currentStateTimeout(2000))
        {
            Log.log("RN2483V2.SETUP_WAITING_VERSION - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("RN2483", rxBuffer, 6) == 0)
            {
                Log.log("RN2483V2 - got version");

                // next - mac pause
                Serial1.println("mac pause");
                // move to next state
                currentState = SETUP_WAITING_ACK_MAC_PAUSE;
                currentStateChangedAt = millis();
                // prepare reading
                rxBufferBytesRead = 0;
            }
            else
            {
                Log.log("RN2483V2.SETUP_WAITING_VERSION - Invalid string received - giving up");
                char buf[1024];
                snprintf(buf, sizeof(buf), "rxBuffer: %s", rxBuffer);
                Log.log(buf);
                scheduleReset();
            }
        }
        break;
    case SETUP_WAITING_ACK_MAC_PAUSE:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.SETUP_WAITING_ACK_MAC_PAUSE - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            // we got an int as response - we ignore it
            Log.log("RN2483V2 - mac pause - ok");

            // next - mac pause
            Serial1.println("radio set pwr -3");
            // move to next state
            currentState = SETUP_WAITING_ACK_SET_PWR;
            currentStateChangedAt = millis();
            // prepare reading
            rxBufferBytesRead = 0;
        }
        break;
    case SETUP_WAITING_ACK_SET_PWR:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.SETUP_WAITING_ACK_SET_PWR - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("ok", rxBuffer, 2) == 0)
            {
                Log.log("RN2483V2 - radio set pwr -3 - ok");

                Serial1.println("radio set sf sf7");
                // move to next state
                currentState = SETUP_WAITING_ACK_SET_SF;
                currentStateChangedAt = millis();
                // prepare reading
                rxBufferBytesRead = 0;
            }
            else
            {
                Log.log("RN2483V2.SETUP_WAITING_ACK_SET_PWR - Invalid string received - giving up");
                scheduleReset();
            }
        }
        break;
    case SETUP_WAITING_ACK_SET_SF:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.SETUP_WAITING_ACK_SET_SF - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("ok", rxBuffer, 2) == 0)
            {
                Log.log("RN2483V2 - radio set sf sf7 - ok");

                // next - setup done - find out what to do next
                calcNextState();
            }
            else
            {
                Log.log("RN2483V2.SETUP_WAITING_ACK_SET_SF - Invalid string received - giving up");
                scheduleReset();
            }
        }
        break;
    case LISTENING_RX_SENT_WAITING_FOR_OK:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.LISTENING_RX_SENT_WAITING_FOR_OK - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("ok", rxBuffer, 2) == 0)
            {
                currentState = LISTENING_WAITING_FOR_DATA;
                currentStateChangedAt = millis();
                Log.log("RN2483V2.LISTENING_RX_SENT_WAITING_FOR_OK - OK, listening...");
                // prepare reading
                rxBufferBytesRead = 0;
            }
            else
            {
                Log.log("RN2483V2.LISTENING_RX_SENT_WAITING_FOR_OK - Invalid string received - giving up");
                scheduleReset();
            }
        }
        break;
    case LISTENING_WAITING_FOR_DATA:
        if (read())
        {
            if (rxBufferBytesRead >= 9 && strncmp("radio_err", rxBuffer, 9) == 0) {
                Log.log("Got radio error");
                stopListening();
            }
            else if (rxBufferBytesRead < 9 || strncmp("radio_rx ", rxBuffer, 9) != 0)
            {
                char buf[350];
                snprintf(buf, 350, "Unexpected response: %s", rxBuffer);
                Log.log(buf);
            }
            else
            {
                char buf[1024];
                snprintf(buf, 1024, "Got message: %s", rxBuffer);
                Log.log(buf);

                // find the start pos of the hex string after 'radio_rx '
                int hexStartPos = 7;
                while (rxBuffer[++hexStartPos] == ' ')
                    ;
                // move data into receivedData
                receivedDataLength = fromHex(rxBuffer + hexStartPos, receivedData, size_t(receivedData));

                // ok - stop listening
                stopListening();
            }
        }
        else
        {
            // check if we have something to send
            if (lastTxStatus == SCHEDULED)
            {
                stopListening();
            }
        }

        break;
    case LISTENING_RXSTOP_SENT_WAITING_FOR_OK:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.LISTENING_RXSTOP_SENT_WAITING_FOR_OK - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("ok", rxBuffer, 2) != 0)
            {
                Log.log("RN2483V2.LISTENING_RXSTOP_SENT_WAITING_FOR_OK - Invalid string received - giving up");
                scheduleReset();
            }
            else
            {
                calcNextState();
            }
        }
        break;
    case LISTENING_WAITING_FOR_DATA_CONSUME:
        calcNextState();
        break;
    case SENDING_TX_SENDT_WAITING_FOR_OK:
        if (currentStateTimeout(1000))
        {
            Log.log("RN2483V2.SENDING_TX_SENDT_WAITING_FOR_OK - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("ok", rxBuffer, 2) != 0)
            {
                Log.log("RN2483V2.SENDING_TX_SENDT_WAITING_FOR_OK - Invalid string received - moving on");
                lastTxStatus = FAILED;
                calcNextState();
            }
            else
            {
                Log.log("RN2483V2.SENDING_TX_SENDT_WAITING_FOR_OK - got ok - moving on");
                currentState = SENDING_WAITING_FOR_TRANSMISSION;
                currentStateChangedAt = millis();
                rxBufferBytesRead = 0;
            }
        }
        break;
    case SENDING_WAITING_FOR_TRANSMISSION:
        if (currentStateTimeout(5000))
        {
            Log.log("RN2483V2.SENDING_WAITING_FOR_TRANSMISSION - timeout - resetting");
            scheduleReset();
            break;
        }
        if (read())
        {
            if (strncmp("radio_tx_ok", rxBuffer, 11) != 0)
            {
                Log.log("RN2483V2.SENDING_WAITING_FOR_TRANSMISSION - Invalid string received - moving on");
                lastTxStatus = FAILED;
            }
            else
            {
                Log.log("RN2483V2.SENDING_TX_SENDT_WAITING_FOR_OK - sending was successful");
                lastTxStatus = SUCCESS;
            }
            rxBufferBytesRead = 0;
            calcNextState();
        }
        break;
    default:
        Log.log("RN2483V2 - Invalid state");
        scheduleReset();
        break;
    }

    return;
}

RN2483V2Class RN2483V2;