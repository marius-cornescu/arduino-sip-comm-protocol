/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.
  
  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/
//= INCLUDES =======================================================================================
#include "Artizan-CommProtocol.h"


//= CONSTANTS ======================================================================================
byte RtznCommProtocol::_maxPayloadSize = MESSAGE_SIZE;
byte RtznCommProtocol::_msgSize = MESSAGE_SIZE + 1;
byte RtznCommProtocol::_msgRawSize = MESSAGE_SIZE + 4; // 2 prefix + 1 command + messageSize + 1 end

byte RtznCommProtocol::_msgCommandCodeIndex = 0;
byte RtznCommProtocol::_msgPayloadStartIndex = 1;
//------------------------------------------------
const char RtznCommProtocol::_MsgPrefix1 = 1; // SOH character
const char RtznCommProtocol::_MsgPrefix2 = 2; // STX character
const char RtznCommProtocol::_MsgEnd = 10;    // \n new-line character
//
byte RtznCommProtocol::_MsgAsciiShift = 20;    // shift all payload values by this char
//
const char RtznCommProtocol::_Command_NOP = 99;
//
const char RtznCommProtocol::_Command_POLL = 20;       // REQUEST UPDATE FROM PARNER
const char RtznCommProtocol::_Command_PUSH = 30;       // INFORM PARTNER
const char RtznCommProtocol::_Command_PUSH_POLL = 40;  // INFORM PARTNER AND REQUEST UPDATE

//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol(Stream& _port, const char *nodeRole) {
    this->port = &_port;
    this->nodeRole = nodeRole;

    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
//ooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol(Stream& _port, const char *nodeRole, 
        bool (*ProcessReceivedMessageFunction)(const char *), 
        const char* (*PrepareMessageToSendFunction)()
    ) {
    this->port = &_port;
    this->nodeRole = nodeRole;
    this->_ProcessReceivedMessageFunction = ProcessReceivedMessageFunction;
    this->_PrepareMessageToSendFunction = PrepareMessageToSendFunction;

    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

//==================================================================================================
bool RtznCommProtocol::hasMessageInInboxThenReadMessage() {
    int replySize = this->receiveData();
    //this->__logDebug("PARTNER reply size =", replySize);

    if (replySize > 0) {
        this->__logDebugMessage("Receive data", messageIn);
        return true;
    } else {
        //this->__logDebug("Receive NO data from PARTNER");
        return false;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnMessage() {
    this->__logDebug("actOnMessage");

    switch (this->messageIn[RtznCommProtocol::_msgCommandCodeIndex]) {
        case RtznCommProtocol::_Command_NOP: /*DO NOTHING*/ break;
        case RtznCommProtocol::_Command_PUSH: this->actOnPushMessage(); break;
        case RtznCommProtocol::_Command_POLL: this->actOnPollMessage(); break;
        case RtznCommProtocol::_Command_PUSH_POLL: this->actOnPushPollMessage(); break;
        default: /*UNKNOWN*/ break;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnPushMessage() {
    /* The PARTNER is informing me of a status change */
    this->__logDebug("actOnPushMessage:");

    const char* payload = &this->messageIn[RtznCommProtocol::_msgPayloadStartIndex];
    byte payloadSize = sizeof(payload) / sizeof(char);
    this->decodePayload(payload, payloadSize);
    this->haveToPublish = this->_ProcessReceivedMessageFunction(payload);
}
//==================================================================================================
void RtznCommProtocol::actOnPollMessage() {
    /* The PARTNER is asking for my status */
    /* The body of the request is empty */
    this->__logDebug("actOnPollMessage: Send data to PARTNER");

    // Send data to PARTNER
    const char* payload = this->_PrepareMessageToSendFunction();
    byte payloadSize = sizeof(payload) / sizeof(char);
    this->encodePayload(payload, payloadSize);
    this->sendMessage(RtznCommProtocol::_Command_PUSH, payload, payloadSize);
}
//==================================================================================================
void RtznCommProtocol::actOnPushPollMessage() {
    /* The PARTNER is informing me of a status change */
    this->actOnPushMessage();
    /* The PARTNER is asking for my status */
    this->actOnPollMessage();
}
//==================================================================================================

//==================================================================================================
bool RtznCommProtocol::isHaveToPublish() {
    return this->haveToPublish;
}
void RtznCommProtocol::setHaveToPublish(bool haveToPublish) {
    this->haveToPublish = haveToPublish;
}
//==================================================================================================

//==================================================================================================
/**
  * 
  */
int RtznCommProtocol::receiveData() {
    // messageSize + 2 prefix + 1 end
    int rawMessageSize = RtznCommProtocol::_msgSize + 3;
    int rlen = 0;
    int available = this->port->available();
    if (available > RtznCommProtocol::_msgSize) {
        if (this->isValidMessage()) {
            // read the incoming bytes:
            rlen = this->port->readBytesUntil(RtznCommProtocol::_MsgEnd, messageIn, RtznCommProtocol::_msgSize);

            // prints the received data
            this->__logDebugRawMessage("I received: ", messageIn, rlen);
        } else {
            if (available > 2 * rawMessageSize) {
                this->purgeDataLine(available - RtznCommProtocol::_msgSize, true);
            }
            this->purgeDataLine(rawMessageSize / 2, false);
        }
    }
    return rlen;
}
//==================================================================================================
bool RtznCommProtocol::sendMessage(char commandCode, const char *payload, byte payloadSize) {
    char message[RtznCommProtocol::_msgSize];

    memset(message, 0, RtznCommProtocol::_msgSize);
    message[RtznCommProtocol::_msgCommandCodeIndex] = commandCode;
    memcpy(&message[RtznCommProtocol::_msgPayloadStartIndex], &payload[0], payloadSize);

    if (this->port->availableForWrite() < RtznCommProtocol::_msgRawSize) {
        this->__logDebug("NOTHING Sent, line is busy");
        return false;
    }

    this->port->write(RtznCommProtocol::_MsgPrefix1);
    this->port->write(RtznCommProtocol::_MsgPrefix2);
    this->port->write(message, RtznCommProtocol::_msgSize);
    this->port->write(RtznCommProtocol::_MsgEnd);

    this->__logDebugMessage("Sent", message);

    return true;
}
//==================================================================================================
const char* RtznCommProtocol::encodePayload(const char *payload, byte payloadSize) {
    char* newPayload = new char[payloadSize];
    for (int i = 0; i < payloadSize; i++) {
        newPayload[i] = payload[i] + RtznCommProtocol::_MsgAsciiShift;
    }
    return newPayload;
}
//==================================================================================================
const char* RtznCommProtocol::decodePayload(const char *payload, byte payloadSize) {
    char* newPayload = new char[payloadSize];
    for (int i = 0; i < payloadSize; i++) {
        newPayload[i] = payload[i] - RtznCommProtocol::_MsgAsciiShift;
    }
    return newPayload;
}
//==================================================================================================


//==================================================================================================
/**
  * 
  */
void RtznCommProtocol::purgeDataLine(int maxToPurge, bool indiscriminate) {
    this->__logDebug("Purge up to", maxToPurge);

    byte purgeCount = 0;
    char char1 = 0;
    while (this->port->available() > 0 && purgeCount <= maxToPurge 
        && (indiscriminate || (char1 != RtznCommProtocol::_MsgEnd && char1 != RtznCommProtocol::_MsgPrefix1)))
    {
        char1 = this->port->read();
        purgeCount = purgeCount + 1;

        DBG_PRINTLN(char1, DEC);
    }

#ifdef DEBUG_COMM_PROTO
    if (purgeCount > 0) {
        this->__logDebug("Purged:", purgeCount);
    }
#endif
}
//==================================================================================================
/**
  * 
  */
bool RtznCommProtocol::isValidMessage() {
  char char1 = this->port->read();
  if (char1 == RtznCommProtocol::_MsgEnd) {
    char1 = this->port->read();
  }
  if (char1 == RtznCommProtocol::_MsgPrefix2) {
    return true;
  }
  char char2 = this->port->read();

  DBG_PRINT("char1: [");
  DBG_PRINT(char1, DEC);
  DBG_PRINT("] char2: [");
  DBG_PRINT(char2, DEC);
  DBG_PRINTLN("]");

  return (char1 == RtznCommProtocol::_MsgPrefix1 && char2 == RtznCommProtocol::_MsgPrefix2);
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char *label) {
  DBG_PRINT(this->nodeRole);
  DBG_PRINT(": ");
  DBG_PRINTLN(label);
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char *label1, int label2) {
  DBG_PRINT(this->nodeRole);
  DBG_PRINT(": ");
  DBG_PRINT(label1);
  DBG_PRINT(" [");
  DBG_PRINT(label2);
  DBG_PRINTLN("]");
}
//==================================================================================================
void RtznCommProtocol::__logDebugMessage(const char *label, char *message) {
#ifdef DEBUG_COMM_PROTO
  DBG_PRINT(this->nodeRole);
  DBG_PRINT(": ");
  DBG_PRINT(label);
  DBG_PRINT(" [0x");
  DBG_PRINT(message[RtznCommProtocol::_msgCommandCodeIndex], HEX);
  DBG_PRINT("](");
  DBG_PRINT(message[RtznCommProtocol::_msgPayloadStartIndex], DEC);
  DBG_PRINT(")\t[");
  for (int i = RtznCommProtocol::_msgPayloadStartIndex; i < RtznCommProtocol::_msgSize; i++) {
    DBG_PRINT((char)message[i], DEC);
    DBG_PRINT("|");
  }
  DBG_PRINTLN("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebugRawMessage(const char *label, char *message, int messageSize) {
#ifdef DEBUG_COMM_PROTO
  DBG_PRINT(this->nodeRole);
  DBG_PRINT(": ");
  DBG_PRINT(label);
  DBG_PRINT(" [");
  for (int i = 0; i < messageSize; i++) {
    DBG_PRINT((char)message[i], DEC);
  }
  DBG_PRINTLN("]");
#endif
}
//==================================================================================================
