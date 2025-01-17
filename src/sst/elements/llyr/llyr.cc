// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/params.h>

#include <regex>
#include <queue>
#include <vector>
#include <sstream>
#include <algorithm>

#include "llyr.h"
#include "llyrTypes.h"
#include "llyrHelpers.h"
#include "parser/parser.h"
#include "mappers/mapperList.h"

namespace SST {
namespace Llyr {

LlyrComponent::LlyrComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    //initial params
    clock_enabled_ = 1;
    compute_complete = 0;
    const uint32_t verbosity = params.find< uint32_t >("verbose", 0);

    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][%s]: ", getName().c_str());
    output_ = new SST::Output(prefix, verbosity, 0, Output::STDOUT);

    //tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our Main Clock
    const std::string clock_rate = params.find< std::string >("clock", "1.0GHz");
    output_->verbose(CALL_INFO, 1, 0, "Clock is configured for %s\n", clock_rate.c_str());
    clock_tick_handler_ = new Clock::Handler<LlyrComponent>(this, &LlyrComponent::tick);
    time_converter_ = registerClock(clock_rate, clock_tick_handler_);

    //set up memory interfaces
    mem_interface_ = loadUserSubComponent<SST::Interfaces::StandardMem>("iface", ComponentInfo::SHARE_NONE, time_converter_,
                                        new StandardMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

    if( !mem_interface_ ) {
        std::string interfaceName = params.find<std::string>("memoryinterface", "memHierarchy.memInterface");
        output_->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

        Params interfaceParams = params.get_scoped_params("memoryinterfaceparams");
        interfaceParams.insert("port", "cache_link");
        mem_interface_ = loadAnonymousSubComponent<SST::Interfaces::StandardMem>(interfaceName, "iface", 0, ComponentInfo::SHARE_PORTS |
            ComponentInfo::INSERT_STATS, interfaceParams, time_converter_, new StandardMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

        if( !mem_interface_ ) {
            output_->fatal(CALL_INFO, -1, "%s, Error loading memory interface\n", getName().c_str());
        }
    }

    // set up device address space
    starting_addr_ = params.find< uint64_t >("starting_addr", 0);

    // set up MMIO address for device
    device_addr_ = params.find< uint64_t >("device_addr", 0);
    if( device_addr_ != 0x00 ) {
        clock_enabled_ = 0;
        mem_interface_->setMemoryMappedAddressRegion(device_addr_, 1);
    }

    //need a 'global' LS queue for reordering
    ls_queue_ = new LSQueue();
    ls_entries_ = params.find< uint32_t >("ls_entries", 1);

    mem_handlers_ = new LlyrMemHandlers(this, ls_queue_, output_);

    //set up param struct
    uint16_t queue_depth = params.find< uint16_t >("queue_depth", 256);
    uint16_t arith_latency = params.find< uint16_t >("arith_latency", 1);
    uint16_t int_latency = params.find< uint16_t >("int_latency", 1);
    uint16_t int_div_latency = params.find< uint16_t >("int_div_latency", 4);
    uint16_t fp_latency = params.find< uint16_t >("fp_latency", 4);
    uint16_t fp_mul_latency = params.find< uint16_t >("fp_mul_latency", 8);
    uint16_t fp_div_latency = params.find< uint16_t >("fp_div_latency", 40);
    uint16_t complex_latency = params.find< uint16_t >("complex_latency_", 80);
    std::string mapping_tool_ = params.find< std::string >("mapping_tool", "");

    configData_ = new LlyrConfig { ls_queue_, mem_interface_, starting_addr_, mapping_tool_, verbosity, queue_depth,
                                   arith_latency, int_latency, int_div_latency, fp_latency, fp_mul_latency, fp_div_latency,
                                   complex_latency };

    memFileName_ = params.find<std::string>("mem_init", "");

    //construct hardware graph
    std::string const& hwFileName = params.find< std::string >("hardware_graph", "hardware.cfg");
    constructHardwareGraph(hwFileName);

    //construct application graph
    std::string const& swFileName = params.find< std::string >("application", "app.in");
    constructSoftwareGraph(swFileName);

    //do the mapping
    Params mapperParams;    //empty but needed for loadModule API
    std::string mapperName = params.find<std::string>("mapper", "llyr.mapper.simple");
    llyr_mapper_ = loadModule<LlyrMapper>(mapperName, mapperParams);
    output_->verbose(CALL_INFO, 1, 0, "Mapping application to hardware with %s\n", mapperName.c_str());
    llyr_mapper_->mapGraph(hardwareGraph_, applicationGraph_, mappedGraph_, configData_);
    mappedGraph_.printDotHardware("llyr_mapped.dot");

    //init stats
    zeroEventCycles_ = registerStatistic< uint64_t >("cycles_zero_events");
    eventCycles_ = registerStatistic< uint64_t >("cycles_events");

    //all done
    output_->verbose(CALL_INFO, 1, 0, "Initialization done.\n");
}

LlyrComponent::LlyrComponent() :
    Component(-1)
{
    // for serialization only
}

LlyrComponent::~LlyrComponent()
{
    output_->verbose(CALL_INFO, 1, 0, "Llyr destructor fired, closing down.\n");

//     output_->verbose(CALL_INFO, 10, 0, "Dumping hardware graph...\n");
//     if( output_->getVerboseLevel() >= 10 ) {
//         hardwareGraph_.printGraph();
//         hardwareGraph_.printDot("llyr_hdwr.dot");
//     }

    output_->verbose(CALL_INFO, 10, 0, "Dumping application graph...\n");
    if( output_->getVerboseLevel() >= 10 ) {
        applicationGraph_.printGraph();
//         applicationGraph_.printDot("llyr_app.dot");

//         auto app_vertex_map_ = applicationGraph_.getVertexMap();
//         for(auto appIterator = app_vertex_map_->begin(); appIterator != app_vertex_map_->end(); ++appIterator) {
//             std::cout << appIterator->first << ": ";
//             std::cout << appIterator->second.getValue().optype_ << " - ";
//             std::cout << appIterator->second.getValue().argument_ << " - ";
//         }
    }

//     output_->verbose(CALL_INFO, 10, 0, "Dumping mapping...\n");
//     if( output_->getVerboseLevel() >= 10 ) {
//         mappedGraph_.printGraph();
//         mappedGraph_.printDot("llyr_mapped.dot");
//     }
}

void LlyrComponent::init( uint32_t phase )
{
    output_->verbose(CALL_INFO, 2, 0, "Initializing...\n");

    mem_interface_->init( phase );
    if( 0 == phase ) {
        std::vector< uint64_t >* initVector;

        //Check to see if there is any memory being initialized
        if( memFileName_ != "" ) {
            initVector = constructMemory(memFileName_);
        } else {
            initVector = new std::vector< uint64_t > {16, 64, 32, 0 , 16382, 0, 0};
        }

        std::vector<uint8_t> memInit;
        constexpr auto buff_size = sizeof(uint64_t);
        uint8_t buffer[buff_size] = {};
        for( auto it = initVector->begin(); it != initVector->end(); ++it ) {
            std::memcpy(buffer, std::addressof(*it), buff_size);
            for( uint32_t i = 0; i < buff_size; ++i ){
                memInit.push_back(buffer[i]);
            }
        }

        output_->verbose(CALL_INFO, 2, 0, ">> Writing memory contents (%" PRIu64 " bytes at index 0)\n",
                        (uint64_t) memInit.size());
//         for( std::vector< uint8_t >::iterator it = memInit.begin() ; it != memInit.end(); ++it ) {
//             std::cout << uint32_t(*it) << ' ';
//         }
//
//         std::cout << "\n";

        StandardMem::Request* initMemory = new StandardMem::Write(starting_addr_, memInit.size(), memInit);
        output_->verbose(CALL_INFO, 1, 0, "Sending initialization data to memory...\n");
        mem_interface_->sendUntimedData(initMemory);
        output_->verbose(CALL_INFO, 1, 0, "Initialization data sent.\n");
    }
}

void LlyrComponent::setup()
{
}

void LlyrComponent::finish()
{
}

bool LlyrComponent::tick(SST::Cycle_t currentCycle)
{
    // TraceFunction trace(CALL_INFO_LONG);
    if( clock_enabled_ == 0 ) {
        return false;
    }

    compute_complete = 0;
    //On each tick perform BFS on graph and compute based on operand availability
    //NOTE node0 is a dummy node to simplify the algorithm
    std::queue< uint32_t > nodeQueue;

    output_->verbose(CALL_INFO, 1, 0, "Device clock tick\n");

    //Mark all nodes in the PE graph un-visited
    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map_ = mappedGraph_.getVertexMap();
    typename std::map< uint32_t, Vertex< ProcessingElement* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        vertexIterator->second.setVisited(0);
    }

    //Node 0 is a dummy node and is always the entry point
    nodeQueue.push(0);

    //BFS and do operations if values available in input queues
    while( nodeQueue.empty() == 0 ) {
        uint32_t currentNode = nodeQueue.front();
        nodeQueue.pop();

        std::vector< Edge* >* adjacencyList = vertex_map_->at(currentNode).getAdjacencyList();

        //set visited for bfs
        vertex_map_->at(currentNode).setVisited(1);

        //send n responses from L/S unit to destination
        doLoadStoreOps(ls_entries_);

        //Let the PE decide whether or not it can do the compute
        vertex_map_->at(currentNode).getValue()->doCompute();

        //send one item from each output queue to destination
        vertex_map_->at(currentNode).getValue()->doSend();

        compute_complete = compute_complete | vertex_map_->at(currentNode).getValue()->getPendingOp();
        output_->verbose(CALL_INFO, 1, 0, "PE(%" PRIu32 ") pending: %" PRIu32 " status: %" PRIu32 "\n\n",
                        currentNode, vertex_map_->at(currentNode).getValue()->getPendingOp(), compute_complete );

        //add the destination vertices from this node to the node queue
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertx = (*it)->getDestination();
            if( vertex_map_->at(destinationVertx).getVisited() == 0 ) {
                vertex_map_->at(destinationVertx).setVisited(1);
                nodeQueue.push(destinationVertx);
            }
        }
    }

