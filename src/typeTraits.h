#ifndef TYPE_TRAITS_H
#define TYPE_TRAITS_H

#include <type_traits>
#include <cstdint>
#include <WString.h>


template <typename KeyType>
struct Keys {    
    using KeyType_t = KeyType;
    KeyType key;
    String value;
};

template <typename ListType>
struct List {    
    using ListType_t = ListType;
    ListType key;    
};


template <typename T>
struct is_keys_ptr : std::false_type {};

template <typename KeyType>
struct is_keys_ptr<Keys<KeyType>*> : std::true_type {};

template <typename T>
struct is_list_ptr : std::false_type {};

template <typename ListType>
struct is_list_ptr<List<ListType>*> : std::true_type {};


// Set Default values for TypeTraits
template <typename T, typename Enable = void> struct TypeTraits {
    static constexpr bool isArray = false;
    static constexpr bool isPointer = false;    
};

// Array but not Pointer
template <typename T>
struct TypeTraits<T, std::enable_if_t<!std::is_pointer<T>::value &&  std::is_array<T>::value>> {
    static constexpr bool isArray = true;
    static constexpr bool isPointer = false;
};

// Pointer but not Array nor Keys
template <typename T>
struct TypeTraits<T, std::enable_if_t<
    !std::is_array<T>::value &&
    std::is_pointer<T>::value &&
    !is_list_ptr<T>::value &&
    !is_keys_ptr<T>::value>>
{    static constexpr bool isArray = false;
    static constexpr bool isPointer = true;
};

// Add this specialization to typeTraits.h
template <typename T>
struct TypeTraits<T, std::enable_if_t<is_keys_ptr<T>::value>> {
   static constexpr bool isArray = false;
   static constexpr bool isPointer = false; // Treat it as not a pointer for SmartData
};

// Add this specialization to typeTraits.h
template <typename T>
struct TypeTraits<T, std::enable_if_t<is_list_ptr<T>::value>> {
   static constexpr bool isArray = false;
   static constexpr bool isPointer = false; // Treat it as not a pointer for SmartData
};


// Helper trait for ValueType
template <typename T>
struct SmartDataKeyType {
    using type = T;
};

template <typename KeyType>
struct SmartDataKeyType<Keys<KeyType>*> {
    using type = KeyType;
};



/*

static_assert(!TypeTraits<int>::isArray,
              "int should not be considered an array");
static_assert(TypeTraits<float[4]*>::isArray,
              "float[4] should be considered an array");
static_assert(TypeTraits<float[4]*>::isPointer,
              "float[4] should be considered a");
static_assert(TypeTraits<double *>::isArray,
              "double* should be considered an array");
static_assert(TypeTraits<double (&)[50]>::isArray,
              "double& should be considered a reference");
static_assert(!TypeTraits<char *>::isArray,
              "char* should not be an array but a pointer");
static_assert(!TypeTraits<const char *>::isArray,
              "const char* should not be an array but a pointer");
// static_assert(!TypeTraits<unsigned char*>::isArray, "unsigned char* should
// not be an array but a pointer");
static_assert(TypeTraits<uint8_t *>::isArray,
              "uint8_t* should not be an array but a pointer");
static_assert(TypeTraits<int8_t *>::isArray,
              "int8_t* should not be an array but a pointer");
static_assert(TypeTraits<uint16_t *>::isArray,
              "uint8_t* should not be an array but a pointer");
static_assert(TypeTraits<int16_t *>::isArray,
              "int8_t* should not be an array but a pointer");
static_assert(TypeTraits<bool>::isBool, "bool is bool");
static_assert(TypeTraits<bool *>::isBool, "bool* is bool");
*/

// Specialization for Keys<KeyType>*

/*
#define SPECIALIZE_KEYS_TRAITS(TYPE) \
template<> \
struct TypeTraits<Keys<TYPE>*> { \
    static constexpr bool isArray = false; \
    static constexpr bool isPointer = false; \
};

SPECIALIZE_KEYS_TRAITS(bool)
SPECIALIZE_KEYS_TRAITS(uint8_t)  // unsigned char
SPECIALIZE_KEYS_TRAITS(int8_t)
SPECIALIZE_KEYS_TRAITS(uint16_t)
SPECIALIZE_KEYS_TRAITS(int16_t)
SPECIALIZE_KEYS_TRAITS(uint32_t)
SPECIALIZE_KEYS_TRAITS(int32_t)
SPECIALIZE_KEYS_TRAITS(float)
SPECIALIZE_KEYS_TRAITS(double)
*/
template <typename T>
struct type2int_base; // force defintion by leaving out result for default case
template <> struct type2int_base<bool> {
    enum { result = 13 };
};
template <> struct type2int_base<uint8_t> {
    enum { result = 6 };
};
template <> struct type2int_base<uint16_t> {
    enum { result = 8 };
};
template <> struct type2int_base<uint32_t> {
    enum { result = 10 };
};
template <> struct type2int_base<unsigned int> {
    enum { result = 10 };
};
template <> struct type2int_base<int8_t> {
    enum { result = 5 };
};
template <> struct type2int_base<int16_t> {
    enum { result = 7 };
};
template <> struct type2int_base<int32_t> {
    enum { result = 9 };
};
template <> struct type2int_base<int> {
    enum { result = 9 };
};
template <> struct type2int_base<float> {
    enum { result = 11 };
};
template <> struct type2int_base<double> {
    enum { result = 12 };
};
template <> struct type2int_base<char> {
    enum { result = 4 };
};
template <> struct type2int_base<String> {
    enum { result = 4 };
};


template <typename T>
struct type2int_base<Keys<T>> {
    enum { result = type2int_base<T>::result };
};

template <typename T>
struct type2int_base<Keys<T>*> {
    enum { result = type2int_base<T>::result };
};

template <typename T>
struct type2int_base<List<T>> {
    enum { result = type2int_base<T>::result };
};

template <typename T>
struct type2int_base<List<T>*> {
    enum { result = type2int_base<T>::result };
};


#endif // TYPE_TRAITS_H