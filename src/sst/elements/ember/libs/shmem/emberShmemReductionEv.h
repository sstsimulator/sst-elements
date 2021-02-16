// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_REDUCTION_EVENT
#define _H_EMBER_SHMEM_REDUCTION_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberReductionShmemEvent : public EmberShmemEvent {

public:
	EmberReductionShmemEvent( Shmem::Interface& api, Output* output,
               Hermes::Vaddr dest, Hermes::Vaddr src, int nelems,
               int PE_start, int logPE_stride, int PE_size, Hermes::Vaddr pSync,
               Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, EmberEventTimeStatistic* stat = NULL ) :

            EmberShmemEvent( api, output, stat ),
            m_dest(dest), m_src(src), m_nelems(nelems), m_pe_start(PE_start), m_PE_stride( logPE_stride),
            m_PE_size(PE_size), m_pSync(pSync), m_op(op), m_dataType(dataType)  {}

	~EmberReductionShmemEvent() {}

    std::string getName() { return "Reduction"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.reduction( m_dest, m_src, m_nelems, m_pe_start, m_PE_stride, m_PE_size,
                            m_pSync, m_op, m_dataType, callback );
    }
private:
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_nelems;
    int m_pe_start;
    int m_PE_stride;
    int m_PE_size;
    Hermes::Shmem::ReduOp m_op;
    Hermes::Value::Type   m_dataType;
    Hermes::Vaddr m_pSync;
};

}
}

#endif
