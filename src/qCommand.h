#ifndef QCOMMAND_h
#define QCOMMAND_h

#include "smartData.h"
#include <Arduino.h>
#include <Stream.h>
#include <string.h>

#include "electricui.h"

// Size of the input buffer in bytes (maximum length of one command plus
// arguments)
#define STREAMCOMMAND_BUFFER 64
// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 25 // 8

/*
template <typename T, typename Enable = void> struct TypeInfo {
    static constexpr uint8_t isChar = 55;
};

template<>
struct TypeInfo<char> {
    static constexpr uint8_t isChar = 11;
};

template<>
struct TypeInfo<uint8_t> {
    static constexpr uint8_t isChar = 22;
};

template<>
struct TypeInfo<uint8_t*> {
    static constexpr uint8_t isChar = 44;
};

template<>
struct TypeInfo<char*> {
    static constexpr uint8_t isChar = 77;
};

template<>
struct TypeInfo<unsigned char*> {
    static constexpr uint8_t isChar = 88;
};
static_assert(TypeInfo<unsigned char*>::isChar != 1, "Should not be 1");
*/

/*
// Specialization for pointer types (treated as pointers to arrays)
template <typename T>
struct TypeInfo<T, std::enable_if_t<
(std::is_same<typename std::remove_pointer<T>::type, char>::value ||
     std::is_same<typename std::remove_pointer<T>::type, unsigned char>::value)>>
    {
    static constexpr bool isChar = false;
};
*/

#ifdef __cplusplus
extern "C" {
#endif

void serial3_write(uint8_t *data, uint16_t len);
void serial2_write(uint8_t *data, uint16_t len);
const void *ptr_settings_from_object(eui_message_t *p_msg_obj);

#ifdef __cplusplus
}
#endif

template<typename T>
struct is_char_type : std::false_type {};

template<>
struct is_char_type<char> : std::true_type {};

template<>
struct is_char_type<unsigned char> : std::true_type {};

template<>
struct is_char_type<signed char> : std::true_type {};


class qCommand {
  public:
    
    template<typename T>
    using char_type = is_char_type<T>;
    
    
    enum class Commands : uint8_t {
        ListCommands = 0,
        Get = 1,
        Set = 2,
        Request = 3, // Request new data (for arrays where data is not
                     // necessarily ready. Only valid for SmartDataPtr objects)
        ACK = 4,     // Acknowledge receipt of data
        Disconnect = 255,
    };

    enum PtrType {
        PTR_RAW_DATA = 0, // default for data
        PTR_QC_CALLBACK = 4,
        PTR_SD_OBJECT = 6,
        PTR_NULL = 7, // maybe this should never happen
    };

    typedef union {
        uint8_t raw;
        struct {
            uint8_t data : 4;   // 4 bits to match eui_header type
            PtrType ptr : 3;    // 4 bits to set what ptr type is used}
            bool read_only : 1; // 1 bit to set if read only
        } sub_types;
    } Types;

    struct TypesOld {
        uint8_t data_type : 4; // 4 bits to match eui_header type
        PtrType ptr_type : 4;  // 4 bits to set what ptr type is used}
    };

    
    qCommand(bool caseSensitive = false);
    void sendBinaryCommands(void);
    void addCommand(const char *command,
                    void (*function)(qCommand &streamCommandParser,
                                     Stream &stream)); // Add a command to the
                                                       // processing dictionary.
    void printTable(void);
    void reset(void);
    void setDefaultHandler(void (*function)(
        const char *, qCommand &streamCommandParser,
        Stream &stream)); // A handler to call when no valid command received.
    bool str2Bool(const char *string); // Convert string of "true" or "false",
                                       // etc to a bool.
    void readSerial(Stream &inputStream); // Main entry point.
    void readBinary(void);
    void clearBuffer(); // Clears the input buffer.
    char *current(); // Returns pointer to current token found in command buffer
                     // (for getting arguments to commands).
    char *next(); // Returns pointer to next token found in command buffer (for
                  // getting arguments to commands).
    void printAvailableCommands(
        Stream &outputStream); // Could be useful for a help menu type list

    // Assign Variable function for booleans: pointer to direct data or
    // DataObject class
    // void assignVariable(const char* command, uint8_t ptr_type, void* object);
    static size_t getOffset(Types type, uint16_t size);
    
