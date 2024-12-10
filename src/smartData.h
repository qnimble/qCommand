#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>
#include "typeTraits.h"

class Base {
  public:
    enum UpdateState {
        STATE_IDLE,                  // A
        STATE_NEED_TOSEND,           // B
        STATE_WAIT_ON_ACK,           // C
        STATE_WAIT_ON_ACK_PLUS_QUEUE // BC
    };

    Base() : stream(0), id(0), updates_needed(STATE_IDLE) {}
    void sendUpdate(void);
    void resetUpdateState(void) { updates_needed = STATE_IDLE; };
    UpdateState getUpdateState(void) { return updates_needed; }
    virtual uint16_t size(void);
 
private:
    Stream *stream;
    uint8_t id;
    void _setPrivateInfo(uint8_t id,Stream *stream) {
        this->id = id;
        this->stream = stream;
    }
protected:
    friend class qCommand;    
    UpdateState updates_needed;
};

//generic SmartData for single values and arrays
template <class DataType, bool isArray = TypeTraits<DataType>::isArray>
class SmartData : public Base {
  public:
    SmartData(DataType data);
};

// Class for common elements in SmartData for arrays
class AllSmartDataPtr : public Base {
  public:
    AllSmartDataPtr(size_t size) : totalElements(size) {};    
    size_t getCurrentElement(void) { return currentElement; };
    size_t getTotalElements(void) { return totalElements; };
    void resetCurrentElement(void) {
        currentElement = 0;
        dataRequested = true;
    };

    //void _get(void *data, size_t element);
    //void _set(void *data, size_t element);    
    void resetUpdateState(void);

    const size_t totalElements; //in public because protected by const flag
  protected:
    size_t currentElement;
    bool dataRequested = false;
};

// Specialization of SmartData for arrays!
template <class DataType>
class SmartData<DataType, true> : public AllSmartDataPtr {
  public:
    //struct ref_of_array_to_pointer is underlying data type of array. 
    //DataType cannot be used directly as it may be a reference.
    template<typename T>
    struct storage_type_specialization {
      using type = T;
    };

    template<typename T, size_t N>
    struct storage_type_specialization<T(&)[N]> {
      using type = T*;
    };
    
    // Add: For basic references
    template<typename T>
    struct storage_type_specialization<T&> {
      using type = T*;
    };
    
    using storage_type = typename storage_type_specialization<DataType>::type;        
    using baseType = typename std::remove_pointer< typename std::remove_reference<storage_type>::type>::type;


    // For pointer arrays with explicitly set size    
    SmartData(storage_type data, size_t size)
        requires (TypeTraits<DataType>::isPointer && !TypeTraits<DataType>::isReference)
        : AllSmartDataPtr(size / sizeof(baseType)), value(data) {
        //Serial.printf("Manual Array: %u with Size = %u and now totalElements=%u\n",
          //            data, size, totalElements);
    };

    //For Reference to arrays
    template <size_t N>
    SmartData(typename std::remove_reference<DataType>::type (&data)[N])
        requires (TypeTraits<DataType>::isReference)
        : AllSmartDataPtr(N),
        value(&data[0]) {
        //Serial.printf("Reference Array: %u with size=%u and totalElements=%u\n",
        //              data[0], totalElements * sizeof(baseType), totalElements);
    };

    //void set(DataType);
    
    void setNext(baseType);

    uint16_t size(void) {
        return totalElements * sizeof(baseType);
    }

    baseType get(size_t element) {
        return value[element];
    }


    
  private:
    //void _setPrivateInfo(uint8_t id, Stream *stream);
    storage_type value;
    friend class qCommand;
        

};

// For non-arrays
template <class DataType>
class SmartData<DataType, false> : public Base {
  public:
    SmartData(DataType data) : value(data) {};
    //void sendValue2(void);
    // For fundamental types like int, float, bool
    template <typename T = DataType>
    typename std::enable_if<
        std::is_fundamental<typename std::remove_pointer<T>::type>::value,
        T>::type
    get() const {
        return value;
    }

    // For complex types like String
    template <typename T = DataType>
    typename std::enable_if<
        !std::is_fundamental<typename std::remove_pointer<T>::type>::value,
        const T &>::type
    get() const {
        return value;
    }

    void set(DataType);
    
    //void _get(void *data);
    //void _set(void *data);

    void resetUpdateState(void);

    uint16_t size(void) { return sizeof(DataType); }
    

  private:
    //void _setPrivateInfo(uint8_t id, Stream *stream);
    DataType value;

    bool dataRequested = false;

    friend class qCommand;
};

template <typename T> struct type2int {
    // enum { result = 0 }; // do this if you want a fallback value, empty to
    // force a definition
};

// template<> struct type2int<char*> { enum { result = 4 }; };
template <std::size_t N> struct type2int<char[N]> {
    enum { result = 4 };
};
template <> struct type2int<char*> {
    enum { result = 4 };
};
template <> struct type2int<char> {
    enum { result = 4 };
};

template <> struct type2int<String> {
    enum { result = 4 };
};

template <> struct type2int<SmartData<String>> {
    enum { result = 4 };
};
template <> struct type2int<bool> {
    enum { result = 6 };
};

template <> struct type2int<bool*> {
    enum { result = 6 };
};
template <> struct type2int<SmartData<bool>> {
    enum { result = 6 };
};

template <> struct type2int<SmartData<bool*>> {
    enum { result = 6 };
};
template <> struct type2int<SmartData<uint8_t>> {
    enum { result = 6 };
};
template <> struct type2int<uint8_t> {
    enum { result = 6 };
};
//template <> struct type2int<uint8_t&> {
//    enum { result = 6 };
//};

template <> struct type2int<SmartData<uint16_t>> {
    enum { result = 8 };
};
template <> struct type2int<SmartData<uint16_t *>> {
    enum { result = 8 };
};
template <> struct type2int<uint16_t> {
    enum { result = 8 };
};
//template <> struct type2int<uint16_t&> {
//    enum { result = 8 };
//};

template <> struct type2int<SmartData<uint>> {
    enum { result = 10 };
};
template <> struct type2int<uint> {
    enum { result = 10 };
};
template <> struct type2int<SmartData<ulong>> {
    enum { result = 10 };
};
template <> struct type2int<ulong> {
    enum { result = 10 };
};
template <> struct type2int<SmartData<int8_t>> {
    enum { result = 5 };
};
template <> struct type2int<int8_t> {
    enum { result = 5 };
};
template <> struct type2int<SmartData<int16_t>> {
    enum { result = 7 };
};
template <> struct type2int<int16_t> {
    enum { result = 7 };
};
template <> struct type2int<SmartData<int>> {
    enum { result = 9 };
};
template <> struct type2int<int> {
    enum { result = 9 };
};
template <> struct type2int<SmartData<long>> {
    enum { result = 9 };
};
template <> struct type2int<long> {
    enum { result = 9 };
};
template <> struct type2int<SmartData<float>> {
    enum { result = 11 };
};

template <> struct type2int<float> {
    enum { result = 11 };
};
template <> struct type2int<float*> {
    enum { result = 11 };
};



template <> struct type2int<SmartData<double>> {
    enum { result = 12 };
};
template <> struct type2int<double> {
    enum { result = 12 };
};

// template<> struct type2int<SmartDataPtr<float*>> { enum { result =
// TYPE2INFO_ARRAY + TYPE2INFO_4BYTE +  TYPE2INFO_FLOAT }; };
template <> struct type2int<SmartData<float *>> {
    enum { result = 11 };
};
template <> struct type2int<SmartData<double *>> {
    enum { result = 12 };
};


#endif // SMARTDATA_h