    // return false so we keep going
    if( compute_complete == 1 ){
        eventCycles_->addData(1);
        output_->verbose(CALL_INFO, 40, 0, "Continuing simulation due to live data...\n");
        return false;
    } else if( ls_queue_->getNumEntries() > 0 ) {
        zeroEventCycles_->addData(1);
        output_->verbose(CALL_INFO, 40, 0, "Continuing simulation due to live memory...\n");
        return false;
    } else {
        output_->verbose(CALL_INFO, 40, 0, "Ending simulation due to flying cows...\n");
        primaryComponentOKToEndSim();
        return true;
    }
}

void LlyrComponent::handleEvent(StandardMem::Request* req) {
    req->handle(mem_handlers_);
}

// Handler for incoming Read requests
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::Read* read) {
    out->verbose(CALL_INFO, 8, 0, "Handle Read for Address p-0x%" PRIx64 " -- v-0x%" PRIx64 ".\n", read->pAddr, read->vAddr);

    // Make a response. Must fill in payload.
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());
    llyr_->mem_interface_->send(resp);
}

// Handler for incoming Write requests
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::Write* write) {
    out->verbose(CALL_INFO, 8, 0, "Handle Write for Address p-0x%" PRIx64 " -- v-0x%" PRIx64 ".\n", write->pAddr, write->vAddr);

    llyr_->clock_enabled_ = 1;

    /* Send response (ack) if needed */
    if (!(write->posted)) {
        llyr_->mem_interface_->send(write->makeResponse());
    }
    delete write;
}

