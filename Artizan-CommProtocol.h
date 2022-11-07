/*
  ArtizanCommProtocol - Arduino libary for custom communication
  Copyright (c) 2022 Marius Cornescu.  All right reserved.
  
  Project home: https://github.com/marius-cornescu/arduino-sip_comm_protocol
*/
#ifndef _HEADERFILE_COMM_PROTOCOL
#define _HEADERFILE_COMM_PROTOCOL
//= DEFINES ========================================================================================
// Uncomment/Define for library debugging
#define DEBUG_COMM_PROTO

#define COMM_PROTO_PRINTER Serial

#ifdef DEBUG_COMM_PROTO
	#define DBG_PRINT(...) { COMM_PROTO_PRINTER.print(__VA_ARGS__); }
	#define DBG_PRINTLN(...) { COMM_PROTO_PRINTER.println(__VA_ARGS__); }
#else
	#define DBG_PRINT(...) {}
	#define DBG_PRINTLN(...) {}
#endif

//= INCLUDES =======================================================================================
#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include <stdint.h>

//= CONSTANTS ======================================================================================
// Define message size in bytes
const byte MESSAGE_SIZE = 17;

//==================================================================================================

class RtznCommProtocol {

  public:
    RtznCommProtocol(Stream& _port, const char *nodeRole);
    RtznCommProtocol(Stream& _port, const char *nodeRole, bool (*ProcessReceivedMessageFunction)(const char *), const char* (*PrepareMessageToSendFunction)());

    bool hasMessageInInboxThenReadMessage();

    void actOnMessage();
    void actOnPushMessage();
    void actOnPollMessage();
    void actOnPushPollMessage();

    bool isHaveToPublish();
    void setHaveToPublish(bool haveToPublish);

  private:
    int receiveData();
    bool sendMessage(char commandCode, const char *payload, byte payloadSize);

    const char* encodePayload(const char *payload, byte payloadSize);
    const char* decodePayload(const char *payload, byte payloadSize);

    void purgeDataLine(int maxToPurge, bool indiscriminate);
    bool isValidMessage();

    bool (*_ProcessReceivedMessageFunction)(const char *){};
    const char* (*_PrepareMessageToSendFunction)(){};

    // debugging
    void __logDebug(const char *label);
    void __logDebug(const char *label1, int label2);
    void __logDebugMessage(const char *label, char *message);
    void __logDebugRawMessage(const char *label, char *message, int messageSize);

    Stream* port;
    char messageIn[MESSAGE_SIZE];
    bool haveToPublish;
    const char *nodeRole;

    //= CONSTANTS ================================
    static byte _maxPayloadSize;
    static byte _msgSize;
    static byte _msgRawSize;
    static byte _msgCommandCodeIndex;
    static byte _msgPayloadStartIndex;

    static const char _MsgPrefix1;
    static const char _MsgPrefix2;
    static const char _MsgEnd;

    static byte _MsgAsciiShift;

    static const char _Command_NOP;
    static const char _Command_POLL;
    static const char _Command_PUSH;
    static const char _Command_PUSH_POLL;
};

#endif  // _HEADERFILE_COMM_PROTOCOL