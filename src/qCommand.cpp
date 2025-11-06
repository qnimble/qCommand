#include "qCommand.h"

#include <limits>

#include "electricui.h"
#include "pt-sleep.h"
#include "pt.h"

static eui_interface_t serial_comms;

qCommand::qCommand(bool caseSensitive)
	: binaryStream(&Serial3),
	  callBack{nullptr},
	  commandCount(0),
	  commandList(NULL),
	  defaultHandler(NULL),
	  term('\n'),  // default terminator for commands, newline character
	  caseSensitive(caseSensitive),
	  cur(NULL),
	  last(NULL),
	  bufPos(0),
	  binaryConnected(false) {
	strcpy(delim, " ");	 // strtok_r needs a null-terminated string
	clearBuffer();
#ifdef ARDUINO_QUARTO
	// getHardwareUUID is Quarto specific. Need some generic Arduino way to
	// getting unique ID....
	uint32_t uuid[4];
	getHardwareUUID(uuid, sizeof(uuid));
	eui_setup_identifier(
		(char *)uuid,
		sizeof(uuid));	// set EUI unique ID based on hardware UUID
#else
#warning No generic way to get unique ID, so for general hardware, hardcoding to 0x13572468
	uint32_t uuid = 0x13572468;
	eui_setup_identifier(
		(char *)uuid,
		sizeof(uuid));	// set EUI unique ID based on hardware UUID
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
		if (isSmartObject(commandList[i].types.sub_types.ptr)) {
			Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
			ptr->resetUpdateState();
		}
	}
}

void qCommand::readBinary(void) {
	PT_SCHEDULE(readBinaryInt2());
	// PT_SCHEDULE(checkHeartBeat());
}

uint16_t qCommand::sizeOfType(qCommand::Types type) {
	switch (type.sub_types.data) {
		case 3:	 // byte
		case 4:	 // char / string
		case 5:	 // int8
		case 6:	 // uint8
			return 1;
		case 7:	 // int16
		case 8:	 // uint16
			return 2;
		case 9:	  // int32
		case 10:  // uint32
		case 11:  // float
			return 4;
		case 12:  // double
			return 8;
		case 13:
			return 1;
		default:
			return 0;
	}
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
			case 13: {
				SmartData<bool> *ptrb = NULL;
				stop_ptr = reinterpret_cast<uintptr_t>(&(ptrb->value));
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
			case 13: {
				SmartData<bool *> *boolData = NULL;
				stop_ptr = reinterpret_cast<uintptr_t>(&(boolData->value));
			} break;
			default:
				stop_ptr = 0;
		}
	}
	return stop_ptr;
}

template <typename T>
void set_smart_data(void *data_ptr, uint8_t *data_in) {
	SmartData<T> *smart_data =
		reinterpret_cast<SmartData<T> *>(const_cast<void *>(data_ptr));
	smart_data->set(*reinterpret_cast<T *>(data_in));
}

void set_object(eui_message_t *p_msg_obj, uint16_t offset, uint8_t *data_in,
				uint16_t len) {
	// Serial2.printf("setObj called: new data has length: %u\n", len);
	if (offset != 0) {
		// Do not support offset writes to SmartData objects
		return;
	} else {
		qCommand::Types type;
		type.raw = p_msg_obj->type;
		if (type.sub_types.data == 4) {
			// String pointer
			SmartData<String> *SD_String =
				reinterpret_cast<SmartData<String> *>(
					const_cast<void *>(p_msg_obj->ptr.data));
			// p_msg_obj->size = strlen(SD_String->get().c_str()) + 1;
			// if (p_msg_obj->size >= len) {
			SD_String->set((char *)data_in);
			//}
		} else {
			// size_t offset = qCommand::getOffset(type, p_msg_obj->size);
			const uint8_t *data =
				(static_cast<const uint8_t *>(p_msg_obj->ptr.data) + offset);
			if (p_msg_obj->size > qCommand::sizeOfType(type)) {
				// Array of data
				memcpy((uint8_t *)data, data_in, len);
			} else {
				switch (type.sub_types.data) {
					case 5:
						set_smart_data<int8_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 6:
						set_smart_data<uint8_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 7:
						set_smart_data<int16_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 8:
						set_smart_data<uint16_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 9:
						set_smart_data<int32_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 10:
						set_smart_data<uint32_t>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 11:
						set_smart_data<float>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 12:
						set_smart_data<double>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
					case 13:
						set_smart_data<bool>(
							const_cast<void *>(p_msg_obj->ptr.data), data_in);
						break;
				}
			}
			// const void *final_ptr = (const void *)(*(uint32_t *)data);
			// return final_ptr;
			//} else {
			//    return static_cast<const void *>(data);
			//}
		}
	}
}

