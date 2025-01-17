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

#include <sst/core/serialization/serializer_fwd.h>
#include <sst/core/serialization/serializer.h>
#include <sst/core/serialization/serializable.h>

#define START_SERIALIZATION_NAMESPACE namespace SST { namespace Core { namespace Serialization {
#define END_SERIALIZATION_NAMESPACE } } }

#define FRIEND_SERIALIZATION   template <class T, class Enable> friend class SST::Core::Serialization::serialize

namespace SST {
namespace Hg {

using serializer = SST::Core::Serialization::serializer;
template <class T> using serializable_type = SST::Core::Serialization::serializable_type<T>;
template <class T> using serialize = typename SST::Core::Serialization::serialize<T>;
using serializable = SST::Core::Serialization::serializable;
using SST::Core::Serialization::array;
using SST::Core::Serialization::raw_ptr;

} // end namespace Hg
} // end namespace SST


