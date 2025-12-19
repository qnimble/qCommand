#include "smartData.h"
#include "qCommand.h"



template <class DataType>
void SmartData<DataType, false>::setImpl(ValueType newValue) {
	const ValueType tempValue = applySetterIfAny(newValue, this->value);
	bool changed = (tempValue != this->value);

    if constexpr (std::is_floating_point_v<ValueType>) {
        if (std::isnan(tempValue) && std::isnan(this->value)) {
            changed = false;
        }
    }

    if (changed) {
		this->value = tempValue;
		this->requestUpdate = true;
	}
}

template <class DataType>
void SmartData<DataType, true>::set(
	typename SmartData<DataType, true>::baseType data, size_t element) {
	size_t total = totalElements;

	if ((qC_parent != nullptr) && (cmdNumber != 255)) {
		total = qC_parent->getCommandSize(cmdNumber) / sizeof(baseType);
	}

	if (element < total) {
		value[element] = data;
		currentElement = element + 1;  // set currentElement to next element
	}
}

template <class DataType>
uint16_t SmartData<DataType, true>:: setActiveSize(uint16_t newSize) {
	if ((qC_parent != nullptr)  && (cmdNumber != 255)) {
		if (newSize > totalElements) {
			newSize = totalElements;
		}

		if (newSize > std::numeric_limits<uint16_t>::max() / sizeof(baseType)) {
			newSize = std::numeric_limits<uint16_t>::max() / sizeof(baseType);
		}

		qC_parent->setCommandSize(cmdNumber, newSize * sizeof(baseType));
		currentElement = 0; //reset array position.
		return newSize;
	}
	return totalElements;
}


template <class DataType>
size_t SmartData<DataType, true>::getActiveSize(void) {
	size_t total = totalElements;
	if ((qC_parent != nullptr) && (cmdNumber != 255)) {
		total = qC_parent->getCommandSize(cmdNumber) / sizeof(baseType);
	}
	return total;
}


template <class DataType>
bool SmartData<DataType, true>::isFull(void){
	size_t total = getActiveSize();
	if (currentElement >= total) {
		return true;
	} else {
		return false;
	}
}

template <class DataType>
bool SmartData<DataType, true>::setNext(
	typename SmartData<DataType, true>::baseType data) {
	size_t total = getActiveSize();

	if (currentElement < total) {
		value[currentElement] = data;
		currentElement++;
		if (currentElement == total) {
			// if this was last element, then
			this->requestUpdate = true;
		}
		return true;  // updated data
	} else {
		return false;  // data full
	}
	//}
}
// SmartData<Option<uint8_t>*> testMap(myMap);

#define INSTANTIATE_SMARTDATA(TYPE)         \
	template class SmartData<TYPE>;         \
	template class SmartData<Option<TYPE> *>; \
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
