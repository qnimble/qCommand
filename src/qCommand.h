#ifndef QCOMMAND_h
#define QCOMMAND_h

#include <Arduino.h>
#include <string.h>
#include <Stream.h>
#include "smartData.h"

// Size of the input buffer in bytes (maximum length of one command plus arguments)
#define STREAMCOMMAND_BUFFER 64
// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 12 //8

// Uncomment the next line to run the library in debug mode (verbose messages)
// #define SERIALCOMMAND_DEBUG


class qCommand {
  public:

    qCommand(bool caseSensitive = false);
    void sendBinaryCommands(void);
    void addCommand(const char *command, void(*function)(qCommand& streamCommandParser, Stream& stream));  // Add a command to the processing dictionary.    
    
    void setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser, Stream& stream));   // A handler to call when no valid command received.    
    bool str2Bool(const char* string);                // Convert string of "true" or "false", etc to a bool.
    void readSerial(Stream& inputStream);             // Main entry point.
    char readBinary(void);
    void clearBuffer();                               // Clears the input buffer.
    char *current();                                  // Returns pointer to current token found in command buffer (for getting arguments to commands).
    char *next();                                     // Returns pointer to next token found in command buffer (for getting arguments to commands).
    void printAvailableCommands(Stream& outputStream); //Could be useful for a help menu type list
    
    // Assign Variable function for booleans: pointer to direct data or DataObject class
    void assignVariable(const char* command, bool* variable);
    void assignVariable(const char* command, SmartData<bool>* object);

    void assignVariable(const char* command, String* variable);
    void assignVariable(const char* command, SmartData<String>* object) ;

    // Assign Variable function for unsigned ints: pointer to direct data or DataObject class
    template <typename argUInt, std::enable_if_t<
      std::is_same<argUInt, uint8_t>::value || 
      std::is_same<argUInt, uint16_t>::value || 
      std::is_same<argUInt, uint>::value || 
      std::is_same<argUInt, ulong>::value
      , uint> = 0>
    void assignVariable(const char* command, argUInt* variable);

    template <typename argUInt, std::enable_if_t<
      std::is_same<argUInt,uint8_t>::value || 
      std::is_same<argUInt, uint16_t>::value || 
      std::is_same<argUInt, uint>::value || 
      std::is_same<argUInt, ulong>::value
      , uint> = 0>
    void assignVariable(const char* command, SmartData<argUInt>* object);
    
    
    // Assign Variable function for signed ints: pointer to direct data or DataObject class
    template <typename argInt, std::enable_if_t<
      std::is_same<argInt,int8_t>::value || 
      std::is_same<argInt, int16_t>::value || 
      std::is_same<argInt, int>::value ||  
      std::is_same<argInt, long>::value
      , int> = 0>        
    void assignVariable(const char* command, SmartData<argInt>* object);
            
    template <typename argInt, std::enable_if_t<
      std::is_same<argInt,int8_t>::value || 
      std::is_same<argInt, int16_t>::value || 
      std::is_same<argInt, int>::value ||  
      std::is_same<argInt, long>::value
      , int> = 0>        
    void assignVariable(const char* command, argInt* variable);

    // Assign Variable function for floating precision unmbers: pointer to direct data or DataObject class
    template <typename argFloat, std::enable_if_t<      
      std::is_floating_point<argFloat>::value
      , int> = 0>        
    void assignVariable(const char* command, argFloat* variable);

    template <typename argFloat, std::enable_if_t<      
      std::is_floating_point<argFloat>::value
      , int> = 0>        
    void assignVariable(const char* command, SmartData<argFloat>* object) ;

    enum class Commands: uint8_t {
      ListCommands = 0,
      Get = 1,
      Set = 2,
      
    };
    #warning move back to private after testing
    union callBack {
        void (*f1)(qCommand& streamCommandParser, Stream& stream);
        void (qCommand::*f2)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object);
      } callBack;
      
    struct StreamCommandParserCallback {
        char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
        union callBack function;
        Base* object;
        void* ptr;
        uint8_t data_type;
      };                                    // Data structure to hold Command/Handler function key-value pairs

    StreamCommandParserCallback *commandList;   // Actual definition for command/handler array
    uint8_t commandCount;

  private:
      Stream* binaryStream;
      
      template <typename DataType>
      void addCommandInternal(const char* command, 
                              void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, DataType* variable, const char* command, SmartData<DataType>* object), 
                              DataType* var, 
                              SmartData<DataType>* object = NULL);
      
      
      void reportString(qCommand& qC, Stream& S, String* ptr, const char* command, SmartData<String>* object) ;
      
      void reportBool(qCommand& qC, Stream& S, bool* ptr, const char* command, SmartData<bool>* object) ;
      
      template <class argInt>
      void reportInt(qCommand& qC, Stream& S, argInt* ptr, const char* command, SmartData<argInt>* object) ;
      
      template <class argUInt>
      void reportUInt(qCommand& qC, Stream& S, argUInt* ptr, const char* command, SmartData<argUInt>* object) ;
      
      template <class argFloating>
      void reportFloat(qCommand& qC, Stream& S, argFloating* ptr, const char* command, SmartData<argFloating>* object) ;
      
      void invalidAddress(qCommand& qC, Stream& S, void* ptr, const char* command, void* object) ;

      


      
      
      
      // Pointer to the default handler function
      void (*defaultHandler)(const char *, qCommand& streamCommandParser, Stream& stream);

      char delim[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
      char term;     // Character that signals end of command (default '\n')
      const bool caseSensitive;
      char *cur;
      char *last;                         // State variable used by strtok_r during processing

      char buffer[STREAMCOMMAND_BUFFER + 1]; // Buffer of stored characters while waiting for terminator character
      byte bufPos;                        // Current position in the buffer
};

#endif //QCOMMAND_h
