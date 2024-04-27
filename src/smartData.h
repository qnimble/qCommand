#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>

// Maximum length of a command excluding the terminating null
#define STREAMCOMMAND_MAXCOMMANDLENGTH 12 //8


template <class SmartDataGeneric>
class DataObjectSpecific  {
public:
    DataObjectSpecific(SmartDataGeneric);
    SmartDataGeneric get(void);
    void set(SmartDataGeneric);
    bool please(void);

private:
    SmartDataGeneric value;

};

template< typename T >
struct type2int
{
   // enum { result = 0 }; // do this if you want a fallback value, empty to force a definition
};

template<> struct type2int<DataObjectSpecific<bool>> { enum { result = 1 }; };
template<> struct type2int<DataObjectSpecific<uint8_t>> { enum { result = 2 }; };
template<> struct type2int<DataObjectSpecific<uint16_t>> { enum { result = 4 }; };
template<> struct type2int<DataObjectSpecific<uint>> { enum { result = 4 }; };
template<> struct type2int<DataObjectSpecific<ulong>> { enum { result = 4 }; };
template<> struct type2int<DataObjectSpecific<int8_t>> { enum { result = 3 }; };
template<> struct type2int<DataObjectSpecific<int16_t>> { enum { result = 5 }; };
template<> struct type2int<DataObjectSpecific<int>> { enum { result = 6 }; };
template<> struct type2int<DataObjectSpecific<long>> { enum { result = 7 }; };
template<> struct type2int<DataObjectSpecific<float>> { enum { result = 8 }; };
template<> struct type2int<DataObjectSpecific<double>> { enum { result = 9 }; };
    

/*
struct DataInfo {
    char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
    uint8_t dataType;
    DataObject* ptr;
};                                    // Data structure to hold Command/Handler function key-value pairs
*/



#endif //SMARTDATA_h
