
#include <regset.h>

VanadisRegisterSet::VanadisRegisterSet(uint32_t thrID,
	uint32_t rCount, uint32_t rWidth) :
	registerCount(rCount), registerWidthBytes(rWidth),
	threadID(thrID)
{
	// Registers must be at least 64-bit
	assert(rWidth >= 8);

	// We must have at least two registers
	assert(rCount >= 2);

	// Allocate the register space
	register_contents = (char*) malloc(sizeof(char) * rCount * rWidth);
}

uint32_t VanadisRegisterSet::getThreadID() {
	return threadID;
}

void copy(void* dest, const void* src, uint32_t length) {
	for(uint32_t i = 0; i < length; ++i) {
		dest[i] = src[i];
	}
}

uint32_t calculateIndex(const uint32_t index, const uint32_t lane, const uint32_t dataWidthBytes) {
#ifdef VANADIS_ENABLE_EXPENSIVE_REGISTER_CHECKS
	assert(index < registerCount);
	assert(lane < (registerWidthBytes / dataWidthBytes);
#endif
	return (index * registerWidthBytes) + (lane * dataWidth);
}

int8_t VanadisRegisterSet::getInt8RegisterValue(uint32_t index, uint32_t lane) {
	int8_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

int16_t VanadisRegisterSet::getInt16RegisterValue(uint32_t index, uint32_t lane) {
	int16_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

int32_t VanadisRegisterSet:getInt32RegisterValue(uint32_t index, uint32_t lane) {
	int32_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

int64_t VanadisRegisterSet::getInt64RegisterValue(uint32_t index, uint32_t lane) {
	int64_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

uint8_t VanadisRegisterSet::getUInt8RegisterValue(uint32_t index, uint32_t lane) {
	uint8_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ]), sizeof(val);

	return val;
}

uint16_t VanadisRegisterSet::getUInt16RegisterValue(uint32_t index, uint32_t lane) {
	uint16_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

uint32_t VanadisRegisterSet::getUInt32RegisterValue(uint32_t index, uint32_t lane) {
	uint32_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

uint64_t VanadisRegisterSet::getUInt64RegisterValue(uint32_t index, uint32_t lane) {
	uint64_t val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

double VanadisRegisterSet::getDoubleRegisterValue(uint32_t index, uint32_t lane) {
	double val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

float VanadisRegisterSet::getDoubleRegisterValue(uint32_t index, uint32_t lane) {
	float val = 0;
	copy(&val, &register_contents[ calculateIndex(index, lane, sizeof(val)) ], sizeof(val));

	return val;
}

void VanadisRegisterSet::setInt8RegisterValue(uint32_t index, uint32_t lane, int8_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setInt16RegisterValue(uint32_t index, uint32_t lane, int16_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setInt32RegisterValue(uint32_t index, uint32_t lane, int32_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setInt64RegisterValue(uint32_t index, uint32_t lane, int64_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setUInt8RegisterValue(uint32_t index, uint32_t lane, uint8_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setUInt16RegisterValue(uint32_t index, uint32_t lane, uint16_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setUInt32RegisterValue(uint32_t index, uint32_t lane, uint32_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setUInt64RegisterValue(uint32_t index, uint32_t lane, uint64_t value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setDoubleRegisterValue(uint32_t index, uint32_t lane, double value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}

void VanadisRegisterSet::setFloatRegisterValue(uint32_t index, uint32_t lane, float value) {
	copy(&register_contents[ calculateIndex(index, lane, value) ], &values, sizeof(value));
}
