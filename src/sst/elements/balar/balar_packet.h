// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef BALAR_PACKET_H
#define BALAR_PACKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "builtin_types.h"
#include "driver_types.h"
#include "balar_consts.h"
#include "balar_packet_wire.h"

namespace SST {
namespace BalarComponent {

    // Function to get the string representation of the enum values
    inline const char* CudaAPIEnumToString(CudaAPI_t api) {
        switch (api) {
            case CUDA_REG_FAT_BINARY: return "CUDA_REG_FAT_BINARY";
            case CUDA_REG_FUNCTION: return "CUDA_REG_FUNCTION";
            case CUDA_MEMCPY: return "CUDA_MEMCPY";
            case CUDA_CONFIG_CALL: return "CUDA_CONFIG_CALL";
            case CUDA_SET_ARG: return "CUDA_SET_ARG";
            case CUDA_LAUNCH: return "CUDA_LAUNCH";
            case CUDA_FREE: return "CUDA_FREE";
            case CUDA_GET_LAST_ERROR: return "CUDA_GET_LAST_ERROR";
            case CUDA_MALLOC: return "CUDA_MALLOC";
            case CUDA_REG_VAR: return "CUDA_REG_VAR";
            case CUDA_MAX_BLOCK: return "CUDA_MAX_BLOCK";
            case CUDA_PARAM_CONFIG: return "CUDA_PARAM_CONFIG";
            case CUDA_THREAD_SYNC: return "CUDA_THREAD_SYNC";
            case CUDA_GET_ERROR_STRING: return "CUDA_GET_ERROR_STRING";
            case CUDA_MEMSET: return "CUDA_MEMSET";
            case CUDA_MEMCPY_TO_SYMBOL: return "CUDA_MEMCPY_TO_SYMBOL";
            case CUDA_MEMCPY_FROM_SYMBOL: return "CUDA_MEMCPY_FROM_SYMBOL";
            case CUDA_SET_DEVICE: return "CUDA_SET_DEVICE";
            case CUDA_CREATE_CHANNEL_DESC: return "CUDA_CREATE_CHANNEL_DESC";
            case CUDA_BIND_TEXTURE: return "CUDA_BIND_TEXTURE";
            case CUDA_REG_TEXTURE: return "CUDA_REG_TEXTURE";
            case CUDA_GET_DEVICE_COUNT: return "CUDA_GET_DEVICE_COUNT";
            case CUDA_FREE_HOST: return "CUDA_FREE_HOST";
            case CUDA_MALLOC_HOST: return "CUDA_MALLOC_HOST";
            case CUDA_MEMCPY_ASYNC: return "CUDA_MEMCPY_ASYNC";
            case CUDA_GET_DEVICE_PROPERTIES: return "CUDA_GET_DEVICE_PROPERTIES";
            case CUDA_SET_DEVICE_FLAGS: return "CUDA_SET_DEVICE_FLAGS";
            case CUDA_STREAM_CREATE: return "CUDA_STREAM_CREATE";
            case CUDA_STREAM_DESTROY: return "CUDA_STREAM_DESTROY";
            case CUDA_STREAM_SYNC: return "CUDA_STREAM_SYNC";
            case CUDA_EVENT_CREATE: return "CUDA_EVENT_CREATE";
            case CUDA_EVENT_CREATE_WITH_FLAGS: return "CUDA_EVENT_CREATE_WITH_FLAGS";
            case CUDA_EVENT_RECORD: return "CUDA_EVENT_RECORD";
            case CUDA_EVENT_SYNCHRONIZE: return "CUDA_EVENT_SYNCHRONIZE";
            case CUDA_EVENT_ELAPSED_TIME: return "CUDA_EVENT_ELAPSED_TIME";
            case CUDA_EVENT_DESTROY: return "CUDA_EVENT_DESTROY";
            case CUDA_DEVICE_GET_ATTRIBUTE: return "CUDA_DEVICE_GET_ATTRIBUTE";
            default: return "Unknown CudaAPI_t";
        }
    }

