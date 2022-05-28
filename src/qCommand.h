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
// #define SERIALCOMMAND_DEBUG

class qCommand {
  public:

	qCommand();

    void addCommand(const char *command, void(*function)(qCommand& streamCommandParser, Stream& stream));  // Add a command to the processing dictionary.
    void assignVariable(const char* command, int* variable);
    void assignVariable(const char* command, uint* variable);
    void assignVariable(const char* command, double* variable);
    void setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser, Stream& stream));   // A handler to call when no valid command received.
    void setCaseSensitive(bool InSensitive);
    void readSerial(Stream& inputStream);             // Main entry point.
    void clearBuffer();                               // Clears the input buffer.
    char *current();                                  // Returns pointer to current token found in command buffer (for getting arguments to commands).
    char *next();                                     // Returns pointer to next token found in command buffer (for getting arguments to commands).
    void printAvailableCommands(Stream& outputStream); //Could be useful for a help menu type list

  private:
    	void addCommandInternal(const char *command, void(qCommand::*function)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command), void* ptr);  // Add a command to the processing dictionary.
    	void reportInt(qCommand& qC, Stream& S, void* ptr, const char* command);
    	void reportUInt(qCommand& qC, Stream& S, void* ptr, const char* command);
    	void reportDouble(qCommand& qC, Stream& S, void* ptr, const char* command);

    union callBack {
        void (*f1)(qCommand& streamCommandParser, Stream& stream);
	    void (qCommand::*f2)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command);
    } callBack;


    struct StreamCommandParserCallback {
      char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
      union callBack function;
      void* ptr;
    };                                    // Data structure to hold Command/Handler function key-value pairs
    StreamCommandParserCallback *commandList;   // Actual definition for command/handler array
    byte commandCount;
    // Pointer to the default handler function
    void (*defaultHandler)(const char *, qCommand& streamCommandParser, Stream& stream);

    char delim[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
    char term;     // Character that signals end of command (default '\n')
    bool caseSensitive;
    char *cur;
    char *last;                         // State variable used by strtok_r during processing

    char buffer[STREAMCOMMAND_BUFFER + 1]; // Buffer of stored characters while waiting for terminator character
    byte bufPos;                        // Current position in the buffer

};

#endif //QCOMMAND_h