//void qCommand::setDefaultLayout(const char* layout) { 
//	::set_default_layout(layout); 
//};

const void *ptr_settings_from_object(eui_message_t *p_msg_obj) {
	qCommand::Types type;
	type.raw = p_msg_obj->type;
	if (type.sub_types.data == 4) {
		// String pointer
		const SmartData<String> *SD_String =
			static_cast<const SmartData<String> *>(p_msg_obj->ptr.data);
		p_msg_obj->size = strlen(SD_String->get().c_str()) + 1;
		return static_cast<const void *>(SD_String->get().c_str());
	} else {
		size_t offset = qCommand::getOffset(type, p_msg_obj->size);
		const uint8_t *data =
			(static_cast<const uint8_t *>(p_msg_obj->ptr.data) + offset);
		if (p_msg_obj->size > qCommand::sizeOfType(type)) {
			// Array of data
			const void *final_ptr = (const void *)(*(uint32_t *)data);
			return final_ptr;
		} else {
			return static_cast<const void *>(data);
		}
	}
}

void reset_object(void *ptr) {
	Base *ptrBase = static_cast<Base *>(ptr);
	ptrBase->resetUpdateState();
}

void ack_object(void *ptr) {
	Base *ptrBase = static_cast<Base *>(ptr);
	ptrBase->ackObject();
}

uint8_t number_of_valid_entries(const void* ptr) {
	const Base *ptrBase = static_cast<const Base *>(ptr);
	return ptrBase->getMapSize();
}

uint16_t list_or_key_pair(const void* ptr, uint8_t index, char* msgBuffer, uint16_t bufferSize) {
	const Base *ptrBase = static_cast<const Base *>(ptr);
	return ptrBase->getKeyPairAsString(index, msgBuffer, bufferSize);	
}



void serial3_write(uint8_t *data, uint16_t len) {
	Serial3.write(data, len);  // output on the main serial port
}

void serial2_write(uint8_t *data, uint16_t len) {
	Serial2.write(data, len);  // output on the main serial port
}

char qCommand::checkHeartBeat(void) {
	PT_FUNC_START(pt);
	static uint8_t lastHeartbeat = 0;
	static uint8_t heartBeatFails = 0;

	while (true) {
		PT_SLEEP(pt, 1500);	 // wait 1.5 second before checking heartbeat again
		if (eui_get_host_setup()) {
			// host is connected
			if (lastHeartbeat == eui_get_heartbeat()) {
				heartBeatFails++;  // increment the heartbeat fails counter
				if (heartBeatFails > 3) {
					// if heartbeat fails more than 3 times, we are not
					// connected
					// Serial2.println("Got 3 heartbeat fails.");
					heartBeatFails = 0;
					reset();  // reset the qCommand states
				}
			} else {
				// heartBeat changed, so we are connected
				heartBeatFails = 0;	 // reset the heartbeat fails counter
				lastHeartbeat = eui_get_heartbeat();
			}
		} else {
			lastHeartbeat = eui_get_heartbeat();  // reset the heartbeat
			heartBeatFails = 0;	 // reset the heartbeat fails counter
		}
	}
	PT_FUNC_END(pt);
}

char qCommand::readBinaryInt2(void) {
	PT_FUNC_START(pt);
	// static uint8_t store[64];
	// static eui_interface_t *p_link = &serial_comms;

	static int dataReady;
	// static uint8_t count = 0;
	static int k = 0;
	dataReady = binaryStream->available();
	if (dataReady != 0) {
		for (k = 0; k < dataReady; k++) {
			uint8_t inbound_byte = binaryStream->read();
			eui_errors_t error = eui_parse(inbound_byte, &serial_comms);
			if (error.parser == eui_parse_errors::EUI_PARSER_ERROR) {
				reset();  // reset the qCommand states
			}
			PT_YIELD(pt);
		}
	}

	if (eui_get_host_setup()) {
		for (uint8_t i = 0; i < commandCount; i++) {
			if (isSmartObject(commandList[i].types.sub_types.ptr)) {
				Base *ptr = static_cast<Base *>(commandList[i].ptr.object);
				if (ptr->updates_needed ==
					Base::UpdateState::STATE_NEED_TOSEND) {
					// Serial2.printf("S%u ", i);
					send_update_on_tracked_variable(i);
					// Serial2.printf(" (%u)\n", Serial3.availableForWrite());
					ptr->updates_needed = Base::UpdateState::STATE_WAIT_ON_ACK;
				}
			}
		}
	}

	PT_RESTART(pt);
	return PT_YIELDED;

	PT_FUNC_END(pt);
}

