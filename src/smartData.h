#ifndef SMARTDATA_h
#define SMARTDATA_h

#include <Arduino.h>
#include "typeTraits.h"

class qCommand; // forward declaration

class Base {
   public:
	enum UpdateState {
		STATE_IDLE,					  // A
		STATE_NEED_TOSEND,			  // B
		STATE_WAIT_ON_ACK,			  // C
		STATE_WAIT_ON_ACK_PLUS_QUEUE  // BC
	};

	Base() : stream(0), id(0), ack_needed(0) {}

	bool getUpdateState(void) { return ack_needed; }
	virtual uint16_t size(void) = 0;
	virtual void resetUpdateState(void) = 0;
	virtual void ackObject(void) = 0;
	virtual uint8_t getMapSize(void) const {return 0;}
	virtual uint16_t getKeyPairAsString(uint8_t index, char* buffer, uint16_t bufferSize) const { return 0; }

   private:
	Stream* stream;
	uint8_t id;
	void _setPrivateInfo(uint8_t id, Stream* stream) {
		this->id = id;
		this->stream = stream;
	}

   protected:
	friend class qCommand;
	bool ack_needed = 0;
	volatile bool requestUpdate = false;

};

template <typename T>
class BaseTyped : public Base {
   public:
	BaseTyped(T data) : value(data) {}

	T get() const { return value; }
	virtual void set(T newValue) = 0;

	operator T() const { return value; }
	// virtual const char* getName() {         Serial2.println("BaseTyped CLASS
	// getName() CALLED"); return "BaseTyped-Result"; }
	const char* getName() { return name; }

   protected:
	T value;
	void setName(const char* newName) { name = newName; }

   private:
	const char* name = "";
};

// generic SmartData for single values and arrays
template <class DataType, bool isArray = TypeTraits<DataType>::isArray ||
										 TypeTraits<DataType>::isPointer>
class SmartData : public BaseTyped<DataType> {
   public:
	SmartData(DataType data);
};

// Class for common elements in SmartData for arrays
class AllSmartDataPtr : public Base {
   public:
	AllSmartDataPtr(size_t size) : totalElements(size) {};
	size_t getCurrentElement(void) { return currentElement; };
	size_t getTotalElements(void) { return totalElements; };
	void resetCurrentElement(void) { currentElement = 0; };

	bool isEmpty(void) { return currentElement == 0; };

	void ackObject(void) {
		if (ack_needed == true) {
			ack_needed = false;  // reset to idle state
			currentElement = 0;			  // reset current element after ack
		}
	}

	void resetUpdateState(void) {
		ack_needed = false;
		currentElement = 0;
	};

	const size_t totalElements;	 // in public because protected by const flag

   protected:
	size_t currentElement;
	// bool dataRequested = false;
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

	void set(baseType, size_t element);
	bool isFull(void);
	bool setNext(baseType);

	uint16_t size(void) { return totalElements * sizeof(baseType); }
	baseType get(size_t element) { return value[element]; }
	explicit operator DataType() const { return value; }

	baseType& operator[](size_t index) { return value[index]; }
	const baseType& operator[](size_t index) const { return value[index]; }
	uint16_t setActiveSize(uint16_t newSize);
	size_t getActiveSize(void);

  protected:
	uint8_t cmdNumber = 255; // command number assigned during registration
	qCommand* qC_parent = nullptr; // parent qCommand assigned during registration

   private:
	DataType value;
	size_t activeElements = 0;
	friend class qCommand;
};

