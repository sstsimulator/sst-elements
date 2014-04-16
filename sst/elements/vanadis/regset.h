// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_VANADIS_REG_SET
#define _H_VANADIS_REG_SET


#include <stdint.h>

namespace SST {
namespace Vanadis {

class VanadisRegisterSet {

   protected:
      char* register_contents;
      const uint32_t registerWidthBytes;
      const uint32_t registerCount;
      const uint32_t threadID;

      inline uint32_t calculateIndex(uint32_t index, uint32_t lane);
      inline void copy(void* dest, const void* src, uint32_t length);

   public:
      VanadisRegisterSet(
		uint32_t threadID,
		uint32_t registerCount,
		uint32_t registerWidthBytes
		);

      uint32_t getThreadID();

      int8_t  getInt8RegisterValue(uint32_t index, uint32_t lane);
      int16_t getInt16RegisterValue(uint32_t index, uint32_t lane);
      int32_t getInt32RegisterValue(uint32_t index, uint32_t lane);
      int64_t getInt64RegisterValue(uint32_t index, uint32_t lane);

      uint8_t  getUInt8RegisterValue(uint32_t index, uint32_t lane);
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

};

}
}

#endif