// Function for SmartData objects
template <typename T>

void qCommand::assignVariable(const char *command, SmartData<T> *object,
							  bool read_only)
	requires(!is_option_ptr<T>::value)
{
	Types types;
	using base_type =
		typename std::conditional<std::is_reference<T>::value,
								  typename std::remove_reference<T>::type,
								  T>::type;

	types.sub_types = {type2int<base_type>::result, PTR_SD_OBJECT_DEFAULT};
	if (read_only) {
		types.sub_types.read_only = true;
	}

	uint16_t size = object->size();
	if (types.sub_types.data == 4) {
		// String subtype, max size is large
		size = 255;
	}
	addCommandInternal(command, types, object, size);
}

// Function for SmartData objects with Options
template <typename T>
void qCommand::assignVariable(const char *command, SmartData<OptionImpl<T> *> *object,
							  bool read_only) {
	Types types;
	using ValueType = typename SmartData<OptionImpl<T> *>::ValueType;
	types.sub_types = {type2int<ValueType>::result, PTR_SD_OBJECT_LIST};
	if (read_only) {
		types.sub_types.read_only = true;
	}

	uint16_t size = object->size();
	if (types.sub_types.data == 4) {
		// String subtype, max size is large
		size = 255;
	}
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
	commandList[commandCount].types.sub_types.data = 0;	 // sets as Callback

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
	commandList = (StreamCommandParserCallback *)realloc(
		commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
	commandList[commandCount].command = command;
	commandList[commandCount].types = types;
	commandList[commandCount].size = size;

	if (isSmartObject(commandList[commandCount].types.sub_types.ptr)) {
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

bool qCommand::str2Bool(const char *string) {
	bool result = false;
	const uint8_t stringLen = 10;
	char tempString[stringLen + 1];
	strncpy(tempString, string,
			stringLen);	 // make copy of argument to convert to lower case
	tempString[stringLen] =
		'\0';  // null terminate in case arg is longer than size of tempString
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

bool qCommand::reportString(qCommand &qC, Stream &S, const char *command,
							uint8_t ptr_type, char *ptr,
							StreamCommandParserCallback *CommandList) {
	bool need_to_send = false;
	// qCommand::Types Type ;
	// Type.sub_types.ptr = (qCommand::PtrType) ptr_type;
	if (isSmartObject((qCommand::PtrType)ptr_type)) {
		BaseTyped<String> *object = (BaseTyped<String> *)ptr;
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
			// Updated value and not SD_Object, so data needs update
			need_to_send = true;
		}
		S.printf("%s is %s\n", command, ptr);
	}
	return need_to_send;
}

bool qCommand::reportBool(qCommand &qC, Stream &S, const char *command,
						  Types types, bool *ptr) {
	bool temp;
	bool need_to_send = false;
	if (qC.next() != NULL) {
		temp = qC.str2Bool(qC.current());
		if (isSmartObject(types.sub_types.ptr)) {
			BaseTyped<bool> *object = (BaseTyped<bool> *)ptr;
			object->set(temp);
		} else {
			*ptr = temp;
			need_to_send = true;
		}
	}

	if (isSmartObject(types.sub_types.ptr)) {
		BaseTyped<bool> *object = (BaseTyped<bool> *)ptr;
		temp = object->get();
		if (strlen(object->getName()) != 0) {
			S.printf("%s is %s (%s)\n", command, object->getName(),
					 temp ? "true" : "false");
		} else {
			S.printf("%s is %s\n", command, temp ? "true" : "false");
		}
	} else {
		temp = *ptr;
		S.printf("%s is %s\n", command, temp ? "true" : "false");
	}

	return need_to_send;
}

template <class argUInt>
bool qCommand::reportUInt(qCommand &qC, Stream &S, const char *command,
						  Types types, argUInt *ptr) {
	bool need_to_send = false;
	unsigned long temp;
	argUInt newValue;
	if (qC.next() != NULL) {
		long temp2 = atoi(qC.current());
		if (temp2 < 0) {
			temp = 0;
		} else {
			temp = strtoul(qC.current(), NULL, 10);
			if (temp > std::numeric_limits<argUInt>::max()) {
				temp = std::numeric_limits<argUInt>::max();
			}
		}
		newValue = temp;
		if (isSmartObject(types.sub_types.ptr)) {
			BaseTyped<argUInt> *object = (BaseTyped<argUInt> *)ptr;
			object->set(newValue);
		} else {
			*ptr = newValue;
			need_to_send = true;
		}
	}

	if (isSmartObject(types.sub_types.ptr)) {
		BaseTyped<argUInt> *object = (BaseTyped<argUInt> *)ptr;
		newValue = object->get();
		if (strlen(object->getName()) != 0) {
			S.printf("%s is %s (%u)\n", command, object->getName(), newValue);
		} else {
			S.printf("%s is %u\n", command, newValue);
		}
	} else {
		newValue = *ptr;
		S.printf("%s is %u\n", command, newValue);
	}

	return need_to_send;
}

template <class argInt>
bool qCommand::reportInt(qCommand &qC, Stream &S, const char *command,
						 Types types, argInt *ptr) {
	int temp;
	bool need_to_send = false;
	if (qC.next() != NULL) {
		temp = atoi(qC.current());
		if (temp < std::numeric_limits<argInt>::min()) {
			temp = std::numeric_limits<argInt>::min();
		} else if (temp > std::numeric_limits<argInt>::max()) {
			temp = std::numeric_limits<argInt>::max();
		}

		if (isSmartObject(types.sub_types.ptr)) {
			BaseTyped<argInt> *object = (BaseTyped<argInt> *)ptr;
			object->set(temp);
		} else {
			*ptr = temp;
			need_to_send = true;
		}
	}

	if (isSmartObject(types.sub_types.ptr)) {
		BaseTyped<argInt> *object = (BaseTyped<argInt> *)ptr;
		temp = object->get();
		if (strlen(object->getName()) != 0) {
			S.printf("%s is %s (%d)\n", command, object->getName(), temp);
		} else {
			S.printf("%s is %d\n", command, temp);
		}
	} else {
		temp = *ptr;
		S.printf("%s is %d\n", command, temp);
	}

	return need_to_send;
}

template <class argFloating>
bool qCommand::reportFloat(qCommand &qC, Stream &S, const char *command,
						   Types types, argFloating *ptr) {
	bool need_to_send = false;
	argFloating newValue;
	if (qC.next() != NULL) {
		newValue = atof(qC.current());

		if (isSmartObject(types.sub_types.ptr)) {
			BaseTyped<argFloating> *object = (BaseTyped<argFloating> *)ptr;
			object->set(newValue);
		} else {
			if (sizeof(argFloating) > 4) {
				// make setting variable atomic for doubles or anything
				// greater than 32bits.
				__disable_irq();
				*ptr = newValue;
				__enable_irq();
			} else {
				*ptr = newValue;
			}
			need_to_send = true;
		}
	}

	if (isSmartObject(types.sub_types.ptr)) {
		BaseTyped<argFloating> *object = (BaseTyped<argFloating> *)ptr;
		newValue = object->get();
		if (strlen(object->getName()) != 0) {
			S.printf("%s is %s (%e)\n", command, object->getName(), newValue);
		} else {
			S.printf("%s is %f\n", command, newValue);
		}
	} else {
		newValue = *ptr;
		S.printf("%s is %f\n", command, newValue);
	}

	return need_to_send;
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

bool qCommand::reportData(qCommand &qC, Stream &inputStream,
						  const char *command, Types types, void *ptr,
						  StreamCommandParserCallback *commandList) {
	// inputStream.printf(
	//     "Command: %s and data_type is %u (ptr_type is %u at addr 0x%08x)\n",
	//     command, types.sub_types.data, types.sub_types.ptr, ptr);
	switch (types.sub_types.data) {
		case 4:
			return reportString(*this, inputStream, command,
								types.sub_types.ptr, static_cast<char *>(ptr),
								commandList);
		case 6:
			return reportUInt(*this, inputStream, command, types,
							  static_cast<uint8_t *>(ptr));
		case 8:
			return reportUInt(*this, inputStream, command, types,
							  static_cast<uint16_t *>(ptr));
		case 10:
			return reportUInt(*this, inputStream, command, types,
							  static_cast<uint32_t *>(ptr));
		case 5:
			return reportInt(*this, inputStream, command, types,
							 static_cast<int8_t *>(ptr));
		case 7:
			return reportInt(*this, inputStream, command, types,
							 static_cast<int16_t *>(ptr));
		case 9:
			return reportInt(*this, inputStream, command, types,
							 static_cast<int32_t *>(ptr));
		case 11:
			return reportFloat(*this, inputStream, command, types,
							   static_cast<float *>(ptr));
		case 12:
			return reportFloat(*this, inputStream, command, types,
							   static_cast<double *>(ptr));
		case 13:
			return reportBool(*this, inputStream, command, types,
							  static_cast<bool *>(ptr));
		default:
			inputStream.printf("Unknown data type %u\n", types.sub_types.data);
			return 0;
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
		char inChar = inputStream.read();  // Read single available character,
										   // there may be more waiting
#ifdef SERIALCOMMAND_DEBUG
		Serial.print(inChar);  // Echo back to serial stream
#endif

		if (inChar == term) {  // Check for the terminator (default '\n')
							   // meaning end of command
#ifdef SERIALCOMMAND_DEBUG
			Serial.print("Received: ");
			Serial.println(buffer);
#endif

			char *command = strtok_r(
				buffer, delim, &last);	// Search for command at start of buffer
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
					if (compareStrings(command, commandList[i].command,
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
							bool need_to_send = reportData(
								*this, inputStream, command,
								commandList[i].types, commandList[i].ptr.object,
								&commandList[i]);
							// Send update on tracked variable if its RAW_DATA
							// as SmartData will flag need to send
							if (need_to_send &&
								commandList[i].types.sub_types.ptr ==
									PTR_RAW_DATA) {
								send_update_on_tracked_variable(i);
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
				0;	// do not clear buffer to enter after command repeats it.
		} else if (inChar == '\b') {  // backspace detected
			if (bufPos > 0) {
				bufPos--;  // move back bufPos to overright previous character
				buffer[bufPos] = '\0';	// Null terminate
			}
		} else if (isprint(
					   inChar)) {  // Only printable characters into the buffer
			if (bufPos < STREAMCOMMAND_BUFFER) {
				buffer[bufPos++] = inChar;	// Put character into buffer
				buffer[bufPos] = '\0';		// Null terminate
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

#define INSTANTIATE_SMARTDATA(TYPE)                                            \
	template void qCommand::assignVariable(const char *command,                \
										   TYPE *variable, bool read_only);    \
	template void qCommand::assignVariable(const char *command,                \
										   const TYPE *variable);              \
	template void qCommand::assignVariable<TYPE>(                              \
		const char *,                                                          \
		SmartData<OptionImpl<TYPE> *, TypeTraits<OptionImpl<TYPE> *, void>::isArray ||     \
									TypeTraits<OptionImpl<TYPE> *, void>::isPointer> \
			*,                                                                 \
		bool);                                                                 \
	template void qCommand::assignVariable(                                    \
		const char *command, SmartData<TYPE> *variable, bool read_only);       \
	template void qCommand::assignVariable(                                    \
		const char *command, SmartData<TYPE *> *variable, bool read_only);
// template void qCommand::assignVariable(const char* command, SmartData<TYPE&>
// *variable, bool real_only);

INSTANTIATE_SMARTDATA(bool);
INSTANTIATE_SMARTDATA(char);

INSTANTIATE_SMARTDATA(uint8_t);
// INSTANTIATE_SMARTDATA(unsigned char);
INSTANTIATE_SMARTDATA(int8_t);
INSTANTIATE_SMARTDATA(uint16_t);
INSTANTIATE_SMARTDATA(int16_t);
INSTANTIATE_SMARTDATA(uint32_t);
INSTANTIATE_SMARTDATA(int32_t);
INSTANTIATE_SMARTDATA(uint);
INSTANTIATE_SMARTDATA(int);

INSTANTIATE_SMARTDATA(float);
INSTANTIATE_SMARTDATA(double);

// Only support Strings via SmartData
template void qCommand::assignVariable(const char *command,
									   SmartData<String> *variable,
									   bool real_only);
