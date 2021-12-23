// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _RTLEVENT_H
#define _RTLEVENT_H

#include "uint.h"
#include "rtl_header.h"
#include <string>
#include <sst/core/event.h>

namespace SST {
    namespace RtlComponent { 

class RTLEvent : public SST::Event {
public:
    typedef std::vector<char> dataVec;
    bool update_inp, update_ctrl, update_registers, verbose, done_reset,
         update_eval_args, sim_done;
    uint64_t sim_cycles;
    RTLEvent() : SST::Event() { }
    dataVec payload;

    void input_sigs(Rtlheader*);
    void control_sigs(Rtlheader*);
    void UpdateRtlSignals(void*, Rtlheader*, uint64_t&);
    void *inp_ptr, *ctrl_ptr;

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        SST::Event::serialize_order(ser);
        ser & payload;
    }

    /*std::string serialization_name() const {
        
        std::string s("defining virtual function!");
        return s;
    }

    const char* cls_name() const { 
        char a = 'A';
        const char* c = &a;
        return c; 
    }

    uint32_t cls_id() const {
        const uint32_t a = 10;
        return a;
    }*/

    ImplementSerializable(SST::RtlComponent::RTLEvent);

    //Cast all the variables to 4 byte integer types for uniform storage for now. Later, we either will remove UInt and SInt and use native types. Even then we would need to cast the every variables based on type, width and order while storing in shmem and accordinly access it at runtime from shmem. Looks dificult task as of now.    
    /*void* input_ctrl_registers;
    UInt<4>* inp_ctrl_reg;*/
};

} //namespace RtlComponent
} //namespace SST

#endif /* _RTLEVENT_H */
