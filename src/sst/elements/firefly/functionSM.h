// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCTIONSM_H
#define COMPONENTS_FIREFLY_FUNCTIONSM_H

#include <sst/core/output.h>
#include <sst/core/params.h>

#include "sst/elements/hermes/msgapi.h"
#include "ctrlMsgFunctors.h"
#include "funcSM/api.h"

#include "info.h"

namespace SST {
namespace Firefly {

class FunctionSMInterface;
class ProtocolAPI;

#define FOREACH_FUNCTION(NAME) \
        NAME(Init)   \
        NAME(Fini)  \
        NAME(Rank)   \
        NAME(Size)   \
        NAME(MakeProgress)   \
        NAME(Barrier)   \
        NAME(Allreduce)   \
        NAME(Allgather)   \
        NAME(Gatherv)   \
        NAME(Alltoallv)   \
        NAME(Send)   \
        NAME(Recv)   \
        NAME(Wait)   \
        NAME(WaitAny)   \
        NAME(WaitAll)   \
        NAME(CommSplit)   \
        NAME(CommCreate)   \
        NAME(CommDestroy)   \
        NAME(NumFunctions)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class FunctionSM  {

    typedef FunctionSMInterface::Retval Retval;
	typedef CtrlMsg::Functor_0<FunctionSM, bool> Functor;

    static const char *m_functionName[];

  public:
    typedef std::function<void()> Callback;

    enum FunctionEnum{
        FOREACH_FUNCTION(GENERATE_ENUM)
    };

    const char *functionName( FunctionEnum x) {return m_functionName[x]; }

    FunctionSM( SST::Params& params, SST::Component*, ProtocolAPI* );
    ~FunctionSM();
    void printStatus( Output& );

    Link* getRetLink() { return m_toMeLink; } 
    void setup( Info* );
    void start(int type, MP::Functor* retFunc,  SST::Event* );
    void start(int type, Callback,  SST::Event* );
    void enter( );

  private:
    void handleStartEvent( SST::Event* );
    void handleToDriver(SST::Event*);
    void handleEnterEvent( SST::Event* );
    void processRetval( Retval& );
    
    void initFunction( SST::Component*, Info*, FunctionEnum,
                                    std::string, Params&, Params& );

    std::vector<FunctionSMInterface*>  m_smV; 
    FunctionSMInterface*    m_sm; 
    MP::Functor*    m_retFunc;
    Callback        m_callback;

    SST::Link*          m_fromDriverLink;    
    SST::Link*          m_toDriverLink;    
    SST::Link*          m_toMeLink;
    Output              m_dbg;
    SST::Params         m_params;
    SST::Component*     m_owner;
    ProtocolAPI*	m_proto;
};

}
}

#endif
