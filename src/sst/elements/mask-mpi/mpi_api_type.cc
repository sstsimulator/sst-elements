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

#include <mpi_api.h>
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/components/operating_system.h>
#include <climits>
#include <memory>

namespace SST::Hg {
extern void apiLock();
extern void apiUnlock();
}

namespace SST::MASKMPI {

struct float_int_t {
  float value;
  int index;
};

struct long_double_int_t {
  long double value;
  int index;
};

struct double_int_t {
  double value;
  int index;
};

struct short_int_t {
  short value;
  int index;
};

struct long_int_t {
  long value;
  int index;
};


struct int_int_t {
  int value;
  int index;
};

template <typename data_t>
struct MaxLocPair
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    if (src.value > dst.value){
      dst.index = src.index;
      dst.value = src.value;
    } else if (src.value == dst.value){
      dst.index = std::min(src.index, dst.index);
    }
  }
};

template <typename data_t>
struct MinLocPair
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    if (src.value < dst.value){
      dst.index = src.index;
      dst.value = src.value;
    } else if (src.value == dst.value){
      dst.index = std::min(src.index, dst.index);
    }
  }
};

struct complex {
  float r;
  float i;
};

struct dcomplex {
  double r;
  double i;
};

struct ldcomplex {
  long double r;
  long double i;
};

