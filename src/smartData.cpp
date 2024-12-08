#include "smartData.h"

void Base::sendUpdate(void) {    
    if ((updates_needed == STATE_IDLE) || (updates_needed == STATE_NEED_TOSEND)) {
        updates_needed = STATE_NEED_TOSEND;
    } else {
        updates_needed =
            STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on
                                          // ACK, so now that plus queue
    }    
}

template <class SmartDataGeneric>
void SmartData<SmartDataGeneric, false>::set(SmartDataGeneric newValue) {    
    value = newValue;
    sendUpdate();    
}

template <class DataType>
void SmartData<DataType, true>::setNext(
    typename SmartData<DataType, true>::baseType data) {    
    if (dataRequested) {
        if (currentElement < totalElements) {
            value[currentElement] = data;
            currentElement++;
            if (currentElement == totalElements) {
                // if this was last element, then
                sendUpdate();
            }
        }
    }
}

template class SmartData<bool>;
template class SmartData<bool *>;

template class SmartData<char>;
template class SmartData<char *>;

template class SmartData<uint8_t>;
template class SmartData<uint8_t *>;

template class SmartData<uint16_t>;
template class SmartData<uint16_t *>;
//template class SmartData<uint16_t (&)[50]>;
//template class SmartData<uint16_t (&),true>;

template class SmartData<uint>;
template class SmartData<uint *>;

template class SmartData<ulong>;
template class SmartData<ulong *>;

template class SmartData<int8_t>;
template class SmartData<int8_t *>;

template class SmartData<int16_t>;
template class SmartData<int16_t *>;

template class SmartData<int>;
template class SmartData<int *>;

template class SmartData<long>;
template class SmartData<long *>;

template class SmartData<float>;
template class SmartData<float *>;
//template class SmartData<float (&)[50]>;
//template class SmartData<float (&),true>;




template class SmartData<double>;
template class SmartData<double *>;

template class SmartData<String>;

// template void cw_pack<uint8_t>(cw_pack_context*, uint8_t);
// template void cw_pack<unsigned char>(cw_pack_context*, unsigned char);
// template bool DataObjectSpecific<bool>::get(void);
// template float DataObjectSpecific<float>::get(void);
// template void cw_pack<unsigned char>(cw_pack_context*, unsigned char);
// template void SmartData<unsigned char>::cw_pack(cw_pack_context*, unsigned
// char);

//void uglytest(cw_pack_context *cw) {
//    cw_pack<unsigned char>(cw, (unsigned char)50);
//}

//void uglytest2(cw_pack_context *cw) { cw_pack<uint16_t>(cw, (uint16_t)50); }