// Handler for incoming Read responses
// - should be a response to a Read we issued
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::ReadResp* resp) {

    // TraceFunction trace(CALL_INFO_LONG);

    std::stringstream dataOut;
    for( auto &it : resp->data ) {
        dataOut << unsigned(it) << " ";
    }
    out->verbose(CALL_INFO, 24, 0, "%s\n", dataOut.str().c_str());

    // Read request needs some special handling
    uint64_t addr = resp->pAddr;
    uint64_t memValue = 0;

    dataOut.str(std::string());
    LlyrData testArg;
    for( auto &it : resp->data ) {
        testArg = it;
        dataOut << testArg << " ";
    }
    out->verbose(CALL_INFO, 24, 0, "\n%s\n", dataOut.str().c_str());

    std::memcpy( std::addressof(memValue), std::addressof(resp->data[0]), sizeof(memValue) );

    testArg = memValue;
//     std::cout << "*" << testArg << std::endl;

    out->verbose(CALL_INFO, 8, 0, "Response to a read, payload=%" PRIu64 ", for addr: %" PRIu64
    " to PE %" PRIu32 "\n", memValue, addr, ls_queue_->lookupEntry( resp->getID() ).second );

    ls_queue_->setEntryData( resp->getID(), testArg );
    ls_queue_->setEntryReady( resp->getID(), 1 );

    // Need to clean up the events coming back from the cache
    delete resp;
    out->verbose(CALL_INFO, 4, 0, "Complete cache response handling.\n");
}

