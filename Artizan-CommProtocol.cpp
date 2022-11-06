/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.
  
  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/
//= INCLUDES =======================================================================================
#include "Artizan-CommProtocol.h"


//= CONSTANTS ======================================================================================
byte RtznCommProtocol::_msgSize = MESSAGE_SIZE;
byte RtznCommProtocol::_msgRawSize = MESSAGE_SIZE + 3; // messageSize + 2 prefix + 1 end

byte RtznCommProtocol::_msgCommandCodeIndex = 0;
byte RtznCommProtocol::_msgPayloadStartIndex = 1;
//------------------------------------------------
const char RtznCommProtocol::_MsgPrefix1 = 254; // &thorn; character
const char RtznCommProtocol::_MsgPrefix2 = 252; // &uuml; character
const char RtznCommProtocol::_MsgEnd = '\n';    // EOL character
//
const char RtznCommProtocol::_Command_NOP = 99;
//
const char RtznCommProtocol::_Command_POLL = 20;       // REQUEST UPDATE FROM PARNER
const char RtznCommProtocol::_Command_PUSH = 30;       // INFORM PARTNER
const char RtznCommProtocol::_Command_PUSH_POLL = 40;  // INFORM PARTNER AND REQUEST UPDATE

//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol(const char *nodeRole) {
    this->nodeRole = nodeRole;

    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
//ooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol(const char *nodeRole, bool (*ProcessReceivedMessageFunction)(String), String (*PrepareMessageToSendFunction)()) {
    this->nodeRole = nodeRole;
    this->_ProcessReceivedMessageFunction = ProcessReceivedMessageFunction;
    this->_PrepareMessageToSendFunction = PrepareMessageToSendFunction;

    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

