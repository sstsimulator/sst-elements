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

#ifndef GENERATOR_H
#define GENERATOR_H
#include <systemc.h>
//SC_MODULE(Generator)
struct Generator: ::sc_core::sc_module 
{
  sc_in_clk           clock_;
  sc_out<sc_uint<8> > sc_out_;
  sc_out<uint8_t >    native_out_;
  sc_uint<8>          count;
  void generateOutput();

  //SC_CTOR(Generator)
  typedef Generator SC_CURRENT_USER_MODULE;
  Generator( ::sc_core::sc_module_name)
  {
   // //SC_METHOD(generateOutput);
   // declare_method_process( generateOutput_handle,
   //                        "generateOutput",
   //                        SC_CURRENT_USER_MODULE,
   //                        generateOutput);
    {
      ::sc_core::sc_process_handle handle =
          sc_core::sc_get_curr_simcontext()->create_method_process(
              "generateOutput", 
              false, 
              /*SC_MAKE_FUNC_PTR(SC_CURRENT_USER_MODULE, generateOutput),*/
              static_cast<sc_core::SC_ENTRY_FUNC>(
                  &SC_CURRENT_USER_MODULE::generateOutput),
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

#endif //GENERATOR_H
