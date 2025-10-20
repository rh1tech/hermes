#ifndef XMODEM_H
#define XMODEM_H

#include <Arduino.h>
#include <Client.h>

enum XModemMode
{
    XMODEM_CHECKSUM,
    XMODEM_CRC,
    XMODEM_1K
};

class XModem
{
public:
    XModem(Client &client, Stream &output, XModemMode mode = XMODEM_CRC);
    bool processIncomingByte(uint8_t rxByte);

    static const uint8_t SOH = 0x01;
    static const uint8_t STX = 0x02;
    static const uint8_t EOT = 0x04;
    static const uint8_t ACK = 0x06;
    static const uint8_t NAK = 0x15;
    static const uint8_t CAN = 0x18;
    static const uint8_t CRC = 'C';

private:
    Client &client_;
    Stream &out_;
    XModemMode mode_;
    size_t recvSize_ = 0;
    uint8_t blkNum_ = 1;
    size_t blockSize_;

    // State machine for receiving blocks
    enum State
    {
        WAIT_HEADER,
        WAIT_SEQ,
        WAIT_SEQ_COMP,
        WAIT_DATA,
        WAIT_CRC_HIGH,
        WAIT_CRC_LOW,
        WAIT_CHECKSUM
    };
    State state_ = WAIT_HEADER;
    
#ifdef ESP8266
    uint8_t blockBuf_[128];  // Use 128 bytes on ESP8266 to save RAM
#else
    uint8_t blockBuf_[1024];  // ESP32 has more RAM
#endif
    
    size_t dataIndex_ = 0;
    uint8_t expectedSeq_, seqComp_;
    uint16_t receivedCrc_ = 0;

    static const size_t BLOCK_SIZE = 128;
    static const size_t BLOCK_1K_SIZE = 1024;

    uint8_t calcChecksum(const uint8_t *data, size_t len);
    uint16_t calcCRC(const uint8_t *data, size_t len);
};

#endif
