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

#include "uint.h"
#include "sint.h"
#include "rtl_header.h"
#include "rtlevent.h"

using namespace SST;
using namespace SST::RtlComponent;

void RTLEvent::UpdateRtlSignals(void *update_data, Rtlheader* cmodel, uint64_t& cycles) {
    bool* update_rtl_params = (bool*)update_data; 
    update_inp = update_rtl_params[0];
    update_ctrl = update_rtl_params[1];
    update_eval_args = update_rtl_params[2];
    update_registers = update_rtl_params[3];
    verbose = update_rtl_params[4];
    done_reset = update_rtl_params[5];
    sim_done = update_rtl_params[6];
    uint64_t* cycles_ptr = (uint64_t*)(&update_rtl_params[7]);
    sim_cycles = *cycles_ptr;
    cycles = sim_cycles;
    cycles_ptr++;

    output.verbose(CALL_INFO, 1, 0, "sim_cycles: %\n" PRIu64, sim_cycles);
    output.verbose(CALL_INFO, 1, 0, "update_inp: %d\n", update_inp);
    output.verbose(CALL_INFO, 1, 0, "update_ctrl: %d\n", update_ctrl);
    if(update_inp) {
        inp_ptr =  (void*)cycles_ptr; 
        input_sigs(cmodel);
    }

    if(update_ctrl) {
        UInt<4>* rtl_inp_ptr = (UInt<4>*)inp_ptr;
        ctrl_ptr = (void*)(&rtl_inp_ptr[5]);
        control_sigs(cmodel);
    }
}

void RTLEvent::input_sigs(Rtlheader* cmodel) {

    cmodel->reset = UInt<1>(1);
    //Cast all the variables to 4 byte UInt types for uniform storage for now. Later, we either will remove UInt and SInt and use native types. Even then we would need to cast the every variables based on type, width and order while storing in shmem and accordinly access it at runtime from shmem.   
    UInt<4>* rtl_inp_ptr = (UInt<4>*)inp_ptr;
    cmodel->io_ins_0 = rtl_inp_ptr[0];
    cmodel->io_ins_1 = rtl_inp_ptr[1];
    cmodel->io_ins_2 = rtl_inp_ptr[2];
    cmodel->io_ins_3 = rtl_inp_ptr[3];
    cmodel->reset = UInt<1>(0);
    output.verbose(CALL_INFO, 1, 0, "input_sigs: %lu", cmodel->io_ins_0);
    output.verbose(CALL_INFO, 1, 0, "input_sigs: %lu", cmodel->io_ins_1);
    output.verbose(CALL_INFO, 1, 0, "input_sigs: %lu", cmodel->io_ins_2);
    output.verbose(CALL_INFO, 1, 0, "input_sigs: %lu", cmodel->io_ins_3);
    return;
}

void RTLEvent::control_sigs(Rtlheader* cmodel) {

    output.verbose(CALL_INFO, 1, 0, "\nctrl_sigs called"); 
    cmodel->reset = UInt<1>(1);
    UInt<1>* rtl_ctrl_ptr = (UInt<1>*)ctrl_ptr;
    cmodel->io_shift = rtl_ctrl_ptr[0];
    cmodel->io_load = rtl_ctrl_ptr[1];
    cmodel->reset = UInt<1>(0);
    output.verbose(CALL_INFO, 1, 0, "ctrl_sigs: %lu", cmodel->io_shift);
    output.verbose(CALL_INFO, 1, 0, "ctrl_sigs: %lu", cmodel->io_load);
    return;
}

