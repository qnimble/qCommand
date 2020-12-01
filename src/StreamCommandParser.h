#ifndef STREAMCOMMANDPARSER_h
#define STREAMCOMMANDPARSER_h

// #if defined(WIRING) && WIRING >= 100
//   #include <Wiring.h>
// #elif defined(ARDUINO) && ARDUINO >= 100
//   #include <Arduino.h>
// #else
//   #include <WProgram.h>
// #endif

#include <Arduino.h>
#include <string.h>
#include <Stream.h>

// Size of the input buffer in bytes (maximum length of one command plus arguments)
#define STREAMCOMMAND_BUFFER 32
// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 12 //8

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define SERIALCOMMAND_DEBUG

class StreamCommandParser {
  public:
    StreamCommandParser();
    StreamCommandParser(Stream&  providedPreferredResponseStream, char *parserName);

    void addCommand(const char *command, void(*function)(StreamCommandParser& streamCommandParser));  // Add a command to the processing dictionary.
    void setDefaultHandler(void (*function)(const char *, StreamCommandParser& streamCommandParser));   // A handler to call when no valid command received.

    void readSerial(Stream& inputStream);             // Main entry point.
    void clearBuffer();                               // Clears the input buffer.
    char *next();                                     // Returns pointer to next token found in command buffer (for getting arguments to commands).
    void printAvailableCommands(Stream& outputStream); //Could be useful for a help menu type list

    bool isPreferredResponseStreamAvailable;
    Stream& preferredResponseStream;

  private:
    // Command/handler dictionary
    struct StreamCommandParserCallback {
      char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
      void (*function)(StreamCommandParser& streamCommandParser);
    };                                    // Data structure to hold Command/Handler function key-value pairs
    StreamCommandParserCallback *commandList;   // Actual definition for command/handler array
    byte commandCount;
    // Pointer to the default handler function
    void (*defaultHandler)(const char *, StreamCommandParser& streamCommandParser);

    char delim[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
    char term;     // Character that signals end of command (default '\n')
    char *last;                         // State variable used by strtok_r during processing
    char *parserName;

    char buffer[STREAMCOMMAND_BUFFER + 1]; // Buffer of stored characters while waiting for terminator character
    byte bufPos;                        // Current position in the buffer

};

#endif //STREAMCOMMANDPARSER_h
