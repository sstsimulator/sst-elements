/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

//#include <sstmac/common/sstmac_config.h>
#include <mercury/common/errors.h>
//#include <sprockit/statics.h>
#include <mpi_types/mpi_type.h>
#include <mpi_types.h>
#include <iostream>
#include <sstream>
#include <cstring>

//sprockit::NeedDeletestatics<sumi::MpiType> delete_static_types;

namespace SST::MASKMPI {

MpiType::MpiType() :
  id(-1),
  type_(NONE),
  contiguous_(true),
  committed_(false),
  pdata_(nullptr),
  vdata_(nullptr),
  idata_(nullptr),
  builtin_(false),
  size_(-1)
{
}

void
MpiType::deleteStatics()
{
  /** JJW make unique_ptr for cleanup
  free_static_ptr(mpi_null);
  free_static_ptr(mpi_char);
  free_static_ptr(mpi_unsigned_char);
  free_static_ptr(mpi_signed_char);
  free_static_ptr(mpi_wchar);
  free_static_ptr(mpi_unsigned_long_long);
  free_static_ptr(mpi_lb);
  free_static_ptr(mpi_ub);
  free_static_ptr(mpi_byte);
  free_static_ptr(mpi_short);
  free_static_ptr(mpi_unsigned_short);
  free_static_ptr(mpi_int);
  free_static_ptr(mpi_unsigned);
  free_static_ptr(mpi_long);
  free_static_ptr(mpi_unsigned_long);
  free_static_ptr(mpi_float);
  free_static_ptr(mpi_double);
  free_static_ptr(mpi_long_double);
  free_static_ptr(mpi_long_long_int);
  free_static_ptr(mpi_long_long);
  free_static_ptr(mpi_packed);
  free_static_ptr(mpi_float_int);
  free_static_ptr(mpi_double_int);
  free_static_ptr(mpi_long_int);
  free_static_ptr(mpi_short_int);
  free_static_ptr(mpi_2int);
  free_static_ptr(mpi_long_double_int);
  free_static_ptr(mpi_complex);
  free_static_ptr(mpi_complex8);
  free_static_ptr(mpi_complex16);
  free_static_ptr(mpi_complex32);
  free_static_ptr(mpi_double_complex);
  free_static_ptr(mpi_logical);
  free_static_ptr(mpi_real);
  free_static_ptr(mpi_real4);
  free_static_ptr(mpi_real8);
  free_static_ptr(mpi_real16);
  free_static_ptr(mpi_double_precision);
  free_static_ptr(mpi_integer);
  free_static_ptr(mpi_integer1);
  free_static_ptr(mpi_integer2);
  free_static_ptr(mpi_integer4);
  free_static_ptr(mpi_integer8);
  free_static_ptr(mpi_2integer);
  free_static_ptr(mpi_2complex);
  free_static_ptr(mpi_2double_complex);
  free_static_ptr(mpi_2real);
  free_static_ptr(mpi_2double_precision);
  free_static_ptr(mpi_character);
  free_static_ptr(mpi_int8_t);
  free_static_ptr(mpi_int16_t);
  free_static_ptr(mpi_int32_t);
  free_static_ptr(mpi_int64_t);
  free_static_ptr(mpi_uint8_t);
  free_static_ptr(mpi_uint16_t);
  free_static_ptr(mpi_uint32_t);
  free_static_ptr(mpi_uint64_t);
  */
}

void
MpiType::init_primitive(const char* labelit, int size)
{
  extent_ = size;
  size_ = size;
  type_ = PRIM;
  label = labelit;
}

//
// MPI datatypes.
//
void
MpiType::init_primitive(const std::string& labelit, const int sizeit, int  /*align*/)
{
  init_primitive(labelit.c_str(), sizeit);
}

void
MpiType::init_primitive(const char* labelit, MpiType* b1, MpiType* b2, int s)
{
  label = labelit;
  type_ = PAIR;
  extent_ = s;
  size_ = b1->size_ + b2->size_;
  pdata_ = new pairdata;
  pdata_->base1 = b1;
  pdata_->base2 = b2;
}

void
MpiType::init_primitive(const std::string &labelit, MpiType* b1,
                   MpiType* b2, int s)
{
  init_primitive(labelit.c_str(), b1, b2, s);
}

Iris::sumi::reduce_fxn
MpiType::op(MPI_Op theOp) const
{
  auto it = fxns_.find(theOp);
  if (it == fxns_.end()){
    sst_hg_throw_printf(SST::Hg::ValueError, "type %s has no operator %d",
           toString().c_str(), theOp);
  }
  return it->second;
}

void
MpiType::init_vector(const std::string &labelit, MpiType* base,
                   int count, int block, MPI_Aint byte_stride)
{
  if (base->id == MPI_Datatype(-1)){
    sst_hg_throw_printf(SST::Hg::ValueError,
        "mpi_type::init_vector: unitialized base type %s",
        base->label.c_str());
  }
  label = labelit;
  type_ = VEC;
  vdata_ = new vecdata();
  pdata_ = NULL;
  vdata_->base = base;
  vdata_->count = count;
  vdata_->blocklen = (block);
  vdata_->byte_stride = byte_stride;

  extent_ = byte_stride;
  size_ = count * block * base->size_;
  int block_extent = block*base->extent();

  //if the byte_stride matches the blocksize
  //and the underlying type is contiguous
  //then this type is again contiguous
  contiguous_ = byte_stride == block_extent && base->contiguous();

}

void
MpiType::init_indexed(const std::string &labelit,
                   inddata* dat, int sz, int ext)
{
  label = labelit;
  type_ = IND;
  size_ = sz;
  extent_ = ext;
  idata_ = dat;
  contiguous_ = extent_ == size_;
}

MpiType::~MpiType()
{
 if (idata_) delete idata_;
 if (vdata_) delete vdata_;
 if (pdata_) delete pdata_;
}

int
MpiType::bytes_to_elements(size_t bytes) const
{
  if (size_ == 0) {
    return 0;
  }

  if (type_ == PRIM) {
    return bytes / size_;
  } else if (type_ == PAIR) {
    int ret = 0;
    size_t total = 0;
    bool one = true;
    while (total <= bytes) {
      if (one) {
        total += pdata_->base1->size_;
      } else {
        total += pdata_->base2->size_;
      }

      ret++;
      one = !one;
    }
    return ret - 1;
  } else if (type_ == VEC) {
    if (vdata_->blocklen == 0) {
      return 0;
    }

    int ret = 0;
    size_t total = 0;
    while (total < bytes) {
      int before = total;
      for (int i = 0; i < vdata_->blocklen && total < bytes; i++) {
        ret += vdata_->base->bytes_to_elements(
                 vdata_->base->packed_size());
        total += vdata_->base->packed_size();
        //ret++;

      }
      if (total == before) {
        return 0;  //stupid check for an empty struct
      }
    }

    return ret;

  } else if (type_ == IND) {

    int ret = 0;
    size_t total = 0;
    while (total < bytes) {
      int before = total;
      for (int i = 0; i < idata_->blocks.size() && total < bytes; i++) {
        for (int j = 0; j < idata_->blocks[i].num && total < bytes; j++) {
          ret += idata_->blocks[i].base->bytes_to_elements(
                   idata_->blocks[i].base->packed_size());
          total += idata_->blocks[i].base->packed_size();
        }
      }
      if (total == before) {
        return 0;  //stupid check for an empty struct
      }
    }

    return ret;
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
                   "mpitype::bytes_to_elements: invalid type %d",
                   type_);
  return 0;
}

void
MpiType::pack(const void* inbuf, void *outbuf) const
{
  //we are packing from inbuf into outbuf
  pack_action(outbuf, const_cast<void*>(inbuf), true);
}

void
MpiType::unpack(const void* inbuf, void *outbuf) const
{
  //we are unpacking from inbuf into outbuf
  //false = unpack
  pack_action(const_cast<void*>(inbuf), outbuf, false);
}

void
MpiType::pack_action_primitive(void* packed_buf, void* unpacked_buf, bool pack) const
{
  char* packed_ptr = (char*) packed_buf;
  char* unpacked_ptr = (char*) unpacked_buf;
  if (pack){
    //copy into packed array
    ::memcpy(packed_ptr, unpacked_ptr, size_);
  } else {
    //copy into unpacked array
    ::memcpy(unpacked_ptr, packed_ptr, size_);
  }
}

void
MpiType::pack_action_vector(void* packed_buf, void* unpacked_buf, bool pack) const
{
  char* packed_ptr = (char*) packed_buf;
  char* unpacked_ptr = (char*) unpacked_buf;
  int packed_stride = vdata_->base->packed_size();
  for (int j=0; j < vdata_->count; ++j){
    char* this_unpacked_ptr = unpacked_ptr + vdata_->byte_stride * j;
    vdata_->base->pack_action(packed_ptr, this_unpacked_ptr, pack);
    packed_ptr += packed_stride;
  }
  unpacked_ptr += extent_;
}

void
MpiType::pack_action_indexed(void* packed_buf, void* unpacked_buf, bool pack) const
{
  char* packed_ptr = (char*) packed_buf;
  char* unpacked_ptr = (char*) unpacked_buf;
  for (int j=0; j < idata_->blocks.size(); ++j){
    const ind_block& next_block = idata_->blocks[j];
    int size = next_block.base->packed_size();
    if (next_block.base->contiguous()){
      char *src, *dst;
      if (pack){ src = unpacked_ptr; dst = packed_ptr; }
      else     { src = packed_ptr;   dst = unpacked_ptr; }
      ::memcpy(dst, src, size*next_block.num);
    } else {
      if (pack) next_block.base->packSend(unpacked_ptr, packed_ptr, next_block.num);
      else      next_block.base->unpack_recv(packed_ptr, unpacked_ptr, next_block.num);
    }
    packed_ptr += size*next_block.num;
    unpacked_ptr += next_block.base->extent()*next_block.num;
  }
}

void
MpiType::packSend(void* srcbuf, void* dstbuf, int sendcnt)
{
  char* src = (char*) srcbuf;
  char* dst = (char*) dstbuf;
  int src_stride = extent_;
  int dst_stride = size_;
  for (int i=0; i < sendcnt; ++i, src += src_stride, dst += dst_stride){
    pack(src, dst);
  }
}

void
MpiType::unpack_recv(void *srcbuf, void *dstbuf, int recvcnt)
{
  char* src = (char*) srcbuf;
  char* dst = (char*) dstbuf;
  int src_stride = size_;
  int dst_stride = extent_;
  for (int i=0; i < recvcnt; ++i, src += src_stride, dst += dst_stride){
    unpack(src, dst);
  }
}

void
MpiType::pack_action_pair(void* packed_buf, void* unpacked_buf, bool pack) const
{
  int bytes_done = 0;
  int first_size = pdata_->base1->size_;
  int second_size = pdata_->base2->size_;
  int pair_alignment = first_size;

  char* packed_ptr = (char*) packed_buf;
  char* unpacked_ptr = (char*) unpacked_buf;

  if (pack){
    ::memcpy(packed_ptr, unpacked_ptr, first_size);
    ::memcpy(packed_ptr + first_size, unpacked_ptr + pair_alignment, second_size);
  } else {
    ::memcpy(unpacked_ptr, packed_ptr, first_size);
    ::memcpy(unpacked_ptr + pair_alignment, packed_ptr + first_size, second_size);
  }
  unpacked_ptr += extent_;
  packed_ptr += size_;
  bytes_done += (first_size + second_size);
}

void
MpiType::pack_action(void* packed_buf, void* unpacked_buf, bool pack) const
{
  switch(type_)
  {
  case PRIM: {
    pack_action_primitive(packed_buf, unpacked_buf, pack);
    break;
  }
  case PAIR: {
    pack_action_pair(packed_buf, unpacked_buf, pack);
    break;
  }
  case VEC: {
    pack_action_vector(packed_buf, unpacked_buf, pack);
    break;
  }
  case IND: {
    pack_action_indexed(packed_buf, unpacked_buf, pack);
    break;
  }
  case NONE: {
    SST::Hg::abort("mpi_type::pack_action: cannot pack NONE type");
  }
  }
}

std::string
MpiType::toString() const
{
  std::stringstream ss(std::stringstream::in | std::stringstream::out);
  if (type_ == PRIM) {
    ss << "mpitype(primitive, label=\"" << label << "\", size=" << size_
       << ")";
  }
  if (type_ == PAIR) {
    ss << "mpitype(pair, label=\"" << label << "\", size=" << size_ << ")";
  } else if (type_ == VEC) {
    ss << "mpitype(vector, label=\"" << label << "\", size=" << size_
       << ", base=" << vdata_->base->toString() << ", count="
       << vdata_->count << ", blocklen=" << vdata_->blocklen
       << ", stride=" << vdata_->byte_stride << ")";
  } else if (type_ == IND) {
    ss << "mpitype(indexed/struct, label=\"" << label << "\", size="
       << size_ << ", base=" << idata_->blocks[0].base->toString()
       << ", blocks=" << idata_->blocks.size() << ")";

  }
  return ss.str();
}

//
// Fairly self-explanatory.
//
std::ostream&
operator<<(std::ostream &os, MpiType* type)
{
  os << type->toString();
  return os;
}

//
// The data types defined by the MPI standard.
//
MpiType::ptr MpiType::mpi_null = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_char = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_signed_char = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_wchar = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_unsigned_long_long = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_lb = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_ub = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_unsigned_char = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_byte = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_short = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_unsigned_short = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_unsigned = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_unsigned_long = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_float = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_double = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long_double = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long_long_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long_long = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_packed = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_float_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_double_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_short_int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_2int = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_long_double_int = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_complex = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_double_complex = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_logical = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_real = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_double_precision = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_integer = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_cxx_bool = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_integer1 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_integer2 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_integer4 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_integer8 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_real4 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_real8 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_real16 = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_complex8 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_complex16 = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_complex32 = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_int8_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_int16_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_int32_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_int64_t = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_uint8_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_uint16_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_uint32_t = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_uint64_t = std::unique_ptr<MpiType>(new MpiType);

MpiType::ptr MpiType::mpi_2integer = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_2complex = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_2double_complex = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_2real = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_2double_precision = std::unique_ptr<MpiType>(new MpiType);
MpiType::ptr MpiType::mpi_character = std::unique_ptr<MpiType>(new MpiType);

std::map<int, MpiType> MpiType::builtins;

}
