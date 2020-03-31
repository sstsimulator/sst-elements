// Copyright 2009-2020 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_GPU_EVENT
#define _H_SST_ARIEL_GPU_EVENT

#include "arielevent.h"
#include "ariel_shmem.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielGpuEvent : public ArielEvent {
   private:
      GpuApi_t api;
      CudaArguments ca;

   public:
      ArielGpuEvent(GpuApi_t API, CudaArguments CA) {api = API; ca = CA;}
      ~ArielGpuEvent() {}

      ArielEventType getEventType() const {
         return GPU;
      }

      GpuApi_t getGpuApi() {
         return api;
      }

      unsigned getFatCubinHandle() {
         return ca.register_function.fat_cubin_handle;
      }

      char* getDeviceFun() {
         return ca.register_function.device_fun;
      }

      uint64_t getHostFun() {
         return ca.register_function.host_fun;
      }

      void** getDevPtr() {
         return ca.cuda_malloc.dev_ptr;
      }

      size_t getSize() {
         printf("%d arielgpuevent\n", ca.cuda_malloc.size);
         return ca.cuda_malloc.size;
      }

      uint64_t get_src() {
         return ca.cuda_memcpy.src;
      }

      uint64_t get_dst() {
         return ca.cuda_memcpy.dst;
      }
      size_t get_count() {
         return ca.cuda_memcpy.count;
      }

      cudaMemcpyKind get_kind() {
         return ca.cuda_memcpy.kind;
      }

      uint8_t* get_data() {
         return ca.cuda_memcpy.data;
      }

      unsigned int get_gridDimx() {
         return ca.cfg_call.gdx;
      }

      unsigned int get_gridDimy() {
         return ca.cfg_call.gdy;
      }

      unsigned int get_gridDimz() {
         return ca.cfg_call.gdz;
      }

      unsigned int get_blockDimx() {
         return ca.cfg_call.bdx;
      }

      unsigned int get_blockDimy() {
         return ca.cfg_call.bdy;
      }

      unsigned int get_blockDimz() {
         return ca.cfg_call.bdz;
      }

      size_t get_shmem() {
         return ca.cfg_call.sharedMem;
      }

      cudaStream_t get_stream() {
         return ca.cfg_call.stream;
      }

      uint64_t get_address() {
         return ca.set_arg.address;
      }

      uint8_t* get_value() {
         return ca.set_arg.value;
      }

      size_t get_size() {
         return ca.set_arg.size;
      }

      size_t get_offset() {
         return ca.set_arg.offset;
      }

      uint64_t get_func(){
         return ca.cuda_launch.func;
      }

      uint64_t get_free_addr(){
         return ca.free_address;
      }

      unsigned getVarFatCubinHandle() {
         return ca.register_var.fatCubinHandle;
      }

      uint64_t getVarHostVar() {
         return ca.register_var.hostVar;
      }

      char* getVarDeviceName() {
         return ca.register_var.deviceName;
      }

      int getVarExt() {
         return ca.register_var.ext;
      }

      int getVarSize() {
         return ca.register_var.size;
      }

      int getVarConstant() {
         return ca.register_var.constant;
      }

      int getVarGlobal() {
         return ca.register_var.global;
      }

      uint64_t getMaxBlockHostFunc() {
         return ca.max_active_block.hostFunc;
      }

      uint64_t getMaxBlockBlockSize() {
         return ca.max_active_block.blockSize;
      }

      uint64_t getMaxBlockSMemSize() {
         return ca.max_active_block.dynamicSMemSize;
      }

      uint64_t getMaxBlockFlag() {
         return ca.max_active_block.flags;
      }
};

}
}

#endif
