#ifndef TESTDATA_h
#define TESTDATA_h

#include "cwpack.h"
#include <Stream.h>



#define DEFAULT_PACK_BUFFER_SIZE 500


#define TYPE2INFO_ARRAY (0)
#define TYPE2INFO_MIN (1)
#define TYPE2INFO_2MIN (2)
#define TYPE2INFO_4MIN (3)
#define TYPE2INFO_BOOL  (0<<2)
#define TYPE2INFO_FLOAT  (1<<2)
#define TYPE2INFO_UINT  (2<<2)
#define TYPE2INFO_INT  (3<<2)





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



class Base {  
  public:
    virtual void sendValue(void);
    virtual void _get(void* data); // pure virtual function
    virtual void _set(void* data ); // pure virtual function
  private:
    
    //virtual void* _get(void) = 0; // pure virtual function

};



template <class DataType>
class testData: public Base{
    public:
        testData(DataType);        
        DataType get(void);
        void set(DataType);
        void sendValue(void);
        void _get(void* data); 
        void _set(void* data); 

    private:
        void _setPrivateInfo(uint8_t id, Stream* stream, void* packer);
        DataType value;
        Stream* stream;
        uint8_t id;
        void* packer;
    friend class qCommand;
};


template< typename T >
struct type2int
{
   // enum { result = 0 }; // do this if you want a fallback value, empty to force a definition
};


template<> struct type2int<testData<bool>> { enum { result = TYPE2INFO_MIN +  TYPE2INFO_BOOL }; };
template<> struct type2int<testData<uint8_t>> { enum { result =  TYPE2INFO_MIN + TYPE2INFO_UINT}; };
template<> struct type2int<testData<uint16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_UINT }; };
template<> struct type2int<testData<uint>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_UINT }; };
template<> struct type2int<testData<ulong>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_UINT }; };
template<> struct type2int<testData<int8_t>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_INT }; };
template<> struct type2int<testData<int16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_INT }; };
template<> struct type2int<testData<int>> { enum { result = TYPE2INFO_4MIN +  TYPE2INFO_INT }; };
template<> struct type2int<testData<long>> { enum { result = TYPE2INFO_4MIN +  TYPE2INFO_INT }; };
template<> struct type2int<testData<float>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_FLOAT }; };
template<> struct type2int<testData<double>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_FLOAT }; };
    


#endif // TESTDATA_h