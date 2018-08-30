// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_WRITE_EVENT
#define _H_SST_ARIEL_WRITE_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielWriteEvent : public ArielEvent {

    public:
        ArielWriteEvent(uint64_t wAddr, uint32_t length, const uint8_t* payloadData) :
                writeAddress(wAddr), writeLength(length) {
                
                payload = new uint8_t[length];
                
                for( int i = 0; i < length; ++i ) {
                	payload[i] = payloadData[i];
                }
        }

        ~ArielWriteEvent() {
        	delete[] payload;
        }

        ArielEventType getEventType() const {
                return WRITE_ADDRESS;
        }

        uint64_t getAddress() const {
                return writeAddress;
        }

        uint32_t getLength() const {
                return writeLength;
        }
        
        uint8_t* getPayload() const {
        		return payload;
        }

    private:
        const uint64_t writeAddress;
        const uint32_t writeLength;
              uint8_t* payload;

};

}
}

#endif
