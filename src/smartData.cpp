#include "smartData.h"

//#include <MsgPack.h>
#include <FastCRC.h>


//MsgPack::Packer packer;
FastCRC16 CRC16;

#include "cwpack.h"




void cw_pack(cw_pack_context* cw, bool value){
	cw_pack_boolean(cw,value);
}

void cw_pack(cw_pack_context* cw, unsigned char value){
	cw_pack_bin(cw,&value, 1);
}


template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value || 
  std::is_same<argUInt, uint>::value || 
  std::is_same<argUInt, ulong>::value
  , uint> = 0>    
void cw_pack(cw_pack_context* cw, argUInt value){
  cw_pack_unsigned(cw,value);
}

template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value || 
  std::is_same<argInt, int16_t>::value || 
  std::is_same<argInt, int>::value ||  
  std::is_same<argInt, long>::value
  , int> = 0>
void cw_pack(cw_pack_context* cw, argInt value){
  cw_pack_signed(cw,value);
}

template <typename argFloat, std::enable_if_t<      
  std::is_floating_point<argFloat>::value
  , int> = 0>        
void cw_pack(cw_pack_context* cw, argFloat value) {
  cw_pack_float(cw,value);
}



template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::_set(void* value) {
  SmartDataGeneric* typedValue = static_cast<SmartDataGeneric*>(value);
  set(*typedValue);  
}


template <class SmartDataGeneric>
void* SmartData<SmartDataGeneric>::_get(void) {
  setDebugWord(0x00344488);
  return &value;
}



template <class SmartDataGeneric>
SmartDataGeneric SmartData<SmartDataGeneric>::get(void) {
  setDebugWord(0x12344488);
  return value;
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::sendValue(void) {
  setDebugWord(0x3310010);
  if (stream) {    
    setDebugWord(0x3310011);
    //uint16_t crc = CRC16.ccitt((uint8_t*) &value, sizeof(value));    
    uint16_t crc = 12;
        setDebugWord(0x3310012);
    cw_pack_array_size(&pc,4);
    cw_pack_unsigned(&pc, id);
    cw_pack_unsigned(&pc, 2); //command for set, maybe expose this enum instead of hard-coding
    cw_pack(&pc, value);
    cw_pack_unsigned(&pc, crc);
    setDebugWord(0x3310013);
    if (pc.return_code != CWP_RC_OK) {
      Serial.printf("Error! Return Code %u\n",pc.return_code);
      return;
    } else {
      Serial.printf("Good! Return Code %u\n",pc.return_code);
    }
    setDebugWord(0x3310015);
    Serial.printf("0x%08x and 0x%08x\n", pc.start,*pc.start);
    Serial.printf("Length %u on id=%u\n", pc.start -pc.current, id);
    Serial.printf("Start: 0x%08x -> Stop 0x%08x -> Max 0x%08x\n", pc.start, pc.current, pc.end  );
    setDebugWord(0x3310017);
    stream->write(pc.start, pc.current - pc.start);
    pc.current = pc.start; //reset for next one.
    setDebugWord(0x3310019);
    //packer->clear();    
    //packer->to_array(id,value, crc );
    //stream->write(packer->data(),packer->size());
  }
}


template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::set(SmartDataGeneric newValue) {
  setDebugWord(0x2210010);
  //value = newValue;  
  value = 1;
  setDebugWord(0x2210012);
  sendValue();  
  setDebugWord(0x2210020);
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::_setPrivateInfo(uint8_t id, Stream* stream, void* packer) {
  this->id = id;
  this->stream = stream;
  this->packer = packer;
}

//template <class SmartDataGeneric>
//DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>() {}

template <class SmartDataGeneric>
SmartData<SmartDataGeneric>::SmartData(SmartDataGeneric initValue): value(initValue), id(0), stream(0) {
  cw_pack_context_init(&pc, buffer, DEFAULT_PACK_BUFFER_SIZE, 0);  
}


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
