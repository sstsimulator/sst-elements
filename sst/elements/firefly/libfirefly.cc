// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/element.h>

#include <nic.h>
#include <testDriver.h>
#include <nicTester.h>
#include <hades.h>
#include <virtNic.h>
#include <funcSM/init.h>
#include <funcSM/fini.h>
#include <funcSM/rank.h>
#include <funcSM/size.h>
#include <funcSM/alltoallv.h>
#include <funcSM/barrier.h>
#include <funcSM/allreduce.h>
#include <funcSM/allgather.h>
#include <funcSM/gatherv.h>
#include <funcSM/recv.h>
#include <funcSM/send.h>
#include <funcSM/wait.h>
#include <funcSM/waitAny.h>
#include <funcSM/waitAll.h>
#include <ctrlMsg.h>
#include <loopBack.h>
#include <merlinEvent.h>

using namespace Firefly;

BOOST_CLASS_EXPORT(MerlinFireflyEvent)

static SST::Component*
create_testDriver(SST::ComponentId_t id, SST::Params& params)
{
    return new TestDriver(id, params);
}

static SST::Component*
create_nicTester(SST::ComponentId_t id, SST::Params& params)
{
    return new NicTester( id, params );
}

static SST::Component*
create_nic(SST::ComponentId_t id, SST::Params& params)
{
    return new Nic( id, params );
}

static SST::Component*
create_loopBack(SST::ComponentId_t id, SST::Params& params)
{
    return new LoopBack( id, params );
}

static Module*
load_hades(Component* comp, Params& params)
{
    return new Hades(comp, params);
}

static Module*
load_VirtNic(Component* comp, Params& params)
{
    return new VirtNic(comp, params);
}

static Module*
load_hermesInitSM(Params& params)
{
    return new InitFuncSM(params);
}

static Module*
load_hermesFiniSM(Params& params)
{
    return new FiniFuncSM(params);
}

static Module*
load_hermesRankSM(Params& params)
{
    return new RankFuncSM(params);
}

static Module*
load_hermesSizeSM(Params& params)
{
    return new SizeFuncSM(params);
}

static Module*
load_hermesAlltoallvSM(Params& params)
{
    return new AlltoallvFuncSM(params);
}

static Module*
load_hermesBarrierSM(Params& params)
{
    return new BarrierFuncSM(params);
}

static Module*
load_hermesAllreduceSM(Params& params)
{
    return new AllreduceFuncSM(params);
}

static Module*
load_hermesAllgatherSM(Params& params)
{
    return new AllgatherFuncSM(params);
}

static Module*
load_hermesGathervSM(Params& params)
{
    return new GathervFuncSM(params);
}

static Module*
load_hermesRecvSM(Params& params)
{
    return new RecvFuncSM(params);
}

static Module*
load_hermesSendSM(Params& params)
{
    return new SendFuncSM(params);
}

static Module*
load_hermesWaitSM(Params& params)
{
    return new WaitFuncSM(params);
}

static Module*
load_hermesWaitAnySM(Params& params)
{
    return new WaitAnyFuncSM(params);
}

static Module*
load_hermesWaitAllSM(Params& params)
{
    return new WaitAllFuncSM(params);
}

static Module*
load_ctrlMsgProtocol( Component* comp, Params& params )
{
    return new CtrlMsg::API( comp, params );
}

static void init_MerlinFireflyEvent()
{
}

static const ElementInfoComponent components[] = {
    { "testDriver",
      "Firefly test driver ",
      NULL,
      create_testDriver,
    },
    { "nicTester",
      "nic tester",
      NULL,
      create_nicTester,
    },
    { "nic",
      "nic",
      NULL,
      create_nic,
    },
    { "loopBack",
      "loopBack",
      NULL,
      create_loopBack,
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "hades",
      "Firefly Hermes module",
      NULL,
      NULL,
      load_hades,
      NULL,
      "SST::Hermes::MessageInterface"
    },
    { "VirtNic",
      "Firefly VirtNic module",
      NULL,
      NULL,
      load_VirtNic,
      NULL,
      "SST::Module"
    },
    { "Init",
      "Hermes Init Function State Machine",
      NULL,
      load_hermesInitSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Fini",
      "Hermes Fini Function State Machine",
      NULL,
      load_hermesFiniSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Rank",
      "Hermes Rank Function State Machine",
      NULL,
      load_hermesRankSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Size",
      "Hermes Size Function State Machine",
      NULL,
      load_hermesSizeSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Alltoall",
      "Hermes Alltoallv Function State Machine",
      NULL,
      load_hermesAlltoallvSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Barrier",
      "Hermes Barrier Function State Machine",
      NULL,
      load_hermesBarrierSM,
      NULL,
      NULL,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Allreduce",
      "Hermes Allreduce Function State Machine",
      NULL,
      load_hermesAllreduceSM,
      NULL,
      NULL,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Reduce",
      "Hermes Reduce Function State Machine",
      NULL,
      load_hermesAllreduceSM,
      NULL,
      NULL,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Allgather",
      "Hermes Allgather Function State Machine",
      NULL,
      load_hermesAllgatherSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Allgatherv",
      "Hermes Allgatherv Function State Machine",
      NULL,
      load_hermesAllgatherSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Gatherv",
      "Hermes Gatherv Function State Machine",
      NULL,
      load_hermesGathervSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Gather",
      "Hermes Gather Function State Machine",
      NULL,
      load_hermesGathervSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Alltoallv",
      "Hermes Alltoallv Function State Machine",
      NULL,
      load_hermesAlltoallvSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Irecv",
      "Hermes Irecv Function State Machine",
      NULL,
      load_hermesRecvSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Isend",
      "Hermes Isend Function State Machine",
      NULL,
      load_hermesSendSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Recv",
      "Hermes Recv Function State Machine",
      NULL,
      load_hermesRecvSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Send",
      "Hermes Send Function State Machine",
      NULL,
      load_hermesSendSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Wait",
      "Hermes Wait Function State Machine",
      NULL,
      load_hermesWaitSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "WaitAny",
      "Hermes WaitAny Function State Machine",
      NULL,
      load_hermesWaitAnySM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "WaitAll",
      "Hermes WaitAll Function State Machine",
      NULL,
      load_hermesWaitAllSM,
      NULL,
      NULL,
      "SST::Firefly::FunctionSMInterface"
    },
    { "CtrlMsgProto",
      "Ctrl Message Pootocol",
      NULL,
      NULL,
      load_ctrlMsgProtocol,
      NULL,
      "SST::Firefly::ProtocolAPI"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL}
};

static const ElementInfoEvent events[] = {
    { "MerlinFireflyEvent",
      "MerlinFireflyEvent",
        NULL,
        init_MerlinFireflyEvent
    },
    { NULL, NULL, NULL, NULL}
};

extern "C" {
    ElementLibraryInfo firefly_eli = {
        "Firefly",
        "Is a communication protocol module used within SST to replicate message-passing style behavior",
        components,
        events,
        NULL,
        modules,
        NULL,
        NULL,
    };
}