void
MpiApi::commitBuiltinTypes()
{
  SST::Hg::apiLock();

  bool need_init = !MpiType::mpi_null->committed();

#define int_precommit_type(datatype, typeObj, id) \
  if (need_init) typeObj->init_integer<datatype>(#id); \
  commitBuiltinType(typeObj.get(), id)

#define op_precommit_type(datatype, typeObj, id) \
  if (need_init) typeObj->init_with_ops<datatype>(#id); \
  commitBuiltinType(typeObj.get(), id)

#define noop_precommit_type(size, typeObj, id) \
  if (need_init) typeObj->initNoOps(#id, size); \
  commitBuiltinType(typeObj.get(), id)

#define index_precommit_type(datatype, typeObj, id) \
  if (need_init) typeObj->initNoOps(#id, sizeof(datatype)); \
  if (need_init) typeObj->initOp(MPI_MAXLOC, &ReduceOp<MaxLocPair,datatype>::op); \
  if (need_init) typeObj->initOp(MPI_MINLOC, &ReduceOp<MinLocPair,datatype>::op); \
  commitBuiltinType(typeObj.get(), id);

#define precommit_builtin(size) \
  if (need_init) MpiType::builtins[size].initNoOps("builtin-" #size, size); \
  allocateTypeId(&MpiType::builtins[size])


  noop_precommit_type(0, MpiType::mpi_null, MPI_NULL);

  int_precommit_type(char, MpiType::mpi_char, MPI_CHAR);

  int_precommit_type(unsigned char, MpiType::mpi_unsigned_char, MPI_UNSIGNED_CHAR);

  int_precommit_type(char, MpiType::mpi_signed_char, MPI_SIGNED_CHAR);

  int_precommit_type(char16_t, MpiType::mpi_wchar, MPI_WCHAR);

  int_precommit_type(unsigned long long, MpiType::mpi_unsigned_long_long, MPI_UNSIGNED_LONG_LONG);

  noop_precommit_type(0, MpiType::mpi_lb, MPI_LB);

  noop_precommit_type(0, MpiType::mpi_ub, MPI_UB);

  int_precommit_type(char, MpiType::mpi_byte, MPI_BYTE);

  int_precommit_type(int, MpiType::mpi_int, MPI_INT);

  int_precommit_type(unsigned, MpiType::mpi_unsigned, MPI_UNSIGNED);

  int_precommit_type(short, MpiType::mpi_short, MPI_SHORT);

  int_precommit_type(unsigned short, MpiType::mpi_unsigned_short, MPI_UNSIGNED_SHORT);

  int_precommit_type(long, MpiType::mpi_long, MPI_LONG);

  int_precommit_type(long long, MpiType::mpi_long_long_int, MPI_LONG_LONG_INT);

  int_precommit_type(unsigned long, MpiType::mpi_unsigned_long, MPI_UNSIGNED_LONG);

  int_precommit_type(char, MpiType::mpi_packed, MPI_PACKED);

  //fortran nonsense
  noop_precommit_type(2*sizeof(float), MpiType::mpi_complex, MPI_COMPLEX);

  noop_precommit_type(2*sizeof(double), MpiType::mpi_double_complex, MPI_DOUBLE_COMPLEX);

  op_precommit_type(float, MpiType::mpi_float, MPI_FLOAT);
  op_precommit_type(float, MpiType::mpi_real, MPI_REAL);
  op_precommit_type(double, MpiType::mpi_double_precision, MPI_DOUBLE_PRECISION);
  op_precommit_type(double, MpiType::mpi_double, MPI_DOUBLE);
  op_precommit_type(float, MpiType::mpi_real4, MPI_REAL4);
  op_precommit_type(double, MpiType::mpi_real8, MPI_REAL8);
  op_precommit_type(long double, MpiType::mpi_long_double, MPI_LONG_DOUBLE);

  int_precommit_type(bool, MpiType::mpi_cxx_bool, MPI_CXX_BOOL);
  int_precommit_type(int, MpiType::mpi_integer, MPI_INTEGER);
  int_precommit_type(char, MpiType::mpi_integer1, MPI_INTEGER1);
  int_precommit_type(int16_t, MpiType::mpi_integer2, MPI_INTEGER2);
  int_precommit_type(int32_t, MpiType::mpi_integer4, MPI_INTEGER4);
  int_precommit_type(int64_t, MpiType::mpi_integer8, MPI_INTEGER8);
  int_precommit_type(int8_t, MpiType::mpi_int8_t, MPI_INT8_T);
  int_precommit_type(int16_t, MpiType::mpi_int16_t, MPI_INT16_T);
  int_precommit_type(int32_t, MpiType::mpi_int32_t, MPI_INT32_T);
  int_precommit_type(int64_t, MpiType::mpi_int64_t, MPI_INT64_T);
  int_precommit_type(uint8_t, MpiType::mpi_uint8_t, MPI_UINT8_T);
  int_precommit_type(uint16_t, MpiType::mpi_uint16_t, MPI_UINT16_T);
  int_precommit_type(uint32_t, MpiType::mpi_uint32_t, MPI_UINT32_T);
  int_precommit_type(uint64_t, MpiType::mpi_uint64_t, MPI_UINT64_T);
  int_precommit_type(char, MpiType::mpi_character, MPI_CHARACTER);

  index_precommit_type(int_int_t, MpiType::mpi_2int, MPI_2INT);
  index_precommit_type(double_int_t, MpiType::mpi_double_int, MPI_DOUBLE_INT);
  index_precommit_type(float_int_t, MpiType::mpi_float_int, MPI_FLOAT_INT);
  index_precommit_type(long_int_t, MpiType::mpi_long_int, MPI_LONG_INT);
  index_precommit_type(short_int_t, MpiType::mpi_short_int, MPI_SHORT_INT);
  index_precommit_type(long_double_int_t, MpiType::mpi_long_double_int, MPI_LONG_DOUBLE_INT);

  precommit_builtin(1);
  precommit_builtin(2);
  precommit_builtin(4);
  precommit_builtin(6);
  precommit_builtin(8);
  precommit_builtin(12);
  precommit_builtin(16);
  precommit_builtin(20);
  precommit_builtin(32);
  precommit_builtin(48);
  precommit_builtin(64);


  SST::Hg::apiUnlock();

}

int
MpiApi::packSize(int incount, MPI_Datatype datatype, MPI_Comm  /*comm*/, int *size)
{
  auto it = known_types_.find(datatype);
  if (it == known_types_.end()){
      return MPI_ERR_TYPE;
  }
  *size = incount * it->second->packed_size();

  return MPI_SUCCESS;
}

int
MpiApi::typeSetName(MPI_Datatype id, const char* name)
{
//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_set_name(%s,%s)",
//                typeStr(id).c_str(), name);
  auto it = known_types_.find(id);
  if (it == known_types_.end()){
      return MPI_ERR_TYPE;
  }
  it->second->label = name;

  return MPI_SUCCESS;
}

int
MpiApi::typeGetName(MPI_Datatype id, char* name, int* resultlen)
{
//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_get_name(%s)",
//                typeStr(id).c_str());
  auto it = known_types_.find(id);
  if (it == known_types_.end()){
      return MPI_ERR_TYPE;
  }
  const std::string& label = it->second->label;
  ::strcpy(name, label.c_str());
  *resultlen = label.size();

  return MPI_SUCCESS;
}

int
MpiApi::opCreate(MPI_User_function *user_fn, int commute, MPI_Op *op)
{
  if (!commute){
    sst_hg_throw_printf(SST::Hg::UnimplementedError,
                      "mpi_api::op_create: non-commutative operations");
  }
  *op = next_op_id_++;
  custom_ops_[*op] = user_fn;
  return MPI_SUCCESS;
}

int
MpiApi::opFree(MPI_Op *op)
{
  custom_ops_.erase(*op);
  return MPI_SUCCESS;
}

int
MpiApi::doTypeHvector(int count, int blocklength, MPI_Aint stride,
                         MpiType* old, MPI_Datatype* new_type)
{
  std::stringstream ss;
  ss << "vector-" << old->label << "\n";

  MpiType* new_type_obj = new MpiType;
  new_type_obj->init_vector(ss.str(), old,
                        count, blocklength, stride);

  allocateTypeId(new_type_obj);
  *new_type = new_type_obj->id;
//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_vector(%d,%d,%d,%s,*%s)",
//                count, blocklength, stride,
//                typeStr(old->id).c_str(), typeStr(*new_type).c_str());

  allocatedTypes_[new_type_obj->id] = MpiType::ptr(new_type_obj);

  return MPI_SUCCESS;
}

int
MpiApi::typeHvector(int count, int blocklength, MPI_Aint stride,
                     MPI_Datatype old_type, MPI_Datatype* new_type)
{
  MpiType* ty = typeFromId(old_type);
  int rc = doTypeHvector(count, blocklength, stride, ty, new_type);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_) {
    OTF2Writer_->writer().register_type(*new_type, count*ty->packed_size());
  }
#endif
  return rc;
}

/// Creates a vector (strided) datatype
int
MpiApi::typeVector(int count, int blocklength, int stride,
                     MPI_Datatype old_type, MPI_Datatype* new_type)
{
  MpiType* old = typeFromId(old_type);
  MPI_Aint byte_stride = stride * old->extent();
  int rc = doTypeHvector(count, blocklength, byte_stride, old, new_type);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_) {
    OTF2Writer_->writer().register_type(*new_type, count*old->packed_size());
  }
#endif
  return rc;
}

int
MpiApi::doTypeHindexed(int count, const int lens[],
                          const MPI_Aint* displs, MpiType *in_type_obj,
                          MPI_Datatype *outtype)
{
  MpiType* out_type_obj = new MpiType;

  inddata* idata = new inddata;
  int packed_size = 0;
  int extent = 0;
  int index = 0;

  for (int i = 0; i < count; i++) {
    if (lens[i] > 0) {
      index++;
    }
  }

  idata->blocks.resize(index);
  index = 0;
  for (int i = 0; i < count; i++) {
    if (lens[i] > 0) {
      ind_block& next = idata->blocks[index];
      next.base = in_type_obj;
      next.byte_disp = extent;
      next.num = lens[i];
      packed_size += lens[i] * in_type_obj->packed_size();
      extent += displs[i];
      index++;
    }
  }

  out_type_obj->init_indexed(in_type_obj->label, idata, packed_size, extent);

  allocateTypeId(out_type_obj);
  *outtype = out_type_obj->id;

//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_indexed(%d,<...>,<...>,%s,*%s)",
//                count, typeStr(in_type_obj->id).c_str(), typeStr(*outtype).c_str());

  allocatedTypes_[out_type_obj->id] = MpiType::ptr(out_type_obj);

  return MPI_SUCCESS;
}

int
MpiApi::typeHindexed(int count, const int lens[], const MPI_Aint* displs,
                      MPI_Datatype intype, MPI_Datatype* outtype)
{
  MpiType* ty = typeFromId(intype);
  int rc = doTypeHindexed(count, lens, displs, ty, outtype);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().register_type(*outtype, count*ty->packed_size());
  }
#endif
  return rc;
}

int
MpiApi::typeIndexed(int count, const int lens[], const int* displs,
                      MPI_Datatype intype, MPI_Datatype* outtype)
{
  MpiType* in_type_obj = typeFromId(intype);
  int type_extent = in_type_obj->extent();
  auto byte_displs = std::make_unique<MPI_Aint[]>(count);
  for (int i=0; i < count; ++i){
    byte_displs[i] = displs[i] * type_extent;
  }

  int rc = doTypeHindexed(count, lens, byte_displs.get(), in_type_obj, outtype);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().register_type(*outtype, count*in_type_obj->packed_size());
  }
#endif
  return rc;
}

std::string
MpiApi::typeLabel(MPI_Datatype tid)
{
  MpiType* ty = typeFromId(tid);
  return ty->label;
}

//
// A datatype object has to be committed before use in communication.
//
int
MpiApi::typeCommit(MPI_Datatype* type)
{
  MpiType* type_obj = typeFromId(*type);
  type_obj->set_committed(true);
  return MPI_SUCCESS;
}

void
MpiApi::allocateTypeId(MpiType* type)
{
  auto end = known_types_.end();
  auto it = end;
  while ((it = known_types_.find(next_type_id_)) != end){
    ++next_type_id_;
  }
  type->id = next_type_id_;
  known_types_[type->id] = type;
}

void
MpiApi::commitBuiltinType(MpiType* type, MPI_Datatype id)
{
  if (known_types_.find(id) != known_types_.end()){
    sst_hg_throw_printf(SST::Hg::ValueError,
      "mpi_api::precommit_type: %d already exists", id);
  }
  type->id = id;
  type->set_builtin(true);
  known_types_[id] = type;
  known_types_[id]->set_committed(true);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_) {
    OTF2Writer_->writer().register_type(id, type->packed_size());
  }
#endif
}

