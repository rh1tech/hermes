#include <Arduino.h>
#include <cstring>
#include "globals.h"

#define TX_BUF_SIZE 256 // Buffer used to read from serial before writing to TCP
static uint8_t txBuf[TX_BUF_SIZE];
static char plusCount = 0;         // Count of consecutive '+' received
static unsigned long plusTime = 0; // Timestamp of detecting "+++"

// Move data from serial terminal to TCP / PPP
void terminalToTcp()
{
  if (!Serial.available())
    return;
  size_t max_buf_size = telnet ? (TX_BUF_SIZE / 2) : TX_BUF_SIZE; // Leave half free for 0xFF escaping
  size_t avail = Serial.available();
  size_t len = (avail < max_buf_size) ? avail : max_buf_size;
  if (len == 0)
    return;
  Serial.readBytes(txBuf, len);

  // Detect "+++"
  for (size_t i = 0; i < len; ++i)
  {
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

  if (ppp)
  {
    pppos_input(ppp, txBuf, len);
  }
  else
  {
    tcpClient.write(txBuf, len);
  }
  yield();
}

// Handle a telnet control code after initial 0xFF consumed
void handleTelnetControlCode()
{
#ifdef DEBUG
  Serial.print("<t>");
#endif
  if (!tcpClient.available())
  {
#ifdef DEBUG
    Serial.print("</t>");
#endif
    return;
  }

  uint8_t b1 = tcpClient.read();

  // Escaped 0xFF (IAC IAC)
  if (b1 == 0xFF)
  {
    Serial.write((uint8_t)0xFF);
    Serial.flush();
#ifdef DEBUG
    Serial.print("</t>");
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
    Serial.print("</t>");
#endif
    return;
  }

  uint8_t b2 = tcpClient.read();

#ifdef DEBUG
  Serial.print(b2);
  Serial.flush();
#endif

  // Respond to DO / WILL
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
  Serial.print("</t>");
#endif
}

// Move data from TCP / PPP to terminal
void tcpToTerminal()
{
  while (tcpClient.available() && txPaused == false)
  {
    uint8_t rxByte = tcpClient.read();
    if (telnet && rxByte == 0xFF)
    {
      handleTelnetControlCode();
    }
    else
    {
      Serial.write(rxByte);
      yield();
      Serial.flush();
      yield();
    }
    handleFlowControl();
  }
}

// Detect escape sequence "+++"
void handleEscapeSequence()
{
  if (plusCount >= 3)
  {
    if (millis() - plusTime > 1000)
    {
      cmdMode = true;
      sendResult(R_OK);
      plusCount = 0;
    }
  }
}

// Connected mode handler
void handleConnectedMode()
{
  terminalToTcp();
  tcpToTerminal();
  handleEscapeSequence();
}