//==================================================================================================
bool RtznCommProtocol::hasMessageInInboxThenReadMessage() {
    int replySize = this->receiveData();
    this->__logDebug("PARTNER reply size =", replySize);

    if (replySize > 0) {
        this->__logDebugMessage("Receive data", messageIn);
        return true;
    } else {
        this->__logDebug("Receive NO data from PARTNER");
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

    if (setValue1(this->messageIn[RtznCommProtocol::_msgPayloadStartIndex])) {
        this->haveToPublish = true;
    }

    if (setValue2(this->messageIn[RtznCommProtocol::_msgPayloadStartIndex + 1])) {
        this->haveToPublish = true;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnPollMessage() {
    /* The PARTNER is asking for my status */
    /* The body of the request is empty */
    this->__logDebug("actOnPollMessage: Send data to PARTNER");

    // Send data to PARTNER
    char message[RtznCommProtocol::_msgSize];
    String payload = this->_PrepareMessageToSendFunction();
    this->newMessage(message, RtznCommProtocol::_Command_PUSH, payload.begin(), payload.length());
    this->sendMessage(message);

    this->__logDebugMessage("Sent", message);
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
byte RtznCommProtocol::getMessageSize() {
    return RtznCommProtocol::_msgSize;
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
    int available = Serial.available();
    if (available > RtznCommProtocol::_msgSize) {
        if (this->isValidMessage()) {
            // read the incoming bytes:
            rlen = Serial.readBytesUntil(RtznCommProtocol::_MsgEnd, messageIn, RtznCommProtocol::_msgSize);

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
void RtznCommProtocol::newMessage(char *data, char commandCode, char *payload, byte payloadSize) {
  memset(data, 0, RtznCommProtocol::_msgSize);
  data[RtznCommProtocol::_msgCommandCodeIndex] = commandCode;
  memcpy(&data[RtznCommProtocol::_msgPayloadStartIndex], &payload[0], payloadSize);
}
//==================================================================================================
bool RtznCommProtocol::sendMessage(char *data) {
  if (Serial.availableForWrite() < RtznCommProtocol::_msgRawSize) {
    return false;
  }
  Serial.write(RtznCommProtocol::_MsgPrefix1);
  Serial.write(RtznCommProtocol::_MsgPrefix2);
  Serial.write(data, RtznCommProtocol::_msgSize);
  Serial.write(RtznCommProtocol::_MsgEnd);
  return true;
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
    while (Serial.available() > 0 && purgeCount <= maxToPurge && (indiscriminate || (char1 != RtznCommProtocol::_MsgEnd && char1 != RtznCommProtocol::_MsgPrefix1))) {
        char1 = Serial.read();
        purgeCount = purgeCount + 1;
#ifdef DEBUG_COMM_PROTOCOL
        DEBUG_OUT_COMM_PROTO.println(char1, DEC);
#endif
    }

#ifdef DEBUG_COMM_PROTOCOL
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
  char char1 = Serial.read();
  if (char1 == RtznCommProtocol::_MsgEnd) {
    char1 = Serial.read();
  }
  if (char1 == RtznCommProtocol::_MsgPrefix2) {
    return true;
  }
  char char2 = Serial.read();

#ifdef DEBUG_COMM_PROTOCOL
  DEBUG_OUT_COMM_PROTO.print("char1: [");
  DEBUG_OUT_COMM_PROTO.print(char1, DEC);
  DEBUG_OUT_COMM_PROTO.print("] char2: [");
  DEBUG_OUT_COMM_PROTO.print(char2, DEC);
  DEBUG_OUT_COMM_PROTO.println("]");
#endif

  return (char1 == RtznCommProtocol::_MsgPrefix1 && char2 == RtznCommProtocol::_MsgPrefix2);
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char *label) {
#ifdef DEBUG_COMM_PROTOCOL
  DEBUG_OUT_COMM_PROTO.print(this->nodeRole);
  DEBUG_OUT_COMM_PROTO.print(": ");
  DEBUG_OUT_COMM_PROTO.println(label);
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char *label1, int label2) {
#ifdef DEBUG_COMM_PROTOCOL
  DEBUG_OUT_COMM_PROTO.print(this->nodeRole);
  DEBUG_OUT_COMM_PROTO.print(": ");
  DEBUG_OUT_COMM_PROTO.print(label1);
  DEBUG_OUT_COMM_PROTO.print(" [");
  DEBUG_OUT_COMM_PROTO.print(label2);
  DEBUG_OUT_COMM_PROTO.println("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebugMessage(const char *label, char *message) {
#ifdef DEBUG_COMM_PROTOCOL
  DEBUG_OUT_COMM_PROTO.print(this->nodeRole);
  DEBUG_OUT_COMM_PROTO.print(": ");
  DEBUG_OUT_COMM_PROTO.print(label);
  DEBUG_OUT_COMM_PROTO.print(" [0x");
  DEBUG_OUT_COMM_PROTO.print(message[RtznCommProtocol::_msgCommandCodeIndex], HEX);
  DEBUG_OUT_COMM_PROTO.print("](");
  DEBUG_OUT_COMM_PROTO.print(message[RtznCommProtocol::_msgPayloadStartIndex], DEC);
  DEBUG_OUT_COMM_PROTO.print(")\t[");
  for (int i = RtznCommProtocol::_msgPayloadStartIndex; i < RtznCommProtocol::_msgSize; i++) {
    DEBUG_OUT_COMM_PROTO.print((char)message[i]);
    DEBUG_OUT_COMM_PROTO.print("|");
  }
  DEBUG_OUT_COMM_PROTO.println("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebugRawMessage(const char *label, char *message, int messageSize) {
#ifdef DEBUG_COMM_PROTOCOL
  DEBUG_OUT_COMM_PROTO.print(this->nodeRole);
  DEBUG_OUT_COMM_PROTO.print(": ");
  DEBUG_OUT_COMM_PROTO.print(label);
  DEBUG_OUT_COMM_PROTO.print(" [");
  for (int i = 0; i < messageSize; i++) {
    DEBUG_OUT_COMM_PROTO.print((char)message[i]);
  }
  DEBUG_OUT_COMM_PROTO.println("]");
#endif
}
//==================================================================================================
