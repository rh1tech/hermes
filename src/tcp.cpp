#if defined(ESP32)
  #include <Arduino.h>
#elif defined(ESP8266)
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
#else
  #include <Arduino.h>
#endif
#include <cstring>
#include "globals.h"
#include "xmodem.h"

#define TX_BUF_SIZE 256

static uint8_t txBuf[TX_BUF_SIZE];
static char plusCount = 0;
static unsigned long plusTime = 0;

// XMODEM state
static XModem *xmodem = nullptr;
static bool xmodemInProgress = false;
static bool waitingForXmodemResponse = false;
static unsigned long lastCOrNakSent = 0;

void terminalToTcp()
{
  if (!Serial.available())
    return;

  size_t max_buf_size = telnet ? (TX_BUF_SIZE / 2) : TX_BUF_SIZE;
  size_t avail = Serial.available();
  size_t len = (avail < max_buf_size) ? avail : max_buf_size;

  if (len == 0)
    return;

  Serial.readBytes(txBuf, len);

  // Check if user sent 'C' or NAK for XMODEM
  for (size_t i = 0; i < len; ++i)
  {
    if (txBuf[i] == 'C' || txBuf[i] == 0x15)
    { // 'C' or NAK
      waitingForXmodemResponse = true;
      lastCOrNakSent = millis();
      break;
    }

    // Detect "+++"
    if (txBuf[i] == '+')
    {
      if (++plusCount == 3)
        plusTime = millis();
    }
    else
    {
      plusCount = 0;
    }
  }

  // Telnet: duplicate each 0xFF (IAC)
  if (telnet)
  {
    for (int i = (int)len - 1; i >= 0; --i)
    {
      if (txBuf[i] == 0xFF)
      {
        memmove(&txBuf[i + 1], &txBuf[i], len - i);
        len++;
        txBuf[i] = 0xFF;
      }
    }
  }
  #ifdef PPP_ENABLED
  if (ppp)
  {
      pppos_input(ppp, txBuf, len);
  }
  else
  {
    tcpClient.write(txBuf, len);
  }
  #endif
  #ifndef PPP_ENABLED
  tcpClient.write(txBuf, len);
  #endif

  yield();
}

void handleTelnetControlCode()
{
#ifdef DEBUG
  Serial.print("t");
#endif
  if (!tcpClient.available())
  {
#ifdef DEBUG
    Serial.print("S");
#endif
    return;
  }

  uint8_t b1 = tcpClient.read();
  if (b1 == 0xFF)
  {
    Serial.write((uint8_t)0xFF);
    Serial.flush();
#ifdef DEBUG
    Serial.print("E");
#endif
    return;
  }

#ifdef DEBUG
  Serial.print(b1);
  Serial.print(",");
#endif
  if (!tcpClient.available())
  {
#ifdef DEBUG
    Serial.print("s");
#endif
    return;
  }

  uint8_t b2 = tcpClient.read();
#ifdef DEBUG
  Serial.print(b2);
  Serial.flush();
#endif

  if (b1 == DO)
  {
    tcpClient.write((uint8_t)0xFF);
    tcpClient.write((uint8_t)WONT);
    tcpClient.write(b2);
  }
  else if (b1 == WILL)
  {
    tcpClient.write((uint8_t)0xFF);
    tcpClient.write((uint8_t)DO);
    tcpClient.write(b2);
  }

#ifdef DEBUG
  Serial.print("T");
#endif
}

void tcpToTerminal()
{
  while (tcpClient.available() && txPaused == false)
  {
    uint8_t rxByte = tcpClient.read();

    // If already in XMODEM transfer, process through XModem
    if (xmodemInProgress)
    {
      if (!xmodem->processIncomingByte(rxByte))
      {
        delete xmodem;
        xmodem = nullptr;
        xmodemInProgress = false;
        waitingForXmodemResponse = false;
      }
      continue;
    }

    // Check for XMODEM response to our 'C' or NAK
    if (waitingForXmodemResponse)
    {
      if (millis() - lastCOrNakSent > 5000)
      {
        // Timeout - no XMODEM response
        waitingForXmodemResponse = false;
      }
      else if (rxByte == XModem::SOH || rxByte == XModem::STX)
      {
        // XMODEM transfer detected!
        Serial.println("\n\r[+] XMODEM transfer detected, starting receive...");

        // Determine mode based on what we sent and block type
        XModemMode mode = (rxByte == XModem::STX) ? XMODEM_1K : XMODEM_CRC;
        xmodem = new XModem(tcpClient, Serial, mode);
        xmodemInProgress = true;
        waitingForXmodemResponse = false;

        // Process this SOH/STX byte
        xmodem->processIncomingByte(rxByte);
        continue;
      }
      // If it's not SOH/STX, fall through to normal processing
      if (rxByte != XModem::SOH && rxByte != XModem::STX)
      {
        waitingForXmodemResponse = false;
      }
    }

    // Telnet handling
    if (telnet && rxByte == 0xFF)
    {
      handleTelnetControlCode();
    }
    else
    {
      Serial.write(rxByte);
    }

    yield();
  }

  Serial.flush();
  yield();
  handleFlowControl();
}

void handleEscapeSequence()
{
  if (plusCount >= 3)
  {
    if (millis() - plusTime > 1000)
    {
      cmdMode = true;
      sendResult(RES_OK);
      plusCount = 0;
    }
  }
}

void handleConnectedMode()
{
  terminalToTcp();
  tcpToTerminal();
  handleEscapeSequence();
}
