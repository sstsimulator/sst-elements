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


#ifndef _H_SST_ARIEL_EVENT
#define _H_SST_ARIEL_EVENT



namespace SST {
namespace ArielComponent {

enum ArielEventType {
    READ_ADDRESS,
    WRITE_ADDRESS,
    START_DMA_TRANSFER,
    WAIT_ON_DMA_TRANSFER,
    CORE_EXIT,
    NOOP,
    MALLOC,
    MMAP,
    FREE,
    SWITCH_POOL,
    FLUSH,
    FENCE,
#ifdef HAVE_CUDA
    GPU
#endif
};

class ArielEvent {

    public:
        ArielEvent();
        virtual ~ArielEvent();
        virtual ArielEventType getEventType() const = 0;

};

}
}

#endif
