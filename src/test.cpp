#include <iostream>
#include "test.h"

#include "FastCRC.h"

static cw_pack_context pc;
static FastCRC16 CRC16;

extern char buffer[];

template <typename Arg>
void test(Arg value) {
    Serial.printf("Hello, value is %u\n",value);
}


template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, unsigned char>::value ||
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value || 
  std::is_same<argUInt, short unsigned int>::value || 
  std::is_same<argUInt, uint>::value || 
  std::is_same<argUInt, ulong>::value
  , uint> = 0>    
void test_pack(cw_pack_context* cw, argUInt value){
  //Serial.printf("Running test_pack_unsigned with %u\n",value);
  cw_pack_unsigned(cw,value);
}

template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value || 
  std::is_same<argInt, int16_t>::value || 
  std::is_same<argInt, int>::value ||  
  std::is_same<argInt, long>::value
  , int> = 0>
void test_pack(cw_pack_context* cw, argInt value){
  cw_pack_signed(cw,value);
}

// Version for float
template <typename argFloat, std::enable_if_t<
    std::is_same<argFloat, float>::value, int> = 0>
void test_pack(cw_pack_context* cw, argFloat value) {
    cw_pack_float(cw,value);
}

// Version for double
template <typename argFloat, std::enable_if_t<
    std::is_same<argFloat, double>::value, int> = 0>
void test_pack(cw_pack_context* cw, argFloat value) {
    cw_pack_double(cw,value);
}




void test_pack(cw_pack_context* cw, bool value){
    Serial.printf("Running test_pack_booleand with %u\n",value);
    cw_pack_boolean(cw,value);
}


template <class SmartDataGeneric>
SmartDataGeneric testData<SmartDataGeneric>::get(void) {  
  return value;
}

template <class SmartDataGeneric>
void testData<SmartDataGeneric>::_get(void* data) {  
  SmartDataGeneric* ptr = static_cast<SmartDataGeneric*>(data);
  *ptr = value;
}

template <class SmartDataGeneric>
void testData<SmartDataGeneric>::_set(void* data) {  
  SmartDataGeneric* ptr = static_cast<SmartDataGeneric*>(data);
  value = *ptr;
}



template <class SmartDataGeneric>
testData<SmartDataGeneric>::testData(SmartDataGeneric initValue): value(initValue) {
  Serial.printf("Initializing testData with %lu\n",value);
  cw_pack_context_init(&pc, buffer, DEFAULT_PACK_BUFFER_SIZE, 0);  
}


template <class SmartDataGeneric>
void testData<SmartDataGeneric>::sendValue(void) {      
  SRC_GPR5 = 0xcccca117;  
  if (stream) {        
    uint16_t crc = CRC16.ccitt((uint8_t*) &value, sizeof(value));
    cw_pack_array_size(&pc,4);
    cw_pack_unsigned(&pc, id);
    cw_pack_unsigned(&pc, 2); //command for set, maybe expose this enum instead of hard-coding
    test_pack(&pc, value);
    cw_pack_unsigned(&pc, crc);
    if (pc.return_code != CWP_RC_OK) {
      Serial.printf("Error! Return Code %u\n",pc.return_code);
      return;    
    }
    uint16_t leng = pc.current - pc.start;
    leng = min(leng,16);
    Serial.print("Sent packet: ");
    for (uint i = 0; i < leng; i++) {
      Serial.printf("0x%02x ",pc.start[i]);
    }
    Serial.println();
    //#warning this should be uncommented
    stream->write(pc.start, pc.current - pc.start);
    pc.current = pc.start; //reset for next one.
    
  }
  
}



template <class SmartDataGeneric>
void testData<SmartDataGeneric>::_setPrivateInfo(uint8_t id, Stream* stream, void* packer) {
  this->id = id;
  this->stream = stream;
  this->packer = packer;
}



template <class SmartDataGeneric>
void testData<SmartDataGeneric>::set(SmartDataGeneric newValue) {  
  SRC_GPR5 = 0xcccca111;
  value = newValue;  
  SRC_GPR5 = 0xcccca112;
  sendValue();  
  SRC_GPR5 = 0xcccca113;
}



template class testData<uint8_t>;
template class testData<uint16_t>;
template class testData<uint32_t>;
template class testData<int8_t>;
template class testData<int16_t>;
template class testData<int32_t>;
template class testData<unsigned int>;
template class testData<bool>;
template class testData<float>;
template class testData<double>;
/*

template void test_pack<uint8_t>(uint8_t);
template void test_pack<uint16_t>(uint16_t);
template void test_pack<uint32_t>(uint32_t);

template void test<uint8_t>(uint8_t);
template void test<uint16_t>(uint16_t);
template void test<uint32_t>(uint32_t);
*/