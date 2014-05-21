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

static const ElementInfoParam testDriver_params[] = {
	{ NULL, NULL, NULL }
};

static SST::Component*
create_nicTester(SST::ComponentId_t id, SST::Params& params)
{
    return new NicTester( id, params );
}

static const ElementInfoParam nicTester_params[] = {
	{ NULL, NULL, NULL }
};

static SST::Component*
create_nic(SST::ComponentId_t id, SST::Params& params)
{
    return new Nic( id, params );
}

static const ElementInfoParam nic_params[] = {
	{ "nid", "node id on network", "-1"},
	{ "rtrPortName", "Port connected to the router", "rtr"},
	{ "corePortName", "Port connected to the core", "core"},
	{ "num_vNics", "Sets number of cores", "1"},
	{ "verboseLevel", "Sets the output verbosity of the component", "1"},
	{ "debug", "Sets the messaging API of the end point", "0"},
	{ "rxMatchDelay_ns", "Sets the delay for a receive match", "100"},
	{ "txDelay_ns", "Sets the delay for a send", "100"},
	{ "topology", "Sets the network topology", "merlin.torus"},
	{ "fattree:loading", "Sets the number of ports on edge router connected to nodes", "8"},
	{ "fattree:radix", "Sets the number of ports on the network switches", "16"},
	{ "packetSize", "Sets the size of the network packet in bytes", "64"},
	{ "link_bw", "Sets the bandwidth of link connected to the router", "500Mhz"},
	{ "buffer_size", "Sets the buffer size of the link connected to the router", "128"},
	{ "module", "Sets the link control module", "merlin.linkcontrol"},
	{ NULL, NULL, NULL }
};

static const char * nic_port_events[] = { "MerlinFireflyEvent", NULL };
static const char * core_port_events[] = { "NicRespEvent", NULL };

static const ElementInfoPort nic_ports[] = {
    {"rtr", "Port connected to the router", nic_port_events},
    {"core%(num_vNics)d", "Ports connected to the network driver", core_port_events},
    {NULL, NULL, NULL}
};

static SST::Component*
create_loopBack(SST::ComponentId_t id, SST::Params& params)
{
    return new LoopBack( id, params );
}

static const char * loopBack_port_events[] = {"LoopBackEvent",NULL};

static const ElementInfoParam loopBack_params[] = {
    {"numCores","Sets the number cores to create links to", "1"},
	{ NULL, NULL, NULL }
};

static const ElementInfoPort loopBack_ports[] = {
    {"core%(num_vNics)d", "Ports connected to the network driver", loopBack_port_events},
    {NULL, NULL, NULL}
};


static Module*
load_hades(Component* comp, Params& params)
{
    return new Hades(comp, params);
}

static const ElementInfoParam hadesModule_params[] = {
    {"nidListString","Sets the rank to nid mapping", ""},
    {"nicModule", "Sets the NIC module", "firefly.VirtNic"},
	{"verboseLevel", "Sets the output verbosity of the component", "1"},
	{"debug", "Sets the messaging API of the end point", "0"},
	{"loopBackPortName","Sets port name to use when connecting to the loopBack component","loop"},
	{"defaultVerbose","Sets the default function verbose level","0"},
	{"defaultDebug","Sets the default function debug level","0"},
	{"defaultEnterLatency","Sets the default function enter latency","30000"},
	{"defaultReturnLatency","Sets the default function return latency","30000"},
	{"defaultModule","Sets the default function module","firefly"},
	{"enterLatency","internal",""},
	{"returnLatency","internal",""},
	{"module","internal",""},
    {"worldRank", "internal", ""},
    {"nodeId", "internal", ""},
    {"name", "internal", ""},
    {NULL, NULL}
};

static Module*
load_VirtNic(Component* comp, Params& params)
{
    return new VirtNic(comp, params);
}

static const ElementInfoParam virtNicModule_params[] = {
	{"debugLevel", "Sets the output verbosity of the component", "1"},
	{"debug", "Sets the messaging API of the end point", "0"},
	{"portName", "Sets the name of the port for the link", "nic"},
    {NULL, NULL}
};

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

static const ElementInfoParam funcSMModule_params[] = {
    {"enterLatency", "Sets the time to enter a function", "30"},
    {"returnLatency", "Sets the time to leave a function", "30"},
    {"debug", "Set the debug level", "0"},
    {"verbose", "Set the verbose level", "1"},
	
    {NULL, NULL}
};

