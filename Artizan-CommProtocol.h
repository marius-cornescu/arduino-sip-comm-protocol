#ifndef _HEADERFILE_COMM_PROTOCOL
#define _HEADERFILE_COMM_PROTOCOL

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include <stdint.h>

// Define message size in bytes
const byte MESSAGE_SIZE = 12;
const byte MESSAGE_RAW_SIZE = 15;  // MESSAGE_SIZE + 2 prefix + 1 end

const byte MESSAGE_COMMAND_CODE_INDEX = 0;
const byte MESSAGE_PAYLOAD_START_INDEX = 1;

//= CONSTANTS ======================================================================================
//------------------------------------------------
const char MESSAGE_PREFIX1 = 254; // &thorn; character
const char MESSAGE_PREFIX2 = 252; // &uuml; character
const char MESSAGE_END = '\n';    // EOL character

const char COMMAND_MAX_VALID = 90;  // any command with higher value will be ignored
//
const char COMMAND_NOP = 99;
//
const char COMMAND_POLL = 20;       // REQUEST UPDATE FROM PARNER
const char COMMAND_PUSH = 30;       // INFORM PARTNER
const char COMMAND_PUSH_POLL = 40;  // INFORM PARTNER AND REQUEST UPDATE
//
//= VARIABLES ======================================================================================
byte getValue1();
void setValue1(byte value1);
byte getValue2();
void setValue2(byte value2);
//==================================================================================================


class RtznCommProtocol {

  public:
    RtznCommProtocol();
    RtznCommProtocol(const char *nodeRole);

    char messageIn[MESSAGE_SIZE];

    bool hasMessageInInboxThenReadMessage();

    void commActOnMessage();
    void commActOnPushMessage();
    void commActOnPollMessage();
    void commActOnPushPollMessage();

    bool isHaveToPublish();
    void setHaveToPublish(bool haveToPublish);

    //#if not defined( ArtizanCommProtocolDisableReceiving )
    //void setReceiveTolerance(int nPercent);
    //#endif

  private:
    int receiveData();
    void newMessage(char *data, char commandCode, char *payload, byte payloadSize);
    bool sendMessage(char *data);

    void __purgeDataLine(int maxToPurge, bool indiscriminate);
    bool __isValidMessage();

    // debugging
    void __logDebug(const char *label);
    void __logDebug(const char *label1, int label2);

    void __logMessage(const char *label, char *message);

    bool haveToPublish;
    const char *nodeRole;

    //#if not defined( ArtizanCommProtocolDisableReceiving )
    //static void handleInterrupt();
    //#endif
};

#endif  // _HEADERFILE_COMM_PROTOCOL