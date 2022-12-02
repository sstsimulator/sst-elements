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


#ifndef _H_SST_ARIEL_RTL_EVENT
#define _H_SST_ARIEL_RTL_EVENT

#include "arielevent.h"
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>

using namespace SST;

namespace SST {
namespace ArielComponent {

typedef struct RtlSharedData {

    void* rtl_inp_ptr;
    void* rtl_ctrl_ptr;   
    void* updated_rtl_params;

    std::unordered_map<uint64_t, uint64_t>* pageTable; 
    std::unordered_map<uint64_t, uint64_t>* translationCache; 
    std::deque<uint64_t> *freePages;
    uint64_t pageSize;
    uint64_t cacheLineSize;
    uint32_t translationCacheEntries;
    bool translationEnabled;

    size_t rtl_inp_size;
    size_t rtl_ctrl_size;
    size_t updated_rtl_params_size;
    bool update_params;

}RtlSharedData;

class ArielRtlEvent : public ArielEvent, public SST::Event {
   public:
      RtlSharedData RtlData;
      bool endSim;
      bool EvRecvAck;

      ArielRtlEvent() : Event() {
        RtlData.pageTable = new std::unordered_map<uint64_t, uint64_t>();
        RtlData.translationCache = new std::unordered_map<uint64_t, uint64_t>();
        RtlData.freePages = new std::deque<uint64_t>();
        endSim = false;
        EvRecvAck = false;
      }
      ~ArielRtlEvent() { }
      
      typedef std::vector<char> dataVec;
      dataVec payload;
      
      /*void serialize_order(SST::Core::Serialization::serializer &ser)  override {
          Event::serialize_order(ser);
          ser & payload;
      }*/
      
      ArielEventType getEventType() const override {
         return RTL;
      }

      void* get_rtl_inp_ptr() {
          return RtlData.rtl_inp_ptr;
      }

      void* get_rtl_ctrl_ptr() {
          return RtlData.rtl_ctrl_ptr;
      }

      void* get_updated_rtl_params() {
          return RtlData.updated_rtl_params;
      }
      
      size_t get_rtl_inp_size() {
          return RtlData.rtl_inp_size;
      }
 
      size_t get_rtl_ctrl_size() {
          return RtlData.rtl_ctrl_size;
      }
      
      size_t get_updated_rtl_params_size() {
          return RtlData.updated_rtl_params_size;
      }

      uint64_t get_cachelinesize() {
          return RtlData.cacheLineSize;
      }

      bool isUpdate_params() {
          return RtlData.update_params;
      }

      bool getEndSim() {
          return endSim;
      }

      bool getEventRecvAck() {
          return EvRecvAck;
      }

      void set_rtl_inp_ptr(void* setPtr) {
          RtlData.rtl_inp_ptr = setPtr;
      }

      void set_rtl_ctrl_ptr(void* setPtr) {
          RtlData.rtl_ctrl_ptr = setPtr;
      }
      
      void set_updated_rtl_params(void* setPtr) {
          RtlData.updated_rtl_params = setPtr;
      }

      void set_rtl_inp_size(size_t size) {
          RtlData.rtl_inp_size = size;
      }
 
      void set_rtl_ctrl_size(size_t size) {
          RtlData.rtl_ctrl_size = size;
      }
      
      void set_updated_rtl_params_size(size_t size) {
          RtlData.updated_rtl_params_size = size;
      }

      void set_cachelinesize(uint64_t cachelinesize) {
          RtlData.cacheLineSize = cachelinesize;
      }

      void set_isUpdate_params(bool update) {
          RtlData.update_params = update;
      }

      void setEndSim(bool endIt) {
          endSim = endIt;
      }

      void setEventRecvAck(bool EvRecvd) {
          EvRecvAck = EvRecvd;
      }
 
      ImplementSerializable(SST::ArielComponent::ArielRtlEvent);
};

}
}

#endif
