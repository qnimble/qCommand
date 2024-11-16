#include "qCommand.h"
#include <limits>

#include "cwpack.h"
#include "basic_contexts.h"
#include "cwpack_utils.h"

#include "protothreads.h"
#include "pt.h"

#include "electricui.h"
#include "eui_binary_transport.h"

static eui_interface_t serial_comms = EUI_INTERFACE(&serial2_write);
// static eui_interface_t serial_comms2 = EUI_INTERFACE( &serial2_write );

/**
 * Constructor
 */
qCommand::qCommand(bool caseSensitive) : commandList(NULL),
                                         commandCount(0),
                                         binaryStream(&Serial3), // should be Serial2
                                         debugStream(&Serial),
                                         defaultHandler(NULL),
                                         term('\n'), // default terminator for commands, newline character
                                         caseSensitive(caseSensitive),
                                         cur(NULL),
                                         last(NULL),
                                         bufPos(0),
                                         binaryConnected(false)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
#ifdef ARDUINO_QUARTO
  // #warning getHardwareUUID is Quarto specific. Need some generic Arduino way to getting unique ID....
  uint32_t uuid[4];
  getHardwareUUID(uuid, sizeof(uuid));
  eui_setup_identifier((char *)uuid, sizeof(uuid)); // set EUI unique ID based on hardware UUID
#else
#warning No generic way to get unique ID, so for general hardware, hardcoding to 0x13572468
  uint32_t uuid = 0x13572468;
  eui_setup_identifier((char *)uuid, sizeof(uuid)); // set EUI unique ID based on hardware UUID
#endif

  if (binaryStream == &Serial3)
  {
    serial_comms.output_cb = &serial3_write;
  }
  else
  {
    serial_comms.output_cb = &serial2_write;
  }
  eui_setup_interfaces(&serial_comms, 1);
}

#warning Debuggin only, remove when done
void qCommand::printTable(void)
{
  for (uint8_t i = 0; i < commandCount; i++)
  {
    debugStream->printf("Command %u: %s (type=%u, size=%u, ptr_type=%u)\n", i, commandList[i].command, commandList[i].types.sub_types.data, commandList[i].size, commandList[i].types.sub_types.ptr);
  }
}

void qCommand::reset(void)
{
  for (uint8_t i = 0; i < commandCount; i++)
  {
    if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT)    {
      Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
      ptr->updates_needed = STATE_IDLE;
    }
  }
}

void qCommand::readBinary(void)
{
  PT_SCHEDULE(readBinaryInt2());
  // readBinaryInt();
}

#include <cstddef> // For offsetof


/*
void* dataPtr(const void* SD_ptr, uint8_t type) {
  qCommand::Types types;
  types.data_type = type & 0x0F;
  types.ptr_type = (type >> 4) & 0x0F;
  if (types.ptr_type == qCommand::PTR_SD_OBJECT) {
    switch (types.data_type) {
      case 4:
        //SmartData<String> *ptr4 = (SmartData<String>*) SD_ptr;
        //return &(ptr4->value);
        break;
      case 6: { //uint8_t
        // Compute the offset of 'value' within 'SmartData<uint8_t>'
        const size_t valueOffset = offsetof(SmartData<uint8_t>, value);
        return SD_ptr - valueOffset;
      }
      default:
        return NULL;
    }
  } else {
    return SD_ptr;
  }
}
*/

uint16_t sizeOfType(qCommand::Types type) {
  switch (type.sub_types.data) {
    case 3: //byte
    case 4: // string
    case 5: //int8
    case 6: //uint8
      return 1;
    case 7: //int16
    case 8: //uint16
      return 2;
    case 9: //int32
    case 10: //uint32
    case 11: //float
      return 4;
    case 12: //double
      return 8;
    default:
      return 0;
  }
}

size_t qCommand::getOffset(Types type, uint16_t size) {
  uintptr_t stop_ptr = 0;

  if ( size == sizeOfType(type) ) {
    // Not an array
    switch(type.sub_types.data) {
      case 4: {
        SmartData<String> *ptrS = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptrS->value));
        }
        break;
      case 5: {
        SmartData<int8_t> *ptri8 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptri8->value));
        }
        break;
      case 6: {
        SmartData<uint8_t> *ptru8 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptru8->value));
        }
        break;
      case 7: {
        SmartData<int16_t> *ptri16 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptri16->value));
        }
        break;
      case 8: {
        SmartData<uint16_t> *ptru16 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptru16->value));
        }
        break;
      case 9: {
        SmartData<int32_t> *ptri32 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptri32->value));
        }
        break;
      case 10: {
        SmartData<uint32_t> *ptru32 = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptru32->value));
        }
        break;
      case 11: {
        SmartData<float> *ptrf = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptrf->value));
        }
        break;

      case 12: {
        SmartData<double> *ptrd = NULL;
        stop_ptr = reinterpret_cast<uintptr_t>(&(ptrd->value));
        }
        break;
      default:
        stop_ptr = 0;
    }
    return stop_ptr;
  } else {
    return -1;
  }
}


