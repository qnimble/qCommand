#include "smartData.h"

#include <MsgPack.h>
#include <FastCRC.h>

//MsgPack::Packer packer;
FastCRC16 CRC16;

template <class SmartDataGeneric>
SmartDataGeneric SmartData<SmartDataGeneric>::get(void) {
  return value;
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::set(SmartDataGeneric newValue) {
  value = newValue;  
  if (stream) {    
    packer->clear();  
    uint16_t crc = CRC16.ccitt((uint8_t*) &value, sizeof(value));    
    packer->to_array(id,value, crc );
    stream->write(packer->data(),packer->size());

  }
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::_setPrivateInfo(uint8_t id, Stream* stream, MsgPack::Packer* packer) {
  this->id = id;
  this->stream = stream;
  this->packer = packer;
}

//template <class SmartDataGeneric>
//DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>() {}

template <class SmartDataGeneric>
SmartData<SmartDataGeneric>::SmartData(SmartDataGeneric initValue): value(initValue), id(0), stream(0) {}


template class SmartData<bool>;
template class SmartData<uint8_t>;
template class SmartData<uint16_t>;
template class SmartData<uint>;
template class SmartData<ulong>;
template class SmartData<int8_t>;
template class SmartData<int16_t>;
template class SmartData<int>;
template class SmartData<long>;
template class SmartData<float>;
template class SmartData<double>;


//template bool DataObjectSpecific<bool>::get(void);
//template float DataObjectSpecific<float>::get(void);