    // Function for arrays with size  
    template <typename T, std::size_t N>
    typename std::enable_if<      
      !std::is_base_of<Base, T>::value>::type
    assignVariable(const char* command, T (&variable)[N], bool read_only = false);

    
    //template <typename T>
    //template <typename T>
    //void assignVariable(char const *command, SmartData<T,TypeTraits<T, void>::isArray> *object, bool read_only = false);

    template <typename T>
    void assignVariable(char const *command, SmartData<T> *object, bool read_only = false);


 
// Specialization for arrays without size
template <typename T>
typename std::enable_if<    
    std::is_pointer<T>::value &&    
    !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value>::type    
    assignVariable(const char* command, T variable, bool read_only = false);





    // Arrays with size
    template <typename T>
    typename std::enable_if<    
    !std::is_pointer<T>::value &&
    std::is_array<T>::value &&
    !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value>::type
    assignVariable(const char* command, T variable, bool read_only = false);


    // Arrays passed bt reference
    //template <typename T, std::size_t N>
    //void assignVariable(const char *command, T (&variable)[N], bool read_only = false);

    // Function for pointers
    //template <typename T>
    //typename std::enable_if<
    //    !std::is_base_of<Base, typename std::decay<T>::type>::value>::type
    //assignVariable(const char *command, T *variable, bool read_only = false);

    // Function for SmartData objects
    //template <typename T>
    //typename std::enable_if<std::is_base_of<Base, T>::value>::type
    // typename std::enable_if<std::is_base_of<Base, typename
    // std::decay<T>::type>::value>::type
    //void assignVariable(const char *command, SmartData<T> *variable, bool read_only = false);

    // Function for by reference
    // template <typename T>
    // typename std::enable_if<!std::is_base_of<Base, typename
    // std::decay<T>::type>::value>::type assignVariable(const char* command, T&
    // variable, bool read_only = false);

    // Function for SmartData by reference
    template <typename T>
    typename std::enable_if<
        std::is_base_of<Base, typename std::decay<T>::type>::value>::type
    assignVariable(const char *command, T &variable, bool read_only = false);

    // template <typename DataType, typename
    // std::enable_if<std::is_same<std::remove_extent_t<DataType>, bool>::value
    // && std::is_array<DataType>::value, int>::type = 0>

    // needed for non-template instantiation
    // void assignVariable(const char* command, SmartData<bool>* object, bool
    // read_only = false);

    // template <typename DataType, typename
    // std::enable_if<std::is_same<DataType, bool>::value, int>::type = 0> void
    // assignVariable(const char* command, bool& variable);

    void assignVariable(const char *command, bool &variable,
                        bool read_only = false);
    void assignVariable(const char *command, SmartData<bool> *object,
                        bool read_only = false);

    //    void assignVariable(const char* command, String* variable);
    // void assignVariable(const char* command, SmartData<String>* object, bool
    // read_only = false);
/*
    template <
        typename DataType,
        typename std::enable_if<TypeTraits<DataType>::isArray || TypeTraits<DataType>::isReference
        , int>::type = 0>
    void assignVariable(const char *command, SmartData<DataType> *object,
                        bool read_only = false);

*/
    // template <typename T>
    // void reportData(qCommand& qC, Stream& inputStream, const char* command,
    // SmartData<T>* baseObject); void reportData(qCommand& qC, Stream&
    // inputStream, const char* command, Base* baseObject);

    /*
        template <typename argArray,
       std::enable_if_t<std::is_pointer<argArray>::value, uint> = 0> void
       assignVariable(const char* command, SmartDataPtr<argArray>* object);
    */
    /*
      //void qCommand::assignVariable(const char* command, argUInt* variable,
      size_t elements);
        // Assign Variable function for unsigned ints: pointer to direct data or
      DataObject class template <typename argUInt, std::enable_if_t<
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


        // Assign Variable function for signed ints: pointer to direct data or
      DataObject class template <typename argInt, std::enable_if_t<
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

        // Assign Variable function for floating precision unmbers: pointer to
      direct data or DataObject class template <typename argFloat,
      std::enable_if_t< std::is_floating_point<argFloat>::value , int> = 0> void
      assignVariable(const char* command, argFloat* variable);

        template <typename argFloat, std::enable_if_t<
          std::is_floating_point<argFloat>::value
          , int> = 0>
        void assignVariable(const char* command, SmartData<argFloat>* object) ;



      */


    

   

