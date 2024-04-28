#include "smartData.h"

#include <MsgPack.h>
#include <FastCRC.h>

MsgPack::Packer packer;
FastCRC16 CRC16;

template <class SmartDataGeneric>
SmartDataGeneric SmartData<SmartDataGeneric>::get(void) {
  return value;
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::set(SmartDataGeneric newValue) {
  value = newValue;

  if (stream) {
    //MsgPack::arr_t<int> test {1, 2, 6}; // std::vector or arx::stdx::vector
    packer.clear();  
    
    //packer.serialize(test);    
    uint16_t crc = CRC16.ccitt((uint8_t*) &value, sizeof(value));
    //SmartDataGeneric xorBase = (SmartDataGeneric) 0xFFFFFFFF;
    //xorBase = value ^ xorBase;
    packer.to_array(id,value, crc );
    //Serial2.write(packer.data(),packer.size());
  //const auto& packet = MsgPacketizer::encode(1, counts);
    stream->write(packer.data(),packer.size());
    Serial.printf("Wrote packet to S2 of size: %d to 0x%08x (expect 0x%08x)\n", packer.size(),stream, &Serial2);
  
  }

}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::_setId(uint8_t id) {
  id = id;
}

//template <class SmartDataGeneric>
//DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>() {}

template <class SmartDataGeneric>
SmartData<SmartDataGeneric>::SmartData(SmartDataGeneric initValue): value(initValue), stream(&Serial2) {}



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
