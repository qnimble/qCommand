#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>
//#include <MsgPack.h>
#include "cwpack.h"



// First bit (0) is the array flag. 0 is not an array, 1 is an array
// Bits 1 and 2 are the type size: 1bytes, 2 bytes, 4bytes or 8 bytes.
// Bits 3 - 5  set the type: bool, float, uint, int, string, and future options
// Bit 6 is the read-only flag. 0 is read/write, 1 is read only
// last bit (7) is TBD / future-proofing.


#define TYPE2INFO_ARRAY (1)
#define TYPE2INFO_1BYTE (0<<1)
#define TYPE2INFO_2BYTE (1<<1)
#define TYPE2INFO_4BYTE (2<<1)
#define TYPE2INFO_8BYTE (3<<1)
#define TYPE2INFO_BOOL  (0<<3)
#define TYPE2INFO_FLOAT (1<<3)
#define TYPE2INFO_UINT  (2<<3)
#define TYPE2INFO_INT  (3<<3)
#define TYPE2INFO_STRING (4<<3)
#define TYPE2INFO_READONLY (1<<6)

//extern cw_pack_context pc;
extern char buffer[];
#define DEFAULT_PACK_BUFFER_SIZE 500

void cw_pack(cw_pack_context* cw, bool value);
//void cw_pack(cw_pack_context* cw, unsigned char value);
void cw_pack(cw_pack_context* cw, String value);

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

enum UpdateState {
    STATE_IDLE,   // A
    STATE_NEED_TOSEND,   // B
    STATE_WAIT_ON_ACK, // C
    STATE_WAIT_ON_ACK_PLUS_QUEUE  // BC
};

template<typename T, typename Enable = void>
struct TypeTraits {
    static constexpr bool isArray = false;
};

template<typename T>
struct TypeTraits<T, std::enable_if_t<std::is_pointer<T>::value>>  {
    static constexpr bool isArray = true;
};


static_assert(!TypeTraits<int>::isArray, "int should not be considered an array");
static_assert(TypeTraits<float*>::isArray, "float* should be considered an array");
static_assert(TypeTraits<double*>::isArray, "double* should be considered an array");


#warning move these to private when done with debug
class Base {  
  public:
    //virtual void _get(void* data); // pure virtual function
    virtual void _set(void* data); // pure virtual function
    void setNeedToSend(void); // set states as if set ran, even though it didn't.
    virtual void sendValue(void);
    void please();
    virtual void _get(void* data); // pure virtual function
    void resetUpdateState(void);
  protected:
    cw_pack_context* pc;
    UpdateState updates_needed = STATE_IDLE;    //virtual void* _get(void) = 0; // pure virtual function    
    friend class qCommand;    
};

template<typename DataType, bool Enable>
class OnlyForArrays {
  public:
    void resetCurrentElement(void);
};

template<typename DataType>
class OnlyForArrays<DataType, true> {
  public:    
    
    using baseType = typename std::remove_pointer<DataType>::type;
    void setNext(baseType);
  protected:
    //OnlyForArrays<DataType, true>(size_t totalElements) : totalElements(totalElements) {}
    
};

template <class SmartDataGeneric, bool HackIsArray>
struct GetHelper;


//template <class DataType>
template <class DataType, bool isArray = TypeTraits<DataType>::isArray>
class SmartData: public Base {
public:    
    SmartData(DataType data);    
    DataType get(void);
};


//For arrays!
template <class DataType>
class SmartData<DataType, true>: public Base {    
  public:    
    SmartData(DataType data, size_t size): value(data), totalElements(size), id(0), stream(0) {};    
    using baseType = typename std::remove_pointer<DataType>::type;
    baseType get(void);
    void set(DataType);
    void please(void);
    void sendValue(void);
    void _get(void* data); 
    void _set(void* data);     
    //void setNeedToSend(void);
    void resetUpdateState(void);
        
    void setNext(baseType);
    size_t getTotalElements(void) {
      return totalElements;
    };
    
    size_t getCurrentElement(void) {
       return currentElement;
    };
    
    void resetCurrentElement(void) {
      currentElement = 0;
      dataRequested = true;
    };

private:
    DataType value;
    void _setPrivateInfo(uint8_t id, Stream* stream, cw_pack_context* pc);
    cw_pack_context* pc;

    const size_t totalElements;  
    size_t currentElement;
    bool dataRequested = false;
    

    friend class qCommand;
    
    //private data that gets set by qC::addCommand
    uint8_t id;
    Stream* stream;  
};