// Specialization for non-arrays
template <class DataType>
class SmartData<DataType, false>
	: public BaseTyped<typename SmartDataKeyType<DataType>::type> {
   public:
	using ValueType = typename SmartDataKeyType<DataType>::type;

	// Default constructor
	SmartData() : BaseTyped<ValueType>(ValueType{}) {}

	template <typename T = DataType,
			  typename std::enable_if<!is_option_ptr<T>::value, int>::type = 0>
	SmartData(T data) : BaseTyped<DataType>(data) {}

	template <typename T = DataType, size_t N,
			  typename std::enable_if<is_option_ptr<T>::value, int>::type = 0>
	SmartData(typename std::remove_pointer<T>::type (&data)[N])
        : BaseTyped<typename SmartDataKeyType<DataType>::type>(
              N > 0 ? static_cast<typename SmartDataKeyType<DataType>::type>(data[0].key) : static_cast<typename SmartDataKeyType<DataType>::type>(0)),
		  mapSize(N),
		  map(reinterpret_cast<decltype(map)>(data)) {
		if ((N > 0) && (data[0].value != "")) {
			this->setName(data[0].value.c_str());
		}
	}

    using PlainSetterFuncPtr = ValueType (*)(ValueType, ValueType);
    using ThunkFuncPtr = ValueType (*)(void* ctx, ValueType newV, ValueType oldV);

     template <typename EnumType>
        requires (std::is_enum_v<EnumType> && (sizeof(EnumType) == sizeof(ValueType)))
    void setSetter(EnumType (*enumSetter)(EnumType, EnumType)) {
        // store the function pointer bytes as enumtype differs from ValueType for compiler type checking
        static_assert(sizeof(enumSetter) == sizeof(StoredSetter.bytes),
                      "Unexpected function pointer size");
        memcpy(StoredSetter.bytes, &enumSetter, sizeof(enumSetter));
        thunk = &SmartData::template enumThunk<EnumType>;
    }

	void setSetter(PlainSetterFuncPtr fn) {
		StoredSetter.plain = fn;
		thunk = &plainThunk;
	}

	void setLimits(ValueType minV, ValueType maxV) {
		//Store limits in class instance
		minValue = minV;
		maxValue = maxV;
		thunk = &limitsThunk;
		setImpl(this->value);
	}


	// For fundamental types like int, float, bool
	template <typename T = ValueType>
	T get() const
		requires std::is_fundamental<
			typename std::remove_pointer<T>::type>::value
	{
		return this->value;
	}

	/*
		const char* getName() override {
			Serial2.print("DataType is option_ptr: ");
			Serial2.println(is_option_ptr<DataType>::value ? "true" : "false");
			if constexpr (is_option_ptr<DataType>::value) {
				for (size_t i = 0; i < mapSize; ++i) {
					if (map[i].key == this->value) {
						return map[i].value.c_str();
					}
				}
				return "This is a SmartData with Options, but could not find key";
			} else {
				return "NOT OPTIONS"; //nullptr; // or return
	   BaseTyped<...>::getName();
			}
		}
	*/
	// For complex types like String that are not arrays nor fundamental types
	template <typename T = ValueType>
	const T& get() const
		requires(
			!std::is_fundamental<typename std::remove_pointer<T>::type>::value)
	{
		return this->value;
	}

	operator ValueType() const {
		return this->value;	 // return the pointer to the array
	}

	 // Add assignment operator to enable testList = 5 syntax
    SmartData& operator=(ValueType newValue) {
        set(newValue);
        return *this;
    }

	// Prefix increment
	template <typename V = ValueType>
    requires std::is_arithmetic_v<V>
    SmartData& operator++() {
        set(this->value + 1);
        return *this;
	}

	// Postfix increment - returns the old VALUE
	template <typename V = ValueType>
    requires std::is_arithmetic_v<V>
	ValueType operator++(int) {
		ValueType oldValue = this->value;
		set(this->value + 1);
		return oldValue;
	}

	// Prefix decrement
	template <typename V = ValueType>
    requires std::is_arithmetic_v<V>
	SmartData& operator--() {
		set(this->value - 1);
		return *this;
	}

	// Postfix decrement - returns the old VALUE
	template <typename V = ValueType>
    requires std::is_arithmetic_v<V>
	ValueType operator--(int) {
		ValueType oldValue = this->value;
		set(this->value - 1);
		return oldValue;
	}

	// Compound assignment operators
	template <typename V = ValueType>
    requires std::is_arithmetic_v<V>
	SmartData& operator+=(ValueType rhs) {
		set(this->value + rhs);
		return *this;
	}

	template <typename V = ValueType>
	requires std::is_arithmetic_v<V>
	SmartData& operator-=(ValueType rhs) {
		set(this->value - rhs);
		return *this;
	}

	template <typename V = ValueType>
	requires std::is_arithmetic_v<V>
	SmartData& operator*=(ValueType rhs) {
		set(this->value * rhs);
		return *this;
	}

	template <typename V = ValueType>
	requires std::is_arithmetic_v<V>
	SmartData& operator/=(ValueType rhs) {
		set(this->value / rhs);
		return *this;
	}

	template <typename V = ValueType>
	requires std::is_integral_v<V>
	SmartData& operator%=(ValueType rhs) {
		set(this->value % rhs);
		return *this;
	}


	void set(ValueType newValue) {
		if constexpr (is_option_ptr<DataType>::value) {
			for (size_t i = 0; i < mapSize; ++i) {
				if (map[i].key == newValue) {
					setImpl(newValue);
					if (map[i].value != "") {
						this->setName(map[i].value.c_str());
					} else {
						this->setName("");	 // clear name if empty string
					}
					break;
				}
			}
		} else {
			setImpl(newValue);
		}
	}

	void resetUpdateState(void) { this->ack_needed = this->STATE_IDLE; }
	void ackObject(void) {
		if (this->ack_needed == true) {
			this->ack_needed = false;
		}
	}
	uint16_t size(void) { return sizeof(ValueType); }

	uint8_t getMapSize(void) const { 
		if constexpr (is_option_ptr<DataType>::value) {
            return mapSize;
        } else {
            return 0;
		}
	}

	uint16_t getKeyPairAsString(uint8_t index, char* buffer, uint16_t bufferSize) const { 
		if constexpr (is_option_ptr<DataType>::value) {
			 if (index >= mapSize || bufferSize == 0) {
                return 0;
            }

            // DataType is OptionImpl<KeyType>*, so just extract it directly
            using StoredOptionType = typename std::remove_pointer<DataType>::type;
            using StoredKeyType = decltype(StoredOptionType::key);

            const StoredOptionType& entry = map[index];
            
            // Access key and value
            StoredKeyType keyValue = entry.key;
			uint16_t len1 = sizeof(keyValue);
			if (len1 > bufferSize) {
				return 0; // Not enough space in buffer
			}
			memcpy(buffer, &keyValue, len1);

            const char* valueStr = entry.value.c_str();
            
            // Copy value string to buffer
            uint16_t len2 = strlen(valueStr);
            if (len1+ len2 + 1 > bufferSize) {
                return 0; // Not enough space in buffer
            }
            memcpy(&buffer[len1], valueStr, len2);
            buffer[len1 + len2] = '\0';
            
            return len1 + len2 + 1;
        } else {
            return 0;
		}
	}

	void runOnUpdate(void (*func)(void)) {
		onUpdate = func;
	}

   private:
	bool dataRequested = false;
	friend class qCommand;

	void (*onUpdate)(void) = nullptr;

	ThunkFuncPtr thunk = nullptr;
	union StoredSetter {
		PlainSetterFuncPtr plain;
		uint8_t bytes[sizeof(void (*)())];

		// default-init to null
		constexpr StoredSetter() : plain(nullptr) {}
	} StoredSetter;

	ValueType minValue;
	ValueType maxValue;

	// Call this inside setImpl instead of std::function
    ValueType applySetterIfAny(ValueType newV, ValueType oldV) {
        return (thunk != nullptr) ? thunk(this, newV, oldV) : newV;
    }

    static ValueType plainThunk(void* ctx, ValueType newV, ValueType oldV) {
        auto* self = static_cast<SmartData*>(ctx);
        return (self->StoredSetter.plain != nullptr) ? self->StoredSetter.plain(newV, oldV) : newV;
    }

    static ValueType limitsThunk(void* ctx, ValueType newV, ValueType /*oldV*/) {
        auto* self = static_cast<SmartData*>(ctx);
        if (newV < self->minValue) newV = self->minValue;
        else if (newV > self->maxValue) newV = self->maxValue;
        return newV;
    }

	// NEW enumThunk: reconstruct the original enum-typed function pointer via memcpy
    template <typename EnumType>
    static ValueType enumThunk(void* ctx, ValueType newV, ValueType oldV) {
        auto* self = static_cast<SmartData*>(ctx);

        EnumType (*fn)(EnumType, EnumType) = nullptr;
        memcpy(&fn, self->StoredSetter.bytes, sizeof(fn));
        if (fn == nullptr) {
            return newV;
        }

        const auto out = fn(static_cast<EnumType>(newV), static_cast<EnumType>(oldV));
        return static_cast<ValueType>(out);
    }

	void setImpl(ValueType newValue);
	// Add Keys-specific members

	// Only used for Option types
	uint8_t mapSize = 0;
	// Use a conditional pointer type that resolves to nullptr for non-Option
	// types
	template <typename T = DataType>
	static constexpr auto GetMapPointerType() {
		if constexpr (is_option_ptr<T>::value) {
            return static_cast<typename std::remove_pointer<T>::type*>(nullptr);
		} else {
			return static_cast<void*>(nullptr);
		}
	}
	// Use decltype to get the correct pointer type
	decltype(GetMapPointerType<DataType>()) map = nullptr;
};

// Helper to unwrap SmartData
template <typename T>
struct unwrap_smart_data {
	using type = T;
};
template <typename T>
struct unwrap_smart_data<SmartData<T>> {
	using type = T;
};

// Helper to remove pointer if not already pointer
template <typename T>
struct remove_ptr_if_not_ptr {
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
template <typename T>
struct type2int {
	using removed_ptr = typename std::remove_pointer<T>::type;
	using unwrapped = typename unwrap_smart_data<removed_ptr>::type;
	using non_const = typename remove_const<unwrapped>::type;
	static constexpr uint8_t result = type2int_base<non_const>::result;
};

#endif	// SMARTDATA_h
