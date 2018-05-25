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

#ifndef RECIEVER_H
#define RECIEVER_H
#include <systemc.h>
//SC_MODULE(Reciever)
struct Reciever: ::sc_core::sc_module 
{
  sc_in_clk           clock_;
  sc_in<sc_uint<8> > sc_in_;
  sc_in<uint8_t >    native_in_;
  sc_uint<8>          count;
  void processInput();

  //SC_CTOR(Reciever)
  typedef Reciever SC_CURRENT_USER_MODULE;
  Reciever( ::sc_core::sc_module_name)
  {
   // //SC_METHOD(processInput);
   // declare_method_process( processInput_handle,
   //                        "processInput",
   //                        SC_CURRENT_USER_MODULE,
   //                        processInput);
    {
      ::sc_core::sc_process_handle handle =
          sc_core::sc_get_curr_simcontext()->create_method_process(
              "processInput", 
              false, 
              /*SC_MAKE_FUNC_PTR(SC_CURRENT_USER_MODULE, processInput),*/
              static_cast<sc_core::SC_ENTRY_FUNC>(
                  &SC_CURRENT_USER_MODULE::processInput),
              this,
              0);
      this->sensitive << handle;
      this->sensitive_pos << handle;
      this->sensitive_neg << handle;
    }
    sensitive << clock_.pos();
    count=0;
  }
};

#endif //RECIEVER_H