//For non-arrays
template <class DataType>
class SmartData<DataType, false>: public Base {    
  public:    
    SmartData(DataType data): value(data), id(0), stream(0) {};    
    DataType get(void);
    void set(DataType);
    void please(void);
    void sendValue(void);
    void _get(void* data); 
    void _set(void* data);     
    //void setNeedToSend(void);
    void resetUpdateState(void);

private:
    DataType value;
    void _setPrivateInfo(uint8_t id, Stream* stream, cw_pack_context* pc);
    cw_pack_context* pc;
    
    bool dataRequested = false;
    
    friend class qCommand;
    
    //private data that gets set by qC::addCommand
    uint8_t id;
    Stream* stream;        
};



class AllSmartDataPtr: public Base {
  public:
    virtual size_t getCurrentElement(void);
    virtual size_t getTotalElements(void);
    virtual void sendIfNeedValue(void);
    virtual void setNeedToSend(void);
    virtual void resetCurrentElement(void);    
};


template <class DataType>
class SmartDataPtr: public AllSmartDataPtr  {
//  class SmartData {
public:
    SmartDataPtr(DataType, size_t);
    DataType get(size_t);
    void get(void);
    void set(DataType, size_t);
    void set(DataType*, unsigned int);    
    void sendValue(void);
    void _set(void* data);
    void please(void);
    void sendIfNeedValue(void);
    void _get(void* data);
    void setNeedToSend(void);
    using baseType = typename std::remove_pointer<DataType>::type;

    void setNext(baseType);
    size_t getCurrentElement(void);
    size_t getTotalElements(void);
    void resetCurrentElement(void);
    void resetUpdateState(void);

private:
    DataType value;
    const size_t totalElements;
    size_t currentElement;
    bool dataRequested = false;
    cw_pack_context* pc;
    void _setPrivateInfo(uint8_t id, Stream* stream, cw_pack_context* pc);

    //void _set(void* value) override;
    //void* _get(void) override;
    friend class qCommand;
    
    //private data that gets set by qC::addCommand
    uint8_t id;
    Stream* stream;  
};

//template <class DataType>
//using SmartDataNew = typename std::conditional<std::is_pointer<DataType>::value, SmartDataPtr<DataType>, SmartData<DataType>>::type;


template< typename T >
struct type2int
{
   // enum { result = 0 }; // do this if you want a fallback value, empty to force a definition
};

template<> struct type2int<SmartData<String>> { enum { result = TYPE2INFO_1BYTE +  TYPE2INFO_STRING }; };
template<> struct type2int<SmartData<bool>> { enum { result = TYPE2INFO_1BYTE +  TYPE2INFO_BOOL }; };
template<> struct type2int<SmartData<uint8_t>> { enum { result =  TYPE2INFO_1BYTE + TYPE2INFO_UINT}; };
template<> struct type2int<SmartData<uint16_t>> { enum { result = TYPE2INFO_2BYTE + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<uint>> { enum { result = TYPE2INFO_4BYTE + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<ulong>> { enum { result = TYPE2INFO_4BYTE + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<int8_t>> { enum { result = TYPE2INFO_1BYTE + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int16_t>> { enum { result = TYPE2INFO_2BYTE + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int>> { enum { result = TYPE2INFO_4BYTE +  TYPE2INFO_INT }; };
template<> struct type2int<SmartData<long>> { enum { result = TYPE2INFO_4BYTE +  TYPE2INFO_INT }; };
template<> struct type2int<SmartData<float>> { enum { result = TYPE2INFO_4BYTE + TYPE2INFO_FLOAT }; };
template<> struct type2int<SmartData<double>> { enum { result = TYPE2INFO_8BYTE + TYPE2INFO_FLOAT }; };
    
//template<> struct type2int<SmartDataPtr<float*>> { enum { result = TYPE2INFO_ARRAY + TYPE2INFO_4BYTE +  TYPE2INFO_FLOAT }; };    
template<> struct type2int<SmartData<float*>> { enum { result = TYPE2INFO_ARRAY + TYPE2INFO_4BYTE + TYPE2INFO_FLOAT }; };    
template<> struct type2int<SmartData<double*>> { enum { result = TYPE2INFO_ARRAY + TYPE2INFO_8BYTE + TYPE2INFO_FLOAT }; };    

/*
struct DataInfo {
    char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
    uint8_t dataType;
    DataObject* ptr;
};                                    // Data structure to hold Command/Handler function key-value pairs
*/



#endif //SMARTDATA_h
