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

    UpdateState getUpdateState(void) { return updates_needed; }
    void sendUpdate(void);
    virtual uint16_t size(void);
    virtual void resetUpdateState(void) = 0;
    virtual void ackObject(void) = 0;

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
template <class DataType, bool isArray = TypeTraits<DataType>::isArray ||
                                         TypeTraits<DataType>::isPointer>
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
    };

    bool isEmpty(void) { return currentElement == 0; };
    bool isFull(void) { return currentElement == totalElements; };

    void ackObject(void) {
        if (updates_needed == STATE_WAIT_ON_ACK) {
            updates_needed = STATE_IDLE; // reset to idle state
            currentElement = 0; // reset current element after ack
        } else if (updates_needed == STATE_WAIT_ON_ACK_PLUS_QUEUE) {
            updates_needed = STATE_NEED_TOSEND; // reset to need to send state
            currentElement = 0; // reset current element after ack
        }
    }

    void resetUpdateState(void) {
        updates_needed = STATE_IDLE;
        currentElement = 0;
    };

    const size_t totalElements; // in public because protected by const flag

  protected:
    size_t currentElement;
    //bool dataRequested = false;
};

// Specialization of SmartData for arrays!
template <class DataType>
class SmartData<DataType, true> : public AllSmartDataPtr {
  public:
    using baseType = typename std::remove_pointer<DataType>::type;

    // For pointer arrays with explicitly set size
    SmartData(DataType data, size_t size)
        requires(TypeTraits<DataType>::isPointer)
        : AllSmartDataPtr(size / sizeof(baseType)), value(data) {};

    // For pointer types with arrays
    template <size_t N>
    SmartData(typename std::remove_pointer<DataType>::type (&data)[N])
        : AllSmartDataPtr(N), value(data){};

    void set(baseType,size_t element);
    bool setNext(baseType);

    uint16_t size(void) { return totalElements * sizeof(baseType); }
    baseType get(size_t element) { return value[element]; }
    explicit operator DataType() const { return value; }

    baseType& operator[](size_t index) {
        return value[index];
    }
    const baseType& operator[](size_t index) const {
        return value[index];
    }

  private:
    DataType value;
    friend class qCommand;
};

// Specialization for non-arrays
template <class DataType> 
class SmartData<DataType, false> : public Base {
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

    // For complex types like String that are not arrays nor fundamental types
    template <typename T = DataType>
    const T &get() const
        requires (!std::is_fundamental<
            typename std::remove_pointer<T>::type>::value)
    {
        return value;
    }

    operator DataType() const {
        return value; // return the pointer to the array
    }
    void set(DataType);
    void resetUpdateState(void) { updates_needed = STATE_IDLE; }
    void ackObject(void) {
        if (updates_needed == STATE_WAIT_ON_ACK) {
            updates_needed = STATE_IDLE; // reset to idle state
        } else if (updates_needed == STATE_WAIT_ON_ACK_PLUS_QUEUE) {
            updates_needed = STATE_NEED_TOSEND; // reset to need to send state
        } else {
        //updates_needed = STATE_IDLE; // reset to idle state
        }
    }
    uint16_t size(void) { return sizeof(DataType); }
    using SetterFuncPtr = DataType (*)(DataType, DataType);
    void setSetter(SetterFuncPtr setter) { this->setter = setter; }

  private:
    DataType value;
    bool dataRequested = false;
    friend class qCommand;
    SetterFuncPtr setter = nullptr;
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

// Helper to remove const qualifier
template <typename T>
struct remove_const {
    using type = T;
};

template <typename T>
struct remove_const<const T> {
    using type = T;
};

// Base template that applies rules
template <typename T> struct type2int {
    using removed_ptr = typename std::remove_pointer<T>::type;
    using unwrapped = typename unwrap_smart_data<removed_ptr>::type;
    using non_const = typename remove_const<unwrapped>::type;
    static constexpr uint8_t result = type2int_base<non_const>::result;
};

#endif // SMARTDATA_h
