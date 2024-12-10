#ifndef SMARTDATA_h
#define SMARTDATA_h

#include "typeTraits.h"
#include <Arduino.h>

class Base {
  public:
    enum UpdateState {
        STATE_IDLE,                  // A
        STATE_NEED_TOSEND,           // B
        STATE_WAIT_ON_ACK,           // C
        STATE_WAIT_ON_ACK_PLUS_QUEUE // BC
    };

    Base() : stream(0), id(0), updates_needed(STATE_IDLE) {}
    void resetUpdateState(void) { updates_needed = STATE_IDLE; };
    UpdateState getUpdateState(void) { return updates_needed; }
    void sendUpdate(void);
    virtual uint16_t size(void);

  private:
    Stream *stream;
    uint8_t id;
    void _setPrivateInfo(uint8_t id, Stream *stream) {
        this->id = id;
        this->stream = stream;
    }

  protected:
    friend class qCommand;
    UpdateState updates_needed;
};

// generic SmartData for single values and arrays
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

    void resetUpdateState(void);

    const size_t totalElements; // in public because protected by const flag
  protected:
    size_t currentElement;
    bool dataRequested = false;
};

// Specialization of SmartData for arrays!
template <class DataType>
class SmartData<DataType, true> : public AllSmartDataPtr {
  public:
    // DataType cannot be used directly as it may be a reference.
    // default is just T.
    template <typename T> struct storage_type_specialization {
        using type = T;
    };

    // But if it is an array, then we need to use a pointer
    template <typename T, size_t N>
    struct storage_type_specialization<T (&)[N]> {
        using type = T *;
    };

    // And references use pointer.
    template <typename T> struct storage_type_specialization<T &> {
        using type = T *;
    };

    using storage_type = typename storage_type_specialization<DataType>::type;
    using baseType = typename std::remove_pointer<
        typename std::remove_reference<storage_type>::type>::type;

    // For pointer arrays with explicitly set size
    SmartData(storage_type data, size_t size)
        requires(TypeTraits<DataType>::isPointer &&
                 !TypeTraits<DataType>::isReference)
        : AllSmartDataPtr(size / sizeof(baseType)), value(data) {};

    // For Reference to arrays
    template <size_t N>
    SmartData(typename std::remove_reference<DataType>::type (&data)[N])
        requires(TypeTraits<DataType>::isReference)
        : AllSmartDataPtr(N), value(&data[0]){};

    void setNext(baseType);

    uint16_t size(void) { return totalElements * sizeof(baseType); }

    baseType get(size_t element) { return value[element]; }

  private:
    storage_type value;
    friend class qCommand;
};

// Specialization for non-arrays
template <class DataType> class SmartData<DataType, false> : public Base {
  public:
    SmartData(DataType data) : value(data) {};

    // For fundamental types like int, float, bool
    template <typename T = DataType>
    T get() const
        requires std::is_fundamental<
            typename std::remove_pointer<T>::type>::value
    {
        return value;
    }

    // For complex types like String
    template <typename T = DataType>
    T get() const
        requires !std::is_fundamental<
            typename std::remove_pointer<T>::type>::value
    {
        return value;
    }

    void set(DataType);
    void resetUpdateState(void);
    uint16_t size(void) { return sizeof(DataType); }

  private:
    DataType value;
    bool dataRequested = false;
    friend class qCommand;
};

// Helper to unwrap SmartData
template <typename T> struct unwrap_smart_data {
    using type = T;
};
template <typename T> struct unwrap_smart_data<SmartData<T>> {
    using type = T;
};

// Helper to remove pointer if not already pointer
template <typename T> struct remove_ptr_if_not_ptr {
    using type =
        typename std::conditional<std::is_pointer<T>::value,
                                  typename std::remove_pointer<T>::type,
                                  T>::type;
};

// Base template that applies rules
template <typename T> struct type2int {
    static constexpr uint8_t result = type2int_base<typename unwrap_smart_data<
        typename remove_ptr_if_not_ptr<T>::type>::type>::result;
};



#endif // SMARTDATA_h
