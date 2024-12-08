#include "smartData.h"

void Base::resetUpdateState(void) { updates_needed = STATE_IDLE; }

/*
//_get for non-array data
template <class DataType> void SmartData<DataType, false>::_get(void *data) {
    DataType *ptr = static_cast<DataType *>(data);
    *ptr = value;
}

void AllSmartDataPtr::_get(void *data, size_t element) {
    dataRequested = true;    
}
*/
/*
template <class DataType> void SmartData<DataType, true>::_get(void *data) {
    dataRequested = true;
}
*/
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
/*
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



void AllSmartDataPtr::_set(void *data, size_t element) {
    data = &value[element];
    return; 
}

*/
/*
template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, true>::_set(void *data) {
    return; // not implemented for arrays
}
*/


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
/*
template <class DataType>
void SmartData<DataType, false>::sendValue2(void) {
    Serial2.printf("Sending Data (sendValue non array) for %u\n", this->id);
    delayMicroseconds(100000);
    setDebugWord(0x3310010);
    if (stream) {
        
    }
}
*/
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

//template <class SmartDataGeneric>
//void SmartData<SmartDataGeneric, true>::sendValue(void) {
/*
void AllSmartDataPtr::sendValue2() {
    Serial.printf("Sending Data (sendValue array) for %u\n", this->id);
    delayMicroseconds(100000);
    if (stream) {

        // #warning maybe do not want to reset pointer send we send...
        // currentElement = 0;
        // dataRequested = false;
        // setDebugWord(0x3310019);
        // packer->clear();
        // packer->to_array(id,value, crc );
        // stream->write(packer->data(),packer->size());
    }
}
*/
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
    Serial.printf("New update for object at 0x%08x with size %u and state was %u / %u ",this, size(), updates_needed, getUpdateState());
    if ((updates_needed == STATE_IDLE) || (updates_needed == STATE_NEED_TOSEND)) {
        updates_needed = STATE_NEED_TOSEND;
    } else {
        updates_needed =
            STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on
                                          // ACK, so now that plus queue
    }
    Serial.printf(" and now is %u\n",updates_needed);
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
                                                        Stream *stream
                                                        ) {
    this->id = id;
    this->stream = stream;
    //this->pc = pc;
    //init_dynamic_memory_pack_context(pc, DEFAULT_PACK_BUFFER_SIZE);
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::_setPrivateInfo(uint8_t id,
                                                         Stream *stream
                                                         ) {
    this->id = id;
    this->stream = stream;
    //this->pc = pc;
    //init_dynamic_memory_pack_context(pc, DEFAULT_PACK_BUFFER_SIZE);
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
template class SmartData<float (&)[50]>;
template class SmartData<float (&),true>;




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

//void uglytest(cw_pack_context *cw) {
//    cw_pack<unsigned char>(cw, (unsigned char)50);
//}

//void uglytest2(cw_pack_context *cw) { cw_pack<uint16_t>(cw, (uint16_t)50); }
