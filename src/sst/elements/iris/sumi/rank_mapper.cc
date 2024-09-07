/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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
*/
#include <iris/sumi/rank_mapper.h>
#include "common/errors.h"
#include <mutex>
#include <string>

namespace SST::Iris::sumi {

std::vector<RankMapping::ptr> RankMapping::app_ids_launched_(1024);
std::map<std::string, RankMapping::ptr> RankMapping::app_names_launched_;
std::vector<int> RankMapping::local_refcounts_(1024);

static std::mutex mutex;

RankMapping::ptr
RankMapping::globalMapping(const std::string& name)
{
  std::lock_guard<std::mutex> lk(mutex);
  auto iter = app_names_launched_.find(name);
  if (iter == app_names_launched_.end()){
    std::string err_msg = "cannot find global task mapping for ";
    err_msg += name.c_str();
    Hg::abort(err_msg);
  }
  auto ret = iter->second;
  return ret;
}

const RankMapping::ptr&
RankMapping::globalMapping(AppId aid)
{
  std::lock_guard<std::mutex> lk(mutex);
  auto& mapping = app_ids_launched_[aid];
  if (!mapping){
    std::string err_msg = "No task mapping exists for app ";
    err_msg += aid;
    Hg::abort(err_msg);
  }
  return mapping;
}

void
RankMapping::addGlobalMapping(AppId aid, const std::string &unique_name, const RankMapping::ptr &mapping)
{
  std::lock_guard<std::mutex> lk(mutex);
  app_ids_launched_[aid] = mapping;
  app_names_launched_[unique_name] = mapping;
  local_refcounts_[aid]++;
}

void
RankMapping::removeGlobalMapping(AppId aid, const std::string& name)
{
  std::lock_guard<std::mutex> lk(mutex);
  local_refcounts_[aid]--;
  if (local_refcounts_[aid] == 0){
    app_ids_launched_[aid] = 0;
    app_names_launched_.erase(name);
  }
}
}