    // void reportData(qCommand& qC, Stream& inputStream, const char* command,
    // Base* baseObject);
   
  private:
    Stream *binaryStream;
    Stream *debugStream;
    
    union callBack {
        void (*f1)(qCommand &streamCommandParser, Stream &stream);
        void (qCommand::*f2)(qCommand &streamCommandParser, Stream &stream,
                             void *ptr, const char *command, void *object);
    } callBack;

    struct StreamCommandParserCallback {
        const char *command;
        Types types;
        // uint8_t data_type : 4; //4 bits to match eui_header type
        // PtrType ptr_type : 4; //4 bits to set what ptr type is used
        uint16_t size;
        union ptr {
            void *data; // ptr to raw data
            void (*f1)(qCommand &streamCommandParser,
                       Stream &stream); // for custom callback to qC object
            Base *object;               // smart object
        } ptr;
    }; // Data structure to hold Command/Handler function key-value pairs

    uint8_t commandCount;
    StreamCommandParserCallback *commandList; // Actual definition for command/handler array
    

    void reportData(qCommand &qC, Stream &inputStream, const char *command,
                    Types types, void *ptr,
                    StreamCommandParserCallback *commandList = NULL);


    char readBinaryInt(void);
    char readBinaryInt2(void);
    // template <typename DataType, typename
    // std::enable_if<!TypeTraits<DataType>::isArray, int>::type = 0>
    void addCommandInternal(const char *command,
                            void (qCommand::*function)(
                                qCommand &streamCommandParser, Stream &stream,
                                const char *command, Types types, void *ptr),
                            Types types, void *object = NULL);

    void addCommandInternal(const char *command, Types types, void *object,
                            uint16_t size);

    /*
      template <typename DataType, typename
      std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0> void
      addCommandInternal(const char* command, void
      (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, const
      char* command, uint8_t ptr_type, void* ptr), DataType* var,
                              SmartData<DataType>* object = NULL);

      */
    // void reportString(qCommand& qC, Stream& S, String* ptr, const char*
    // command, SmartData<String>* object) ; void reportString(qCommand& qC,
    // Stream& S, const char* command, uint8_t ptr_type,void* ptr);
    void reportString(qCommand &qC, Stream &S, const char *command,
                      uint8_t ptr_type, char *ptr,
                      StreamCommandParserCallback *CommandList);
    void reportBool(qCommand &qC, Stream &S, bool *ptr, const char *command,
                    SmartData<bool> *object);

    template <class argInt>
    void reportInt(qCommand &qC, Stream &S, const char *command, Types types,
                   argInt *ptr);

    template <class argUInt>
    void reportUInt(qCommand &qC, Stream &S, const char *command, Types types,
                    argUInt *ptr);

    template <class argFloating>
    void reportFloat(qCommand &qC, Stream &S, const char *command, Types types,
                     argFloating *ptr);

    template <class argFloating>
    void reportFloat(qCommand &qC, Stream &S, argFloating *ptr,
                     const char *command, SmartData<argFloating> *object);

    // void reportData(qCommand& qC, Stream& inputStream, const char* command,
    // Base* baseObject); void reportData(qCommand &qC, Stream &inputStream,
    // const char *command, Types types, void *ptr, StreamCommandParserCallback*
    // commandList = NULL);
    void invalidAddress(qCommand &qC, Stream &S, void *ptr, const char *command,
                        void *object);

    // Pointer to the default handler function
    void (*defaultHandler)(const char *, qCommand &streamCommandParser,
                           Stream &stream);

    char delim[2]; // null-terminated list of character to be used as delimeters
                   // for tokenizing (default " ")
    char term;     // Character that signals end of command (default '\n')
    const bool caseSensitive;
    char *cur;
    char *last; // State variable used by strtok_r during processing

    char buffer[STREAMCOMMAND_BUFFER + 1]; // Buffer of stored characters while
                                           // waiting for terminator character
    byte bufPos;                           // Current position in the buffer
    bool binaryConnected;
};

/*
template <typename T, std::size_t N>
void qCommand::assignVariable(const char* command, T (&variable)[N]) {
    Types types = {type2int<SmartData<T>>::result, PTR_RAW_DATA};
    Serial.printf("Adding %s for T, N array\n", command);
    addCommandInternal(command, types, variable, N*sizeof(T));
}
*/


