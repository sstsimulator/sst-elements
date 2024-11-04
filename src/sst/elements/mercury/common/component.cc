// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mercury/common/component.h>

namespace SST {
namespace Hg {

static int _self_id_(-1);

template<> int HgBase<SST::Component>::self_id() {
    ++_self_id_;
    return _self_id_;
  }

template<> int HgBase<SST::SubComponent>::self_id() {
    ++_self_id_;
    return _self_id_;
  }

template class HgBase<SST::Component>;
template class HgBase<SST::SubComponent>;

} // end of namespace Hg
} // end of namespace SST
