// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _riscvins_H
#define _riscvins_H

#include <boost/utility.hpp>

namespace SST {
namespace Verona {

const uint32_t OP_CODE_MASK = BOOST_BINARY_UL( 00000000000000000000000001111111 );
const uint32_t LUI_MASK     = BOOST_BINARY_UL( 00000111111111111111111110000000 );
const uint32_t LUI_EXT_ONE  = BOOST_BINARY_UL( 11111111111100000000000000000000 );
const uint32_t LUI_EXT_ZERO = BOOST_BINARY_UL( 00000000000011111111111111111111 );
const uint32_t RD_MASK      = BOOST_BINARY_UL( 11111000000000000000000000000000 );
const uint32_t RS1_MASK     = BOOST_BINARY_UL( 00000111110000000000000000000000 );
const uint32_t RS2_MASK     = BOOST_BINARY_UL( 00000000001111100000000000000000 );
const uint32_t RS2_MASK     = BOOST_BINARY_UL( 00000000000000011111000000000000 );
const uint32_t FUNC10_MASK  = BOOST_BINARY_UL( 00000000000000011111111110000000 );
const uint32_t FUNC5_MASK   = BOOST_BINARY_UL( 00000000000000000000111110000000 );
const uint32_t FUNC3_MASK   = BOOST_BINARY_UL( 00000000000000000000001110000000 );
const uint32_t IMM12_MASK   = BOOST_BINARY_UL( 00000000001111111111110000000000 );
const uint32_t IMML6_MASK   = BOOST_BINARY_UL( 00000000000000001111110000000000 );
const uint32_t IMMU5_MASK   = BOOST_BINARY_UL( 11111000000000000000000000000000 );

//EXTRACT_OP_CODE(v)      (v & OP_CODE_MASK)
//EXTRACT_LUI_ADDRESS(v)  (v & 

}
}
#endif
