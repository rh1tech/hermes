#include "xmodem.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

XModem::XModem(Client &client, Stream &output, XModemMode mode)
    : client_(client), out_(output), mode_(mode)
{
    blockSize_ = (mode_ == XMODEM_1K) ? BLOCK_1K_SIZE : BLOCK_SIZE;
}

uint8_t XModem::calcChecksum(const uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    while (len--)
        sum += *data++;
    return sum;
}

uint16_t XModem::calcCRC(const uint8_t *data, size_t len)
{
    uint16_t crc = 0;
    while (len--)
    {
        crc ^= (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool XModem::processIncomingByte(uint8_t rxByte)
{
    switch (state_)
    {
    case WAIT_HEADER:
        if (rxByte == EOT)
        {
            client_.write(ACK);
            out_.println("\n\rTransfer completed");
            return false; // Transfer done
        }
        if (rxByte == SOH)
        {
            blockSize_ = BLOCK_SIZE;
            state_ = WAIT_SEQ;
            dataIndex_ = 0;
            return true;
        }
        if (rxByte == STX)
        {
            blockSize_ = BLOCK_1K_SIZE;
            state_ = WAIT_SEQ;
            dataIndex_ = 0;
            return true;
        }
        // Invalid header, send NAK
        client_.write(NAK);
        return true;

    case WAIT_SEQ:
        expectedSeq_ = rxByte;
        state_ = WAIT_SEQ_COMP;
        return true;

    case WAIT_SEQ_COMP:
        seqComp_ = rxByte;
        if (expectedSeq_ != blkNum_ || seqComp_ != (uint8_t)(255 - expectedSeq_))
        {
            client_.write(NAK);
            state_ = WAIT_HEADER;
            return true;
        }
        state_ = WAIT_DATA;
        return true;

    case WAIT_DATA:
        blockBuf_[dataIndex_++] = rxByte;
        if (dataIndex_ >= blockSize_)
        {
            if (mode_ == XMODEM_CRC || mode_ == XMODEM_1K)
            {
                state_ = WAIT_CRC_HIGH;
            }
            else
            {
                state_ = WAIT_CHECKSUM;
            }
        }
        return true;

    case WAIT_CRC_HIGH:
        receivedCrc_ = (uint16_t)rxByte << 8;
        state_ = WAIT_CRC_LOW;
        return true;

    case WAIT_CRC_LOW:
        receivedCrc_ |= rxByte;
        if (receivedCrc_ == calcCRC(blockBuf_, blockSize_))
        {
            recvSize_ += blockSize_;
            client_.write(ACK);
            blkNum_++;
            out_.print("\rReceived: ");
            out_.print(recvSize_);
            out_.print(" bytes");
        }
        else
        {
            client_.write(NAK);
        }
        state_ = WAIT_HEADER;
        dataIndex_ = 0;
        return true;

    case WAIT_CHECKSUM:
        if (rxByte == calcChecksum(blockBuf_, blockSize_))
        {
            // *** Process block data here ***
            recvSize_ += blockSize_;
            client_.write(ACK);
            blkNum_++;
            out_.print("\rReceived: ");
            out_.print(recvSize_);
            out_.print(" bytes");
        }
        else
        {
            client_.write(NAK);
        }
        state_ = WAIT_HEADER;
        dataIndex_ = 0;
        return true;
    }
    return true;
}
