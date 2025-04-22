#include <Arduino.h>
#include "globals.h"

#define TX_BUF_SIZE 256 // Buffer where to read from serial before writing to TCP
// (that direction is very blocking by the ESP TCP stack,
// so we can't do one byte a time.)
uint8_t txBuf[TX_BUF_SIZE];

char plusCount = 0;         // Go to AT mode at "+++" sequence, that has to be counted
unsigned long plusTime = 0; // When did we last receive a "+++" sequence

void terminalToTcp()
{
  // Transmit from terminal to TCP
  if (Serial.available())
  {
    led_on();

    // In telnet in worst case we have to escape every byte
    // so leave half of the buffer always free
    int max_buf_size;
    if (telnet == true)
      max_buf_size = TX_BUF_SIZE / 2;
    else
      max_buf_size = TX_BUF_SIZE;

    // Read from serial, the amount available up to
    // maximum size of the buffer
    size_t len = std::min(Serial.available(), max_buf_size);
    Serial.readBytes(&txBuf[0], len);

    // Enter command mode with "+++" sequence
    for (int i = 0; i < (int)len; i++)
    {
      if (txBuf[i] == '+')
        plusCount++;
      else
        plusCount = 0;
      if (plusCount >= 3)
      {
        plusTime = millis();
      }
      if (txBuf[i] != '+')
      {
        plusCount = 0;
      }
    }

    // Double (escape) every 0xff for telnet, shifting the following bytes
    // towards the end of the buffer from that point
    if (telnet == true)
    {
      for (int i = len - 1; i >= 0; i--)
      {
        if (txBuf[i] == 0xff)
        {
          for (int j = TX_BUF_SIZE - 1; j > i; j--)
          {
            txBuf[j] = txBuf[j - 1];
          }
          len++;
        }
      }
    }

    // Write the buffer to PPP or TCP finally
    if (ppp)
    {
      pppos_input(ppp, &txBuf[0], len);
    }
    else
    {
      tcpClient.write(&txBuf[0], len);
    }
    yield();
  }
}

void handleTelnetControlCode(uint8_t rxByte)
{
#ifdef DEBUG
  Serial.print("<t>");
#endif

  rxByte = tcpClient.read();
  if (rxByte == 0xff)
  {
    // 2 times 0xff is just an escaped real 0xff
    Serial.write(0xff);
    Serial.flush();
  }
  else
  {
// rxByte has now the first byte of the actual non-escaped control code
#ifdef DEBUG
    Serial.print(rxByte);
    Serial.print(",");
#endif

    uint8_t cmdByte1 = rxByte;
    rxByte = tcpClient.read();
    uint8_t cmdByte2 = rxByte;

// rxByte has now the second byte of the actual non-escaped control code
#ifdef DEBUG
    Serial.print(rxByte);
    Serial.flush();
#endif

    // We are asked to do some option, respond we won't
    if (cmdByte1 == DO)
    {
      tcpClient.write((uint8_t)255);
      tcpClient.write((uint8_t)WONT);
      tcpClient.write(cmdByte2);
    }
    // Server wants to do any option, allow it
    else if (cmdByte1 == WILL)
    {
      tcpClient.write((uint8_t)255);
      tcpClient.write((uint8_t)DO);
      tcpClient.write(cmdByte2);
    }
  }
#ifdef DEBUG
  Serial.print("</t>");
#endif
}

void tcpToTerminal()
{
  // why is txpaused checked here, sounds like the wrong direction.
  // tcp to terminal is receiving
  // shouldn't txpaused be checked in transmitting to pc, so pc can say, slow down?
  while (tcpClient.available() && txPaused == false)
  {
    led_on();
    uint8_t rxByte = tcpClient.read();

    // Is a telnet control code starting?
    if ((telnet == true) && (rxByte == 0xff))
    {
      handleTelnetControlCode(rxByte);
    }
    else
    {
      // Non-control codes pass through freely
      Serial.write(rxByte);
      yield();
      Serial.flush();
      yield();
    }
    handleFlowControl();
  }
}

void handleEscapeSequence()
{
  // If we have received "+++" as last bytes from serial port and there
  // has been over a second without any more bytes
  if (plusCount >= 3)
  {
    if (millis() - plusTime > 1000)
    {
      // tcpClient.stop();
      cmdMode = true;
      sendResult(R_OK);
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
