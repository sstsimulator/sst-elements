// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/params.h>

#include <regex>
#include <queue>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "llyr.h"
#include "llyrTypes.h"
#include "mappers/simpleMapper.h"

namespace SST {
namespace Llyr {

LlyrComponent::LlyrComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    //initial params
    compute_complete = 0;
    const uint32_t verbosity = params.find< uint32_t >("verbose", 0);
    clock_count = params.find< int64_t >("clockcount", 10);

    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][%s]: ", getName().c_str());
    output_ = new SST::Output(prefix, verbosity, 0, Output::STDOUT);

    //tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our Main Clock
    const std::string clock_rate = params.find< std::string >("clock", "1.0GHz");
    std::cout << "Clock is configured for: " << clock_rate << std::endl;
    clock_tick_handler_ = new Clock::Handler<LlyrComponent>(this, &LlyrComponent::tick);
    time_converter_ = registerClock(clock_rate, clock_tick_handler_);

    //set up memory interfaces
    mem_interface_ = loadUserSubComponent<SimpleMem>("memory", ComponentInfo::SHARE_NONE, time_converter_,
        new SimpleMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

    if( !mem_interface_ ) {
        std::string interfaceName = params.find<std::string>("memoryinterface", "memHierarchy.mem_interface_");
        output_->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

        Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
        interfaceParams.insert("port", "cache_link");
        mem_interface_ = loadAnonymousSubComponent<SimpleMem>(interfaceName, "memory", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
                interfaceParams, time_converter_, new SimpleMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

        if( !mem_interface_ ) {
            output_->fatal(CALL_INFO, -1, "%s, Error loading memory interface\n", getName().c_str());
        }
    }

    //need a 'global' LS queue for reordering
    ls_queue_ = new LSQueue();
    ls_entries_ = params.find< uint32_t >("ls_entries", 1);

    //construct hardware graph
    std::string const& hwFileName = params.find< std::string >("hardwareGraph", "grid.cfg");
    output_->verbose(CALL_INFO, 1, 0, "Constructing Hardware Graph From: %s\n", hwFileName.c_str());
    constructHardwareGraph(hwFileName);

    std::string const& swFileName = params.find< std::string >("hardwareGraph", "app.in");
    output_->verbose(CALL_INFO, 1, 0, "Constructing Application Graph From: %s\n", swFileName.c_str());
    constructSoftwareGraph(swFileName);

    //do the mapping
    Params mapperParams;    //empty but needed for loadModule API
    std::string mapperName = params.find<std::string>("mapper", "llyr.simpleMapper");
    llyr_mapper_ = dynamic_cast<LlyrMapper*>( loadModule(mapperName, mapperParams) );
    output_->verbose(CALL_INFO, 1, 0, "Mapping application to hardware\n");
    llyr_mapper_->mapGraph(hardwareGraph_, applicationGraph_, mappedGraph_, ls_queue_, mem_interface_);

    //all done
    output_->verbose(CALL_INFO, 1, 0, "Initialization done.\n");
}

LlyrComponent::~LlyrComponent()
{
    output_->verbose(CALL_INFO, 1, 0, "Llyr destructor fired, closing down.\n");

    output_->verbose(CALL_INFO, 1, 0, "Dumping hardware graph\n");
    hardwareGraph_.printGraph();
    hardwareGraph_.printDot("llyr_hdwr.dot");

    output_->verbose(CALL_INFO, 1, 0, "Dumping application graph\n");
    applicationGraph_.printGraph();
    applicationGraph_.printDot("llyr_app.dot");

    output_->verbose(CALL_INFO, 1, 0, "Dumping mapping\n");
    mappedGraph_.printGraph();
//     mappedGraph.printDot("llyr_mapped.dot");
}

LlyrComponent::LlyrComponent() :
    Component(-1)
{
    // for serialization only
}

void LlyrComponent::init( uint32_t phase )
{
    mem_interface_->init( phase );

    const uint32_t mooCows = 128;
    if( 0 == phase ) {
        std::vector<uint8_t> memInit;
        memInit.reserve( mooCows );

        for( size_t i = 0; i < mooCows; ++i ) {
            if( i % 8 == 0 )
                memInit.push_back(i);
            else
                memInit.push_back(0);
        }

        output_->verbose(CALL_INFO, 2, 0, ">> Writing memory contents (%" PRIu64 " bytes at index 0)\n",
                        (uint64_t) memInit.size());
        for( std::vector< uint8_t >::iterator it = memInit.begin() ; it != memInit.end(); ++it ) {
            std::cout << uint32_t(*it) << ' ';
        }

        std::cout << "\n";

        SimpleMem::Request* initMemory = new SimpleMem::Request(SimpleMem::Request::Write, 0, memInit.size(), memInit);
        output_->verbose(CALL_INFO, 1, 0, "Sending initialization data to memory...\n");
        mem_interface_->sendInitData(initMemory);
        output_->verbose(CALL_INFO, 1, 0, "Initialization data sent.\n");
    }
}

void LlyrComponent::setup()
{
}

void LlyrComponent::finish()
{
}

bool LlyrComponent::tick( Cycle_t )
{
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

//         std::cout << "\n Adjacency list of vertex " << currentNode << "\n head ";
        std::vector< Edge* >* adjacencyList = vertex_map_->at(currentNode).getAdjacencyList();

        //set visited for bfs
        vertex_map_->at(currentNode).setVisited(1);

        //send one item from each output queue to destination
        vertex_map_->at(currentNode).getType()->doSend();

        //send n responses from L/S unit to destination
        doLoadStoreOps(ls_entries_);

        //Let the PE decide whether or not it can do the compute
        vertex_map_->at(currentNode).getType()->doCompute();
        compute_complete = compute_complete | vertex_map_->at(currentNode).getType()->getPendingOp();

        //add the destination vertices from this node to the node queue
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertx = (*it)->getDestination();
            if( vertex_map_->at(destinationVertx).getVisited() == 0 ) {
//                 std::cout << " -> " << destinationVertx;
                vertex_map_->at(destinationVertx).setVisited(1);
                nodeQueue.push(destinationVertx);
            }
        }
        std::cout << std::endl;
    }

