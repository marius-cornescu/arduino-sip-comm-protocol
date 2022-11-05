/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.
  
  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/

#include "Artizan-CommProtocol.h"




RtznCommProtocol::RtznCommProtocol() {
  this->haveToPublish = false; // have to inform 3rd party (not partner)
}


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
#ifdef DEBUG
  Serial.print("Purge up to [");
  Serial.print(maxToPurge);
  Serial.println("]");
#endif
  byte purgeCount = 0;
  char char1 = 0;
  while (Serial.available() > 0 && purgeCount <= maxToPurge && (indiscriminate || (char1 != MESSAGE_END && char1 != MESSAGE_PREFIX1))) {
    char1 = Serial.read();
#ifdef DEBUG
    Serial.println(char1, DEC);
#endif
    purgeCount = purgeCount + 1;
  }
#ifdef DEBUG
  if (purgeCount > 0) {
    Serial.print("PURGED: [");
    Serial.print(purgeCount);
    Serial.println("]");
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



