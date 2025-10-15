// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Copyright (c) 2004, 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2004, 2011-2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2005, 2006, 2007 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2005 PathScale, Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SUMI_SUMI_HPP
#define SUMI_SUMI_HPP

#include <cstdint>
#include <vector>
#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include <mercury/common/errors.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sumi/sim_transport.h>

class FabricMessage : public SST::Iris::sumi::Message {
 public:
  constexpr static uint64_t no_tag = std::numeric_limits<uint64_t>::max();
  constexpr static uint64_t no_imm_data = std::numeric_limits<uint64_t>::max();

  template <class... Args>
  FabricMessage(uint64_t tag, uint64_t flags, uint64_t imm_data, void* ctx,
                Args&&... args) :
    SST::Iris::sumi::Message(std::forward<Args>(args)...),
    tag_(tag),
    flags_(flags),
    imm_data_(imm_data),
    context_(ctx)
  {
  }

  uint64_t tag() const {
    return tag_;
  }

  uint64_t flags() const {
    return flags_;
  }

  uint64_t immData() const {
    return imm_data_;
  }

  void* context() const {
    return context_;
  }

 private:
  uint64_t flags_;
  uint64_t imm_data_;
  uint64_t tag_;
  void* context_;
};

class FabricTransport : public SST::Iris::sumi::SimTransport {

 public:
  SST_ELI_REGISTER_DERIVED(
    Library,
    FabricTransport,
    "macro",
    "libfabric",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "implements the libfabric transport API")

  public:
    FabricTransport(SST::Params& params,
                    SST::Hg::App* parent) :
      SST::Iris::sumi::SimTransport(params, parent),
      inited_(false)
  {
  }

  void init() override {
    SST::Iris::sumi::SimTransport::init();
    inited_ = true;
  }

  bool inited() const {
    return inited_;
  }

 private:
  bool inited_;
  std::vector<FabricMessage*> unmatched_recvs_;
};

FabricTransport* sumi_fabric();

#endif // SUMI_SUMI_HPP