// Handler for incoming Write responses
// should be a response to a Write we issued
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::WriteResp* resp) {

    out->verbose(CALL_INFO, 8, 0, "Response to a write for addr: %" PRIu64 " to PE %" PRIu32 "\n",
                 resp->pAddr, ls_queue_->lookupEntry( resp->getID() ).second );
    ls_queue_->setEntryReady( resp->getID(), 2 );

    // Need to clean up the events coming back from the cache
    delete resp;
    out->verbose(CALL_INFO, 4, 0, "Complete cache response handling.\n");
}

void LlyrComponent::doLoadStoreOps( uint32_t numOps )
{
    // TraceFunction trace(CALL_INFO_LONG);
    output_->verbose(CALL_INFO, 10, 0, "Doing L/S ops\n");
    for(uint32_t i = 0; i < numOps; ++i ) {
        if( ls_queue_->getNumEntries() > 0 ) {
            StandardMem::Request::id_t next = ls_queue_->getNextEntry();

            if( ls_queue_->getEntryReady(next) == 1) {
                output_->verbose(CALL_INFO, 10, 0, "--(1)Mem Req ID %" PRIu32 "\n", uint32_t(next));
                LlyrData data = ls_queue_->getEntryData(next);
                //pass the value to the appropriate PE
                uint32_t srcPe = ls_queue_->lookupEntry( next ).first;

                mappedGraph_.getVertex(srcPe)->getValue()->doReceive(data);

                ls_queue_->removeEntry( next );
            } else if( ls_queue_->getEntryReady(next) == 2 ){
                output_->verbose(CALL_INFO, 10, 0, "--(2)Mem Req ID %" PRIu32 "\n", uint32_t(next));
                ls_queue_->removeEntry( next );
            }
        }
    }
}

void LlyrComponent::constructHardwareGraph(std::string fileName)
{
    output_->verbose(CALL_INFO, 1, 0, "Constructing Hardware Graph From: %s\n", fileName.c_str());

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {
        std::string thisLine;
        uint64_t position;
        while( std::getline( inputStream, thisLine ) ) {
            output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());

            // skip first and last lines if this is truly dot
            if( thisLine.find( "{" ) != std::string::npos || thisLine.find( "}" ) != std::string::npos) {
                continue;
            }

            // skip if this description includes dot layout information
            if( thisLine.find( "layout" ) != std::string::npos ) {
                continue;
            }

            //Ignore blank lines
            if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 ) {
                //First read all nodes
                //If all nodes read, must mean we're at edge list
                position = thisLine.find_first_of( "[" );
                if( position !=  std::string::npos ) {
                    uint32_t vertex = std::stoul( thisLine.substr( 0, position ) );

                    uint64_t posA = thisLine.find_first_of( "=" ) + 1;
                    uint64_t posB = thisLine.find_last_of( "]" );
                    std::string op = thisLine.substr( posA, posB-posA );
                    opType operation = getOptype(op);

                    output_->verbose(CALL_INFO, 10, 0, "OpString:  %s\t\t%" PRIu32 "\n", op.c_str(), operation);
                    hardwareGraph_.addVertex( vertex, operation );
                } else {
                    //edge delimiter
                    std::regex delimiter( "\\--" );

                    std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                    std::sregex_token_iterator iterB;
                    std::vector<std::string> edges( iterA, iterB );

                    edges[0].erase(remove_if(edges[0].begin(), edges[0].end(), isspace), edges[0].end());
                    edges[1].erase(remove_if(edges[1].begin(), edges[1].end(), isspace), edges[1].end());

                    output_->verbose(CALL_INFO, 10, 0, "Edges %s--%s\n", edges[0].c_str(), edges[1].c_str());

                    hardwareGraph_.addEdge( std::stoul(edges[0]), std::stoul(edges[1]) );
                }
            }
        }

        inputStream.close();
    }
    else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open %s\n", fileName.c_str() );
        exit(0);
    }

}