//
// Creates a contiguous datatype
//
int
MpiApi::typeContiguous(int count, MPI_Datatype old_type,
                         MPI_Datatype* new_type)
{
  MpiType* new_type_obj = new MpiType;
  MpiType* old_type_obj = typeFromId(old_type);
  MPI_Aint byte_stride = count * old_type_obj->extent();
  new_type_obj->init_vector("contiguous-" + old_type_obj->label,
                        old_type_obj,
                        count, 1, byte_stride);

  allocateTypeId(new_type_obj);
  *new_type = new_type_obj->id;

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().register_type(*new_type, count*old_type_obj->packed_size());
  }
#endif

  allocatedTypes_[*new_type] = MpiType::ptr(new_type_obj);

  return MPI_SUCCESS;
}

int
MpiApi::typeCreateStruct(const int count, const int* blocklens,
                     const MPI_Aint* indices, const MPI_Datatype* old_types,
                     MPI_Datatype* newtype)
{
  auto new_ind = std::make_unique<int[]>(count);
  for (int i=0; i < count; ++i){
    new_ind[i] = indices[i];
  }
  return typeCreateStruct(count, blocklens, new_ind.get(), old_types, newtype);
}

//
// Creates a struct datatype
//
int
MpiApi::typeCreateStruct(const int count, const int* blocklens,
                     const int* indices, const MPI_Datatype* old_types,
                     MPI_Datatype* newtype)
{
  MpiType* new_type_obj = new MpiType;

  inddata* idata = new inddata;

  int packed_size = 0;
  int extent = 0;
  int index = 0;

  for (int i = 0; i < count; i++) {
    if (blocklens[i] > 0) {
      ++index;
    }
  }

  idata->blocks.resize(index);
  index = 0;

  for (int i = 0; i < count; i++) {
    if (blocklens[i] > 0) {
      MpiType* old_type_obj = typeFromId(old_types[i]);
      ind_block& next = idata->blocks[index];
      next.base = old_type_obj;
      next.byte_disp = indices[i];
      next.num = blocklens[i];
      packed_size += old_type_obj->packed_size() * blocklens[i];
      extent = next.byte_disp + old_type_obj->packed_size() * blocklens[i];
      index++;
    }
  }

  new_type_obj->init_indexed("struct", idata, packed_size, extent);

  allocateTypeId(new_type_obj);
  *newtype = new_type_obj->id;

//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_struct(%d,<...>,<...>,<...>,*%s)",
//                count, typeStr(*newtype).c_str());

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().register_type(*newtype, new_type_obj->packed_size());
  }