void* data_ptr_from_object(void* ptr, uint8_t raw_type, uint16_t size) {
  qCommand::Types type;
  type.raw = raw_type;
  size_t offset = qCommand::getOffset(type, size);
  return static_cast<void*>(static_cast<uint8_t*>(ptr) + offset);
}

void ack_object(void* ptr) {
  Serial.printf("Acking ptr at 0x%08x\n",ptr);
  Base *ptrBase = static_cast<Base *>(ptr);
  ptrBase->resetUpdateState();

}

void serial3_write(uint8_t *data, uint16_t len)
{
  Serial3.write(data, len); // output on the main serial port
  // Serial3.println("Was there data???123456");
  Serial.printf("\nSending (3) %u bytes: 0x  ", len);
  len = min(len, 16);
  for (uint8_t i = 0; i < len; i++)
  {
    Serial.printf("%02x", data[i]);
  }
  Serial.println();
}

void serial2_write(uint8_t *data, uint16_t len)
{
  Serial2.write(data, len); // output on the main serial port

  //Serial.printf("\nSending (2) %u bytes: 0x  ", len);
  len = min(len, 16);
  for (uint8_t i = 0; i < len; i++)
  {
    //Serial.printf("%02x", data[i]);
  }
  //Serial.println();
}

extern "C"
{
  void desc(const char *msg, uint16_t value);
  void descs(const char *msg, const char *info);
  void descl(const char *msg, uint32_t value);
}

void desc(const char *msg, uint16_t value)
{
  Serial.printf("%s: %u (0x%04x)\n", msg, value, value);
}
void descl(const char *msg, uint32_t value)
{
   Serial.printf("%s: %u (0x%08x)\n", msg, value, value);
}
void descs(const char *msg, const char *info)
{
   Serial.printf("%s: %s\n", msg, info);
}

char qCommand::readBinaryInt2(void)
{
  PT_FUNC_START(pt);
  static uint8_t store[64];
  eui_interface_t *p_link = &serial_comms;

  static int dataReady;
  static uint8_t count = 0;
  static uint8_t k = 0;
  dataReady = binaryStream->available();
  if (dataReady != 0) {
    debugStream->printf("Got %u bytes available... (next is 0x%02x) (k=%u)\n", dataReady, binaryStream->peek(),k);
    for (k = 0; k < dataReady; k++) {
      uint8_t inbound_byte = binaryStream->read();
      if (inbound_byte == 0) {
        debugStream->println("");
        debugStream->printf("Got a 0 with count = %u\n", count);
        if ( count > 0) {
          debugStream->printf("Received Packet: ");
          for (uint8_t i=0; i < count; i++) {
            debugStream->printf(" %02x", store[i]);
          }
          debugStream->println();
          count = 0;
        }      
      } else {
        store[count++] = inbound_byte;
        //debugStream->printf("SB\n");
        //debugStream->printf(" %02x ", inbound_byte);
      }

      // eui_errors_t stat_parse = eui_parse(inbound_byte, p_link);
      eui_parse(inbound_byte, p_link);
      // Serial.printf("Yield with count=%u and dataready=%u (data=%02x)\n",count,dataReady,inbound_byte);
      PT_YIELD(pt);
    }
  }

  for (uint8_t i = 0; i < commandCount; i++)
  {
    if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT)
    {
      Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
      if (ptr->updates_needed == STATE_NEED_TOSEND)
      {
        //debugStream->printf("Sending tracked variable %u\n", i);
        send_update_on_tracked_variable(i);
        ptr->updates_needed = STATE_WAIT_ON_ACK;
      }
    }
  }

  PT_RESTART(pt);
  return PT_YIELDED;

  PT_FUNC_END(pt);
}

