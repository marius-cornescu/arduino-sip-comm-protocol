#ifndef _HEADERFILE_COMM_PROTOCOL
#define _HEADERFILE_COMM_PROTOCOL

//#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

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
void __purgeDataLine(int maxToPurge, bool indiscriminate) {
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
bool __isValidMessage() {
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
int receiveData() {
  int rlen = 0;
  int available = Serial.available();
  if (available > MESSAGE_SIZE) {
    if (__isValidMessage()) {
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
        __purgeDataLine(available - MESSAGE_RAW_SIZE - 2, true);
      }
      __purgeDataLine(MESSAGE_RAW_SIZE / 2, false);
    }
  }
  return rlen;
}
//==================================================================================================
void newMessage(char *data, char commandCode, char *payload, byte payloadSize) {
  memset(data, 0, MESSAGE_SIZE);
  data[MESSAGE_COMMAND_CODE_INDEX] = commandCode;
  memcpy(&data[MESSAGE_PAYLOAD_START_INDEX], &payload[0], payloadSize);
}
//==================================================================================================
bool sendMessage(char *data) {
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

bool haveToPublish = false; // have to inform 3rd party (not partner)
byte lastCommand[MESSAGE_SIZE];

void commActOnPushMessage();
void commActOnPollMessage();
void commActOnPushPollMessage();

// debugging
void printMessage(const char *prefix, const char *label, char *data, int size);


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
  memcpy(lastCommand, message, MESSAGE_SIZE);

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