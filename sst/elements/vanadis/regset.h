
#ifndef _H_VANADIS_REG_SET
#define _H_VANADIS_REG_SET

namespace SST {
namespace Vanadis {

class RegisterSet {

   private:
      void* fp_reg;
      void* int_reg;
      uint32_t threadID;

   public:
      RegisterSet(uint32_t fpRegisterWidth,
		uint32_t fpRegCount,
		uint32_t intRegisterWidth,
		uint32_t intRegCount,
		uint32_t threadID);

      uint32_t getThreadID();

      int8_t  getInt8RegisterValue(uint32_t index, uint32_t lane);
      int16_t getInt16RegisterValue(uint32_t index, uint32_t lane);
      int32_t getInt32RegisterValue(uint32_t index, uint32_t lane);
      int64_t getInt64RegisterValue(uint32_t index, uint32_t lane);

      uint8_t getUInt8RegisterValue(uint32_t index, uint32_t lane);
      uint16_t getUInt16RegisterValue(uint32_t index, uint32_t lane);
      uint32_t getUInt32RegisterValue(uint32_t index, uint32_t lane);
      uint64_t getUInt64RegisterValue(uint32_t index, uint32_t lane);

      double  getDoubleRegisterValue(uint32_t index, uint32_t lane);
      float   getFloatRegisterValue(uint32_t index, uint32_t lane);

      void setInt8RegisterValue(uint32_t index, uint32_t lane, int8_t value);
      void setInt16RegisterValue(uint32_t index, uint32_t lane, int16_t value);
      void setInt32RegisterValue(uint32_t index, uint32_t lane, int32_t value);
      void setInt64RegisterValue(uint32_t index, uint32_t lane, int64_t value);

      void setUInt8RegisterValue(uint32_t index, uint32_t lane, uint8_t value);
      void setUInt16RegisterValue(uint32_t index, uint32_t lane, uint16_t value);
      void setUInt32RegisterValue(uint32_t index, uint32_t lane, uint32_t value);
      void setUInt64RegisterValue(uint32_t index, uint32_t lane, uint64_t value);

      void setDoubleRegisterValue(uint32_t index, uint32_t lane, double value);
      void setFloatRegisterValue(uint32_t index, uint32_t lane, float value);

}

}
}

#endif