static Module*
load_ctrlMsgProtocol( Component* comp, Params& params )
{
    return new CtrlMsg::API( comp, params );
}

static const ElementInfoParam ctrlMsgProtocolModule_params[] = {
    {"shortMsgLength","Sets the short to long message transition point", "16000"},
    {"verboseLevel","Set the verbose level", "1"},
    {"debug","Set the debug level", "0"},
    {"memcpyDelay_ps","Sets the time to copy memory", "200"},
    {"matchDelay_ns","Sets the time to do a match", "100"},
    {"txDelay_ns","Sets the time to setup a Send", "100"},
    {"rxDelay_ns", "Sets the time setup a Recv","100"},
    {"rxNicDelay_ns","", "0"},
    {"txNicDelay_ns","", "0"},
    {"regRegionXoverLength","Sets the transition point page pinning", "4096"},
    {"regRegionPerByteDelay_ps","Sets the time to pin pages", "28"},
    {"regRegionBaseDelay_ps","Sets the base time to pin pages", "10000000"},
    {NULL, NULL}
};

static void init_MerlinFireflyEvent()
{
}

static const ElementInfoComponent components[] = {
    { "testDriver",
      "Firefly test driver ",
      NULL,
      create_testDriver,
      NULL,//testDriver_params
    },
    { "nicTester",
      "nic tester",
      NULL,
      create_nicTester,
	  NULL,//nicTester_params
    },
    { "nic",
      "nic",
      NULL,
      create_nic,
      nic_params,
      nic_ports
    },
    { "loopBack",
      "loopBack",
      NULL,
      create_loopBack,
      loopBack_params,
      loopBack_ports
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "hades",
      "Firefly Hermes module",
      NULL,
      NULL,
      load_hades,
      hadesModule_params,
      "SST::Hermes::MessageInterface"
    },
    { "VirtNic",
      "Firefly VirtNic module",
      NULL,
      NULL,
      load_VirtNic,
      virtNicModule_params,
      "SST::Module"
    },
    { "Init",
      "Hermes Init Function State Machine",
      NULL,
      load_hermesInitSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Fini",
      "Hermes Fini Function State Machine",
      NULL,
      load_hermesFiniSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Rank",
      "Hermes Rank Function State Machine",
      NULL,
      load_hermesRankSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Size",
      "Hermes Size Function State Machine",
      NULL,
      load_hermesSizeSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Alltoall",
      "Hermes Alltoallv Function State Machine",
      NULL,
      load_hermesAlltoallvSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Barrier",
      "Hermes Barrier Function State Machine",
      NULL,
      load_hermesBarrierSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Allreduce",
      "Hermes Allreduce Function State Machine",
      NULL,
      load_hermesAllreduceSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Reduce",
      "Hermes Reduce Function State Machine",
      NULL,
      load_hermesAllreduceSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::CollectiveTreeFuncSM"
    },
    { "Allgather",
      "Hermes Allgather Function State Machine",
      NULL,
      load_hermesAllgatherSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Allgatherv",
      "Hermes Allgatherv Function State Machine",
      NULL,
      load_hermesAllgatherSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Gatherv",
      "Hermes Gatherv Function State Machine",
      NULL,
      load_hermesGathervSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Gather",
      "Hermes Gather Function State Machine",
      NULL,
      load_hermesGathervSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Alltoallv",
      "Hermes Alltoallv Function State Machine",
      NULL,
      load_hermesAlltoallvSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Irecv",
      "Hermes Irecv Function State Machine",
      NULL,
      load_hermesRecvSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Isend",
      "Hermes Isend Function State Machine",
      NULL,
      load_hermesSendSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Recv",
      "Hermes Recv Function State Machine",
      NULL,
      load_hermesRecvSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Send",
      "Hermes Send Function State Machine",
      NULL,
      load_hermesSendSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "Wait",
      "Hermes Wait Function State Machine",
      NULL,
      load_hermesWaitSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "WaitAny",
      "Hermes WaitAny Function State Machine",
      NULL,
      load_hermesWaitAnySM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "WaitAll",
      "Hermes WaitAll Function State Machine",
      NULL,
      load_hermesWaitAllSM,
      NULL,
      funcSMModule_params,
      "SST::Firefly::FunctionSMInterface"
    },
    { "CtrlMsgProto",
      "Ctrl Message Pootocol",
      NULL,
      NULL,
      load_ctrlMsgProtocol,
	  ctrlMsgProtocolModule_params,
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
