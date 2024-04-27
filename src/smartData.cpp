#include "smartData.h"

template <class SmartDataGeneric>
SmartDataGeneric DataObjectSpecific<SmartDataGeneric>::get(void) {
  return value;
}

template <class SmartDataGeneric>
void DataObjectSpecific<SmartDataGeneric>::set(SmartDataGeneric newValue) {
  value = newValue;
}

//template <class SmartDataGeneric>
//DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific<SmartDataGeneric>() {}

template <class SmartDataGeneric>
DataObjectSpecific<SmartDataGeneric>::DataObjectSpecific(SmartDataGeneric initValue): value(initValue) {}



template class DataObjectSpecific<bool>;
template class DataObjectSpecific<uint8_t>;
template class DataObjectSpecific<uint16_t>;
template class DataObjectSpecific<uint>;
template class DataObjectSpecific<ulong>;
template class DataObjectSpecific<int8_t>;
template class DataObjectSpecific<int16_t>;
template class DataObjectSpecific<int>;
template class DataObjectSpecific<long>;
template class DataObjectSpecific<float>;
template class DataObjectSpecific<double>;


//template bool DataObjectSpecific<bool>::get(void);
//template float DataObjectSpecific<float>::get(void);