#endif

  allocatedTypes_[new_type_obj->id] = MpiType::ptr(new_type_obj);

  return MPI_SUCCESS;
}

int
MpiApi::typeSize(MPI_Datatype type, int* size)
{
  *size = typeFromId(type)->packed_size();
  return MPI_SUCCESS;
}

int
MpiApi::typeExtent(MPI_Datatype type, MPI_Aint *extent)
{
  *extent = typeFromId(type)->extent();
  return MPI_SUCCESS;
}

int
MpiApi::typeDup(MPI_Datatype intype, MPI_Datatype* outtype)
{
  MpiType* new_type_obj = typeFromId(intype);
  allocateTypeId(new_type_obj);
  *outtype = new_type_obj->id;
//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_dup(%s,*%s)",
//                typeStr(intype).c_str(), typeStr(*outtype).c_str());
  return MPI_SUCCESS;
}


//
// Mark datatype for deallocation.
//
int
MpiApi::typeFree(MPI_Datatype* type)
{
//  mpi_api_debug(sprockit::dbg::mpi,
//                "MPI_Type_free(%s)",
//                typeStr(*type).c_str());

  //these maps are separated because all known types
  //are not necessarily allocated by this rank
  known_types_.erase(*type);
  allocatedTypes_.erase(*type);

  return MPI_SUCCESS;
}

//
// Get the derived mpitype mapped to an id
//
MpiType*
MpiApi::typeFromId(MPI_Datatype id)
{
  auto it = known_types_.find(id);
  if (it == known_types_.end()){
    sst_hg_throw_printf(SST::Hg::InvalidKeyError,
        "mpi_api: unknown type id %d",
        int(id));
  }
  return it->second;
}

}
