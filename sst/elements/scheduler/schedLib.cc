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

#include "linkBuilder.h"
#include "nodeComponent.h"
#include "schedComponent.h"
#include "faultInjectionComponent.h"

using namespace SST::Scheduler;


static const char * node_events[] = {"FaultEvent","JobKillEvent","JobStartEvent",NULL};

static const char * fault_events[] = {"faultActivationEvents",NULL};

static const char * link_events[] = {NULL};

static const char * builder_events[] = {"ObjectRetrievalEvent", NULL};

static const char * sched_events[] = {"ArrivalEvent","CompletionEvent","FaultEvent","FinalTimeEvent", "JobKillEvent", "JobStartEvent",NULL};

    static SST::Component*
create_schedComponent(SST::ComponentId_t id, 
                      SST::Params& params)
{
    return new schedComponent( id, params );
}

    static SST::Component*
create_nodeComponent(SST::ComponentId_t id, 
                     SST::Params& params)
{
    return new nodeComponent( id, params );
}

static SST::Component * create_linkBuilder( SST::ComponentId_t id, SST::Params & params ){
    return new linkBuilder(id, params);
}

static SST::Component * create_faultInjectionComponent( SST::ComponentId_t id, SST::Params & params ){
    return new faultInjectionComponent(id, params);
}

//name, description, events
static const SST::ElementInfoPort sched_ports[] = {
    {"Scheduler",
      "Used to communicate with the scheduler",
      sched_events
    },
    {"nodeLink%(number of node)d",
     "Each node has an associated port to send events",
     node_events
    },
    {NULL,NULL,NULL} 
};


static const SST::ElementInfoPort node_ports[] = {
    {"Scheduler",
      "Used to communicate with the scheduler",
      sched_events
    },
    {"nodeLink%(number of node)d",
     "Each node has an associated port to send events",
     node_events
    },
    {"faultInjector",
     "Causes nodes to fail",
     fault_events
    },
    {"Builder",
     "Link to communicate with parent",
     builder_events,
    },
    {"Parent0",
     "Link to communicate with parent",
     link_events
    },
    {"Child0",
     "Link to communicate with children",
     link_events
    },
    {NULL,NULL,NULL} 
};


static const SST::ElementInfoPort fault_ports[] = {
    {"faultInjector",
     "Causes nodes to fail",
     fault_events
    },
    {NULL,NULL,NULL}
};

static const SST::ElementInfoPort link_ports[] = {
    {NULL,NULL,NULL}
};

static const SST::ElementInfoParam sched_params[] = {
    { "traceName",
      "Name of the simulation trace file",
      NULL
    },
    { "scheduler",
      "Determines when jobs are run",
      "First in first out priority queue"
    },
    { "machine",
      "The geometry of the machine; how the nodes are laid out",
      "Simple machine"
    },
    { "allocator",
      "Assigns available nodes to each job",
      "Simple allocator"
    },
    { "FST",
      "Metric to analyze scheduler in terms of social justice",
      "None"
    },
    { "timeperdistance",
      "How much the layout of a job affects its run time",
      "None"
    },
    { "faultSeed",
        "PRNG seed for node faults",
        "Current time"
    },
    { "seed",
        "general PRNG seed for node faults",
        "Current time"
    },
    { "errorLogSeed",
        "PRNG seed for error log",
        "Current time"
    },
    { "jobKillSeed",
        "PRNG seed for job kill",
        "Current time"
    },
    { "useYumYumSimulationKill",
        "Simulation ends as the last job is finished being simulated",
        "False"
    },
    { "YumYumPollWait",
        "Frequency with which component polls the trace file ",
        "250"
    },
    { "useYumYumTraceFormat",
        "Should yumyum trace format be used",
        "False"
    },
    { "printYumYumJobLog",
        "Should yumyum job log be used",
        "False"
    },
    { "printJobLog",
        "Should job log be printed",
        "False"
    },
    { "errorLatencySeed",
        "PRNG seed for error latency for node failure",
        "Current time"
    },
    { "errorCorrectionSeed",
        "PRNG seed for probability an error is corrected",
        "Current time"
    },
    { "jobLogFileName",
        "Name for file to output job log",
        "REQUIRED if job log used"
    },
    {NULL,NULL,NULL}
};

static const SST::ElementInfoParam node_params[] = {
    { "nodeNum",
      "The number of the node",
      NULL
    },
    { "id",
      "The id of the node",
      NULL
    },
    { "type",
      "The type of the node",
      "None"
    },
    { "faultLogFileName",
      "File to store the fault log",
      "None"
    },
    { "errorLogFileName",
      "File to store the error log",
      "None"
    }, 
    { "faultActivationRate",
        "CSV specifying the fault type and corresponding rates",
        "None"
    },
    { "errorMessageProbability",
        "error log is written according to this probability",
        "None"
    },
    { "errorCorrectionProbability",
        "Probability that a node corrects an error",
        "None"
    },
    { "jobFailureProbability",
        "Probability that a node ends a job when a failure propogates",
        "None"
    } ,
    { "errorPropagationDelay",
        "Time taken for a fault to travel",
        "None"
    },
    {NULL,NULL,NULL}
};

static const SST::ElementInfoParam fault_params[] = {
    { "faultInjectionFilename",
        "File to use for fault injections",
        NULL
    },
    { "injectionFrequency",
        "Frequency with which injections occur",
        NULL
    },
    {NULL,NULL,NULL}
};

static const SST::ElementInfoParam link_params[] = {
    {NULL,NULL,NULL} 
};


static const SST::ElementInfoComponent components[] = {
    { "schedComponent",
        "Schedules and allocates nodes for jobs from a simulation file",
        NULL,
        create_schedComponent,
        sched_params,
        sched_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED 
    },
    { "nodeComponent",
        "Implements nodes for use with schedComponent",
        NULL,
        create_nodeComponent,
        node_params,
        node_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED
    },
    { "linkBuilder",
        "Node Graph Link Modifier",
        NULL,
        create_linkBuilder,
        link_params,
        link_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED
    },
    { "faultInjectionComponent",
        "Generates and injects failures from a flatfile",
        NULL,
        create_faultInjectionComponent,
        fault_params,
        fault_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED 
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

//name, description
static const SST::ElementInfoEvent events[] = {
    {"ArrivalEvent",
     "A new job arrives"
    },
    {"CompletionEvent",
     "A job completes"
    },
    {"FaultEvent",
     "A node fails"
    },
    {"FinalTimeEvent",
     "Used internally to denote that all events at a given time have finished"
    },
    {"JobKillEvent",
     "Tells a node the job it is running has been killed"
    },
    {"JobStartEvent",
     "Tells a node to begin executing a job"
    }, 
    {NULL,NULL,NULL,NULL}
};


extern "C" {
    SST::ElementLibraryInfo scheduler_eli = {
        "scheduler",
        "High Level Scheduler Components",
        components,
        events,
        NULL, //introspectors
        NULL, //modules
        NULL, //partitioners
        NULL, //generators
        NULL //Python Module Generator
    };
}