    clock_count--;

    // return false so we keep going
    if( ls_queue_->getNumEntries() > 0 ) {
        return false;
    } else if (compute_complete == 1 ){
        return false;
    } else {
        primaryComponentOKToEndSim();
        return true;
    }
}

void LlyrComponent::handleEvent( SimpleMem::Request* ev ) {
    output_->verbose(CALL_INFO, 4, 0, "Recv response from cache\n");

    for( auto &it : ev->data ) {
        std::cout << unsigned(it) << " ";
    }
    std::cout << std::endl;

    if( ev->cmd == SimpleMem::Request::Command::ReadResp ) {
        // Read request needs some special handling
        uint64_t addr = ev->addr;
        uint64_t memValue = 0;

        LlyrData testArg;
        for( auto &it : ev->data ) {
            testArg = it;
            std::cout << testArg << " ";
        }
        std::cout << std::endl;

        std::memcpy( std::addressof(memValue), std::addressof(ev->data[0]), sizeof(memValue) );

        testArg = memValue;
        std::cout << "*" << testArg << std::endl;

        output_->verbose(CALL_INFO, 8, 0, "Response to a read, payload=%" PRIu64 ", for addr: %" PRIu64
                         " to PE %" PRIu32 "\n", memValue, addr, ls_queue_->lookupEntry( ev->id ).second );

        ls_queue_->setEntryData( ev->id, testArg );
        ls_queue_->setEntryReady( ev->id, 1 );
    } else {
        output_->verbose(CALL_INFO, 8, 0, "Response to a write for addr: %" PRIu64 " to PE %" PRIu32 "\n",
                         ev->addr, ls_queue_->lookupEntry( ev->id ).second );
        ls_queue_->setEntryReady( ev->id, 2 );
    }

    // Need to clean up the events coming back from the cache
    delete ev;
    output_->verbose(CALL_INFO, 4, 0, "Complete cache response handling.\n");
}

void LlyrComponent::doLoadStoreOps( uint32_t numOps )
{
    output_->verbose(CALL_INFO, 0, 0, "Doing L/S ops\n");
    for(uint32_t i = 0; i < numOps; ++i ) {
        if( ls_queue_->getNumEntries() > 0 ) {
            SimpleMem::Request::id_t next = ls_queue_->getNextEntry();

            if( ls_queue_->getEntryReady(next) == 1) {
                output_->verbose(CALL_INFO, 0, 0, "Mem Req ID %" PRIu32 "\n", uint32_t(next));
                LlyrData data = ls_queue_->getEntryData(next);
                //pass the value to the appropriate PE
                uint32_t dstPe = ls_queue_->lookupEntry( next ).second;
                uint32_t srcPe = ls_queue_->lookupEntry( next ).first;

                uint32_t dstQueue = mappedGraph_.getVertex(dstPe)->getType()->getInputQueueId(srcPe);
                mappedGraph_.getVertex(dstPe)->getType()->pushInputQueue(dstQueue, data);
                std::cout << "src PE " << srcPe;
                std::cout << " dst PE " << dstPe;
                std::cout << "-" << dstQueue;
                std::cout << std::endl;

                ls_queue_->removeEntry( next );
            } else if( ls_queue_->getEntryReady(next) == 2 ){
                output_->verbose(CALL_INFO, 0, 0, "Mem Req ID %" PRIu32 "\n", uint32_t(next));
                ls_queue_->removeEntry( next );
            }
        }
    }
}

