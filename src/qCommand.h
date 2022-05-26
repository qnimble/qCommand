#ifndef QCOMMAND_h
#define QCOMMAND_h

#include <Arduino.h>
#include <string.h>
#include <Stream.h>

// Size of the input buffer in bytes (maximum length of one command plus arguments)
#define STREAMCOMMAND_BUFFER 64
// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 12 //8

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define SERIALCOMMAND_DEBUG

class qCommand {
  public:
	qCommand();
	qCommand(Stream&  providedPreferredResponseStream, char *parserName);

    void addCommand(const char *command, void(*function)(qCommand& streamCommandParser));  // Add a command to the processing dictionary.
    void setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser));   // A handler to call when no valid command received.

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
      void (*function)(qCommand& streamCommandParser);
    };                                    // Data structure to hold Command/Handler function key-value pairs
    StreamCommandParserCallback *commandList;   // Actual definition for command/handler array
    byte commandCount;
    // Pointer to the default handler function
    void (*defaultHandler)(const char *, qCommand& streamCommandParser);

    char delim[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
    char term;     // Character that signals end of command (default '\n')
    char *last;                         // State variable used by strtok_r during processing
    char *parserName;

    char buffer[STREAMCOMMAND_BUFFER + 1]; // Buffer of stored characters while waiting for terminator character
    byte bufPos;                        // Current position in the buffer

};

#endif //QCOMMAND_h
