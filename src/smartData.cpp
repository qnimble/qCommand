#include "smartData.h"

// #include <MsgPack.h>
#include <FastCRC.h>

// MsgPack::Packer packer;
static FastCRC16 CRC16;

#include "basic_contexts.h"
#include "cwpack.h"
#warning No need for this afger debugging
#include "quarto_wdog.h"

void cw_pack(cw_pack_context *cw, String value) {
    cw_pack_str(cw, value.c_str(), value.length());
}

void cw_pack(cw_pack_context *cw, bool value) { cw_pack_boolean(cw, value); }

/*
//No idea why I need this as it should be built from the template, but for some
reason it isn't. void cw_pack(cw_pack_context* cw, unsigned char value){
   Serial.printf("Running cw_pack with unsigned char as argument: %u\n",value);
  cw_pack_unsigned(cw,value);
}
*/

/*

template <typename argChar, std::enable_if_t<
  std::is_same<argChar, unsigned char>::value, char> = 0>
void cw_pack(cw_pack_context* cw, argChar value){
   Serial.printf("Running cw_pack_bin with start %02x\n",value);
  cw_pack_bin(cw,&value, 1);
}
*/

template <
    typename argUInt,
    std::enable_if_t<std::is_same<argUInt, unsigned char>::value ||
                         std::is_same<argUInt, uint8_t>::value ||
                         std::is_same<argUInt, uint16_t>::value ||
                         std::is_same<argUInt, short unsigned int>::value ||
                         std::is_same<argUInt, uint>::value ||
                         std::is_same<argUInt, ulong>::value,
                     uint> = 0>
void cw_pack(cw_pack_context *cw, argUInt value) {
    // Serial.printf("Running cw_pack_unsigned with %u\n",value);
    cw_pack_unsigned(cw, value);
}

template <typename argInt,
          std::enable_if_t<std::is_same<argInt, int8_t>::value ||
                               std::is_same<argInt, int16_t>::value ||
                               std::is_same<argInt, int>::value ||
                               std::is_same<argInt, long>::value,
                           int> = 0>
void cw_pack(cw_pack_context *cw, argInt value) {
    cw_pack_signed(cw, value);
}

template <typename argFloat,
          std::enable_if_t<std::is_floating_point<argFloat>::value, int> = 0>
void cw_pack(cw_pack_context *cw, argFloat value) {
    cw_pack_float(cw, value);
}

template <class DataType> void SmartData<DataType, false>::_get(void *data) {
    DataType *ptr = static_cast<DataType *>(data);
    *ptr = value;
}

template <class DataType> void SmartData<DataType, true>::_get(void *data) {
    dataRequested = true;
}

