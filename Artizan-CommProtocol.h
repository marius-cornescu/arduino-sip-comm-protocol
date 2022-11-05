#ifndef _HEADERFILE_COMM_PROTOCOL
#define _HEADERFILE_COMM_PROTOCOL

// Define message size in bytes
#define MESSAGE_SIZE 12
#define MESSAGE_RAW_SIZE 15  // MESSAGE_SIZE + 2 prefix + 1 end

#define MESSAGE_COMMAND_CODE_INDEX 0
#define MESSAGE_PAYLOAD_START_INDEX 1

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
char messageIn[MESSAGE_SIZE];
//------------------------------------------------
byte getValue1();
void setValue1(byte value1);
byte getValue2();
void setValue2(byte value2);
//==================================================================================================


class RtznCommProtocol {

  public:
    RtznCommProtocol();

    char messageIn[MESSAGE_SIZE];

    int receiveData();
    void newMessage(char *data, char commandCode, char *payload, byte payloadSize);
    bool sendMessage(char *data);



    
    void switchOff(const char* sGroup, int nSwitchNumber);
    #if not defined( ArtizanCommProtocolDisableReceiving )
    void setReceiveTolerance(int nPercent);
    #endif

  private:
    void __purgeDataLine(int maxToPurge, bool indiscriminate);
    bool __isValidMessage();

    bool haveToPublish;

    #if not defined( ArtizanCommProtocolDisableReceiving )
    static void handleInterrupt();
    #endif
};


//==================================================================================================
bool hasMessageInInboxThenReadMessage() {
  int replySize = receiveData();
#ifdef DEBUG
  Serial.print(COMM_ROLE);
  Serial.print(": PARTNER reply size = [");
  Serial.print(replySize);
  Serial.println("]");
#endif

  if (replySize > 0) {
    printMessage(COMM_ROLE, ": Receive data", messageIn, MESSAGE_SIZE);
    return true;
  } else {
#ifdef DEBUG
    Serial.print(COMM_ROLE);
    Serial.println(": Receive NO data from PARTNER");
#endif
    return false;
  }
}
//==================================================================================================
void commActOnMessage() {
#ifdef DEBUG
  Serial.println("commActOnMessage");
#endif
  switch (messageIn[MESSAGE_COMMAND_CODE_INDEX]) {
    case COMMAND_NOP: /*DO NOTHING*/ break;
    case COMMAND_PUSH: commActOnPushMessage(); break;
    case COMMAND_POLL: commActOnPollMessage(); break;
    case COMMAND_PUSH_POLL: commActOnPushPollMessage(); break;
    default: /*UNKNOWN*/ break;
  }
}
//==================================================================================================
void commActOnPushMessage() {
  /* The PARTNER is informing me of a status change */
#ifdef DEBUG
  Serial.print(COMM_ROLE);
  Serial.println(":commActOnPushMessage:");
#endif
  byte newVentSpeed = messageIn[MESSAGE_PAYLOAD_START_INDEX];
  if (newVentSpeed != getValue1()) {
    setValue1(newVentSpeed);
    haveToPublish = true;
  }

  byte newActionCode = messageIn[MESSAGE_PAYLOAD_START_INDEX + 1];
  if (newActionCode != getValue2()) {
    setValue2(newActionCode);
    haveToPublish = true;
  }
}
//==================================================================================================
void commActOnPollMessage() {
  /* The PARTNER is asking for my status */
  /* The body of the request is empty */
#ifdef DEBUG
  Serial.print(COMM_ROLE);
  Serial.println(":commActOnPollMessage: Send data to PARTNER");
#endif

  // Send data to PARTNER
  char message[MESSAGE_SIZE];
  char payload[] = { (char)getValue1(), (char)getValue2() };
  newMessage(message, COMMAND_PUSH, payload, 2);
  sendMessage(message);

  printMessage(COMM_ROLE, ": Sent", message, MESSAGE_SIZE);
}
//==================================================================================================
void commActOnPushPollMessage() {
  /* The PARTNER is informing me of a status change */
  /* The PARTNER is asking for my status */
  commActOnPushMessage();
  commActOnPollMessage();
}
//==================================================================================================

//==================================================================================================
void printMessage(const char *prefix, const char *label, char *data, int size) {
#ifdef DEBUG
  Serial.print(prefix);
  Serial.print(label);
  Serial.print(" [0x");
  Serial.print(data[MESSAGE_COMMAND_CODE_INDEX], HEX);
  Serial.print("](");
  Serial.print(data[1], DEC);
  Serial.print(")\t[");
  for (int i = 2; i < size; i++) {
    Serial.print((char)data[i]);
    Serial.print("|");
  }
  Serial.println("]");
#endif
}
//==================================================================================================

#endif  // _HEADERFILE_COMM_PROTOCOL