// Base template for fixed-size arrays
template <typename T, std::size_t N>
typename std::enable_if<    
    !std::is_base_of<Base, T>::value>::type
qCommand::assignVariable(const char* command, T (&variable)[N], bool read_only) {
    Types types;
    types.sub_types = {type2int<T>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for fixed array size %zu\n", command, N);
    addCommandInternal(command, types, variable, N * sizeof(T));
}


// Specialization for arrays without size
template <typename T>
typename std::enable_if<    
    std::is_pointer<T>::value &&    
    !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value>::type    
qCommand::assignVariable(const char* command, T variable, bool read_only) {
    using base_type = typename std::remove_pointer<T>::type;  // G
    using array_type = typename std::remove_extent<base_type>::type;  // G
    Types types;
    types.sub_types = {type2int<array_type>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for pointer to array (no size)\n", command);
    addCommandInternal(command, types, variable, sizeof(base_type));
}



// Specialization for arrays without size
template <typename T>
typename std::enable_if<    
    !std::is_pointer<T>::value &&
    std::is_array<T>::value &&
    !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value>::type    
qCommand::assignVariable(const char* command, T variable, bool read_only) {
    using base_type = typename std::remove_pointer<T>::type;  // G
    Types types;
    types.sub_types = {type2int<base_type>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for pointer to array (no size)\n", command);
    addCommandInternal(command, types, variable, sizeof(T) );
}




/*

template <typename T>
typename std::enable_if<
    !std::is_base_of<Base, typename std::decay<T>::type>::value>::type
qCommand::assignVariable(const char *command, T *variable, bool read_only) {
    Types types;
    types.sub_types = {type2int<T>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for raw pointer (types:0x%02x)\n", command, types.raw);
    addCommandInternal(command, types, variable, sizeof(T));
}
*/


/*
// Function for by reference
template <typename T>
typename std::enable_if<!std::is_base_of<Base, typename
std::decay<T>::type>::value>::type qCommand::assignVariable(const char* command,
T& variable, bool read_only) { Types types; types.sub_types =
{type2int<T>::result, PTR_RAW_DATA}; if (read_only)  { types.sub_types.read_only
= true;
  }
  uint16_t size = sizeof(typename std::remove_reference<T>::type);
  Serial.printf("Adding %s for reference data (types: 0x%02x\n", command,
types); addCommandInternal(command, types, &variable,size);
}
*/

// Function for SmartData by reference
template <typename T>
typename std::enable_if<
    std::is_base_of<Base, typename std::decay<T>::type>::value>::type
qCommand::assignVariable(const char *command, T &variable, bool read_only) {
    Types types;
    types.sub_types = {type2int<T>::result, PTR_SD_OBJECT};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    // Serial.printf("Adding %s for reference SD Object\n", command);
    uint16_t size = variable.size();
    Serial.printf("Adding %s for SD reference data (types: 0x%02x) at 0x%08x\n", command,
                  types, &variable);
    addCommandInternal(command, types, &(variable), size);
}

uint16_t sizeOfType(qCommand::Types type);

/*
// Function for SmartData by pointer
template <typename T>
void qCommand::assignVariable(char const *command, SmartData<T, TypeTraits<T, void>::isArray> *object, bool read_only) {
    Types types;
    types.sub_types = {type2int<T>::result, PTR_SD_OBJECT};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    // Serial.printf("Adding %s for reference SD Object\n", command);
    uint16_t size = object->size();
    Serial.printf("Adding %s for SD reference data (types: 0x%02x)\n", command,
                  types);
    addCommandInternal(command, types, object, size);
}
*/

/*

template <typename T>
void qCommand::assignVariable(const char* command, SmartData<T> (&variable)) {
    Types types = {type2int<SmartData<T>>::result, PTR_SD_OBJECT};
    addCommandInternal(command, types, &variable, sizeof(T));
}



template <typename T>
void qCommand::assignVariable(const char* command, T (&variable)) {
    Types types = {type2int<T>::result, PTR_RAW_DATA};
    addCommandInternal(command, types, &variable, sizeof(T));
}

template <typename T>
void qCommand::assignVariable(const char* command, T* variable) {
    Types types = {type2int<T>::result, PTR_RAW_DATA};
    addCommandInternal(command, types, variable, sizeof(T));
}

*/

#endif // QCOMMAND_h
