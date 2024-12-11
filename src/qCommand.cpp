#include "qCommand.h"
#include <limits>

#include "protothreads.h"
#include "pt.h"

#include "electricui.h"
#include "eui_binary_transport.h"

static eui_interface_t serial_comms;

qCommand::qCommand(bool caseSensitive)
    : binaryStream(&Serial3), debugStream(&Serial), callBack{nullptr},
      commandCount(0), commandList(NULL), defaultHandler(NULL),
      term('\n'), // default terminator for commands, newline character
      caseSensitive(caseSensitive), cur(NULL), last(NULL), bufPos(0),
      binaryConnected(false) {
    strcpy(delim, " "); // strtok_r needs a null-terminated string
    clearBuffer();
#ifdef ARDUINO_QUARTO
    // getHardwareUUID is Quarto specific. Need some generic Arduino way to
    // getting unique ID....
    uint32_t uuid[4];
    getHardwareUUID(uuid, sizeof(uuid));
    eui_setup_identifier(
        (char *)uuid, sizeof(uuid)); // set EUI unique ID based on hardware UUID
#else
#warning No generic way to get unique ID, so for general hardware, hardcoding to 0x13572468
    uint32_t uuid = 0x13572468;
    eui_setup_identifier(
        (char *)uuid, sizeof(uuid)); // set EUI unique ID based on hardware UUID
#endif

    if (binaryStream == &Serial3) {
        serial_comms.output_cb = &serial3_write;
        eui_setup_interfaces(&serial_comms, 1);
    } else if (binaryStream == &Serial2) {
        serial_comms.output_cb = &serial2_write;
        eui_setup_interfaces(&serial_comms, 1);
    }
}

void qCommand::reset(void) {
    for (uint8_t i = 0; i < commandCount; i++) {
        if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT) {
            Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
            ptr->updates_needed = Base::UpdateState::STATE_IDLE;
        }
    }
}

void qCommand::readBinary(void) { PT_SCHEDULE(readBinaryInt2()); }

uint16_t qCommand::sizeOfType(qCommand::Types type) {
    switch (type.sub_types.data) {
    case 3: // byte
    case 4: // char / string
    case 5: // int8
    case 6: // uint8
        return 1;
    case 7: // int16
    case 8: // uint16
        return 2;
    case 9:  // int32
    case 10: // uint32
    case 11: // float
        return 4;
    case 12: // double
        return 8;
    default:
        return 0;
    }
}

// Function for SmartData by reference
template <typename T>
void qCommand::assignVariable(const char *command, T &variable, bool read_only)
    requires(std::is_base_of<Base, typename std::decay<T>::type>::value)
{
    Types types;
    types.sub_types = {type2int<T>::result, PTR_SD_OBJECT};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    // Serial.printf("Adding %s for reference SD Object\n", command);
    uint16_t size = variable.size();
    Serial.printf("Adding %s for SD reference data (types: 0x%02x) at 0x%08x\n",
                  command, types, &variable);
    addCommandInternal(command, types, &(variable), size);
}

