#include "StreamCommandParser.h"

struct NullStream : public Stream {
      NullStream( void ) { return; }
      int available( void ) { return 0; }
      void flush( void ) { return; }
      int peek( void ) { return -1; }
      int read( void ){ return -1; };
      size_t write( uint8_t u_Data ){ return sizeof(u_Data); }
    } nullStream;

/**
 * Constructor with preferred response stream
 */
StreamCommandParser::StreamCommandParser(Stream& providedPreferredResponseStream, char *providedParserName)
  :     
    isPreferredResponseStreamAvailable(true),
    preferredResponseStream(providedPreferredResponseStream),
    commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    last(NULL),
    parserName(providedParserName)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

StreamCommandParser::StreamCommandParser()
  : 
    isPreferredResponseStreamAvailable(false),
    preferredResponseStream(nullStream),
    commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    last(NULL),
    parserName((char*)"none")
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void StreamCommandParser::addCommand(const char *command, void (*function)(StreamCommandParser& streamCommandParser)) {
  #ifdef SERIALCOMMAND_DEBUG
    Serial.print(parserName);
    Serial.print(" - Adding command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif

  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function = function;
  commandCount++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void StreamCommandParser::setDefaultHandler(void (*function)(const char *, StreamCommandParser& streamCommandParser)) {
  defaultHandler = function;
}

/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void StreamCommandParser::readSerial(Stream& inputStream) {
  // Serial.print("Time IN: ");
  // Serial.println(millis());
  while (inputStream.available() > 0) {
    char inChar = inputStream.read();   // Read single available character, there may be more waiting
    #ifdef SERIALCOMMAND_DEBUG
      Serial.print(inChar);   // Echo back to serial stream
    #endif

    if (inChar == term) {     // Check for the terminator (default '\n') meaning end of command
      #ifdef SERIALCOMMAND_DEBUG
        Serial.print("Received: ");
        Serial.println(buffer);
      #endif

      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
      if (command != NULL) {
        boolean matched = false;
        for (int i = 0; i < commandCount; i++) {
          #ifdef SERIALCOMMAND_DEBUG
            Serial.print(parserName);
            Serial.print(" - Comparing [");
            Serial.print(command);
            Serial.print("] to [");
            Serial.print(commandList[i].command);
            Serial.println("]");
          #endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, STREAMCOMMAND_MAXCOMMANDLENGTH) == 0) {
            #ifdef SERIALCOMMAND_DEBUG
              Serial.print("Matched Command: ");
              Serial.println(command);
            #endif

            // Execute the stored handler function for the command
            (*commandList[i].function)(*this);
            matched = true;
            break;
          }
        }
        if (!matched && (defaultHandler != NULL)) {
          (*defaultHandler)(command, *this);
        }
      }
      clearBuffer();
    } else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < STREAMCOMMAND_BUFFER) {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      } else {
        #ifdef SERIALCOMMAND_DEBUG
          Serial.println("Line buffer is full - increase STREAMCOMMAND_BUFFER");
        #endif
      }
    }
  }
  // Serial.print("Time OUT: ");
  // Serial.println(millis());
}

void StreamCommandParser::printAvailableCommands(Stream& outputStream) {
  for (int i = 0; i < commandCount; i++) {
    outputStream.println(commandList[i].command);
  }
}


/*
 * Clear the input buffer.
 */
void StreamCommandParser::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *StreamCommandParser::next() {
  return strtok_r(NULL, delim, &last);
}

