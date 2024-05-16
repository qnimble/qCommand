#ifndef TESTDATA_h
#define TESTDATA_h

#include "cwpack.h"
#include <Stream.h>

template <typename Arg>
void test(Arg value);

void test_pack(cw_pack_context* cw, bool value);

template <typename argUInt, std::enable_if_t<
  std::is_same<argUInt, unsigned char>::value ||
  std::is_same<argUInt, uint8_t>::value ||
  std::is_same<argUInt, uint16_t>::value || 
  std::is_same<argUInt, short unsigned int>::value || 
  std::is_same<argUInt, uint>::value || 
  std::is_same<argUInt, ulong>::value
  , uint> = 0>    
void cw_pack(cw_pack_context* cw, argUInt value);

template <typename argInt, std::enable_if_t<
  std::is_same<argInt,int8_t>::value || 
  std::is_same<argInt, int16_t>::value || 
  std::is_same<argInt, int>::value ||  
  std::is_same<argInt, long>::value
  , int> = 0>
void test_pack(cw_pack_context* cw, argInt value);

template <typename argFloat, std::enable_if_t<
    std::is_same<argFloat, float>::value, int> = 0>
void test_pack(cw_pack_context* cw, argFloat value) ;

template <typename argFloat, std::enable_if_t<
    std::is_same<argFloat, double>::value, int> = 0>
void test_pack(cw_pack_context* cw, argFloat value);


template <class DataType>
class testData {
    public:
        testData(DataType);        
        DataType get(void);
        void set(DataType);
        void sendValue(void);
        void _get(void* data); 
        void _set(void* data); 

    private:
        DataType value;
        Stream* stream;
        uint8_t id;
};


#endif // TESTDATA_h