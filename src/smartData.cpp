#include "smartData.h"

void Base::sendUpdate(void) {
    switch (updates_needed){
        case STATE_IDLE:
            updates_needed = STATE_NEED_TOSEND;
            break;
        case STATE_NEED_TOSEND:
            //alread in need to send state, do nothing
            break;
        case STATE_WAIT_ON_ACK:
            updates_needed = STATE_WAIT_ON_ACK_PLUS_QUEUE; // remaining states were waiting on
                                                          // ACK, so now that plus queue
            break;
        case STATE_WAIT_ON_ACK_PLUS_QUEUE:
            //already in wait on ack plus queue, do nothing
            break;
        default:
            break;
        }
}

template <class DataType>
void SmartData<DataType, false>::set(DataType newValue) {
    if (setter != nullptr) {
        value = setter(newValue, value);
    } else {
        value = newValue;
    }
    sendUpdate();
}


template <class DataType>
void SmartData<DataType, true>::set(
    typename SmartData<DataType, true>::baseType data, size_t element) {
    if (element < totalElements) {
        value[element] = data;
        currentElement = element + 1; // set currentElement to next element
    }
}

template <class DataType>
bool SmartData<DataType, true>::setNext(
    typename SmartData<DataType, true>::baseType data) {
    // if (dataRequested) {
    if (currentElement < totalElements) {
        value[currentElement] = data;
        currentElement++;
        if (currentElement == totalElements) {
            // if this was last element, then
            sendUpdate();
        }
        return true; // updated data
    } else {
        return false; // data full
    }
    //}
}

#define INSTANTIATE_SMARTDATA(TYPE)                                            \
    template class SmartData<TYPE>;                                            \
    template class SmartData<TYPE *>;

INSTANTIATE_SMARTDATA(bool);
INSTANTIATE_SMARTDATA(char);

INSTANTIATE_SMARTDATA(uint8_t);
INSTANTIATE_SMARTDATA(int8_t);
INSTANTIATE_SMARTDATA(uint16_t);
INSTANTIATE_SMARTDATA(int16_t);
INSTANTIATE_SMARTDATA(uint32_t);
INSTANTIATE_SMARTDATA(int32_t);
INSTANTIATE_SMARTDATA(uint);
INSTANTIATE_SMARTDATA(int);


INSTANTIATE_SMARTDATA(float);
INSTANTIATE_SMARTDATA(double);

INSTANTIATE_SMARTDATA(String);