/*
template <class SmartDataGeneric, bool isArray>
struct GetHelper<SmartData<SmartDataGeneric, isArray>,true> {
  static void _get(SmartData<SmartDataGeneric,isArray>& self, void* data) {
        self.dataRequested = true;
    }
};

template <class SmartDataGeneric, bool isArray>
struct GetHelper<SmartData<SmartDataGeneric, isArray>,false> {
  static void _get(SmartData<SmartDataGeneric,isArray>& self, void* data) {
    SmartDataGeneric* ptr = static_cast<SmartDataGeneric*>(data);
    *ptr = self.value;
  }
};


template <class SmartDataGeneric, bool isArray>
void SmartData<SmartDataGeneric,isArray>::_get(void* data) {
    GetHelper<SmartDataGeneric, isArray>::_get(*this, data);
}


*/

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::_set(void *data) {
    SmartDataGeneric *ptr = static_cast<SmartDataGeneric *>(data);
    value = *ptr;

    if ((updates_needed == STATE_IDLE) ||
        (updates_needed == STATE_NEED_TOSEND)) {
        updates_needed = STATE_NEED_TOSEND;
    } else {
        updates_needed =
            STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on
                                          // ACK, so now that plus queue
    }
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, true>::_set(void *data) {
    return; // not implemented for arrays
}

void Base::resetUpdateState(void) { updates_needed = STATE_IDLE; }

/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::resetUpdateState(void) {
  updates_needed = STATE_IDLE;
}


template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::_set(void* data) {
  //set not support for array data (pointers
}

*/

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::sendValue(void) {
    Serial.printf("Sending Data (sendValue non array) for %u\n", this->id);
    setDebugWord(0x3310010);
    if (stream) {
        setDebugWord(0x3310011);
        uint16_t crc = CRC16.ccitt((uint8_t *)&value, sizeof(value));
        setDebugWord(0x3310012);
        cw_pack_array_size(pc, 4);
        cw_pack_unsigned(pc, id);
        cw_pack_unsigned(pc, 2); // command for set, maybe expose this enum
                                 // instead of hard-coding
        cw_pack(pc, value);
        cw_pack_unsigned(pc, crc);
        setDebugWord(0x3310013);
        if (pc->return_code != CWP_RC_OK) {
            Serial.printf("Error! Return Code %u\n", pc->return_code);
            return;
        }
        setDebugWord(0x3310015);
        // Serial.printf("0x%08x and 0x%08x\n", pc.start,*pc.start);
        // Serial.printf("Length %u on id=%u\n", pc.start -pc.current, id);
        // Serial.printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n",
        // pc.start, pc.current, pc.end  );
        setDebugWord(0x3310017);
        uint16_t leng = pc->current - pc->start;
        leng = min(leng, 16);
        // Serial.print("Sent packet: ");
        for (uint i = 0; i < leng; i++) {
            // Serial.printf("0x%02x ",pc.start[i]);
        }
        // Serial.println();
        stream->write(pc->start, pc->current - pc->start);
        pc->current = pc->start; // reset for next one.
        setDebugWord(0x3310019);
        // packer->clear();
        // packer->to_array(id,value, crc );
        // stream->write(packer->data(),packer->size());
    }
}

/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::please(void) {
  Serial.println("Please called on SmartDataPtr");
}

template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::_get(void* data) {
  Serial.println("_get called on SmartDataPtr");
  get();
}

template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::get(void) {
  Serial.printf("Get requested on SmartDataPtr\n");
  dataRequested = true;
}


template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::resetCurrentElement(void) {
  currentElement = 0;
  dataRequested = true;
}
*/

/*
template <typename DataType>
typename std::enable_if<TypeTraits<DataType>::isArray, void>::type
//template<typename T, typename std::enable_if<TypeTraits<T>::isArray,
int>::type> SmartData<DataType>::resetCurrentElement(void) { currentElement = 0;
  dataRequested = true;
}


*/

/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::sendIfNeedValue(void) {
  if ((dataRequested) && (currentElement == totalElements)) {
    Serial.println("Sending value on SmartDataPtr");
    sendValue();
    dataRequested = false;
    currentElement = 0;
  }
}
*/

void Base::please(void) {
    Serial.printf("Please called on SmartData with updates_needed state %u\n",
                  updates_needed);
}

/*
template <typename DataType>
typename std::enable_if<TypeTraits<DataType>::isArray, size_t>::type
 SmartData<DataType>::getCurrentElement(void) {
  return currentElement;
}
*/

/*

template <typename DataType>
typename std::enable_if<TypeTraits<DataType>::isArray, size_t>::type
 SmartData<DataType>::getTotalElements(void) {
  return totalElements;
}
*/
/*
template <typename SmartDataGeneric, typename
std::enable_if<TypeTraits<SmartDataGeneric>::isArray, int>::type = 0> size_t
SmartData<SmartDataGeneric>::getCurrentElement(void) { return currentElement;
}


template <typename SmartDataGeneric, typename
std::enable_if<TypeTraits<SmartDataGeneric>::isArray, int>::type = 0> size_t
SmartData<SmartDataGeneric>::getTotalElements(void) { return totalElements;
}

*/

/*
template <class SmartDataGeneric>
size_t SmartDataPtr<SmartDataGeneric>::getCurrentElement(void) {
  return currentElement;
}

template <class SmartDataGeneric>
size_t SmartDataPtr<SmartDataGeneric>::getTotalElements(void) {
  return totalElements;
}
*/

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, true>::sendValue(void) {
    Serial.printf("Sending Data (sendValue array) for %u\n", this->id);
    if (stream) {
        uint16_t crc = CRC16.ccitt((uint8_t *)value,
                                   totalElements * sizeof(SmartDataGeneric));
        cw_pack_array_size(pc, 4);
        cw_pack_unsigned(pc, id);
        cw_pack_unsigned(pc, 2); // command for set, maybe expose this enum
                                 // instead of hard-coding
        // Serial.printf("Check 1: Return Code %d\n",pc->return_code);
        // Serial.printf("Will run cw_pack_bin with size of %u\n", totalElements
        // * sizeof(SmartDataGeneric));
        cw_pack_bin(pc, value, totalElements * sizeof(SmartDataGeneric));
        // Serial.printf("Check 2: Return Code %d\n",pc->return_code);
        cw_pack_unsigned(pc, crc);
        if (pc->return_code != CWP_RC_OK) {
            Serial.printf("Error! Return Code %ld\n", pc->return_code);
            return;
        }
        // uint16_t leng = pc.current - pc.start;
        // leng = min(leng,16);
        // Serial.print("Sent packet: ");
        // for (uint i = 0; i < leng; i++) {
        // Serial.printf("0x%02x ",pc.start[i]);
        //}
        // Serial.println();
        stream->write(pc->start, pc->current - pc->start);
        pc->current = pc->start; // reset for next one.

        // #warning maybe do not want to reset pointer send we send...
        // currentElement = 0;
        // dataRequested = false;
        // setDebugWord(0x3310019);
        // packer->clear();
        // packer->to_array(id,value, crc );
        // stream->write(packer->data(),packer->size());
    }
}
/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::setNeedToSend(void) {
   if ((updates_needed == STATE_IDLE) || (updates_needed == STATE_NEED_TOSEND))
{ updates_needed = STATE_NEED_TOSEND; } else { updates_needed =
STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on ACK, so now
that plus queue
  }
}
*/
void Base::setNeedToSend(void) {
    // Serial.printf("New update and state is %u\n",this->pc, updates_needed);
    if ((updates_needed == STATE_IDLE) ||
        (updates_needed == STATE_NEED_TOSEND)) {
        updates_needed = STATE_NEED_TOSEND;
    } else {
        updates_needed =
            STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on
                                          // ACK, so now that plus queue
    }
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::set(SmartDataGeneric newValue) {
    setDebugWord(0x2210010);
    value = newValue;
    // value = 1;
    setDebugWord(0x2210012);
    setNeedToSend();
    // sendValue();
    setDebugWord(0x2210020);
}

// template <class DataType>
// template <class DataType, typename
// std::enable_if<TypeTraits<DataType>::isArray, int>::type = 0>
template <class DataType>
void SmartData<DataType, true>::setNext(
    typename SmartData<DataType, true>::baseType data) {
    value = 'asdf';//just to learn
    if (dataRequested) {
        if (currentElement < totalElements) {
            value[currentElement] = data;
            currentElement++;
            if (currentElement == totalElements) {
                // if this was last element, then
                setNeedToSend();
            }
        }
    }
}
/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::setNext(typename
SmartDataPtr<SmartDataGeneric>::baseType data) { if (dataRequested) { if
(currentElement < totalElements ) { value[currentElement] = data;
      currentElement++;
      if (currentElement == totalElements) {
        //if this was last element, then
        setNeedToSend();
      }
    }
  }
}
*/

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, true>::_setPrivateInfo(uint8_t id,
                                                        Stream *stream,
                                                        cw_pack_context *pc) {
    this->id = id;
    this->stream = stream;
    this->pc = pc;
    init_dynamic_memory_pack_context(pc, DEFAULT_PACK_BUFFER_SIZE);
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::_setPrivateInfo(uint8_t id,
                                                         Stream *stream,
                                                         cw_pack_context *pc) {
    this->id = id;
    this->stream = stream;
    this->pc = pc;
    init_dynamic_memory_pack_context(pc, DEFAULT_PACK_BUFFER_SIZE);
}

/*
template <class SmartDataGeneric>
void SmartDataPtr<SmartDataGeneric>::_setPrivateInfo(uint8_t id, Stream* stream,
cw_pack_context* pc) { this->id = id; this->stream = stream; this->pc = pc;
  init_dynamic_memory_pack_context(pc, DEFAULT_PACK_BUFFER_SIZE);
}
*/
// template <class SmartDataGeneric>
// DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>()
// {}

/*
template <class SmartDataGeneric>
template <typename T, typename std::enable_if<!TypeTraits<T>::is_array,
T>::type*> SmartData<SmartDataGeneric>::SmartData(T initValue):
value(initValue), id(0), stream(0) {
  //init_dynamic_memory_pack_context(&pc, DEFAULT_PACK_BUFFER_SIZE);
}

template <class SmartDataGeneric>
template <typename T, typename std::enable_if<TypeTraits<T>::is_array,
T>::type*> SmartData<SmartDataGeneric>::SmartData(SmartDataGeneric initValue,
size_t size): value(initValue), totalElements(size), id(0), stream(0) {
  //init_dynamic_memory_pack_context(&pc, DEFAULT_PACK_BUFFER_SIZE);
}
*/

/*
template <class SmartDataGeneric>
SmartDataPtr<SmartDataGeneric>::SmartDataPtr(SmartDataGeneric initValue,
unsigned int size) :  value(initValue),totalElements(size), id(0), stream(0) {
  //init_dynamic_memory_pack_context(&pc, DEFAULT_PACK_BUFFER_SIZE);
}
*/

template class SmartData<bool>;
template class SmartData<bool *>;

template class SmartData<char>;
template class SmartData<char *>;

template class SmartData<uint8_t>;
template class SmartData<uint8_t *>;

template class SmartData<uint16_t>;
template class SmartData<uint16_t *>;
template class SmartData<uint16_t (&)[50]>;
template class SmartData<uint16_t (&),true>;

template class SmartData<uint>;
template class SmartData<uint *>;

template class SmartData<ulong>;
template class SmartData<ulong *>;

template class SmartData<int8_t>;
template class SmartData<int8_t *>;

template class SmartData<int16_t>;
template class SmartData<int16_t *>;

template class SmartData<int>;
template class SmartData<int *>;

template class SmartData<long>;
template class SmartData<long *>;

template class SmartData<float>;
template class SmartData<float *>;

template class SmartData<double>;
template class SmartData<double *>;

template class SmartData<String>;

// template void cw_pack<uint8_t>(cw_pack_context*, uint8_t);
// template void cw_pack<unsigned char>(cw_pack_context*, unsigned char);
// template bool DataObjectSpecific<bool>::get(void);
// template float DataObjectSpecific<float>::get(void);
// template void cw_pack<unsigned char>(cw_pack_context*, unsigned char);
// template void SmartData<unsigned char>::cw_pack(cw_pack_context*, unsigned
// char);

void uglytest(cw_pack_context *cw) {
    cw_pack<unsigned char>(cw, (unsigned char)50);
}

void uglytest2(cw_pack_context *cw) { cw_pack<uint16_t>(cw, (uint16_t)50); }