void LlyrComponent::constructHardwareGraph(std::string fileName)
{
    std::ifstream inputStream(fileName, std::ios::in);

    if( inputStream.is_open() )
    {
        std::string thisLine;
        std::uint64_t position;
        while( std::getline( inputStream, thisLine ) )
        {
//             std::cout << "Parse " << thisLine << std::endl;

            //Ignore blank lines
            if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 )
            {
                //First read all nodes
                //If all nodes read, must mean we're at edge list
                position = thisLine.find_first_of( "[" );
                if( position !=  std::string::npos )
                {
                    uint32_t vertex = std::stoi( thisLine.substr( 0, position ) );

                    std::uint64_t posA = thisLine.find_first_of( "\"" ) + 1;
                    std::uint64_t posB = thisLine.find_last_of( "\"" );
                    std::string op = thisLine.substr( posA, posB-posA );
                    opType operation = getOptype(op);

                    std::cout << "OpString " << op << "\t\t" << operation << std::endl;
                    hardwareGraph_.addVertex( vertex, operation );
                }
                else
                {
//                     std::cout << "\t*Parse " << thisLine << std::endl;
                    std::regex delimiter( "\\->" );

                    std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                    std::sregex_token_iterator iterB;
                    std::vector<std::string> edges( iterA, iterB );

                    hardwareGraph_.addEdge( std::stoi(edges[0].substr(0, edges[0].find(" "))), std::stoi(edges[1].substr(0, edges[1].find(" "))) );
                }
            }
        }

        inputStream.close();
    }
    else
    {
        std::cout << "Unable to open file";
        exit(0);
    }

}

void LlyrComponent::constructSoftwareGraph(std::string fileName)
{
    std::ifstream inputStream(fileName, std::ios::in);

    if( inputStream.is_open() )
    {
        std::string thisLine;
        std::uint64_t position;
        while( std::getline( inputStream, thisLine ) )
        {
            std::cout << "Parse " << thisLine << std::endl;

            //Ignore blank lines
            if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 )
            {
                //First read all nodes
                //If all nodes read, must mean we're at edge list
                position = thisLine.find_first_of( "[" );
                if( position !=  std::string::npos )
                {
                    uint32_t vertex = std::stoi( thisLine.substr( 0, position ) );

                    std::uint64_t posA = thisLine.find_first_of( "\"" ) + 1;
                    std::uint64_t posB = thisLine.find_last_of( "\"" );
                    std::string op = thisLine.substr( posA, posB-posA );
                    opType operation = getOptype(op);

                    std::cout << "OpString " << op << "\t\t" << operation << std::endl;
                    applicationGraph_.addVertex( vertex, operation );
                }
                else
                {
                    std::cout << "\t*Parse " << thisLine << std::endl;
                    std::regex delimiter( "\\->" );

                    std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                    std::sregex_token_iterator iterB;
                    std::vector<std::string> edges( iterA, iterB );

                    applicationGraph_.addEdge( std::stoi(edges[0].substr(0, edges[0].find(" "))), std::stoi(edges[1].substr(0, edges[1].find(" "))) );
                }
            }
        }

        inputStream.close();
    }
    else
    {
        std::cout << "Unable to open file";
        exit(0);
    }

}

opType LlyrComponent::getOptype(std::string &opString) const
{
    opType operation;

    if( opString == "ANY" )
        operation = ANY;
    else if( opString == "LD" )
        operation = LD;
    else if( opString == "ST" )
        operation = ST;
    else if( opString == "ADD" )
        operation = ADD;
    else if( opString == "SUB" )
        operation = SUB;
    else if( opString == "MUL" )
        operation = MUL;
    else if( opString == "DIV" )
        operation = DIV;
    else if( opString == "FPADD" )
        operation = FPADD;
    else if( opString == "FPSUB" )
        operation = FPSUB;
    else if( opString == "FPMUL" )
        operation = FPMUL;
    else if( opString == "FPDIV" )
        operation = FPDIV;
    else
        operation = OTHER;

    return operation;
}

} // namespace llyr
} // namespace SST


