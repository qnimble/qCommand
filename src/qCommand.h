#ifndef QCOMMAND_h
#define QCOMMAND_h

#include <Arduino.h>
#include <string.h>
#include <Stream.h>
#include "smartData.h"

#include "electricui.h"

// Size of the input buffer in bytes (maximum length of one command plus arguments)
#define STREAMCOMMAND_BUFFER 64
// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 25 //8

void serial_write( uint8_t *data, uint16_t len );

// Uncomment the next line to run the library in debug mode (verbose messages)
// #define SERIALCOMMAND_DEBUG
eui_message_t* find_message_object( const char * search_id, uint8_t is_internal );
uint8_t handle_packet_action(   eui_interface_t *valid_packet,
                        eui_header_t    *header,
                        eui_message_t   *p_msg_obj );

uint8_t handle_packet_ack(  eui_interface_t *valid_packet,
                   eui_header_t    *header,
                    eui_message_t   *p_msg_obj );

uint8_t handle_packet_query(    eui_interface_t *valid_packet,
                       eui_header_t    *header,
                        eui_message_t   *p_msg_obj );


class qCommand {
  public:

  enum class Commands: uint8_t {
    ListCommands = 0,
    Get = 1,
    Set = 2,
    Request = 3, // Request new data (for arrays where data is not necessarily ready. Only valid for SmartDataPtr objects)
    ACK = 4, // Acknowledge receipt of data
    Disconnect = 255,
    };

  enum PtrType {
    PTR_RAW_DATA = 0, //default for data
    PTR_QC_CALLBACK = 4,
    PTR_SD_OBJECT = 6,  
    PTR_NULL = 15, //maybe this should never happen  
  };

  struct Types {
    uint8_t data_type : 4; //4 bits to match eui_header type
    PtrType ptr_type : 4; //4 bits to set what ptr type is used}
  } ;


    qCommand(bool caseSensitive = false);
    void sendBinaryCommands(void);
    void addCommand(const char *command, void(*function)(qCommand& streamCommandParser, Stream& stream));  // Add a command to the processing dictionary.    
    void printTable(void);
    void reset(void);
    void setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser, Stream& stream));   // A handler to call when no valid command received.    
    bool str2Bool(const char* string);                // Convert string of "true" or "false", etc to a bool.
    void readSerial(Stream& inputStream);             // Main entry point.
    void readBinary(void);
    void clearBuffer();                               // Clears the input buffer.
    char *current();                                  // Returns pointer to current token found in command buffer (for getting arguments to commands).
    char *next();                                     // Returns pointer to next token found in command buffer (for getting arguments to commands).
    void printAvailableCommands(Stream& outputStream); //Could be useful for a help menu type list
    
    // Assign Variable function for booleans: pointer to direct data or DataObject class
    void assignVariable(const char* command, uint8_t ptr_type, void* object); 

    void assignVariable(const char* command, bool* variable);
    void assignVariable(const char* command, SmartData<bool>* object);

    void assignVariable(const char* command, String* variable);
    void assignVariable(const char* command, SmartData<String>* object) ;
    
    template <typename DataType, typename std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0>
    void assignVariable(const char* command, SmartData<DataType>* object);

    //template <typename T>
    //void reportData(qCommand& qC, Stream& inputStream, const char* command, SmartData<T>* baseObject);    
    //void reportData(qCommand& qC, Stream& inputStream, const char* command, Base* baseObject);
    void reportData(qCommand& qC, Stream& inputStream, const char* command, Types types, void* ptr);
    //void reportData(qCommand& qC, Stream& inputStream, const char* command, Base* baseObject);
    
/*
    template <typename argArray, std::enable_if_t<std::is_pointer<argArray>::value, uint> = 0>    
    void assignVariable(const char* command, SmartDataPtr<argArray>* object);
*/

  //void qCommand::assignVariable(const char* command, argUInt* variable, size_t elements);
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

    

  


    #warning move back to private after testing
    union callBack {
        void (*f1)(qCommand& streamCommandParser, Stream& stream);
        void (qCommand::*f2)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object);
      } callBack;
      
    struct StreamCommandParserCallback {
        const char* command;
        Types types;
        //uint8_t data_type : 4; //4 bits to match eui_header type
        //PtrType ptr_type : 4; //4 bits to set what ptr type is used
        uint16_t size;
        union ptr {
          void* data;  // ptr to raw data
          void (*f1)(qCommand& streamCommandParser, Stream& stream); // for custom callback to qC object          
          Base* object; //smart object
        } ptr;
      };                                    // Data structure to hold Command/Handler function key-value pairs

    StreamCommandParserCallback *commandList;   // Actual definition for command/handler array
    uint8_t commandCount;
    

  private:
      Stream* binaryStream;
      cw_pack_context pc;
      char readBinaryInt(void);
      char readBinaryInt2(void);
      //template <typename DataType, typename std::enable_if<!TypeTraits<DataType>::isArray, int>::type = 0>
      void addCommandInternal(const char* command, 
                              void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, const char* command, Types types, void* ptr), 
                              Types types, 
                              void* object = NULL);

    void addCommandInternal(const char *command, Types types, void* object);

    /*
      template <typename DataType, typename std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0>
      void addCommandInternal(const char* command, 
                              void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, const char* command, uint8_t ptr_type, void* ptr), 
                              DataType* var, 
                              SmartData<DataType>* object = NULL);

      */
      //void reportString(qCommand& qC, Stream& S, String* ptr, const char* command, SmartData<String>* object) ;
      void reportString(qCommand& qC, Stream& S, const char* command, uint8_t ptr_type,void* ptr);
      void reportBool(qCommand& qC, Stream& S, bool* ptr, const char* command, SmartData<bool>* object) ;
      
      template <class argInt>
      void reportInt(qCommand& qC, Stream& S, const char* command, Types types, argInt* ptr);
      
      template <class argUInt>      
      void reportUInt(qCommand& qC, Stream& S, const char* command, Types types, argUInt* ptr);
      
      template <class argFloating>
      void reportFloat(qCommand& qC,Stream &S, const char* command, Types types, argFloating* ptr);
      
      template <class argFloating>
      void reportFloat(qCommand& qC, Stream& S, argFloating* ptr, const char* command, SmartData<argFloating>* object) ;
          
      void reportData(qCommand& qC, Stream& inputStream, const char* command, Base* baseObject);
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
      bool binaryConnected;
};

#endif //QCOMMAND_h