size_t qCommand::getOffset(Types type, uint16_t size) {
    uintptr_t stop_ptr = 0;

    if (size == sizeOfType(type)) {
        // Not an array
        switch (type.sub_types.data) {
        case 4: {
            SmartData<String> *ptrS = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptrS->value));
        } break;
        case 5: {
            SmartData<int8_t> *ptri8 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri8->value));
        } break;
        case 6: {
            SmartData<uint8_t> *ptru8 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru8->value));
        } break;
        case 7: {
            SmartData<int16_t> *ptri16 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri16->value));
        } break;
        case 8: {
            SmartData<uint16_t> *ptru16 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru16->value));
        } break;
        case 9: {
            SmartData<int32_t> *ptri32 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri32->value));
        } break;
        case 10: {
            SmartData<uint32_t> *ptru32 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru32->value));
        } break;
        case 11: {
            SmartData<float> *ptrf = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptrf->value));
        } break;

        case 12: {
            SmartData<double> *ptrd = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptrd->value));
        } break;
        default:
            stop_ptr = 0;
        }
    } else {
        switch (type.sub_types.data) {
        case 5: {
            SmartData<int8_t *> *ptri8 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri8->value));
        } break;
        case 6: {
            SmartData<uint8_t *> *ptru8 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru8->value));
        } break;
        case 7: {
            SmartData<int16_t *> *ptri16 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri16->value));
        } break;
        case 8: {
            SmartData<uint16_t *> *ptru16 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru16->value));
        } break;
        case 9: {
            SmartData<int32_t *> *ptri32 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptri32->value));
        } break;
        case 10: {
            SmartData<uint32_t *> *ptru32 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(ptru32->value));
        } break;
        case 11: {
            SmartData<float *> *float32 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(float32->value));
        } break;
        case 12: {
            SmartData<double *> *float64 = NULL;
            stop_ptr = reinterpret_cast<uintptr_t>(&(float64->value));
        } break;
        default:
            stop_ptr = 1000;
        }
    }
    // Serial2.printf("Got array with sub_type %u and returning 0x%08x\n",
    //                type.sub_types.data, stop_ptr);

    return stop_ptr;
}

const void *ptr_settings_from_object(eui_message_t *p_msg_obj) {
    qCommand::Types type;
    type.raw = p_msg_obj->type;
    // Serial2.printf(
    //    "Command is %s with type=0x%02x and size=%u and ptr to 0x%08x\n",
    //    p_msg_obj->id, p_msg_obj->type, p_msg_obj->size, p_msg_obj->ptr.data);
    if (type.sub_types.data == 4) {
        // String pointer
        const SmartData<String> *SD_String =
            static_cast<const SmartData<String> *>(p_msg_obj->ptr.data);
        // p_msg_obj->size = SD_String->get().length()+1;
        p_msg_obj->size = strlen(SD_String->get().c_str()) + 1;
        // Serial.printf("Get Smart String: length set to %u with content %s\n",
        //             p_msg_obj->size, SD_String->get().c_str());
        return static_cast<const void *>(SD_String->get().c_str());
    } else {
        size_t offset = qCommand::getOffset(type, p_msg_obj->size);
        const uint8_t *data =
            (static_cast<const uint8_t *>(p_msg_obj->ptr.data) + offset);
        // Serial2.printf(
        //     "Offset is 0x%08x and data is 0x%08x and *data is 0x%08x\n",
        //     offset, data, *((uint32_t *)data));
        if (p_msg_obj->size > qCommand::sizeOfType(type)) {
            // Array of data
            const void *final_ptr = (const void *)(*(uint32_t *)data);
            // Serial2.printf("Returning 0x%08x\n", final_ptr);
            return final_ptr;
        } else {
            return static_cast<const void *>(data);
        }
    }
}

void ack_object(void *ptr) {
    Base *ptrBase = static_cast<Base *>(ptr);
    ptrBase->resetUpdateState();
}

void serial3_write(uint8_t *data, uint16_t len) {
    Serial3.write(data, len); // output on the main serial port
    // Serial3.println("Was there data???123456");
    // Serial.printf("\nSending (3) %u bytes: 0x  ", len);
    // len = min(len, 16);
    // for (uint8_t i = 0; i < len; i++) {
    //    Serial.printf("%02x", data[i]);
    //}
    // Serial.println();
}

void serial2_write(uint8_t *data, uint16_t len) {
    Serial2.write(data, len); // output on the main serial port

    // Serial.printf("\nSending (2) %u bytes: 0x  ", len);
    // len = min(len, 16);
    // for (uint8_t i = 0; i < len; i++) {
    // Serial.printf("%02x", data[i]);
    //}
    // Serial.println();
}

