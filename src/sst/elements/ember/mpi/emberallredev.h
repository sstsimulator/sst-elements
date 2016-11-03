// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_ALLREDUCE_EV
#define _H_EMBER_ALLREDUCE_EV

#include "emberMPIEvent.h"

namespace SST {
namespace Ember {

class EmberAllreduceEvent : public EmberMPIEvent {
public:
    EmberAllreduceEvent( MP::Interface& api, Output* output,
                        EmberEventTimeStatistic* stat,
            const Hermes::MemAddr& mydata, 
            const Hermes::MemAddr& result,
            uint32_t count, PayloadDataType dtype,
            ReductionOperation op, Communicator group ) :
        EmberMPIEvent( api, output, stat ),
        m_mydata(mydata),
        m_result(result),
        m_count(count),
        m_dtype(dtype),
        m_op(op), 
        m_group(group)
    {}

	~EmberAllreduceEvent() {}

    std::string getName() { return "Allreduce"; }

    void issue( uint64_t time, FOO* functor ) {

        EmberEvent::issue( time );

        m_api.allreduce( m_mydata, m_result, m_count, m_dtype, m_op,
                                                    m_group, functor );
    }

private:
    Hermes::MemAddr     m_mydata;
    Hermes::MemAddr     m_result;
    uint32_t            m_count;
    PayloadDataType     m_dtype;
    ReductionOperation  m_op;
    Communicator        m_group;
};

}
}

#endif
