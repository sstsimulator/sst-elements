// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MLM_H
#define _MLM_H

#include <vector>
#include <utility>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <inttypes.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

using namespace std;
typedef int mlm_Tag;


void* mlm_malloc(size_t size, int level);
void  mlm_free(void* ptr);
mlm_Tag mlm_memcpy(void* dest, void* src, size_t length);
void mlm_waitComplete(mlm_Tag in);
void mlm_set_pool(int pool);
void ariel_enable();
void ariel_fence();

class RTL_shmem_info {

  //  public:
        /*RTL input information and control information corresponds to type(std::string) and size (in bytes) to which the void pointer will be casted to. Order of each stored input and control values should be maintained in accordance with the stored information in the TYPEINFO(std::array). Increment of void pointer once casted will align with the stored inp/ctrl data type. Any wrong cast can corrupt all the input and control signal information. */
    private:      
        void* rtl_inp_ptr;
        void* rtl_ctrl_ptr;
        void* updated_rtl_params;
        size_t rtl_inp_size, rtl_ctrl_size, params_size;

    public:

        RTL_shmem_info(size_t inp_size, size_t ctrl_size) {
            rtl_inp_size = inp_size;
            rtl_ctrl_size = ctrl_size;
            rtl_inp_ptr = mlm_malloc(inp_size, 0); 
            rtl_ctrl_ptr = mlm_malloc(ctrl_size, 0); 
            params_size = (7 * sizeof(bool) + sizeof(uint64_t));
            updated_rtl_params = mlm_malloc(params_size, 0);
            std::cout << "\nNew memory allocated at: ";
        }
        ~RTL_shmem_info() {
            fprintf(stderr, "\nDeleting Shared Memory..\n");
            free(rtl_inp_ptr);
            free(rtl_ctrl_ptr);
            free(updated_rtl_params);
        }
        size_t get_inp_size() { return rtl_inp_size; }
        size_t get_ctrl_size() { return rtl_ctrl_size; }
        size_t get_params_size() { return params_size; }
        void* get_inp_ptr() { return rtl_inp_ptr; }
        void* get_ctrl_ptr() { return rtl_ctrl_ptr; }
        void* get_updated_rtl_params() { return updated_rtl_params; }
};

class Update_RTL_Params {
    
    private:
        bool update_inp, update_ctrl, update_eval_args, update_reg, verbose, done_reset, sim_done;
        uint64_t sim_cycles;
    
    public:
        Update_RTL_Params(bool inp = true, bool ctrl = true, bool eval_args = true, bool reg = false, bool verb = true, bool done_rst = false, bool done = false, uint64_t cycles = 5) {
            update_inp = inp;
            update_ctrl = ctrl;
            update_eval_args = eval_args;
            update_reg = reg;
            verbose = verb;
            done_reset = done_rst;
            sim_done = done;
            sim_cycles = cycles;
            std::cout << "\nUpdating RTL Params to default\n";
        }

        void perform_update(bool inp, bool ctrl, bool eval_args, bool reg, bool verb, bool done_rst, bool done, uint64_t cycles) {
            update_inp = inp;
            update_ctrl = ctrl;
            update_eval_args = eval_args;
            update_reg = reg;
            verbose = verb;
            done_reset = done_rst;
            sim_done = done;
            sim_cycles = cycles;

            std::cout <<"\nPerforming update on RTL params\n";

            return;
        }

        void storetomem(RTL_shmem_info* shmem) {
            std::cout << "\nStore to mem called\n";
            bool* Ptr = (bool*)shmem->get_updated_rtl_params();
            *Ptr = update_inp;
            Ptr++;
            *Ptr = update_ctrl;
            Ptr++;
            *Ptr = update_eval_args;
            Ptr++;
            *Ptr = update_reg;
            Ptr++;
            *Ptr = verbose;
            Ptr++;
            *Ptr = done_reset;
            Ptr++;
            *Ptr = sim_done;
            Ptr++;
            uint64_t* Cycles_ptr  = (uint64_t*)Ptr;
            *Cycles_ptr = sim_cycles; 
            std::cout << "\nStore to mem finished \n"; 
            return;
        }

        void check(RTL_shmem_info* shmem) {

            bool* Ptr = (bool*)shmem->get_updated_rtl_params();
            std::cout << "\n";
            std::cout << *Ptr << " ";
            Ptr++;
            std::cout << *Ptr << " ";
            Ptr++;
            std::cout << *Ptr <<" ";
            Ptr++;
            std::cout << *Ptr <<" ";
            Ptr++;
            std::cout << *Ptr <<" ";
            Ptr++;
            std::cout << *Ptr <<" ";
            Ptr++;
            std::cout << *Ptr <<" ";
            Ptr++;
            uint64_t* Cycles_ptr = (uint64_t*)Ptr;
            std::cout<< *Cycles_ptr <<" ";
            std::cout << "\n";
            return;
        }
};


//Empty dummy function as PIN tool's dynamic instrumentation will replace it with its Ariel Equivalent API in PIN frontend - fesimple.cc (sst-elements/src/sst/elements/ariel/frontend/pin3/)
void start_RTL_sim(RTL_shmem_info* shmem);

//Empty function as PIN tool's dynamic instrumentation will replace it with Ariel Equivalent API in PIN frontend - fesimple.cc (sst-elements/src/sst/elements/ariel/frontend/pin3/)
void update_RTL_sig(RTL_shmem_info* shmem); 

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif //MLM_H
