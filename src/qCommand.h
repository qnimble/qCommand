#ifndef QCOMMAND_h
#define QCOMMAND_h

#include "smartData.h"
#include <Stream.h>
#include <string.h>

#include "electricui.h"

#define STREAMCOMMAND_BUFFER 63 // Max length of a command (excludes null term)

#ifdef __cplusplus
extern "C" {
#endif

// functions that need to be exposed to pure C for electricui
void serial3_write(uint8_t *data, uint16_t len);
void serial2_write(uint8_t *data, uint16_t len);
const void *ptr_settings_from_object(eui_message_t *p_msg_obj);

#ifdef __cplusplus
}
#endif

class qCommand {
  public:
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

    /**
     * Retrieve the pointer to the current token ("word" or "argument") from the
     * command buffer. Returns NULL if no more tokens exist.
     */
    char *current() { return cur; }
    char *next(); // Returns pointer to next token found in command buffer (for
                  // getting arguments to commands).
    void printAvailableCommands(
        Stream &outputStream); // Could be useful for a help menu type list

    static size_t getOffset(Types type, uint16_t size);
    static uint16_t sizeOfType(qCommand::Types type);

    // Function for arrays with size explicitly defined
    template <typename T, std::size_t N>
    void assignVariable(const char *command, T (&variable)[N],
                        bool read_only = false)
        requires(!std::is_base_of<Base, T>::value);

    // Function for SmartData objects
    template <typename T>
    void assignVariable(char const *command, SmartData<T> *object,
                        bool read_only = false);

    // Specialization for pointers without size (assuming not arrays but pointer
    // to single value)
    template <typename T>
    void assignVariable(const char *command, T variable, bool read_only = false)
        requires(std::is_pointer<T>::value &&
                 !std::is_base_of<Base,
                                  typename std::remove_pointer<T>::type>::value)
    ;

    // Arrays with size
    template <typename T>
    void assignVariable(const char *command, T variable, bool read_only = false)
        requires(!std::is_pointer<T>::value && std::is_array<T>::value &&
                 !std::is_base_of<Base,
                                  typename std::remove_pointer<T>::type>::value)
    ;
/*
    // Function for SmartData by reference
    template <typename T>
    void assignVariable(const char *command, T &variable,
                        bool read_only = false)
        requires(std::is_base_of<Base, typename std::decay<T>::type>::value);
*/

  private:
    Stream *binaryStream;    

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
    StreamCommandParserCallback
        *commandList; // Actual definition for command/handler array

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

    void reportString(qCommand &qC, Stream &S, const char *command,
                      uint8_t ptr_type, char *ptr,
                      StreamCommandParserCallback *CommandList);
    void reportBool(qCommand &qC, Stream &S, const char *command, Types types,
                   bool *ptr);

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
// Base template for fixed-size arrays
// Needs to be in header to get size N
template <typename T, std::size_t N>
void qCommand::assignVariable(const char *command, T (&variable)[N],
                              bool read_only)
    requires(!std::is_base_of<Base, T>::value)
{
    Types types;
    types.sub_types = {type2int<T>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for fixed array size %zu\n", command, N);
    addCommandInternal(command, types, variable, N * sizeof(T));
}
*/
// Specialization for pointers to arrays with size
// Needs to be in header since each size its own template / initialization
template <typename T>
void qCommand::assignVariable(const char *command, T variable, bool read_only)
    requires(
        std::is_pointer<T>::value &&
        !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value)
{

    using base_type = typename std::remove_pointer<T>::type;         // G
    using array_type = typename std::remove_extent<base_type>::type; // G
    Types types;
    types.sub_types = {type2int<array_type>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for pointer to array with size = %u)\n", command,
                  sizeof(base_type));
    addCommandInternal(command, types, variable, sizeof(base_type));
}
/*
// Specialization for arrays without size
template <typename T>
void qCommand::assignVariable(const char *command, T variable, bool read_only)
    requires(
        !std::is_pointer<T>::value && std::is_array<T>::value &&
        !std::is_base_of<Base, typename std::remove_pointer<T>::type>::value)
{
    using base_type = typename std::remove_pointer<T>::type; // G
    Types types;
    types.sub_types = {type2int<base_type>::result, PTR_RAW_DATA};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    Serial.printf("Adding %s for pointer to array (no size -> %u)\n", command,
sizeof(T)); addCommandInternal(command, types, variable, sizeof(T));
}
*/
#endif // QCOMMAND_h
