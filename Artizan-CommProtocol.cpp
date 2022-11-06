/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.
  
  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/

#include "Artizan-CommProtocol.h"


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol() {
    this->nodeRole = "";
    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
RtznCommProtocol::RtznCommProtocol(const char *nodeRole) {
    this->nodeRole = nodeRole;
    this->haveToPublish = false; // have to inform 3rd party (not partner)
}
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

//==================================================================================================
bool RtznCommProtocol::hasMessageInInboxThenReadMessage() {
    int replySize = this->receiveData();
    this->__logDebug("PARTNER reply size =", replySize);

    if (replySize > 0) {
        this->__logMessage("Receive data", messageIn);
        return true;
    } else {
        this->__logDebug("Receive NO data from PARTNER");
        return false;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnMessage() {
    this->__logDebug("actOnMessage");

    switch (this->messageIn[MESSAGE_COMMAND_CODE_INDEX]) {
        case COMMAND_NOP: /*DO NOTHING*/ break;
        case COMMAND_PUSH: this->actOnPushMessage(); break;
        case COMMAND_POLL: this->actOnPollMessage(); break;
        case COMMAND_PUSH_POLL: this->actOnPushPollMessage(); break;
        default: /*UNKNOWN*/ break;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnPushMessage() {
    /* The PARTNER is informing me of a status change */
    this->__logDebug("actOnPushMessage:");

    if (setValue1(this->messageIn[MESSAGE_PAYLOAD_START_INDEX])) {
        this->haveToPublish = true;
    }

    if (setValue2(this->messageIn[MESSAGE_PAYLOAD_START_INDEX + 1])) {
        this->haveToPublish = true;
    }
}
//==================================================================================================
void RtznCommProtocol::actOnPollMessage() {
    /* The PARTNER is asking for my status */
    /* The body of the request is empty */
    this->__logDebug("actOnPollMessage: Send data to PARTNER");

    // Send data to PARTNER
    char message[MESSAGE_SIZE];
    char payload[] = { (char)getValue1(), (char)getValue2() };
    this->newMessage(message, COMMAND_PUSH, payload, 2);
    this->sendMessage(message);

    this->__logMessage("Sent", message);
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
  int rlen = 0;
  int available = Serial.available();
  if (available > MESSAGE_SIZE) {
    if (this->__isValidMessage()) {
      // read the incoming bytes:
      rlen = Serial.readBytesUntil(MESSAGE_END, messageIn, MESSAGE_SIZE);

      // prints the received data
#ifdef DEBUG
      Serial.print("I received: ");
      for (int i = 0; i < rlen; i++) {
        Serial.print(messageIn[i]);
      }
#endif
    } else {
      if (available > 2 * MESSAGE_RAW_SIZE) {
        this->__purgeDataLine(available - MESSAGE_RAW_SIZE - 2, true);
      }
      this->__purgeDataLine(MESSAGE_RAW_SIZE / 2, false);
    }
  }
  return rlen;
}
//==================================================================================================
void RtznCommProtocol::newMessage(char *data, char commandCode, char *payload, byte payloadSize) {
  memset(data, 0, MESSAGE_SIZE);
  data[MESSAGE_COMMAND_CODE_INDEX] = commandCode;
  memcpy(&data[MESSAGE_PAYLOAD_START_INDEX], &payload[0], payloadSize);
}
//==================================================================================================
bool RtznCommProtocol::sendMessage(char *data) {
  if (Serial.availableForWrite() < MESSAGE_SIZE + 3) {
    return false;
  }
  Serial.write(MESSAGE_PREFIX1);
  Serial.write(MESSAGE_PREFIX2);
  Serial.write(data, MESSAGE_SIZE);
  Serial.write(MESSAGE_END);
  return true;
}
//==================================================================================================



//==================================================================================================
/**
  * 
  */
void RtznCommProtocol::__purgeDataLine(int maxToPurge, bool indiscriminate) {
    this->__logDebug("Purge up to", maxToPurge);

    byte purgeCount = 0;
    char char1 = 0;
    while (Serial.available() > 0 && purgeCount <= maxToPurge && (indiscriminate || (char1 != MESSAGE_END && char1 != MESSAGE_PREFIX1))) {
        char1 = Serial.read();
        purgeCount = purgeCount + 1;
#ifdef DEBUG
        Serial.println(char1, DEC);
#endif
    }
#ifdef DEBUG
    if (purgeCount > 0) {
        this->__logDebug("Purged:", purgeCount);
    }
#endif
}
//==================================================================================================
/**
  * 
  */
bool RtznCommProtocol::__isValidMessage() {
  char char1 = Serial.read();
  if (char1 == MESSAGE_END) {
    char1 = Serial.read();
  }
  if (char1 == MESSAGE_PREFIX2) {
    return true;
  }
  char char2 = Serial.read();
#ifdef DEBUG
  Serial.print("char1: [");
  Serial.print(char1, DEC);
  Serial.print("] char2: [");
  Serial.print(char2, DEC);
  Serial.println("]");
#endif
  return (char1 == MESSAGE_PREFIX1 && char2 == MESSAGE_PREFIX2);
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char *label) {
#ifdef DEBUG
  Serial.print(this->nodeRole);
  Serial.print(": ");
  Serial.println(label);
#endif
}
//==================================================================================================
void __logDebug(const char *label1, int label2) {
#ifdef DEBUG
  Serial.print(this->nodeRole);
  Serial.print(": ");
  Serial.print(label1);
  Serial.print(" [");
  Serial.print(label2);
  Serial.println("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logMessage(const char *label, char *message) {
#ifdef DEBUG
  Serial.print(this->nodeRole);
  Serial.print(": ");
  Serial.print(label);
  Serial.print(" [0x");
  Serial.print(message[MESSAGE_COMMAND_CODE_INDEX], HEX);
  Serial.print("](");
  Serial.print(message[MESSAGE_PAYLOAD_START_INDEX], DEC);
  Serial.print(")\t[");
  for (int i = MESSAGE_PAYLOAD_START_INDEX; i < MESSAGE_SIZE; i++) {
    Serial.print((char)message[i]);
    Serial.print("|");
  }
  Serial.println("]");
#endif
}
//==================================================================================================
