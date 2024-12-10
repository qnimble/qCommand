#include "smartData.h"

void Base::sendUpdate(void) {
    if ((updates_needed == STATE_IDLE) ||
        (updates_needed == STATE_NEED_TOSEND)) {
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
template class SmartData<String>;

template class SmartData<uint8_t>;
template class SmartData<uint8_t *>;

template class SmartData<uint16_t>;
template class SmartData<uint16_t *>;

template class SmartData<uint32_t>;
template class SmartData<uint32_t *>;

template class SmartData<int8_t>;
template class SmartData<int8_t *>;

template class SmartData<int16_t>;
template class SmartData<int16_t *>;

template class SmartData<int32_t>;
template class SmartData<int32_t *>;

template class SmartData<float>;
template class SmartData<float *>;

template class SmartData<double>;
template class SmartData<double *>;
