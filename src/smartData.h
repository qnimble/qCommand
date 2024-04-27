#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>

// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 12 //8

#define TYPE2INFO_ARRAY (0)
#define TYPE2INFO_MIN (1)
#define TYPE2INFO_2MIN (2)
#define TYPE2INFO_4MIN (3)
#define TYPE2INFO_ID(x) (x<<2)
#define TYPE2INFO_BOOL  (0)
#define TYPE2INFO_FLOAT  (1<<5)
#define TYPE2INFO_UINT  (2<<5)
#define TYPE2INFO_INT  (3<<5)

template <class DataType>
class SmartData  {
public:
    SmartData(DataType);
    DataType get(void);
    void set(DataType);
    bool please(void);

private:
    DataType value;

};

template< typename T >
struct type2int
{
   // enum { result = 0 }; // do this if you want a fallback value, empty to force a definition
};

template<> struct type2int<SmartData<bool>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_ID(0) + TYPE2INFO_BOOL }; };
template<> struct type2int<SmartData<uint8_t>> { enum { result =  TYPE2INFO_MIN + TYPE2INFO_ID(0)  + TYPE2INFO_UINT}; };
template<> struct type2int<SmartData<uint16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_ID(1)  + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<uint>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_ID(2)  + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<ulong>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_ID(3)  + TYPE2INFO_UINT }; };
template<> struct type2int<SmartData<int8_t>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_ID(0)  + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int16_t>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_ID(1)  + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<int>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_ID(2)  + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<long>> { enum { result = TYPE2INFO_4MIN + TYPE2INFO_ID(3)  + TYPE2INFO_INT }; };
template<> struct type2int<SmartData<float>> { enum { result = TYPE2INFO_MIN + TYPE2INFO_ID(0)  + TYPE2INFO_FLOAT }; };
template<> struct type2int<SmartData<double>> { enum { result = TYPE2INFO_2MIN + TYPE2INFO_ID(1)  + TYPE2INFO_FLOAT }; };
    

/*
struct DataInfo {
    char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
    uint8_t dataType;
    DataObject* ptr;
};                                    // Data structure to hold Command/Handler function key-value pairs
*/



#endif //SMARTDATA_h
