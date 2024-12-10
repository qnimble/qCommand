#ifndef TYPE_TRAITS_H
#define TYPE_TRAITS_H

#include <type_traits>
#include <cstdint>
#include <WString.h>


// Set Default values for TypeTraits
template <typename T, typename Enable = void> struct TypeTraits {
    static constexpr bool isArray = false;
    static constexpr bool isPointer = false;
    static constexpr bool isReference = false;
    static constexpr bool isBool = false;
};

// Specialization for pointer types (treated as pointers to arrays)
template <typename T>
struct TypeTraits<T, std::enable_if_t<std::is_pointer<T>::value>> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = true;
    static constexpr bool isBool = false;
};

// Specialization for reference types that are not arrays. Set to arrays anyway.
template <typename T>
struct TypeTraits<
    T, std::enable_if_t<std::is_reference<T>::value &&
                        !std::is_array<std::remove_reference_t<T>>::value>> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = true;
    static constexpr bool isArray = true;
    static constexpr bool isBool = false;
};

// Specialization for references to arrays
template <typename T>
struct TypeTraits<
    T, std::enable_if_t<std::is_reference<T>::value &&
                        std::is_array<std::remove_reference_t<T>>::value>> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = true;
    static constexpr bool isArray = true;
    static constexpr bool isBool = false;
};

template <> struct TypeTraits<String> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
    static constexpr bool isBool = false;
};

// Specialization for const char*
template <> struct TypeTraits<const char *> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
    static constexpr bool isBool = false;
};

// Specialization for char*
template <> struct TypeTraits<char *> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
    static constexpr bool isBool = false;
};

// Specialization for bool
template <> struct TypeTraits<bool> {
    static constexpr bool isPointer = false;
    static constexpr bool isReference = false;
    static constexpr bool isArray = false;
    static constexpr bool isBool = true;
};

// Specialization for bool
template <> struct TypeTraits<bool *> {
    static constexpr bool isPointer = true;
    static constexpr bool isReference = false;
    static constexpr bool isArray = true;
    static constexpr bool isBool = true;
};

static_assert(!TypeTraits<int>::isArray,
              "int should not be considered an array");
static_assert(TypeTraits<float *>::isArray,
              "float* should be considered an array");
static_assert(TypeTraits<double *>::isArray,
              "double* should be considered an array");
static_assert(TypeTraits<double &>::isReference,
              "double& should be considered a reference");
// static_assert(TypeTraits<double&>::isArray,
//"double& should be considered a reference");
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


template <typename T>
struct type2int_base; // force defintion by leaving out result for default case
template <> struct type2int_base<bool> {
    enum { result = 6 };
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


#endif // TYPE_TRAITS_H