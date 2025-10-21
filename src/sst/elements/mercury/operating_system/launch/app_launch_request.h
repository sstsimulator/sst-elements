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

#pragma once

#include <string>
#include <sst/core/event.h>
#include <sst/core/params.h>

#include <mercury/operating_system/process/app_id.h>

namespace SST {
namespace Hg {

/**
 * A message to trigger app launch
 */
class AppLaunchRequest : public SST::Event
{
 public:
  AppLaunchRequest(SST::Params& params,
                   AppId aid,
                   const std::string& name);

  AppLaunchRequest() {}

  ~AppLaunchRequest() throw () override {}

  void
  serialize_order(Core::Serialization::serializer& ser) override
  {
    Event::serialize_order(ser);
    SST_SER(aid_);
    SST_SER(name_);
    SST_SER(param_string_);
  }

  SST::Params params();

  AppId aid() const {
    return aid_;
  }

  std::string name() const {
      return name_;
  }

  ImplementSerializable(AppLaunchRequest);

private:
 AppId aid_;

 std::string name_;

 std::string param_string_;
};

} // end of namespace Hg
} // end of namespace SST