char qCommand::readBinaryInt2(void) {
    PT_FUNC_START(pt);
    // static uint8_t store[64];
    eui_interface_t *p_link = &serial_comms;

    static int dataReady;
    // static uint8_t count = 0;
    static uint8_t k = 0;
    dataReady = binaryStream->available();
    if (dataReady != 0) {
        debugStream->printf(
            "Got %u bytes available... (next is 0x%02x) (k=%u)\n", dataReady,
            binaryStream->peek(), k);
        for (k = 0; k < dataReady; k++) {
            uint8_t inbound_byte = binaryStream->read();
            if (inbound_byte == 0) {
                // debugStream->println("");
                // debugStream->printf("Got a 0 with count = %u\n", count);
                // if (count > 0) {
                // debugStream->printf("Received Packet: ");
                // for (uint8_t i = 0; i < count; i++) {
                //     debugStream->printf(" %02x", store[i]);
                // }
                // debugStream->println();
                // count = 0;
                //}
            } else {
                // store[count++] = inbound_byte;
                //  debugStream->printf("SB\n");
                //  debugStream->printf(" %02x ", inbound_byte);
            }

            // eui_errors_t stat_parse = eui_parse(inbound_byte, p_link);
            eui_parse(inbound_byte, p_link);
            // Serial.printf("Yield with count=%u and dataready=%u
            // (data=%02x)\n",count,dataReady,inbound_byte);
            PT_YIELD(pt);
        }
    }

    for (uint8_t i = 0; i < commandCount; i++) {
        if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT) {
            Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
            /*
                        if (i==13){
                            Serial.printf("SD obj at %u / 0x%08x
               updates_needed=%u\n", i,ptr, ptr->getUpdateState());
                            delayMicroseconds(50000);
                        }
                */
            if (ptr->updates_needed == Base::UpdateState::STATE_NEED_TOSEND) {
                // debugStream->printf("Sending tracked variable %u\n", i);
                Serial.printf("Sending tracked variable %u\n", i);
                send_update_on_tracked_variable(i);
                ptr->updates_needed = Base::UpdateState::STATE_WAIT_ON_ACK;
            }
        }
    }

    PT_RESTART(pt);
    return PT_YIELDED;

    PT_FUNC_END(pt);
}

void qCommand::sendBinaryCommands(void) {
    uint8_t elements = 0;
    for (uint8_t i = 0; i < commandCount; i++) {
        if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT ||
            commandList[i].types.sub_types.ptr == PTR_RAW_DATA) {
            elements++;
            Serial.printf("Adding(%u): %s (ptr=0x%08x) data_type=0x%02x)\n", i,
                          commandList[i].command, commandList[i].ptr.object,
                          commandList[i].types.sub_types.data);
        } else {
            Serial.printf("Skipping: %s (ptr=0x%08x) data_type=0x%02x)\n",
                          commandList[i].command, commandList[i].ptr.object,
                          commandList[i].types.sub_types.data);
        }
    }
}

