/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.

  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/
//= INCLUDES =======================================================================================
#include "RtznCommProtocol.h"


//= CONSTANTS ======================================================================================
byte RtznCommProtocol::_msgSize;
byte RtznCommProtocol::_msgRawSize;

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

//ooooooooooooooooooooooooooo
RtznCommProtocol::RtznCommProtocol(const char* nodeRole,
	byte maxMessageSize,
	bool (*ProcessReceivedMessageFunction)(const char*),
	void (*PrepareMessageToSendFunction)(char*)
) {
	this->nodeRole = nodeRole;

	RtznCommProtocol::_msgSize = maxMessageSize + 1; // adding command byte
	RtznCommProtocol::_msgRawSize = RtznCommProtocol::_msgSize + 3; // messageSize + 2 prefix + 1 end

	this->_ProcessReceivedMessageFunction = ProcessReceivedMessageFunction;
	this->_PrepareMessageToSendFunction = PrepareMessageToSendFunction;

	this->haveToPublish = false; // have to inform 3rd party (not partner)
}

RtznCommProtocol::~RtznCommProtocol() {
}
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

//==================================================================================================
bool RtznCommProtocol::hasMessageInInboxThenAct(bool justRead) {
	int replySize = this->receiveData();
	this->__logDebug("PARTNER reply size =", replySize);

	if (replySize > 0) {
		this->__logDebugMessage("Receive data", messageIn);
		if (!justRead) {
			this->actOnMessage();
		}
		return true;
	}
	else {
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

	this->haveToPublish = this->_ProcessReceivedMessageFunction(&this->messageIn[RtznCommProtocol::_msgPayloadStartIndex]);
}
//==================================================================================================
void RtznCommProtocol::actOnPollMessage() {
	/* The PARTNER is asking for my status */
	/* The body of the request is empty */
	this->__logDebug("actOnPollMessage: Send data to PARTNER");

	// Send data to PARTNER
	char payload[RtznCommProtocol::_msgSize - 1];
	memset(payload, 0, RtznCommProtocol::_msgSize - 1);
	this->_PrepareMessageToSendFunction(payload);
	this->sendMessage(RtznCommProtocol::_Command_PUSH, payload, RtznCommProtocol::_msgSize - 1);
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
void RtznCommProtocol::sendPollMessage() {
	/* Ask PARTNER for status */
	/* The body of the request is empty */
	this->__logDebug("sendPollMessage: Ask data from PARTNER");

	// Send empty data to PARTNER
	char payload[RtznCommProtocol::_msgSize - 1];
	memset(payload, 0, RtznCommProtocol::_msgSize - 1);
	this->sendMessage(RtznCommProtocol::_Command_POLL, payload, RtznCommProtocol::_msgSize - 1);
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
	if (available > RtznCommProtocol::_msgSize) {
		if (this->isValidMessage()) {
			// read the incoming bytes:
			rlen = Serial.readBytesUntil(RtznCommProtocol::_MsgEnd, messageIn, RtznCommProtocol::_msgSize);

			// prints the received data
			this->__logDebugRawMessage("I received: ", messageIn, rlen);
		}
		else {
			if (available > 2 * RtznCommProtocol::_msgRawSize) {
				this->purgeDataLine(available - RtznCommProtocol::_msgSize, true);
			}
			this->purgeDataLine(RtznCommProtocol::_msgRawSize / 2, false);
		}
	}
	return rlen;
}
//==================================================================================================
bool RtznCommProtocol::sendMessage(char commandCode, const char* payload, byte payloadSize) {
	char message[RtznCommProtocol::_msgSize];

	memset(message, 0, RtznCommProtocol::_msgSize);
	message[RtznCommProtocol::_msgCommandCodeIndex] = commandCode;
	memcpy(&message[RtznCommProtocol::_msgPayloadStartIndex], &payload[0], payloadSize);

	if (Serial.availableForWrite() < RtznCommProtocol::_msgRawSize) {
		this->__logDebug("NOTHING Sent, line is busy");
		return false;
	}

	Serial.write(RtznCommProtocol::_MsgPrefix1);
	Serial.write(RtznCommProtocol::_MsgPrefix2);
	Serial.write(message, RtznCommProtocol::_msgSize);
	Serial.write(RtznCommProtocol::_MsgEnd);

	this->__logDebugMessage("Sent", message);

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
#ifdef COMM_PROTO_PRINT
		COMM_PROTO_PRINT.println(char1, DEC);
#endif
	}

#ifdef COMM_PROTO_PRINT
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

#ifdef COMM_PROTO_PRINT
	COMM_PROTO_PRINT.print("char1: [");
	COMM_PROTO_PRINT.print(char1, DEC);
	COMM_PROTO_PRINT.print("] char2: [");
	COMM_PROTO_PRINT.print(char2, DEC);
	COMM_PROTO_PRINT.println("]");
#endif

	return (char1 == RtznCommProtocol::_MsgPrefix1 && char2 == RtznCommProtocol::_MsgPrefix2);
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char* label) {
#ifdef COMM_PROTO_PRINT
	COMM_PROTO_PRINT.print(this->nodeRole);
	COMM_PROTO_PRINT.print(": ");
	COMM_PROTO_PRINT.println(label);
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebug(const char* label1, int label2) {
#ifdef COMM_PROTO_PRINT
	COMM_PROTO_PRINT.print(this->nodeRole);
	COMM_PROTO_PRINT.print(": ");
	COMM_PROTO_PRINT.print(label1);
	COMM_PROTO_PRINT.print(" [");
	COMM_PROTO_PRINT.print(label2);
	COMM_PROTO_PRINT.println("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebugMessage(const char* label, char* message) {
#ifdef COMM_PROTO_PRINT
	COMM_PROTO_PRINT.print(this->nodeRole);
	COMM_PROTO_PRINT.print(": ");
	COMM_PROTO_PRINT.print(label);
	COMM_PROTO_PRINT.print(" [0x");
	COMM_PROTO_PRINT.print(message[RtznCommProtocol::_msgCommandCodeIndex], HEX);
	COMM_PROTO_PRINT.print("](");
	COMM_PROTO_PRINT.print(message[RtznCommProtocol::_msgPayloadStartIndex], DEC);
	COMM_PROTO_PRINT.print(")\t[");
	for (int i = RtznCommProtocol::_msgPayloadStartIndex; i < RtznCommProtocol::_msgSize; i++) {
		COMM_PROTO_PRINT.print((char)message[i]);
		COMM_PROTO_PRINT.print("|");
	}
	COMM_PROTO_PRINT.println("]");
#endif
}
//==================================================================================================
void RtznCommProtocol::__logDebugRawMessage(const char* label, char* message, int messageSize) {
#ifdef COMM_PROTO_PRINT
	COMM_PROTO_PRINT.print(this->nodeRole);
	COMM_PROTO_PRINT.print(": ");
	COMM_PROTO_PRINT.print(label);
	COMM_PROTO_PRINT.print(" [");
	for (int i = 0; i < messageSize; i++) {
		COMM_PROTO_PRINT.print((char)message[i]);
	}
	COMM_PROTO_PRINT.println("]");
#endif
}
//==================================================================================================
