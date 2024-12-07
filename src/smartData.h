#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>


enum UpdateState {
    STATE_IDLE,                  // A
    STATE_NEED_TOSEND,           // B
    STATE_WAIT_ON_ACK,           // C
    STATE_WAIT_ON_ACK_PLUS_QUEUE // BC
};

//Set Default values for TypeTraits
template <typename T, typename Enable = void> struct TypeTraits {
    static constexpr bool isArray = false;
    static constexpr bool isPointer = false;
    static constexpr bool isReference = false;
};

// Specialization for pointer types (treated as pointers to arrays)
template <typename T>
struct TypeTraits<T, std::enable_if_t<std::is_pointer<T>::value>> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = true;
};

// Specialization for reference types that are not arrays
template <typename T>
struct TypeTraits<T, std::enable_if_t<std::is_reference<T>::value &&
                    !std::is_array<std::remove_reference_t<T>>::value>> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = true;
    static constexpr bool isArray = false;
};

// Specialization for references to arrays
template <typename T>
struct TypeTraits<
    T, std::enable_if_t<std::is_reference<T>::value &&
                        std::is_array<std::remove_reference_t<T>>::value>> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = true;
    static constexpr bool isArray = true;
};

template <> struct TypeTraits<String> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
};

// Specialization for const char*
template <> struct TypeTraits<const char *> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
};

// Specialization for char*
template <> struct TypeTraits<char *> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
};

static_assert(!TypeTraits<int>::isArray,
              "int should not be considered an array");
static_assert(TypeTraits<float *>::isArray,
              "float* should be considered an array");
static_assert(TypeTraits<double *>::isArray,
              "double* should be considered an array");
static_assert(TypeTraits<double&>::isReference,
              "double& should be considered a reference");
static_assert(!TypeTraits<double&>::isArray,
              "double& should be considered a reference");
static_assert(TypeTraits<double(&)[50]>::isArray,
              "double& should be considered a reference");

/*
template <typename T, std::size_t N> constexpr std::size_t arraySize(T (&)[N]) {
    return N;
}*/

#warning move these to private when done with debug
class Base {
  public:
    // virtual void _get(void* data); // pure virtual function
    virtual void _set(void *data); // pure virtual function
    void setNeedToSend(void); // set states as if set ran, even though it didn't.
    virtual void sendValue(void);    
    virtual void _get(void *data); // pure virtual function
    void resetUpdateState(void);
    uint16_t size(void);

  protected:
    UpdateState updates_needed = STATE_IDLE;   
    friend class qCommand;
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
    virtual size_t getCurrentElement(void);
    virtual size_t getTotalElements(void);
    virtual void resetCurrentElement(void);
    size_t size(void);
    const size_t totalElements;
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
        : AllSmartDataPtr(size / sizeof(baseType)), value(data) , id(0), stream(0) {
        Serial.printf("Manual Array: %u with Size = %u and now totalElements=%u\n",
                      data, size, totalElements);
    };

    //For Reference to arrays
    template <size_t N>
    SmartData(typename std::remove_reference<DataType>::type (&data)[N])
        requires (TypeTraits<DataType>::isReference)
        : AllSmartDataPtr(N / sizeof(baseType)),
        value(&data[0]), 
        id(0), 
        stream(0) {
        Serial.printf("Reference Array: %u with size=%u and totalElements=%u\n",
                      data[0], N, totalElements);
    };

    void set(DataType);
    void sendValue(void);
    void _get(void *data);
    void _set(void *data);
    // void setNeedToSend(void);
    void resetUpdateState(void);

    void setNext(baseType);
    size_t getTotalElements(void) { return totalElements; };

    uint16_t size(void) {
        return totalElements *
               sizeof(typename std::remove_pointer<DataType>::type);
    }

    baseType get(size_t element) {
        return value[element];
    }



    size_t getCurrentElement(void) { return currentElement; };

    void resetCurrentElement(void) {
        currentElement = 0;
        dataRequested = true;
    };

  #warning move to protected
    storage_type value;
  protected:
    

  private:
    void _setPrivateInfo(uint8_t id, Stream *stream);
    //cw_pack_context *pc;

    
    size_t currentElement;
    bool dataRequested = false;

    friend class qCommand;

    // private data that gets set by qC::addCommand
    uint8_t id;
    Stream *stream;
};

// For non-arrays
template <class DataType>
// class SmartData<DataType, typename
// std::enable_if<!std::is_array<DataType>::value>::type>: public Base {
class SmartData<DataType, false> : public Base {
  public:
    SmartData(DataType data) : value(data), id(0), stream(0) {};

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
        T &>::type
    get() {
        return value;
    }

    void set(DataType);
    void please(void);
    void sendValue(void);
    void _get(void *data);
    void _set(void *data);

    // void setNeedToSend(void);
    void resetUpdateState(void);

    uint16_t size(void) { return sizeof(DataType); }
    DataType value;
#warning move back value to protected after testing
  protected:
    // DataType value;

  private:
    void _setPrivateInfo(uint8_t id, Stream *stream);
    //cw_pack_context *pc;

    bool dataRequested = false;

    friend class qCommand;

    // private data that gets set by qC::addCommand
    uint8_t id;
    Stream *stream;
};

/*
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
*/
// template <class DataType>
// using SmartDataNew = typename
// std::conditional<std::is_pointer<DataType>::value, SmartDataPtr<DataType>,
// SmartData<DataType>>::type;

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
template <> struct type2int<SmartData<uint16_t>> {
    enum { result = 8 };
};
template <> struct type2int<SmartData<uint16_t *>> {
    enum { result = 8 };
};
template <> struct type2int<uint16_t> {
    enum { result = 8 };
};
template <> struct type2int<uint16_t&> {
    enum { result = 8 };
};

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

/*
struct DataInfo {
    char command[STREAMCOMMAND_MAXCOMMANDLENGTH + 1];
    uint8_t dataType;
    DataObject* ptr;
};                                    // Data structure to hold Command/Handler
function key-value pairs
*/

#endif // SMARTDATA_h
