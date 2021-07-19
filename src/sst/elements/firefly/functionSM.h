// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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
        NAME(Scatterv)   \
        NAME(Alltoallv)   \
        NAME(Send)   \
        NAME(Recv)   \
        NAME(Cancel)   \
        NAME(Test)   \
        NAME(Testany)   \
        NAME(Wait)   \
        NAME(WaitAny)   \
        NAME(WaitAll)   \
        NAME(CommSplit)   \
        NAME(CommCreate)   \
        NAME(CommDestroy)   \
        NAME(NumFunctions)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class FunctionSM : public SubComponent {

    typedef FunctionSMInterface::Retval Retval;
	typedef CtrlMsg::Functor_0<FunctionSM, bool> Functor;

    static const char *m_functionName[];

  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::FunctionSM,ProtocolAPI*)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        FunctionSM,
        "firefly",
        "functionSM",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Firefly::FunctionSM
    )

	SST_ELI_DOCUMENT_PARAMS(
		{"verboseLevel","Sets the output level","0"},
		{"defaultModule","Sets the component where functions reside","firefly"},
		{"defaultEnterLatency","Sets the default latency to enter a function","0"},
		{"defaultReturnLatency","Sets the default latency to return from a function","0"},
		{"smallCollectiveVN","Sets the VN to use for small collectives","0"},
		{"smallCollectiveSize","Sets the size of small collectives","0"},
		{"nodeId","Sets the node ID",""},
	)
	/* PARAMS
		This component also looks for function names as the top of a parameter hierarchy such as "Fini.*"
	*/

    typedef std::function<void()> Callback;

    enum FunctionEnum{
        FOREACH_FUNCTION(GENERATE_ENUM)
    };

    const char *functionName( FunctionEnum x) {return m_functionName[x]; }

    FunctionSM( ComponentId_t, Params& params, ProtocolAPI* );
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

    void initFunction( Info*, FunctionEnum,
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
    ProtocolAPI*	m_proto;
};

}
}

#endif
