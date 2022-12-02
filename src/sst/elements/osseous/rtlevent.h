// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
#include <sst/core/simulation.h>
#include <string>
#include <sst/core/event.h>
using namespace SST;
using namespace std;

namespace SST {
    namespace RtlComponent { 

class RTLEvent : public SST::Event {
public:

    bool update_inp, update_ctrl, update_registers, verbose, done_reset,
         update_eval_args, sim_done;
    uint64_t sim_cycles;
    RTLEvent() : SST::Event() {
        //output.init("RtlEvent-" + getName() + "-> ", 1, 0, SST::Output::STDOUT); 
        output.init("Rtlmodel->", 1, 0, SST::Output::STDOUT);
    }

    void input_sigs(Rtlheader*);
    void control_sigs(Rtlheader*);
    void UpdateRtlSignals(void*, Rtlheader*, uint64_t&);
    void *inp_ptr, *ctrl_ptr;

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        SST::Event::serialize_order(ser);
        ser & update_inp;
        ser & update_ctrl;
        ser & update_registers;
        ser & verbose;
        ser & done_reset;
        ser & update_eval_args;
        ser & sim_done;
    }

    ImplementSerializable(SST::RtlComponent::RTLEvent);
private:
    SST::Output output;


};

} //namespace RtlComponent
} //namespace SST

#endif /* _RTLEVENT_H */
