#include "smartData.h"

template <class SmartDataGeneric>
SmartDataGeneric SmartData<SmartDataGeneric>::get(void) {
  return value;
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric>::set(SmartDataGeneric newValue) {
  value = newValue;
}

//template <class SmartDataGeneric>
//DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>() {}

template <class SmartDataGeneric>
SmartData<SmartDataGeneric>::SmartData(SmartDataGeneric initValue): value(initValue) {}



template class SmartData<bool>;
template class SmartData<uint8_t>;
template class SmartData<uint16_t>;
template class SmartData<uint>;
template class SmartData<ulong>;
template class SmartData<int8_t>;
template class SmartData<int16_t>;
template class SmartData<int>;
template class SmartData<long>;
template class SmartData<float>;
template class SmartData<double>;


//template bool DataObjectSpecific<bool>::get(void);
//template float DataObjectSpecific<float>::get(void);
