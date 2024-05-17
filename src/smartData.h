#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>
//#include <MsgPack.h>
#include "cwpack.h"


// Maximum length of a command excluding the terminating null

//First 2 bits are the type size: array, 1bytes, 2 bytes or 4bytes. When type is float, the bytes are 4x, so array, 4bytes, 8bytes, 16bytes
//next 3 bits are the type: bool, float, uint, int, string
//last bit (7) is a read-only flag. 0 is read/write, 1 is read only
// bits 5-6 are the TBD.
#define TYPE2INFO_ARRAY (0)
#define TYPE2INFO_MIN (1)
#define TYPE2INFO_2MIN (2)
#define TYPE2INFO_4MIN (3)
#define TYPE2INFO_BOOL  (0<<2)
#define TYPE2INFO_FLOAT  (1<<2)
#define TYPE2INFO_UINT  (2<<2)
#define TYPE2INFO_INT  (3<<2)
#define TYPE2INFO_STRING (4<<2)

extern cw_pack_context pc;
extern char buffer[];
#define DEFAULT_PACK_BUFFER_SIZE 500

void cw_pack(cw_pack_context* cw, bool value);
void cw_pack(cw_pack_context* cw, unsigned char value);


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
void cw_pack(cw_pack_context* cw, argInt value);

template <typename argFloat, std::enable_if_t<      
  std::is_floating_point<argFloat>::value
  , int> = 0>        
void cw_pack(cw_pack_context* cw, argFloat value);

#warning move these to private when done with debug
class Base {  
  public:
    virtual void _get(void* data); // pure virtual function
    virtual void _set(void* data ); // pure virtual function
    virtual void sendValue(void);
  private:
    
    
    //virtual void* _get(void) = 0; // pure virtual function

};



template <class DataType>
class SmartData: public Base  {
//  class SmartData {
public:
    SmartData(DataType);
    DataType get(void);
    void set(DataType);
    bool please(void);
    void sendValue(void);
    void _get(void* data); 
    void _set(void* data); 

private:
    DataType value;
    void* packer;
    void _setPrivateInfo(uint8_t id, Stream* stream, void* packer);
    //void _set(void* value) override;
    //void* _get(void) override;
    friend class qCommand;
    
    //private data that gets set by qC::addCommand
    uint8_t id;
    Stream* stream;  
};

template< typename T >
struct type2int
{
   // enum { result = 0 }; // do this if you want a fallback value, empty to force a definition
};

template<> struct type2int<SmartData<String>> { enum { result = TYPE2INFO_MIN +  TYPE2INFO_STRING }; };
template<> struct type2int<SmartData<bool>> { enum { result = TYPE2INFO_MIN +  TYPE2INFO_BOOL }; };
template<> struct type2int<SmartData<uint8_t>> { enum { result =  TYPE2INFO_MIN + TYPE2INFO_UINT}; };
template<> struct type2int<SmartData<uint16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<uint>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<ulong>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<int8_t>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int>> { enum { result = TYPE2INFO_4MIN +  TYPE2INFO_INT }; };
template<> struct type2int<SmartData<long>> { enum { result = TYPE2INFO_4MIN +  TYPE2INFO_INT }; };
template<> struct type2int<SmartData<float>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_FLOAT }; };
template<> struct type2int<SmartData<double>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_FLOAT }; };
    

/*
struct DataInfo {
    char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
    uint8_t dataType;
    DataObject* ptr;
};                                    // Data structure to hold Command/Handler function key-value pairs
*/



#endif //SMARTDATA_h