    inline bool isSameBalarCudaCallPacket(BalarCudaCallPacket_t* p1, BalarCudaCallPacket_t* p2, bool ignore_sstmem_check) {
        bool res = true;
        res &= p1->cuda_call_id == p2->cuda_call_id;
        res &= (ignore_sstmem_check || (p1->isSSTmem == p2->isSSTmem));
        switch (p1->cuda_call_id) {
            case CUDA_REG_FAT_BINARY:
                res &= strcmp(p1->register_fatbin.file_name, p2->register_fatbin.file_name) == 0;
            case CUDA_REG_FUNCTION:
                res &= p1->register_function.fatCubinHandle == p2->register_function.fatCubinHandle;
                res &= p1->register_function.hostFun == p2->register_function.hostFun;
                res &= strcmp(p1->register_function.deviceFun, p2->register_function.deviceFun) == 0;
                break;
            case CUDA_MEMCPY:
                res &= p1->cuda_memcpy.dst == p2->cuda_memcpy.dst;
                res &= p1->cuda_memcpy.src == p2->cuda_memcpy.src;
                res &= p1->cuda_memcpy.count == p2->cuda_memcpy.count;
                res &= p1->cuda_memcpy.kind == p2->cuda_memcpy.kind;
                break;
            case CUDA_CONFIG_CALL:
                res &= p1->configure_call.sharedMem == p2->configure_call.sharedMem;
                res &= p1->configure_call.stream == p2->configure_call.stream;
                res &= p1->configure_call.gdx == p2->configure_call.gdx;
                res &= p1->configure_call.gdy == p2->configure_call.gdy;
                res &= p1->configure_call.gdz == p2->configure_call.gdz;
                res &= p1->configure_call.bdx == p2->configure_call.bdx;
                res &= p1->configure_call.bdy == p2->configure_call.bdy;
                res &= p1->configure_call.bdz == p2->configure_call.bdz;
                break;
            case CUDA_SET_ARG:
                res &= p1->setup_argument.arg == p2->setup_argument.arg;
                res &= p1->setup_argument.size == p2->setup_argument.size;
                res &= p1->setup_argument.offset == p2->setup_argument.offset;
                res &= memcmp(p1->setup_argument.value, p2->setup_argument.value, BALAR_CUDA_MAX_ARG_SIZE) == 0;
                break;
            case CUDA_LAUNCH:
                res &= p1->cuda_launch.func == p2->cuda_launch.func;
                break;
            case CUDA_FREE:
                res &= p1->cuda_free.devPtr == p2->cuda_free.devPtr;
                break;
            case CUDA_REG_VAR:
                res &= p1->register_var.fatCubinHandle == p2->register_var.fatCubinHandle;
                res &= strcmp(p1->register_var.hostVar, p2->register_var.hostVar) == 0;
                res &= strcmp(p1->register_var.deviceAddress, p2->register_var.deviceAddress) == 0;
                res &= strcmp(p1->register_var.deviceName, p2->register_var.deviceName) == 0;
                res &= p1->register_var.ext == p2->register_var.ext;
                res &= p1->register_var.size == p2->register_var.size;
                res &= p1->register_var.constant == p2->register_var.constant;
                res &= p1->register_var.global == p2->register_var.global;
                break;
            case CUDA_MAX_BLOCK:
                res &= p1->max_active_block.dynamicSMemSize == p2->max_active_block.dynamicSMemSize;
                res &= p1->max_active_block.numBlock == p2->max_active_block.numBlock;
                res &= p1->max_active_block.hostFunc == p2->max_active_block.hostFunc;
                res &= p1->max_active_block.blockSize == p2->max_active_block.blockSize;
                res &= p1->max_active_block.flags == p2->max_active_block.flags;
                break;
            case CUDA_PARAM_CONFIG:
                res &= p1->cudaparamconfig.hostFun == p2->cudaparamconfig.hostFun;
                res &= p1->cudaparamconfig.index == p2->cudaparamconfig.index;
                break;
            case CUDA_MEMSET:
                res &= p1->cudamemset.mem == p2->cudamemset.mem;
                res &= p1->cudamemset.c == p2->cudamemset.c;
                res &= p1->cudamemset.count == p2->cudamemset.count;
                break;
            case CUDA_SET_DEVICE:
                res &= p1->cudasetdevice.device == p2->cudasetdevice.device;
                break;
            case CUDA_CREATE_CHANNEL_DESC:
                // Add comparison for CUDA_CREATE_CHANNEL_DESC if needed
                break;
            case CUDA_BIND_TEXTURE:
                res &= p1->cudabindtexture.offset == p2->cudabindtexture.offset;
                res &= p1->cudabindtexture.hostVar_ptr == p2->cudabindtexture.hostVar_ptr;
                res &= memcmp(&p1->cudabindtexture.texRef, &p2->cudabindtexture.texRef, sizeof(textureReference)) == 0;
                res &= p1->cudabindtexture.devPtr == p2->cudabindtexture.devPtr;
                res &= memcmp(&p1->cudabindtexture.desc_struct, &p2->cudabindtexture.desc_struct, sizeof(cudaChannelFormatDesc)) == 0;
                res &= p1->cudabindtexture.size == p2->cudabindtexture.size;
                break;
            case CUDA_REG_TEXTURE:
                res &= p1->cudaregtexture.fatCubinHandle == p2->cudaregtexture.fatCubinHandle;
                res &= p1->cudaregtexture.hostVar_ptr == p2->cudaregtexture.hostVar_ptr;
                res &= memcmp(&p1->cudaregtexture.texRef, &p2->cudaregtexture.texRef, sizeof(textureReference)) == 0;
                res &= p1->cudaregtexture.deviceAddress == p2->cudaregtexture.deviceAddress;
                res &= strcmp(p1->cudaregtexture.deviceName, p2->cudaregtexture.deviceName) == 0;
                res &= p1->cudaregtexture.dim == p2->cudaregtexture.dim;
                res &= p1->cudaregtexture.norm == p2->cudaregtexture.norm;
                res &= p1->cudaregtexture.ext == p2->cudaregtexture.ext;
                break;
            case CUDA_GET_DEVICE_COUNT:
                // Add comparison for CUDA_GET_DEVICE_COUNT if needed
                break;
            case CUDA_FREE_HOST:
                // Add comparison for CUDA_FREE_HOST if needed
                break;
            case CUDA_MALLOC_HOST:
                res &= p1->cudamallochost.addr == p2->cudamallochost.addr;
                res &= p1->cudamallochost.size == p2->cudamallochost.size;
                break;
            case CUDA_MEMCPY_ASYNC:
                res &= p1->cudaMemcpyAsync.dst == p2->cudaMemcpyAsync.dst;
                res &= p1->cudaMemcpyAsync.src == p2->cudaMemcpyAsync.src;
                res &= p1->cudaMemcpyAsync.count == p2->cudaMemcpyAsync.count;
                res &= p1->cudaMemcpyAsync.kind == p2->cudaMemcpyAsync.kind;
                res &= p1->cudaMemcpyAsync.stream == p2->cudaMemcpyAsync.stream;
                break;
            case CUDA_GET_DEVICE_PROPERTIES:
                res &= p1->cudaGetDeviceProperties.device == p2->cudaGetDeviceProperties.device;
                break;
            case CUDA_SET_DEVICE_FLAGS:
                res &= p1->cudaSetDeviceFlags.flags == p2->cudaSetDeviceFlags.flags;
                break;
            case CUDA_STREAM_DESTROY:
                res &= p1->cudaStreamDestroy.stream == p2->cudaStreamDestroy.stream;
                break;
            case CUDA_EVENT_CREATE_WITH_FLAGS:
                res &= p1->cudaEventCreateWithFlags.flags == p2->cudaEventCreateWithFlags.flags;
                break;
            case CUDA_EVENT_RECORD:
                res &= p1->cudaEventRecord.event == p2->cudaEventRecord.event;
                res &= p1->cudaEventRecord.stream == p2->cudaEventRecord.stream;
                break;
            case CUDA_EVENT_SYNCHRONIZE:
                res &= p1->cudaEventSynchronize.event == p2->cudaEventSynchronize.event;
                break;
            case CUDA_EVENT_ELAPSED_TIME:
                res &= p1->cudaEventElapsedTime.start == p2->cudaEventElapsedTime.start;
                res &= p1->cudaEventElapsedTime.end == p2->cudaEventElapsedTime.end;
                break;
            case CUDA_EVENT_DESTROY:
                res &= p1->cudaEventDestroy.event == p2->cudaEventDestroy.event;
                break;
            case CUDA_DEVICE_GET_ATTRIBUTE:
                res &= p1->cudaDeviceGetAttribute.attr == p2->cudaDeviceGetAttribute.attr;
                res &= p1->cudaDeviceGetAttribute.device == p2->cudaDeviceGetAttribute.device;
                break;
            default:
                // For calls with no input args, as long
                // as their cuda_call_id matches, we consider them the same
                break;
        }
        return res;
    }
}
}
#endif