void qCommand::sendBinaryCommands(void)
{
  // packer.clear();
  // uint8_t i = 5;
  // uint8_t j = 9;
  // packer.serialize(i,j);

  uint8_t elements = 0;
  for (uint8_t i = 0; i < commandCount; i++)
  {
    if (commandList[i].types.sub_types.ptr == PTR_SD_OBJECT || commandList[i].types.sub_types.ptr == PTR_RAW_DATA)
    {
      elements++;
      Serial.printf("Adding(%u): %s (ptr=0x%08x) data_type=0x%02x)\n", i, commandList[i].command, commandList[i].ptr.object, commandList[i].types.sub_types.data);
    }
    else
    {
      Serial.printf("Skipping: %s (ptr=0x%08x) data_type=0x%02x)\n", commandList[i].command, commandList[i].ptr.object, commandList[i].types.sub_types.data);
    }
  }
  // packer.serialize(MsgPack::arr_size_t(elements));
  cw_pack_array_size(&pc, 3); // three elements, first is id = 0 for internal. Second is the command (list Commands). Third is the data
  cw_pack_unsigned(&pc, 0);   // set ID = 0 for internal
  cw_pack_unsigned(&pc, static_cast<uint8_t>(Commands::ListCommands));
  cw_pack_array_size(&pc, elements);
  debugStream->printf("Start Binary Command send with %u elements (error=%d)\n", elements, pc.return_code);
  debugStream->printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n", pc.start, pc.current, pc.end);
  for (uint8_t i = 0; i < commandCount; i++)
  {
    if (commandList[i].ptr.object != NULL)
    {
      cw_pack_array_size(&pc, 3);
      cw_pack_unsigned(&pc, i + 1);
      cw_pack_str(&pc, commandList[i].command, strlen(commandList[i].command));
      cw_pack_unsigned(&pc, commandList[i].types.sub_types.data);

      // MsgPack::str_t cmd_string = commandList[i].command;
      // packer.serialize(MsgPack::arr_size_t(3),i+1, cmd_string, commandList[i].data_type );

      // packer.packBinary32(const uint8_t* value, const uint32_t size);
      // packer.to_array(id,value, crc );
      // Serial.printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n", pc.start, pc.current, pc.end  );
      // Serial.printf("So far, binary data of size %u (error=%d)\n",pc.current - pc.start, pc.return_code );
    }
  }
  debugStream->printf("Send binary data of size %u (error=%d)\n", pc.current - pc.start, pc.return_code);
  binaryStream->write(pc.start, pc.current - pc.start);
  pc.current = pc.start;
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void qCommand::addCommand(const char *command, void (*function)(qCommand &streamCommandParser, Stream &stream))
{
#ifdef SERIALCOMMAND_DEBUG
  Serial.print(" - Adding Command (");
  Serial.print(commandCount);
  Serial.print("): ");
  Serial.println(command);
#endif

  commandList = (StreamCommandParserCallback *)realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  // strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].command = command;
  commandList[commandCount].ptr.f1 = function;
  commandList[commandCount].types.sub_types.ptr = PTR_QC_CALLBACK;
  commandList[commandCount].size = 0;
  commandList[commandCount].types.sub_types.data = 0; // sets as Callback

  //Serial.printf("Adding Command %s: (count to track is %u\n", command, commandCount + 1);
  //Serial.printf("commandList starts at 0x%08x\n", commandList);
  for (uint8_t i = 0; i < commandCount + 1; i++)
  {
    //Serial.printf("Command %s: has length %u\n", commandList[i].command, strlen(commandList[i].command));
  }
  //Serial.printf("Setting commandCount in eui to %u", commandCount + 1);
  //Serial.printf("Add Command pre eui setup: commandList is %08x\n", commandList);
  eui_setup_tracked((eui_message_t *)&commandList[0], commandCount + 1);

#warning with const char cannot make lower case
  // if (!caseSensitive) {
  //   strlwr((char*) commandList[commandCount].command);
  // }
  // Serial.printf("CC from %u", commandCount);
  commandCount++;
  //printTable();
  // Serial.printf(" to %u\n", commandCount);
}

// template <typename DataType, typename std::enable_if<!TypeTraits<DataType>::isArray, int>::type = 0>
void qCommand::addCommandInternal(const char *command, Types types, void *object, uint16_t size)
{
#ifdef SERIALCOMMAND_DEBUG
  Serial.print(" - Adding Assign Variable Command (");
  Serial.print(commandCount);
  Serial.print("): ");
  Serial.println(command);
#endif

  commandList = (StreamCommandParserCallback *)realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  // strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  //Serial.printf("commandList starts at 0x%08x\n", commandList);
  // Serial.printf("Setting commandCount in eui to %u", commandCount+1);
  //Serial.printf("Command addr is 0x%08x\n", command);
  //Serial.printf("Data_Type 0x%02x and  ptr_type = 0x%02x\n", types.data_type, types.ptr_type);
  commandList[commandCount].command = command;
  // commandList[commandCount].data_type = type2int<SmartData<DataType>>::result;
  commandList[commandCount].types = types;
  commandList[commandCount].size = size;

  // commandList[commandCount].size = sizeof(DataType);
  if (commandList[commandCount].types.sub_types.ptr == PTR_SD_OBJECT)
  {
    // have SmartData object pointer
    // commandList[commandCount].types.ptr_type = PTR_SD_OBJECT;
    //Base *ptr = static_cast<Base *>(object);
    commandList[commandCount].ptr.object = object;

#warning hardcoding array size to 7, BAD!!
// commandList[commandCount].size = 7;
// commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
// commandList[commandCount].ptr = NULL;
// object->_setPrivateInfo(commandCount+1, binaryStream, &packer);
#warning do I need to set private info?
    // ptr->_setPrivateInfo(commandCount+1, binaryStream, &pc);
  }
  else
  {
    if (object == NULL)
    {
      // catch NULL pointer and trap with function that can handle it
      // commandList[commandCount].ptr.f1 =  &qCommand::invalidAddress;
      commandList[commandCount].types.sub_types.ptr = PTR_NULL;
    }
    else
    {
      commandList[commandCount].ptr.data = (void *)object;
      commandList[commandCount].types.sub_types.ptr = types.sub_types.ptr;
      commandList[commandCount].types.sub_types.data = types.sub_types.data;
    }
    Serial.printf("Data_Type 0x%02x and  ptr_type = 0x%02x and size=%u\n", commandList[commandCount].types.sub_types.data, commandList[commandCount].types.sub_types.ptr, commandList[commandCount].size);
  }

#warning skipping case sensitive stuff
  // if (!caseSensitive) {
  //   strlwr(commandList[commandCount].command);
  //}
  //Serial.printf("CC from %u\n", commandCount);
  commandCount++;
  //Serial.printf("Setting commandCount in eui to %u\n", commandCount);
  //Serial.printf("Add Cmd int: pre eui setup: commandList is %08x\n", commandList);

  eui_setup_tracked((eui_message_t *)commandList, commandCount);

  //Serial.printf(" to %u\n", commandCount);
  //printTable();
}

/*
template <typename DataType, typename std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0>
void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, const char* command, uint8_t ptr_type, void* object),DataType* var, SmartData<DataType>* object)  {
  using baseType = typename std::remove_pointer<DataType>::type;

  constexpr std::size_t arraySize = std::extent<DataType>::value;

  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  //strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  Serial.printf("commandList starts at 0x%08x\n", commandList);
  commandList[commandCount].command = command;
  commandList[commandCount].data_type = TYPE2INFO_ARRAY + type2int<SmartData<baseType>>::result;


  if (object != NULL)  {
    //have SmartData object pointer
    commandList[commandCount].ptr.object = object;
    //commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
    commandList[commandCount].ptr_type = PTR_SD_OBJECT;
    //object->_setPrivateInfo(commandCount+1, binaryStream, &packer);
    object->_setPrivateInfo(commandCount+1, binaryStream, &pc);

    commandList[commandCount].size = object->getTotalElements();

  } else {
    //commandList[commandCount].object = NULL;
    if ( var == NULL) {
      //catch NULL pointer and trap with function that can handle it
      commandList[commandCount].ptr.data = (void*) 1;
      commandList[commandCount].ptr_type = PTR_NULL;
      commandList[commandCount].size = 0;
    } else {
      //commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
      commandList[commandCount].ptr.data = (void*) var;
      commandList[commandCount].ptr_type = PTR_RAW_DATA;
      commandList[commandCount].size = arraySize;
    }
  }
  #warning with const char cannot make lower case
  //if (!caseSensitive) {
  //   strlwr(commandList[commandCount].command);
  //}
  Serial.printf("CC from %u", commandCount);
  commandCount++;
  Serial.printf(" to %u\n", commandCount);
  printTable();
}
*/
/*
void qCommand::assignVariable(const char* command, uint8_t ptr_type, void* object) {
  //void (qCommand::*function)(qCommand&, Stream&, const char*, uint8_t, void*) = nullptr;
  addCommandInternal(command, ptr_type, object);

}
*/

template void qCommand::assignVariable(const char *command, unsigned char *object, bool read_only);

/*
template <typename DataType, typename std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0>
void qCommand::assignVariable(const char* command, SmartData<DataType>* object) {
  Serial.printf("Trying to assign array to command %s\n",command);
  //void (qCommand::*function)(qCommand&, Stream&, DataType*, const char*, SmartData<DataType>*) = nullptr;
  Types types = {type2int<SmartData<DataType>>::result, PTR_SD_OBJECT};
  addCommandInternal(command, types, object);
}
*/

#warning add back when we have pointers to float and double arrays
//template void qCommand::assignVariable(const char *command, SmartData<float *> *object, bool read_only);
//template void qCommand::assignVariable(const char *command, SmartData<double *> *object, bool read_only);

/*
template <typename argArray, std::enable_if_t<std::is_pointer<argArray>::value, uint> = 0>
void qCommand::assignVariable(const char* command, SmartDataPtr<argArray>* object) {
  Serial.printf("Trying to assign array to command %s\n",command);
  void (qCommand::*function)(qCommand&, Stream&, argArray*, const char*, SmartDataPtr<argArray>*) = nullptr;
  addCommandInternal(command, function, (argArray*) NULL, object);
}

template void qCommand::assignVariable(const char* command, SmartDataPtr<float*>* object);
*/

// Assign variable to command list for string. Takes pointer to either data or DataObject.
/*
void qCommand::assignVariable(const char* command, String* variable) {
  Types types = {type2int<SmartData<String>>::result, PTR_RAW_DATA};
  addCommandInternal(command,types, variable, sizeof(String));
}

void qCommand::assignVariable(const char* command, SmartData<String>* object) {
  Serial.printf("Trying to assign object to command %s\n",command);
  Types types = {type2int<SmartData<String>>::result, PTR_SD_OBJECT};
  addCommandInternal(command,types, object, object->size());
}

*/

/*
//Assign variable to command list for booleans. Takes pointer to either data or DataObject.
template <typename DataType, typename std::enable_if<std::is_same<std::remove_extent_t<DataType>, bool>::value && std::is_array<DataType>::value, int>::type = 0>
void qCommand::assignVariable(const char* command, bool& variable) {
  uint16_t size = arraySize(variable);
  Types types = {type2int<SmartData<bool>>::result, PTR_RAW_DATA};
  //if constexpr (std::is_array_v<std::remove_reference_t<decltype(variable)>>) {
  addCommandInternal(command,types, variable, size);
}
template <typename DataType, typename std::enable_if<std::is_same<DataType, bool>::value, int>::type = 0>
void qCommand::assignVariable(const char* command, bool& variable) {
  uint16_t size = sizeof(DataType);
  Types types = {type2int<SmartData<bool>>::result, PTR_RAW_DATA};
  //if constexpr (std::is_array_v<std::remove_reference_t<decltype(variable)>>) {
  addCommandInternal(command,types, variable, size);
}
*/
void qCommand::assignVariable(const char *command, SmartData<bool> *object, bool read_only) {
  Types types;
  types.sub_types = {type2int<SmartData<bool>>::result, PTR_SD_OBJECT};
  if (read_only) {
    types.sub_types.read_only = true;
  }
  //Serial.printf("Adding Bool SmartData wit %s\n", command);
  addCommandInternal(command, types, object, object->size());
}

// void qCommand::assignVariable(const char* command, argUInt* variable, size_t elements) {
//   addCommandInternal(command,NULL, variable, (SmartData<argUInt>*) NULL);
// }
/*
//Assign variable to command list for unsigned ints (calls ReportUInt). Takes pointer to either data or DataObject.
template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value ||
  std::is_same<argUInt, uint>::value ||
  std::is_same<argUInt, ulong>::value
  , uint> = 0>
void qCommand::assignVariable(const char* command, argUInt* variable) {
  Types types = {type2int<SmartData<argUInt>>::result, PTR_RAW_DATA};
    Serial.printf("assignVariable on Pointer Uint with types %u and %u at %08x\n",type2int<SmartData<argUInt>>::result, PTR_SD_OBJECT, variable);

  addCommandInternal(command,types, variable);
}

template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value ||
  std::is_same<argUInt, uint>::value ||
  std::is_same<argUInt, ulong>::value
  , uint> = 0>
void qCommand::assignVariable(const char* command, SmartData<argUInt>* object) {
  Serial.printf("assignVariable on SD Uint with types %u and %u\n",type2int<SmartData<argUInt>>::result, PTR_SD_OBJECT);
  Types types = {type2int<SmartData<argUInt>>::result, PTR_SD_OBJECT};
  Serial.printf("assignVariable on SD Uint with types %u and %u\n",types.data_type, types.ptr_type);

  addCommandInternal(command,types, object);
}
*/
// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command, SmartData<uint8_t> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<uint16_t> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<uint> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<unsigned long> *object, bool read_only);
template void qCommand::assignVariable(const char *command, uint8_t *variable, bool read_only);
template void qCommand::assignVariable(const char *command, uint16_t *variable, bool read_only);
template void qCommand::assignVariable(const char *command, uint *variable, bool read_only);
template void qCommand::assignVariable(const char *command, ulong *variable, bool read_only);

template void qCommand::assignVariable(const char *command, SmartData<String> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<String,false> *object, bool read_only);
//template void qCommand::assignVariable(const char *command, SmartData<String,true> *object, bool read_only);

/*
//Assign variable to command list for signed ints (calls ReportInt). Takes pointer to either data or DataObject.
template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value ||
  std::is_same<argInt, int16_t>::value ||
  std::is_same<argInt, int>::value ||
  std::is_same<argInt, long>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, argInt* variable) {
  Types types = {type2int<SmartData<argInt>>::result, PTR_RAW_DATA};
  addCommandInternal(command,types, variable);
}

template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value ||
  std::is_same<argInt, int16_t>::value ||
  std::is_same<argInt, int>::value ||
  std::is_same<argInt, long>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, SmartData<argInt>* object) {
  Types types = {type2int<SmartData<argInt>>::result, PTR_SD_OBJECT};
  addCommandInternal(command ,types, object);
}
*/

// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command, SmartData<int8_t> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<int16_t> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<int> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<long> *object, bool read_only);
template void qCommand::assignVariable(const char *command, int8_t *variable, bool read_only);
template void qCommand::assignVariable(const char *command, int16_t *variable, bool read_only);
template void qCommand::assignVariable(const char *command, int *variable, bool read_only);
template void qCommand::assignVariable(const char *command, long *variable, bool read_only);

/*
//Assign variable to command list for floating point numbers (calls ReportFloat). Takes pointer to either data or DataObject.
template <typename argFloat, std::enable_if_t<
  std::is_floating_point<argFloat>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, argFloat* variable) {
  Types types = {type2int<SmartData<argFloat>>::result, PTR_RAW_DATA};
  addCommandInternal(command,types, variable);
}

template <typename argFloat, std::enable_if_t<
  std::is_floating_point<argFloat>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, SmartData<argFloat>* object) {
  Types types = {type2int<SmartData<argFloat>>::result, PTR_SD_OBJECT};
  addCommandInternal(command,types, object);
}
*/
// Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char *command, float *variable, bool read_only);
template void qCommand::assignVariable(const char *command, double *variable, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<float> *object, bool read_only);
template void qCommand::assignVariable(const char *command, SmartData<double> *object, bool read_only);

void qCommand::invalidAddress(qCommand &qC, Stream &S, void *ptr, const char *command, void *object)
{
  S.printf("Invalid memory address assigned to command %s\n", command);
}

bool qCommand::str2Bool(const char *string)
{
  bool result = false;
  const uint8_t stringLen = 10;
  char tempString[stringLen + 1];
  strncpy(tempString, string, stringLen); // make copy of argument to convert to lower case
  tempString[stringLen] = '\0';           // null terminate in case arg is longer than size of tempString
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

void qCommand::reportString(qCommand &qC, Stream &S, const char *command, uint8_t ptr_type, void *ptr)
{
  if (ptr_type == PTR_SD_OBJECT)
  {
    SmartData<String> *object = (SmartData<String> *)ptr;
    if (qC.next() != NULL)
    {
      object->set(qC.current());
    }
    S.printf("%s is %s\n", command, object->get().c_str());
  }
  else
  {
    String *object = (String *)ptr;
    if (qC.next() != NULL)
    {
      *object = qC.current();
    }
    S.printf("%s is %s\n", command, object->c_str());
  }
}

void qCommand::reportBool(qCommand &qC, Stream &S, bool *ptr, const char *command, SmartData<bool> *object)
{
  bool temp;
  // Serial2.printf("String is %s\n", );
  if (qC.next() != NULL)
  {
    temp = qC.str2Bool(qC.current());
    if (object != NULL)
    {
      // Serial2.printf("Set Bool to %u\n",temp);
      object->set(temp);
    }
    else
    {
      *ptr = temp;
    }
  }

  if (object != NULL)
  {
    temp = object->get();
  }
  else
  {
    temp = *ptr;
  }

  S.printf("%s is %s\n", command, temp ? "true" : "false");
}

template <class argUInt>
void qCommand::reportUInt(qCommand &qC, Stream &S, const char *command, Types types, argUInt *ptr)
{
  Serial.printf("reportUInt called with with ptr at 0x%08x and value of %u\n", ptr, *(argUInt *)ptr);
  setDebugWord(0x12344489);
  unsigned long temp;
  argUInt newValue;
  setDebugWord(0x01010001);
  if (qC.next() != NULL)
  {
    setDebugWord(0x01010002);
    long temp2 = atoi(qC.current());
    setDebugWord(0x01010003);
    if (temp2 < 0)
    {
      temp = 0;
      setDebugWord(0x01010004);
    }
    else
    {
      setDebugWord(0x01010005);
      temp = strtoul(qC.current(), NULL, 10);
      setDebugWord(0x01010006);
      if (temp > std::numeric_limits<argUInt>::max())
      {
        setDebugWord(0x01010007);
        temp = std::numeric_limits<argUInt>::max();
        setDebugWord(0x01010008);
      }
    }
    setDebugWord(0x01010010);
    newValue = temp;
    if (types.sub_types.ptr == PTR_SD_OBJECT)
    {
      SmartData<argUInt> *object = (SmartData<argUInt> *)ptr;
      setDebugWord(0x01010011);
      object->set(newValue);
      setDebugWord(0x01010012);
    }
    else
    {
      setDebugWord(0x01010013);
      *ptr = newValue;
      setDebugWord(0x01010014);
    }
  }
  setDebugWord(0x01010015);
  if (types.sub_types.ptr == PTR_SD_OBJECT)
  {
    debugStream->printf("SD Object with %08x\n",types);
    SmartData<argUInt> *object = (SmartData<argUInt> *)ptr;
    setDebugWord(0x01010016);
    newValue = object->get();
    setDebugWord(0x01010017);
  }
  else
  {
    setDebugWord(0x01010018);
    newValue = *ptr;
    Serial.printf("And now newValue is %u but ptr deref is %u\n", newValue, *(uint8_t *)ptr);
    setDebugWord(0x01010019);
  }
  setDebugWord(0x12344480);
  S.printf("%s is %u\n", command, newValue);
}

template <class argInt>
void qCommand::reportInt(qCommand &qC, Stream &S, const char *command, Types types, argInt *ptr)
{
  int temp;
  if (qC.next() != NULL)
  {
    temp = atoi(qC.current());
    if (temp < std::numeric_limits<argInt>::min())
    {
      temp = std::numeric_limits<argInt>::min();
    }
    else if (temp > std::numeric_limits<argInt>::max())
    {
      temp = std::numeric_limits<argInt>::max();
    }

    if (types.sub_types.ptr == PTR_SD_OBJECT)
    {
      SmartData<argInt> *object = (SmartData<argInt> *)ptr;
      object->set(temp);
    }
    else
    {
      *ptr = temp;
    }
  }
  if (types.sub_types.ptr == PTR_SD_OBJECT)
  {
    SmartData<argInt> *object = (SmartData<argInt> *)ptr;
    temp = object->get();
  }
  else
  {
    temp = *ptr;
  }
  S.printf("%s is %d\n", command, temp);
}

template <class argFloating>
void qCommand::reportFloat(qCommand &qC, Stream &S, const char *command, Types types, argFloating *ptr)
{
  argFloating newValue;
  if (qC.next() != NULL)
  {
    newValue = atof(qC.current());
    if (types.sub_types.ptr == PTR_SD_OBJECT)
    {
      SmartData<argFloating> *object = (SmartData<argFloating> *)ptr;
      object->set(newValue);
    }
    else
    {
      if (sizeof(argFloating) > 4)
      { // make setting variable atomic for doubles or anything greater than 32bits.
        __disable_irq();
        *ptr = newValue;
        __enable_irq();
      }
      else
      {
        *ptr = newValue;
      }
    }
  }

  if (types.sub_types.ptr == PTR_SD_OBJECT)
  {
    SmartData<argFloating> *object = (SmartData<argFloating> *)ptr;
    newValue = object->get();
  }
  else
  {
    newValue = *ptr;
  }

  if ((abs(newValue) > 10) || (abs(newValue) < .1))
  {
    S.printf("%s is %e\n", command, newValue); // print gain in scientific notation
  }
  else
  {
    S.printf("%s is %f\n", command, newValue);
  }
}
/*
template <class argType>
void qCommand::reportGetSet(qCommand& qC, Stream& S, argType (*get_ptr)(void), void (*set_ptr)(argType), const char* command) {
  if ( qC.next() != NULL) {
    argType newValue = atof(qC.current());

    if ( sizeof(argFloating) >  4 ) { //make setting variable atomic for doubles or anything greater than 32bits.
      __disable_irq();
      //*ptr = newValue;
      set_ptr(newValue);
      __enable_irq();
    } else {
      set_ptr(newValue);
      //*ptr = newValue;
    }
  }
  if ( ( abs(*ptr) > 10 ) || (abs(*ptr) < .1) ) {
    S.printf("%s is %e\n",command,*ptr); //print gain in scientific notation
  } else {
    S.printf("%s is %f\n",command,*ptr);
  }
}
*/

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void qCommand::setDefaultHandler(void (*function)(const char *, qCommand &streamCommandParser, Stream &stream))
{
  defaultHandler = function;
}

void qCommand::reportData(qCommand &qC, Stream &inputStream, const char *command, Types types, void *ptr)
{
  inputStream.printf("Command: %s and data_type is %u (ptr_type is %u at addr 0x%08x)\n", command, types.sub_types.data, types.sub_types.ptr, ptr);
  switch (types.sub_types.data)
  {
  case 4:
    reportString(*this, inputStream, command, types.sub_types.ptr, static_cast<SmartData<String> *>(ptr));
    break;
  case 6: 
    reportUInt(*this, inputStream, command, types, static_cast<uint8_t *>(ptr));
    break;
  case 8:
    reportUInt(*this, inputStream, command, types, static_cast<uint16_t *>(ptr));
    break;
  case 10:
    reportUInt(*this, inputStream, command, types, static_cast<uint32_t *>(ptr));
    break;
  case 5:
    reportInt(*this, inputStream, command, types, static_cast<int8_t *>(ptr));
    break;
  case 7:
    reportInt(*this, inputStream, command, types, static_cast<int16_t *>(ptr));
    break;
  case 9:
    reportInt(*this, inputStream, command, types, static_cast<int32_t *>(ptr));
    break;
  case 11:
    reportFloat(*this, inputStream, command, types, static_cast<float *>(ptr));
    break;
  case 12:
    reportFloat(*this, inputStream, command, types, static_cast<double *>(ptr));
    break;
  default:
    inputStream.printf("Unknown data type %u\n", types.sub_types.data);
    break;
  }
}

/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void qCommand::readSerial(Stream &inputStream)
{
  while (inputStream.available() > 0)
  {
    char inChar = inputStream.read(); // Read single available character, there may be more waiting
#ifdef SERIALCOMMAND_DEBUG
    Serial.print(inChar); // Echo back to serial stream
#endif

    if (inChar == term)
    { // Check for the terminator (default '\n') meaning end of command
#ifdef SERIALCOMMAND_DEBUG
      Serial.print("Received: ");
      Serial.println(buffer);
#endif

      if (!caseSensitive)
      {
        strlwr(buffer);
      }
      char *command = strtok_r(buffer, delim, &last); // Search for command at start of buffer
      if (command != NULL)
      {
        boolean matched = false;
        for (int i = 0; i < commandCount; i++)
        {
#ifdef SERIALCOMMAND_DEBUG
          Serial.print(" - Comparing [");
          Serial.print(command);
          Serial.print("] to [");
          Serial.print(commandList[i].command);
          Serial.println("]");
#endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, STREAMCOMMAND_MAXCOMMANDLENGTH) == 0)
          {
            matched = true;
            // Serial.printf("Found match on command %s\n",command);
            if (commandList[i].types.sub_types.ptr == PTR_QC_CALLBACK)
            {
              (commandList[i].ptr.f1)(*this, inputStream);
            }
            else if (commandList[i].types.sub_types.ptr == PTR_NULL)
            {
              inputStream.printf("Error: command %s has null pointer\n", command);
            }
            else
            {
              // SD-Object or Raw PTR
              reportData(*this, inputStream, command, commandList[i].types, commandList[i].ptr.object);
              //Send update on tracked variable if its RAW_DATA as SmartData will flag need to send
              Serial.printf("Send reportData on SD or Raw (0x%02x)\n", commandList[i].types.sub_types.ptr);
              if (commandList[i].types.sub_types.ptr == PTR_RAW_DATA) {
                send_update_on_tracked_variable(i);
                Serial.printf("Send update on tracked variable %u\n", i); 
              }
            }
#ifdef SERIALCOMMAND_DEBUG
            Serial.print("Matched Command: ");
            Serial.println(command);
#endif
            break;
          }
        }
        if (!matched)
        {
          if (defaultHandler != NULL)
          {
            (*defaultHandler)(command, *this, inputStream);
          }
          else
          {
            inputStream.print("Unknown command: ");
            inputStream.println(command);
          }
        }
      }
      bufPos = 0; // do not clear buffer to enter after command repeats it.
    }
    else if (inChar == '\b')
    { // backspace detected
      if (bufPos > 0)
      {
        bufPos--;              // move back bufPos to overright previous character
        buffer[bufPos] = '\0'; // Null terminate
      }
    }
    else if (isprint(inChar))
    { // Only printable characters into the buffer
      if (bufPos < STREAMCOMMAND_BUFFER)
      {
        buffer[bufPos++] = inChar; // Put character into buffer
        buffer[bufPos] = '\0';     // Null terminate
      }
      else
      {
#ifdef SERIALCOMMAND_DEBUG
        Serial.println("Line buffer is full - increase STREAMCOMMAND_BUFFER");
#endif
      }
    }
  }
}

/*
 * Print list of all available commands
 */
void qCommand::printAvailableCommands(Stream &outputStream)
{
  for (int i = 0; i < commandCount; i++)
  {
    outputStream.printf("%s (0x%08x)\n", commandList[i].command, commandList[i].ptr.object);
  }
}

/*
 * Clear the input buffer.
 */
void qCommand::clearBuffer()
{
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::next()
{
  cur = strtok_r(NULL, delim, &last);
  return cur;
}

/**
 * Retrieve the current token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::current()
{
  return cur;
}

// template class DataObject<unsigned int>;

// instatiate template from addCommandInternal
// template void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, bool* variable, const char* command), bool* var);

// template <typename DataType>
// void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, DataType* variable, const char* command), DataType* var) {
