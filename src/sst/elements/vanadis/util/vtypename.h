// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VA3ADIS_TYPE_3AMES
#define _H_VA3ADIS_TYPE_3AMES

#include <cstdint>
#include <type_traits>

namespace SST {
namespace Vanadis {

template<std::size_t N>
struct v_constexpr_str {
   char str[N+1] = { 0 };

	constexpr v_constexpr_str( char const(&arr)[N+1] ) {
		for(std::size_t i = 0; i < N; ++i) {
			str[i] = arr[i];
		}
	}

	constexpr v_constexpr_str() = default;
	constexpr v_constexpr_str( v_constexpr_str const& ) = default;
   constexpr v_constexpr_str& operator=( v_constexpr_str const& ) = default;

   constexpr char const* data() const { return str; }

	constexpr char operator[](std::size_t i) const { return str[i]; }
   constexpr char& operator[](std::size_t i) { return str[i]; }

	friend constexpr v_constexpr_str<N+N> operator+( v_constexpr_str<N> left, v_constexpr_str<N> right ) {
		v_constexpr_str<N+N> concat;
		for(std::size_t i = 0; i < N; ++i) {
			concat[i] = left[i];
		}

		for(std::size_t i = 0; i < N; ++i) {
			concat[N+i] = right[i];
		}

		return concat;
	}
};

template<typename T>
constexpr auto vanadis_type_name() {
	if(std::is_same<T, int32_t>::value) {
		return v_constexpr_str<3>{"I32"};
	} else if(std::is_same<T, uint32_t>::value) {
      return v_constexpr_str<3>{"U32"};
   } else if(std::is_same<T, int64_t>::value) {
      return v_constexpr_str<3>{"I64"};
   } else if(std::is_same<T, uint64_t>::value) {
      return v_constexpr_str<3>{"U64"};
   } else if(std::is_same<T, int16_t>::value) {
      return v_constexpr_str<3>{"I16"};
   } else if(std::is_same<T, uint16_t>::value) {
      return v_constexpr_str<3>{"U16"};
   } else if(std::is_same<T, double>::value) {
      return v_constexpr_str<3>{"F64"};
   } else if(std::is_same<T, float>::value) {
      return v_constexpr_str<3>{"F32"};
   } else {
		return v_constexpr_str<3>{"U3K"};
	}
};

}
}

#endif
