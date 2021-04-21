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

#ifndef _SWM_EVENT_H
#define _SWM_EVENT_H


namespace SST {
namespace Swm {

class SwmEvent : public SST::Event {
  public:
    enum Type { StartWorkload, MP_Returned, Exit } type;
    SwmEvent( Type type, int arg1 = 0, int arg2 = 0 ) : type(type),arg1(arg1),arg2(arg2) {};
    int arg1;
    int arg2;
    NotSerializable(SwmEvent)
};

}
}


#endif