// Function for SmartData objects
template <typename T>
void qCommand::assignVariable(char const *command, SmartData<T> *object,
                              bool read_only) {
    Types types;
    using base_type =
        typename std::conditional<std::is_reference<T>::value,
                                  typename std::remove_reference<T>::type,
                                  T>::type;

    types.sub_types = {type2int<base_type>::result, PTR_SD_OBJECT};
    if (read_only) {
        types.sub_types.read_only = true;
    }

    uint16_t size = object->size();
    if (types.sub_types.data == 4) {
        // String subtype, max size is large
        size = 255;
    }
    Serial.printf("Adding %s for smartData (types:0x%02x, size=%u) @0x%08x\n",
                  command, types.raw, size, object);
    addCommandInternal(command, types, object, size);
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void qCommand::addCommand(const char *command,
                          void (*function)(qCommand &streamCommandParser,
                                           Stream &stream)) {
#ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
#endif

    commandList = (StreamCommandParserCallback *)realloc(
        commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
    // strncpy(commandList[commandCount].command, command,
    // STREAMCOMMAND_MAXCOMMANDLENGTH);
    commandList[commandCount].command = command;
    commandList[commandCount].ptr.f1 = function;
    commandList[commandCount].types.sub_types.ptr = PTR_QC_CALLBACK;
    commandList[commandCount].size = 0;
    commandList[commandCount].types.sub_types.data = 0; // sets as Callback

    eui_setup_tracked((eui_message_t *)&commandList[0], commandCount + 1);
    commandCount++;
}

void qCommand::addCommandInternal(const char *command, Types types,
                                  void *object, uint16_t size) {
#ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Assign Variable Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
#endif
    Serial.printf("Adding %s with types 0x%02x and size=%u\n", command,
                  types.sub_types.data, size);
    commandList = (StreamCommandParserCallback *)realloc(
        commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
    commandList[commandCount].command = command;
    commandList[commandCount].types = types;
    commandList[commandCount].size = size;

    if (commandList[commandCount].types.sub_types.ptr == PTR_SD_OBJECT) {
        // have SmartData object pointer
        commandList[commandCount].ptr.object = (Base *)object;
    } else {
        if (object == NULL) {
            // catch NULL pointer and trap with function that can handle it
            commandList[commandCount].types.sub_types.ptr = PTR_NULL;
        } else {
            commandList[commandCount].ptr.data = (void *)object;
            commandList[commandCount].types.sub_types.ptr = types.sub_types.ptr;
            commandList[commandCount].types.sub_types.data =
                types.sub_types.data;
        }
    }

    commandCount++;
    eui_setup_tracked((eui_message_t *)commandList, commandCount);
}

/*
void qCommand::assignVariable(const char *command, SmartData<bool> *object,
                              bool read_only) {
    Types types;
    types.sub_types = {type2int<SmartData<bool>>::result, PTR_SD_OBJECT};
    if (read_only) {
        types.sub_types.read_only = true;
    }
    // Serial.printf("Adding Bool SmartData wit %s\n", command);
    addCommandInternal(command, types, object, object->size());
}
*/

void qCommand::invalidAddress(qCommand &qC, Stream &S, void *ptr,
                              const char *command, void *object) {
    S.printf("Invalid memory address assigned to command %s\n", command);
}

bool qCommand::str2Bool(const char *string) {
    bool result = false;
    const uint8_t stringLen = 10;
    char tempString[stringLen + 1];
    strncpy(tempString, string,
            stringLen); // make copy of argument to convert to lower case
    tempString[stringLen] =
        '\0'; // null terminate in case arg is longer than size of tempString
    strlwr(tempString);

    if (strcmp(tempString, "on") == 0)
        result = true;
    else if (strcmp(tempString, "true") == 0)
        result = true;
    else if (strcmp(tempString, "1") == 0)
        result = true;
    else if (strcmp(tempString, "off") == 0)
        result = false;
    else if (strcmp(tempString, "false") == 0)
        result = false;
    else if (strcmp(tempString, "0") == 0)
        result = false;
    return result;
}

void qCommand::reportString(qCommand &qC, Stream &S, const char *command,
                            uint8_t ptr_type, char *ptr,
                            StreamCommandParserCallback *CommandList) {
    if (ptr_type == PTR_SD_OBJECT) {
        SmartData<String> *object = (SmartData<String> *)ptr;
        if (qC.next() != NULL) {
            object->set(qC.current());
            if (CommandList != nullptr) {
                // CommandList->size = object->value.length();
            }
        }
        S.printf("%s is %s\n", command, object->get().c_str());
    } else {
        if (qC.next() != NULL) {
            strlcpy(ptr, qC.current(), CommandList->size);
            Serial.printf("Command Size is %u\n", CommandList->size);
        }
        S.printf("%s is %s\n", command, ptr);
    }
}

void qCommand::reportBool(qCommand &qC, Stream &S, bool *ptr,
                          const char *command, SmartData<bool> *object) {
    bool temp;
    // Serial2.printf("String is %s\n", );
    if (qC.next() != NULL) {
        temp = qC.str2Bool(qC.current());
        if (object != NULL) {
            // Serial2.printf("Set Bool to %u\n",temp);
            object->set(temp);
        } else {
            *ptr = temp;
        }
    }

    if (object != NULL) {
        temp = object->get();
    } else {
        temp = *ptr;
    }

    S.printf("%s is %s\n", command, temp ? "true" : "false");
}

template <class argUInt>
void qCommand::reportUInt(qCommand &qC, Stream &S, const char *command,
                          Types types, argUInt *ptr) {
    // Serial.printf("reportUInt called with with ptr at 0x%08x and value of
    // %u\n",
    //               ptr, *(argUInt *)ptr);
    // setDebugWord(0x12344489);
    unsigned long temp;
    argUInt newValue;
    setDebugWord(0x01010001);
    if (qC.next() != NULL) {
        setDebugWord(0x01010002);
        long temp2 = atoi(qC.current());
        setDebugWord(0x01010003);
        if (temp2 < 0) {
            temp = 0;
            setDebugWord(0x01010004);
        } else {
            setDebugWord(0x01010005);
            temp = strtoul(qC.current(), NULL, 10);
            setDebugWord(0x01010006);
            if (temp > std::numeric_limits<argUInt>::max()) {
                setDebugWord(0x01010007);
                temp = std::numeric_limits<argUInt>::max();
                setDebugWord(0x01010008);
            }
        }
        setDebugWord(0x01010010);
        newValue = temp;
        if (types.sub_types.ptr == PTR_SD_OBJECT) {
            SmartData<argUInt> *object = (SmartData<argUInt> *)ptr;
            setDebugWord(0x01010011);
            object->set(newValue);
            setDebugWord(0x01010012);
        } else {
            setDebugWord(0x01010013);
            *ptr = newValue;
            setDebugWord(0x01010014);
        }
    }
    setDebugWord(0x01010015);
    if (types.sub_types.ptr == PTR_SD_OBJECT) {
        debugStream->printf("SD Object with %08x\n", types);
        SmartData<argUInt> *object = (SmartData<argUInt> *)ptr;
        setDebugWord(0x01010016);
        newValue = object->get();
        setDebugWord(0x01010017);
    } else {
        setDebugWord(0x01010018);
        newValue = *ptr;
        Serial.printf("And now newValue is %u but ptr deref is %u\n", newValue,
                      *(uint8_t *)ptr);
        setDebugWord(0x01010019);
    }
    setDebugWord(0x12344480);
    S.printf("%s is %u\n", command, newValue);
}

template <class argInt>
void qCommand::reportInt(qCommand &qC, Stream &S, const char *command,
                         Types types, argInt *ptr) {
    int temp;
    if (qC.next() != NULL) {
        temp = atoi(qC.current());
        if (temp < std::numeric_limits<argInt>::min()) {
            temp = std::numeric_limits<argInt>::min();
        } else if (temp > std::numeric_limits<argInt>::max()) {
            temp = std::numeric_limits<argInt>::max();
        }

        if (types.sub_types.ptr == PTR_SD_OBJECT) {
            SmartData<argInt> *object = (SmartData<argInt> *)ptr;
            object->set(temp);
        } else {
            *ptr = temp;
        }
    }
    if (types.sub_types.ptr == PTR_SD_OBJECT) {
        SmartData<argInt> *object = (SmartData<argInt> *)ptr;
        temp = object->get();
    } else {
        temp = *ptr;
    }
    S.printf("%s is %d\n", command, temp);
}

template <class argFloating>
void qCommand::reportFloat(qCommand &qC, Stream &S, const char *command,
                           Types types, argFloating *ptr) {
    argFloating newValue;
    if (qC.next() != NULL) {
        newValue = atof(qC.current());
        if (types.sub_types.ptr == PTR_SD_OBJECT) {
            SmartData<argFloating> *object = (SmartData<argFloating> *)ptr;
            object->set(newValue);
        } else {
            if (sizeof(argFloating) >
                4) { // make setting variable atomic for doubles or anything
                     // greater than 32bits.
                __disable_irq();
                *ptr = newValue;
                __enable_irq();
            } else {
                *ptr = newValue;
            }
        }
    }

    if (types.sub_types.ptr == PTR_SD_OBJECT) {
        SmartData<argFloating> *object = (SmartData<argFloating> *)ptr;
        newValue = object->get();
    } else {
        newValue = *ptr;
    }

    if ((abs(newValue) > 10) || (abs(newValue) < .1)) {
        S.printf("%s is %e\n", command,
                 newValue); // print gain in scientific notation
    } else {
        S.printf("%s is %f\n", command, newValue);
    }
}

/**
 * This sets up a handler to be called in the event that the receveived command
 * string isn't in the list of commands.
 */
void qCommand::setDefaultHandler(void (*function)(const char *,
                                                  qCommand &streamCommandParser,
                                                  Stream &stream)) {
    defaultHandler = function;
}

void qCommand::reportData(qCommand &qC, Stream &inputStream,
                          const char *command, Types types, void *ptr,
                          StreamCommandParserCallback *commandList) {
    inputStream.printf(
        "Command: %s and data_type is %u (ptr_type is %u at addr 0x%08x)\n",
        command, types.sub_types.data, types.sub_types.ptr, ptr);
    switch (types.sub_types.data) {
    case 4:
        reportString(*this, inputStream, command, types.sub_types.ptr,
                     static_cast<char *>(ptr), commandList);
        break;
    case 6:
        reportUInt(*this, inputStream, command, types,
                   static_cast<uint8_t *>(ptr));
        break;
    case 8:
        reportUInt(*this, inputStream, command, types,
                   static_cast<uint16_t *>(ptr));
        break;
    case 10:
        reportUInt(*this, inputStream, command, types,
                   static_cast<uint32_t *>(ptr));
        break;
    case 5:
        reportInt(*this, inputStream, command, types,
                  static_cast<int8_t *>(ptr));
        break;
    case 7:
        reportInt(*this, inputStream, command, types,
                  static_cast<int16_t *>(ptr));
        break;
    case 9:
        reportInt(*this, inputStream, command, types,
                  static_cast<int32_t *>(ptr));
        break;
    case 11:
        reportFloat(*this, inputStream, command, types,
                    static_cast<float *>(ptr));
        break;
    case 12:
        reportFloat(*this, inputStream, command, types,
                    static_cast<double *>(ptr));
        break;
    default:
        inputStream.printf("Unknown data type %u\n", types.sub_types.data);
        break;
    }
}

/**
 * This checks the Serial stream for characters, and assembles them into a
 * buffer. When the terminator character (default '\n') is seen, it starts
 * parsing the buffer for a prefix command, and calls handlers setup by
 * addCommand() member
 */
void qCommand::readSerial(Stream &inputStream) {
    while (inputStream.available() > 0) {
        char inChar = inputStream.read(); // Read single available character,
                                          // there may be more waiting
#ifdef SERIALCOMMAND_DEBUG
        Serial.print(inChar); // Echo back to serial stream
#endif

        if (inChar == term) { // Check for the terminator (default '\n') meaning
                              // end of command
#ifdef SERIALCOMMAND_DEBUG
            Serial.print("Received: ");
            Serial.println(buffer);
#endif

            if (!caseSensitive) {
                strlwr(buffer);
            }
            char *command = strtok_r(
                buffer, delim, &last); // Search for command at start of buffer
            if (command != NULL) {
                boolean matched = false;
                for (int i = 0; i < commandCount; i++) {
#ifdef SERIALCOMMAND_DEBUG
                    Serial.print(" - Comparing [");
                    Serial.print(command);
                    Serial.print("] to [");
                    Serial.print(commandList[i].command);
                    Serial.println("]");
#endif

                    // Compare the found command against the list of known
                    // commands for a match
                    if (strncmp(command, commandList[i].command,
                                STREAMCOMMAND_BUFFER) == 0) {
                        matched = true;
                        // Serial.printf("Found match on command %s\n",command);
                        if (commandList[i].types.sub_types.ptr ==
                            PTR_QC_CALLBACK) {
                            (commandList[i].ptr.f1)(*this, inputStream);
                        } else if (commandList[i].types.sub_types.ptr ==
                                   PTR_NULL) {
                            inputStream.printf(
                                "Error: command %s has null pointer\n",
                                command);
                        } else {
                            // SD-Object or Raw PTR
                            reportData(*this, inputStream, command,
                                       commandList[i].types,
                                       commandList[i].ptr.object,
                                       &commandList[i]);
                            // Send update on tracked variable if its RAW_DATA
                            // as SmartData will flag need to send
                            Serial.printf(
                                "Send reportData on SD or Raw (0x%02x)\n",
                                commandList[i].types.sub_types.ptr);
                            if (commandList[i].types.sub_types.ptr ==
                                PTR_RAW_DATA) {
                                send_update_on_tracked_variable(i);
                                Serial.printf(
                                    "Send update on tracked variable %u\n", i);
                            }
                        }
#ifdef SERIALCOMMAND_DEBUG
                        Serial.print("Matched Command: ");
                        Serial.println(command);
#endif
                        break;
                    }
                }
                if (!matched) {
                    if (defaultHandler != NULL) {
                        (*defaultHandler)(command, *this, inputStream);
                    } else {
                        inputStream.print("Unknown command: ");
                        inputStream.println(command);
                    }
                }
            }
            bufPos =
                0; // do not clear buffer to enter after command repeats it.
        } else if (inChar == '\b') { // backspace detected
            if (bufPos > 0) {
                bufPos--; // move back bufPos to overright previous character
                buffer[bufPos] = '\0'; // Null terminate
            }
        } else if (isprint(
                       inChar)) { // Only printable characters into the buffer
            if (bufPos < STREAMCOMMAND_BUFFER) {
                buffer[bufPos++] = inChar; // Put character into buffer
                buffer[bufPos] = '\0';     // Null terminate
            } else {
#ifdef SERIALCOMMAND_DEBUG
                Serial.println(
                    "Line buffer is full - increase STREAMCOMMAND_BUFFER");
#endif
            }
        }
    }
}

/*
 * Print list of all available commands
 */
void qCommand::printAvailableCommands(Stream &outputStream) {
    for (int i = 0; i < commandCount; i++) {
        outputStream.printf("%s (0x%08x)\n", commandList[i].command,
                            commandList[i].ptr.object);
    }
}

/*
 * Clear the input buffer.
 */
void qCommand::clearBuffer() {
    buffer[0] = '\0';
    bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::next() {
    cur = strtok_r(NULL, delim, &last);
    return cur;
}

// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command, char *object,
                                       bool read_only);

template void qCommand::assignVariable(const char *command, uint8_t *variable,
                                       bool read_only);

template void qCommand::assignVariable(const char *command, uint16_t *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, uint *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, ulong *variable,
                                       bool read_only);

template void qCommand::assignVariable(const char *command,
                                       SmartData<uint8_t> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<uint16_t> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<uint> *object, bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<unsigned long> *object,
                                       bool read_only);

template void qCommand::assignVariable(const char *command,
                                       SmartData<String> *object,
                                       bool read_only);

template void qCommand::assignVariable(const char *command,
                                       SmartData<uint16_t(&)> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<int16_t(&)> *object,
                                       bool read_only);

// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command,
                                       SmartData<int8_t> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<int16_t> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<int> *object, bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<long> *object, bool read_only);
template void qCommand::assignVariable(const char *command, int8_t *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, int16_t *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, int *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, long *variable,
                                       bool read_only);

// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command, float *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command, double *variable,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<float> *object,
                                       bool read_only);
template void qCommand::assignVariable(const char *command,
                                       SmartData<double> *object,
                                       bool read_only);