void LlyrComponent::constructSoftwareGraph(std::string fileName)
{
    output_->verbose(CALL_INFO, 1, 0, "Constructing Application Graph From: %s\n", fileName.c_str());

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {
        std::string thisLine;
        uint64_t position;

        std::getline( inputStream, thisLine );
        position = thisLine.find( "ModuleID" );

        output_->verbose(CALL_INFO, 16, 0, "Parsing:  %s\n", thisLine.c_str());
        if( position !=  std::string::npos ) {
            constructSoftwareGraphIR(inputStream);
        } else {
            constructSoftwareGraphApp(inputStream);
        }

        inputStream.close();
    } else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open %s\n", fileName.c_str() );
        exit(0);
    }
}

void LlyrComponent::constructSoftwareGraphIR(std::ifstream& inputStream)
{
    std::string thisLine;

    output_->verbose(CALL_INFO, 16, 0, "Sending to LLVM parser\n");

    inputStream.seekg (0, inputStream.beg);
    std::string irString( (std::istreambuf_iterator< char >( inputStream )),
                          (std::istreambuf_iterator< char >() ));
    Parser parser(irString, output_);
    parser.generateAppGraph("offload_");
}

void LlyrComponent::constructSoftwareGraphApp(std::ifstream& inputStream)
{
    std::string thisLine;
    uint64_t position;

    inputStream.seekg (0, inputStream.beg);
    while( std::getline( inputStream, thisLine ) ) {
        output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());

        //Ignore blank lines
        if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 ) {
            //First read all nodes, if all nodes read, must mean we're at edge list
            position = thisLine.find_first_of( "pe_type" );
            if( position !=  std::string::npos ) {
                AppNode tempNode;
                uint32_t vertex = std::stoul( thisLine.substr( 0, position - 2 ) );

                std::regex delimiter( ",| " );
                std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                std::sregex_token_iterator iterB;
                std::vector< std::string > edges( iterA, iterB );

                //clean up the strings a bit
                for( auto testIter = edges.begin(); testIter != edges.end(); ++testIter ) {
                    testIter->erase(remove_if(testIter->begin(), testIter->end(), isspace), testIter->end());
                    testIter->erase(remove(testIter->begin(), testIter->end(), '['), testIter->end());
                    testIter->erase(remove(testIter->begin(), testIter->end(), ']'), testIter->end());
                    output_->verbose(CALL_INFO, 10, 0, "Hiho %s\n", testIter->c_str());
                }

                //pe_type= - 8chars
                std::string op = edges[1].substr(8);
                opType operation = getOptype(op);
                tempNode.optype_ = operation;
                output_->verbose(CALL_INFO, 10, 0, "OpString:  %s\t\t%" PRIu32 "\n", op.c_str(), tempNode.optype_);

                //Check to see if this PE has any arguments
                for( uint32_t i = 2; i < edges.size(); ++i ) {
                    tempNode.argument_[i - 2] = edges[i];

                    std::cout << i << " - " << edges[i] << " ";
                    std::cout << i - 2 << " : " <<tempNode.argument_[i - 2] << " ";
                }
                std::cout << std::endl;

                applicationGraph_.addVertex( vertex, tempNode );
            } else {
                std::regex delimiter( "\\--" );
                std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                std::sregex_token_iterator iterB;
                std::vector< std::string > edges( iterA, iterB );

                edges[0].erase(remove_if(edges[0].begin(), edges[0].end(), isspace), edges[0].end());
                edges[1].erase(remove_if(edges[1].begin(), edges[1].end(), isspace), edges[1].end());

                output_->verbose(CALL_INFO, 10, 0, "Edges %s--%s\n", edges[0].c_str(), edges[1].c_str());

                applicationGraph_.addEdge( std::stoul(edges[0]), std::stoul(edges[1]) );
            }
        }
    }
}

std::vector< uint64_t >* LlyrComponent::constructMemory(std::string fileName)
{
    std::vector< uint64_t >* tempVector = new std::vector< uint64_t >;

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {

        std::string thisLine;
        while( std::getline( inputStream, thisLine ) )
        {
            std::string value;
            std::stringstream stringIn(thisLine);
            while( std::getline(stringIn, value, ',') ) {
                tempVector->push_back(std::stoull(value));
            }
        }

//         std::cout << "Init Vector(" << tempVector->size() << "):  ";
//         for( auto it = tempVector->begin(); it != tempVector->end(); ++it ) {
//             std::cout << *it;
//             std::cout << " ";
//         }
//         std::cout << std::endl;

        inputStream.close();
    } else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open %s\n", fileName.c_str() );
        exit(0);
    }

    return tempVector;
}

} // namespace llyr
} // namespace SST


