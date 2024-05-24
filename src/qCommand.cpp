#include "qCommand.h"
#include <limits>

#include "cwpack.h"
#include "basic_contexts.h"
#include "cwpack_utils.h"

#define PACK_BUFFER_SIZE 20
cw_pack_context pc;
char buffer[DEFAULT_PACK_BUFFER_SIZE];

/**
 * Constructor
 */
qCommand::qCommand(bool caseSensitive) :
  commandList(NULL),
  commandCount(0),
  binaryStream(&Serial2),
  defaultHandler(NULL),
  term('\n'),           // default terminator for commands, newline character
  caseSensitive(caseSensitive),
	cur(NULL),
	last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}



char qCommand::readBinary(void) {
  //PT_FUNC_START(pt);


  for(uint8_t i=0; i<commandCount; i++) {
      if ( ( commandList[i].object != NULL) && ( (commandList[i].data_type & 0x03) == TYPE2INFO_ARRAY)) {
        //array
        //AllSmartDataPtr *ptr = (AllSmartDataPtr*) commandList[i].object;
        //SmartDataPtr<float*> *ptr = (void*) commandList[i].object;        
        AllSmartDataPtr *ptr = static_cast<AllSmartDataPtr*>(commandList[i].object);
        ptr->sendIfNeedValue();
        //ptr->please();
      }
  }


  setDebugWord(0xbbbb0001);
  //static uint8_t* data_ptr = 0;
  const uint8_t buffer_size_default = 3;

  static stream_unpack_context suc;
  static cw_unpack_context* uc = (cw_unpack_context*) &suc; //uc can point to suc but not visa versa since suc is uc + more in struct  
  int dataReady = binaryStream->available();
  if (dataReady != 0) {
    Serial.printf("Got %u bytes available...\n", dataReady);
  }  
  setDebugWord(0xbbbb0002);
  int ri;
  float rf;
  dataReady = min(dataReady,buffer_size_default); //only read 4k at a time for reduced memory allocation  
  setDebugWord(0xbbbc0000 + dataReady);
  if (dataReady != 0) {
    //Serial.printf("Got %d bytes to process: ",dataReady);        
    init_stream_unpack_context(&suc, buffer_size_default, binaryStream);    
    setDebugWord(0xbbbd0000 + dataReady);
    binaryStream->readBytes(uc->start,dataReady);    
    setDebugWord(0xbbbe0000 + dataReady);
    //void init_stream_unpack_context (stream_unpack_context* suc, unsigned long initial_buffer_length, size_t (*file)(const uint8_t*, size_t))
    //Serial.printf("Starting errors: %d %d\n", uc->return_code, uc->err_no  );
    
    //Serial.printf("Read %u bytes\n", dataReady);
    //uint8_t* temp = uc->current;
    //Serial.printf("Data: 0x%02x %02x %02x %02x\n", temp[0], temp[1], temp[2], temp[3]);
    //for (int i=0;i<dataReady;i++) {
    //    Serial.printf("0x%02x ", temp[i]);
    //}
    //Serial.println();
    //Serial.printf("Start: 0x%08x, current: 0x%08x, end:0x%08x\n",uc->start, uc->current, uc->end);
    uint itemsTotal = cw_unpack_next_array_size(uc);
    setDebugWord(0xbbbf0000 + dataReady);
    uint itemsReceived = 0;
    //cw_unpack_next(uc);
    //Serial.printf("1: 0x%08x, current: 0x%08x, end:0x%08x\n",uc->start, uc->current, uc->end);
    if (uc->return_code != CWP_RC_OK)  {
      Serial.printf("Error parsing array size, error is %d\n",uc->return_code);
      return 0;
    } 
    //Serial.printf("Error: %d %d\n", uc->return_code, uc->err_no  );
    uint index = cw_unpack_next_unsigned8(uc);  
    itemsReceived++;  
    setDebugWord(0xbbc10000 + dataReady);
    //Serial.printf("2: 0x%08x, current: 0x%08x, end:0x%08x\n",uc->start, uc->current, uc->end);
    //Serial.printf("First item has id: %u\n", index);
    
    Commands command = static_cast<Commands>(cw_unpack_next_unsigned8(uc));
    itemsReceived++;
    
    setDebugWord(0xbbc20000 + dataReady);
    //uint command;
    switch (index) {
      case 0:
        //internal command
        if (command == Commands::ListCommands && itemsTotal == 2) {          
          setDebugWord(0xbbc30000 + dataReady);
          sendBinaryCommands();    

        } else {
          Serial.println("Error parsing internal command");
        }
        break;
      default:
        if (index > commandCount) {
          Serial.println("Error: Command index out of range");
          break;
        }
        setDebugWord(0xbbc40000 + dataReady);          
        Serial.printf("About to run command %u (%s)\n",command, commandList[index-1].command);
        switch (command) {          
          case Commands::Get:
            Serial.printf("Command is get and object ptr is 0x%08x\n", commandList[index-1].object);
            if (commandList[index-1].object != NULL) {
              //Serial.printf("About sendValue on %s\n", commandList[index-1].command);
              //Serial.printf("Function callback address is 0x%08x and base is 0x%08x\n", &decltype(commandList[index-1].object)::sendValue, commandList[index-1].object);
              //Serial.print(commandList[index-1].command);
              //Serial.println();
              setDebugWord(0x4432abab);
              //#warning this is right, but crashes on SmartDataPtr
              commandList[index-1].object->sendValue();
              //Base* base = commandList[index-1].object;
              //Serial.printf("Now running please on base object 0x%08x with data=%u\n", base,commandList[index-1].data_type);
              
              
              //base->please();
              //setDebugWord(0x4432abcc);
              
            } else {
              Serial.println("Error: object not found Get command without object");
            }
            break;
          case Commands::Set:              
            setDebugWord(0xbbe41000 + dataReady);
            if (commandList[index-1].object == NULL) {
                //cannot update object that isn't of type SmartData                
                goto cleanup;
              }
              
            if (uc->return_code != CWP_RC_OK) {
              Serial.printf("Error parsing next from set, error is %d\n",uc->return_code);
              goto cleanup;                 
            }
            
            Serial.printf("Got new data (%s) for type: 0x%02x\n",commandList[index-1].command, commandList[index-1].data_type);
            setDebugWord(0xbbc50000 + dataReady);
            switch(commandList[index-1].data_type & 0x0F) {
                case TYPE2INFO_MIN + TYPE2INFO_BOOL:{
                  //bool 
                  //bool res = cw_unpack_next_boolean(uc);
                  cw_unpack_next(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&(uc->item.as));
                  break;
                }
                case TYPE2INFO_MIN + TYPE2INFO_UINT:{
                  //uint8_t
                  uint8_t res = cw_unpack_next_unsigned8(uc);
                  itemsReceived++;
                  if ( uc->return_code == CWP_RC_COERCED_VALUE) {
                    //do not care about coerced values
                    uc->return_code = CWP_RC_OK;
                  }

                  if (uc->return_code == CWP_RC_OK)  {
                  commandList[index-1].object->_set(&res);
                  } else {
                    commandList[index-1].object->sendValue();
                  }
                  break;}
                case TYPE2INFO_2MIN + TYPE2INFO_UINT:{
                  //uint16_t
                  uint16_t res = cw_unpack_next_unsigned16(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                case TYPE2INFO_4MIN + TYPE2INFO_UINT:{
                  //uint32_t
                  uint32_t res = cw_unpack_next_unsigned32(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                case TYPE2INFO_MIN + TYPE2INFO_INT:{
                  //uint8_t
                  int8_t res = cw_unpack_next_signed8(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                case TYPE2INFO_2MIN + TYPE2INFO_INT:{
                  //uint16_t
                  int16_t res = cw_unpack_next_signed16(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                case TYPE2INFO_4MIN + TYPE2INFO_INT:{
                  //uint32_t
                  int32_t res = cw_unpack_next_signed32(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                case TYPE2INFO_MIN + TYPE2INFO_FLOAT:{
                  //float
                  float res = cw_unpack_next_float(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  Serial.printf("Got float %f\n",res);
                  break;}
                case TYPE2INFO_2MIN + TYPE2INFO_FLOAT: {
                  //float
                  double res = cw_unpack_next_double(uc);
                  itemsReceived++;
                  commandList[index-1].object->_set(&res);
                  break;}
                default:
                  Serial.printf("Unknown type: 0x02x\n",commandList[index-1].data_type);
                break;
              }


            if (commandList[index-1].object != NULL) {
                setDebugWord(0x4432abdd);
                commandList[index-1].object->sendValue();
                setDebugWord(0x4432abff);
                Serial.println("Shold run sendValue here for ");
                Serial.print(commandList[index-1].command);
                Serial.println();
            } else {
              Serial.println("Error: object not found Set command without object");
            }
            break;
            
            case Commands::Request:
              Serial.printf("Command is request and object ptr is 0x%08x\n", commandList[index-1].object);
              if ( (commandList[index-1].object != NULL) && ( (commandList[index-1].data_type & 0x03 )== TYPE2INFO_ARRAY ) ) {
                //Got SmartDataPtr object
                commandList[index-1].object->_get(NULL);
                //Base* base = commandList[index-1].object;
                //Serial.printf("Now running please on base object 0x%08x with data=%u\n", base,commandList[index-1].data_type);
                
                
                //base->please();
                //setDebugWord(0x4432abcc);
              
            } else {
              Serial.println("Error: SmartDataPtr object not found for Request command");
            }

            default:
              Serial.printf("Unknown command: %u, index: %u\n",command, index);
              break;        
        }
        
        //Serial.println("Uknown command");
        break;

    }
    
    //uint junk = cw_unpack_next_unsigned32(uc);
    setDebugWord(0xbbcf0000 + dataReady);          
  cleanup:
  //clear out the data
  while (itemsReceived < itemsTotal) {
    cw_unpack_next(uc);
    itemsReceived++;
  }
  return 0;


  //Serial.printf("4: 0x%08x, current: 0x%08x, end:0x%08x\n",uc->start, uc->current, uc->end);
  //Serial.printf("Third item has id: %u\n", junk);

    //if ((uc->return_code == 0) && (itemsTotal == 2)) {
    //  Serial.println("Got a full packet with 2 items and we have their values");
    //  return 0;
    //}

    setDebugWord(0xdea12410);
    return 1;
    uint8_t* data = new uint8_t[dataReady];
    if (data) {
      //bool result = unpacker.feed(data,dataReady);
      bool result = false;
      if (result) {
        Serial.printf("Got success on feed with size %u\n", dataReady);
        //unpacker.deserialize(ri,rf);
        Serial.printf("Got data: %d, %f\n",ri,rf);
      }
    }
  }  
  return 0;
}

void qCommand::sendBinaryCommands(void) {
  //packer.clear();
  //uint8_t i = 5;
  //uint8_t j = 9;
  //packer.serialize(i,j);

  uint8_t elements = 0;
  for (uint8_t i=0; i < commandCount; i++) {    
    if (commandList[i].object != NULL) {
      elements++;
      Serial.printf("Adding: %s (ptr=0x%08x) data_type=0x%02x)\n",commandList[i].command, commandList[i].object, commandList[i].data_type);
    } else {
      Serial.printf("Skipping: %s (ptr=0x%08x) data_type=0x%02x)\n",commandList[i].command, commandList[i].object, commandList[i].data_type);
    }
  }
  //packer.serialize(MsgPack::arr_size_t(elements));
  cw_pack_array_size(&pc,3); // three elements, first is id = 0 for internal. Second is the command (list Commands). Third is the data
  cw_pack_unsigned(&pc,0); // set ID = 0 for internal
  cw_pack_unsigned(&pc,static_cast<uint8_t>(Commands::ListCommands));
  cw_pack_array_size(&pc,elements);
  Serial.printf("Start Binary Command send with %u elements\n",elements);  
  Serial.printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n", pc.start, pc.current, pc.end  );
  for (uint8_t i=0; i < commandCount; i++) {    
    if ( commandList[i].object != NULL) {               
      cw_pack_array_size(&pc,3);
      cw_pack_unsigned(&pc,i+1);
      cw_pack_str(&pc, commandList[i].command, strlen(commandList[i].command));
      cw_pack_unsigned(&pc,commandList[i].data_type);

      //MsgPack::str_t cmd_string = commandList[i].command;
      //packer.serialize(MsgPack::arr_size_t(3),i+1, cmd_string, commandList[i].data_type );
  
      //packer.packBinary32(const uint8_t* value, const uint32_t size);      
      //packer.to_array(id,value, crc );
      //Serial.printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n", pc.start, pc.current, pc.end  );
      //Serial.printf("So far, binary data of size %u (error=%d)\n",pc.current - pc.start, pc.return_code );
    }
  }
  Serial.printf("Send binary data of size %u (error=%d)\n",pc.current - pc.start, pc.return_code );
  binaryStream->write(pc.start,pc.current - pc.start);  
  pc.current = pc.start;
  
}


/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void qCommand::addCommand(const char *command, void (*function)(qCommand& streamCommandParser, Stream& stream)) {
  #ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif

  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function.f1 = function;
  commandList[commandCount].ptr = NULL;
  commandList[commandCount].object = NULL;

  if (!caseSensitive) {
    strlwr(commandList[commandCount].command);
  }
  //Serial.printf("CC from %u", commandCount);
  commandCount++;
  //Serial.printf(" to %u\n", commandCount);
}

template <typename DataType>
void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, DataType* variable, const char* command, SmartData<DataType>* object),DataType* var, SmartData<DataType>* object)  {
  #ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Assign Variable Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif
  
  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  
  if (object != NULL)  {
    //have SmartData object pointer
    commandList[commandCount].object = object;
    commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
    commandList[commandCount].ptr = NULL;    
    //object->_setPrivateInfo(commandCount+1, binaryStream, &packer);
    object->_setPrivateInfo(commandCount+1, binaryStream, NULL);
    commandList[commandCount].data_type = type2int<SmartData<DataType>>::result;

  } else {
    commandList[commandCount].object = NULL;
  
    if ( var == NULL) {
		  //catch NULL pointer and trap with function that can handle it
		  commandList[commandCount].function.f2 =  &qCommand::invalidAddress;
		  commandList[commandCount].ptr = (void*) 1;
	  } else {
		  commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
		  commandList[commandCount].ptr = (void*) var;
    }
	}

	if (!caseSensitive) {
	   strlwr(commandList[commandCount].command);
	}
	Serial.printf("CC from %u", commandCount);
  commandCount++;
  Serial.printf(" to %u\n", commandCount);
}

template <typename DataType>
void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, DataType* variable, const char* command, SmartDataPtr<DataType>* object),DataType* var, SmartDataPtr<DataType>* object)  {  
  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  
  if (object != NULL)  {
    //have SmartData object pointer
    commandList[commandCount].object = object;
    commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
    commandList[commandCount].ptr = NULL;    
    //object->_setPrivateInfo(commandCount+1, binaryStream, &packer);
    object->_setPrivateInfo(commandCount+1, binaryStream, NULL);
    commandList[commandCount].data_type = type2int<SmartDataPtr<DataType>>::result;

  } else {
    commandList[commandCount].object = NULL;
  
    if ( var == NULL) {
		  //catch NULL pointer and trap with function that can handle it
		  commandList[commandCount].function.f2 =  &qCommand::invalidAddress;
		  commandList[commandCount].ptr = (void*) 1;
	  } else {
		  commandList[commandCount].function.f2 = (void(qCommand::*)(qCommand& streamCommandParser, Stream& stream, void* ptr, const char* command, void* object)) function;
		  commandList[commandCount].ptr = (void*) var;
    }
	}

	if (!caseSensitive) {
	   strlwr(commandList[commandCount].command);
	}
	Serial.printf("CC from %u", commandCount);
  commandCount++;
  Serial.printf(" to %u\n", commandCount);
}



template <typename argArray, std::enable_if_t<std::is_pointer<argArray>::value, uint> = 0>    
void qCommand::assignVariable(const char* command, SmartDataPtr<argArray>* object) {
	Serial.printf("Trying to assign array to command %s\n",command);
  void (qCommand::*function)(qCommand&, Stream&, argArray*, const char*, SmartDataPtr<argArray>*) = nullptr;
  addCommandInternal(command, function, (argArray*) NULL, object);
}

template void qCommand::assignVariable(const char* command, SmartDataPtr<float*>* object);



//Assign variable to command list for string. Takes pointer to either data or DataObject.
void qCommand::assignVariable(const char* command, String* variable) {
	addCommandInternal(command,&qCommand::reportString, variable, (SmartData<String>*) NULL);
}

void qCommand::assignVariable(const char* command, SmartData<String>* object) {
	Serial.printf("Trying to assign object to command %s\n",command);
  addCommandInternal(command,&qCommand::reportString, (String*) NULL, object);
}




//Assign variable to command list for booleans. Takes pointer to either data or DataObject.
void qCommand::assignVariable(const char* command, bool* variable) {
	addCommandInternal(command,&qCommand::reportBool, variable, (SmartData<bool>*) NULL);
}

void qCommand::assignVariable(const char* command, SmartData<bool>* object) {
	addCommandInternal(command,&qCommand::reportBool, (bool*) NULL, object);
}

//Assign variable to command list for unsigned ints (calls ReportUInt). Takes pointer to either data or DataObject.
template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value || 
  std::is_same<argUInt, uint>::value || 
  std::is_same<argUInt, ulong>::value
  , uint> = 0>    
void qCommand::assignVariable(const char* command, argUInt* variable) {
	addCommandInternal(command,&qCommand::reportUInt, variable, (SmartData<argUInt>*) NULL);
}

template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value || 
  std::is_same<argUInt, uint>::value || 
  std::is_same<argUInt, ulong>::value
  , uint> = 0>    
void qCommand::assignVariable(const char* command, SmartData<argUInt>* object) {
	addCommandInternal(command,&qCommand::reportUInt, (argUInt*) NULL, object);
}

//Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char* command, SmartData<uint8_t>* object);
template void qCommand::assignVariable(const char* command, SmartData<uint16_t>* object);
template void qCommand::assignVariable(const char* command, SmartData<uint>* object);
template void qCommand::assignVariable(const char* command, SmartData<unsigned long>* object);
template void qCommand::assignVariable(const char* command, uint8_t* variable);
template void qCommand::assignVariable(const char* command, uint16_t* variable);
template void qCommand::assignVariable(const char* command, uint* variable);
template void qCommand::assignVariable(const char* command, ulong* variable);



//Assign variable to command list for signed ints (calls ReportInt). Takes pointer to either data or DataObject.
template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value || 
  std::is_same<argInt, int16_t>::value || 
  std::is_same<argInt, int>::value ||  
  std::is_same<argInt, long>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, argInt* variable) {
	addCommandInternal(command,&qCommand::reportInt, variable, (SmartData<argInt>*) NULL);
}

template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value || 
  std::is_same<argInt, int16_t>::value || 
  std::is_same<argInt, int>::value ||  
  std::is_same<argInt, long>::value
  , int> = 0>
void qCommand::assignVariable(const char* command, SmartData<argInt>* object) {
	addCommandInternal(command,&qCommand::reportInt, (argInt*) NULL, object);
}


//Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char* command, SmartData<int8_t>* object);
template void qCommand::assignVariable(const char* command, SmartData<int16_t>* object);
template void qCommand::assignVariable(const char* command, SmartData<int>* object);
template void qCommand::assignVariable(const char* command, SmartData<long>* object);
template void qCommand::assignVariable(const char* command, int8_t* variable);
template void qCommand::assignVariable(const char* command, int16_t* variable);
template void qCommand::assignVariable(const char* command, int* variable);
template void qCommand::assignVariable(const char* command, long* variable);


//Assign variable to command list for floating point numbers (calls ReportFloat). Takes pointer to either data or DataObject.
template <typename argFloat, std::enable_if_t<      
  std::is_floating_point<argFloat>::value
  , int> = 0>        
void qCommand::assignVariable(const char* command, argFloat* variable) {
  addCommandInternal(command,&qCommand::reportFloat, variable, (SmartData<argFloat>*) NULL);
}

template <typename argFloat, std::enable_if_t<      
  std::is_floating_point<argFloat>::value
  , int> = 0>        
void qCommand::assignVariable(const char* command, SmartData<argFloat>* object) {
  addCommandInternal(command,&qCommand::reportFloat, (argFloat*) NULL, object);
}

//Add template lines here so functions get compiled into file for linking
template void qCommand::assignVariable(const char* command, float* variable);
template void qCommand::assignVariable(const char* command, double* variable);
template void qCommand::assignVariable(const char* command, SmartData<float>* object);
template void qCommand::assignVariable(const char* command, SmartData<double>* object);


void qCommand::invalidAddress(qCommand& qC, Stream& S, void* ptr, const char* command, void* object) {
	S.printf("Invalid memory address assigned to command %s\n",command);
}

bool qCommand::str2Bool(const char* string) {
	bool result = false;
	const uint8_t stringLen = 10;
	char tempString[stringLen+1];
	strncpy(tempString,string,stringLen); //make copy of argument to convert to lower case
	tempString[stringLen] = '\0'; //null terminate in case arg is longer than size of tempString
	strlwr(tempString);

	if (strcmp(tempString,"on") == 0) result = true;
	else if (strcmp(tempString,"true") == 0) result = true;
	else if (strcmp(tempString,"1") == 0) result = true;
	else if (strcmp(tempString,"off") == 0) result = false;
	else if (strcmp(tempString,"false") == 0) result = false;
	else if (strcmp(tempString,"0") == 0) result = false;
	return result;
}

void qCommand::reportString(qCommand& qC, Stream& S, String* ptr, const char* command, SmartData<String>* object) {  
  if ( qC.next() != NULL) {		    
    if (object != NULL) {      
      object->set(qC.current());
    } else {
      *ptr = qC.current();
    }
  }

  if (object != NULL) {  
    S.printf("%s is %s\n",command, object->get().c_str());        
  } else {    
    S.printf("%s is %s\n",command, ptr->c_str());
  }
  
	
}



void qCommand::reportBool(qCommand& qC, Stream& S, bool* ptr, const char* command, SmartData<bool>* object) {  
  bool temp;
  //Serial2.printf("String is %s\n", );
  if ( qC.next() != NULL) {
		temp = qC.str2Bool(qC.current());
    if (object != NULL) {
      //Serial2.printf("Set Bool to %u\n",temp);
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
  
	S.printf("%s is %s\n",command, temp ? "true":"false");
}


template <class argUInt>
void qCommand::reportUInt(qCommand& qC, Stream& S, argUInt* ptr, const char* command, SmartData<argUInt>* object) {
	setDebugWord(0x12344489);
  unsigned long temp;
  argUInt newValue;
  setDebugWord(0x01010001);
  if ( qC.next() != NULL) {
		setDebugWord(0x01010002);
    long temp2 = atoi(qC.current());
    setDebugWord(0x01010003);
		if (temp2 < 0) {		
      temp = 0;          
      setDebugWord(0x01010004);
		} else {
			setDebugWord(0x01010005);
      temp = strtoul(qC.current(),NULL,10);
      setDebugWord(0x01010006);
			if ( temp > std::numeric_limits<argUInt>::max()) {
				setDebugWord(0x01010007);
        temp = std::numeric_limits<argUInt>::max();
        setDebugWord(0x01010008);
			} 
		}
    setDebugWord(0x01010010);
    newValue = temp;
    if (object != NULL) {
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
  if (object != NULL) {  
    setDebugWord(0x01010016);
    newValue = object->get();
    setDebugWord(0x01010017);
  } else {
    setDebugWord(0x01010018);
    newValue = *ptr;
    setDebugWord(0x01010019);
  }
  setDebugWord(0x12344480);
	S.printf("%s is %u\n",command,newValue);
}


template <class argInt>
void qCommand::reportInt(qCommand& qC, Stream& S, argInt* ptr, const char* command, SmartData<argInt>* object) {
	int temp;
  if ( qC.next() != NULL) {
		temp = atoi(qC.current());
		if ( temp < std::numeric_limits<argInt>::min()) {
			temp = std::numeric_limits<argInt>::min();
		} else if ( temp > std::numeric_limits<argInt>::max()) {
			temp = std::numeric_limits<argInt>::max();
		} 
    if (object != NULL) {
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
  S.printf("%s is %d\n",command,temp);
}



template <class argFloating>
void qCommand::reportFloat(qCommand& qC, Stream& S, argFloating* ptr, const char* command, SmartData<argFloating>* object) {	
  argFloating newValue;
  if ( qC.next() != NULL) {
		newValue = atof(qC.current());        
    if (object != NULL) {
        object->set(newValue);
    } else {
		  if ( sizeof(argFloating) >  4 ) { //make setting variable atomic for doubles or anything greater than 32bits.
  			__disable_irq();
	  		*ptr = newValue;
		  	__enable_irq();
		  } else {
			  *ptr = newValue;
		  }
    }
	}

  if (object != NULL) {
    newValue = object->get();
  } else {
    newValue = *ptr;
  }
	
  if ( ( abs(newValue) > 10 ) || (abs(newValue) < .1) ) {
		S.printf("%s is %e\n",command,newValue); //print gain in scientific notation
	} else {
		S.printf("%s is %f\n",command,newValue);
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
void qCommand::setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser, Stream& stream)) {
  defaultHandler = function;
}



/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void qCommand::readSerial(Stream& inputStream) {
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

        if (!caseSensitive) {
		  strlwr(buffer);
        }
      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
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

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, STREAMCOMMAND_MAXCOMMANDLENGTH) == 0) {
            #ifdef SERIALCOMMAND_DEBUG
              Serial.print("Matched Command: ");
              Serial.println(command);
            #endif
            // Execute the stored handler function for the command
              if (commandList[i].object != NULL) {
                //only run object function if object is single and not array
                if ( (commandList[i].data_type & 0x03 ) != TYPE2INFO_ARRAY ) {
                  //#warning this is right
                  Serial.print("Running on non array object:\n");
                  (this->*commandList[i].function.f2)(*this,inputStream,commandList[i].ptr,commandList[i].command, commandList[i].object);
                  Base* base = commandList[i].object;
                  base->please();
                } else {
                  inputStream.println("Arrays do not support ASCII command line interaction, and must use binary command structure");                  
                  Base* base = commandList[i].object;        
                  Serial.printf("Cmd: %s, with data type: %u or 0x%02x and base of 0x%08x\n",commandList[i].command, commandList[i].data_type, commandList[i].data_type, base);
                }
              } else {                
                if (commandList[i].ptr == NULL) {
            	    (*commandList[i].function.f1)(*this,inputStream);
                } else {
            	    (this->*commandList[i].function.f2)(*this,inputStream,commandList[i].ptr,commandList[i].command, NULL);
                }
              }
            matched = true;
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
      bufPos = 0; //do not clear buffer to enter after command repeats it.
    } else if (inChar == '\b') { //backspace detected
      if (bufPos > 0) {
          bufPos--; //move back bufPos to overright previous character
          buffer[bufPos] = '\0';// Null terminate
      }
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


/*
 * Print list of all available commands
 */
void qCommand::printAvailableCommands(Stream& outputStream) {
  for (int i = 0; i < commandCount; i++) {
    outputStream.println(commandList[i].command);
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

/**
 * Retrieve the current token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::current() {
  return cur;
}



//template class DataObject<unsigned int>;

//instatiate template from addCommandInternal
//template void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, bool* variable, const char* command), bool* var);




//template <typename DataType>
//void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, DataType* variable, const char* command), DataType* var) {
