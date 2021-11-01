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
#include <algorithm>

#include "llyr.h"
#include "llyrTypes.h"
#include "parser/parser.h"
#include "mappers/mapperList.h"

namespace SST {
namespace Llyr {

LlyrComponent::LlyrComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    //initial params
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
    std::cout << "Clock is configured for: " << clock_rate << std::endl;
    clock_tick_handler_ = new Clock::Handler<LlyrComponent>(this, &LlyrComponent::tick);
    time_converter_ = registerClock(clock_rate, clock_tick_handler_);

    //set up memory interfaces
    mem_interface_ = loadUserSubComponent<SST::Experimental::Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, time_converter_,
                                        new StandardMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

    if( !mem_interface_ ) {
        std::string interfaceName = params.find<std::string>("memoryinterface", "memHierarchy.memInterface");
        output_->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

        Params interfaceParams = params.get_scoped_params("memoryinterfaceparams");
        interfaceParams.insert("port", "cache_link");
        mem_interface_ = loadAnonymousSubComponent<SST::Experimental::Interfaces::StandardMem>(interfaceName, "memory", 0, ComponentInfo::SHARE_PORTS |
            ComponentInfo::INSERT_STATS, interfaceParams, time_converter_, new StandardMem::Handler<LlyrComponent>(this, &LlyrComponent::handleEvent));

        if( !mem_interface_ ) {
            output_->fatal(CALL_INFO, -1, "%s, Error loading memory interface\n", getName().c_str());
        }
    }

    // set up MMIO address for device
    device_addr_ = params.find< uint64_t >("device_addr", 0);
    mem_interface_->setMemoryMappedAddressRegion(device_addr_, 1);

    //need a 'global' LS queue for reordering
    ls_queue_ = new LSQueue();
    ls_entries_ = params.find< uint32_t >("ls_entries", 1);

    mem_handlers_ = new LlyrMemHandlers(this, ls_queue_, output_);

    //set up param struct
    uint16_t queue_depth = params.find< uint16_t >("queue_depth", 256);
    uint16_t arith_latency = params.find< uint16_t >("arith_latency", 1);
    uint16_t int_latency = params.find< uint16_t >("int_latency", 1);
    uint16_t fp_latency = params.find< uint16_t >("fp_latency", 1);
    uint16_t fp_mul_latency = params.find< uint16_t >("fp_mul_latency", 1);
    uint16_t fp_div_latency = params.find< uint16_t >("fp_div_latency", 1);

    configData_ = new LlyrConfig { ls_queue_, mem_interface_, verbosity, queue_depth, arith_latency,
                                   int_latency, fp_latency, fp_mul_latency, fp_div_latency };

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
    llyr_mapper_ = dynamic_cast<LlyrMapper*>( loadModule(mapperName, mapperParams) );
    output_->verbose(CALL_INFO, 1, 0, "Mapping application to hardware with %s\n", mapperName.c_str());
    llyr_mapper_->mapGraph(hardwareGraph_, applicationGraph_, mappedGraph_, configData_);
    mappedGraph_.printDot("llyr_mapped.dot");
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

//     output_->verbose(CALL_INFO, 10, 0, "Dumping hardware graph\n");
//     if( output_->getVerboseLevel() >= 10 ) {
//         hardwareGraph_.printGraph();
//         hardwareGraph_.printDot("llyr_hdwr.dot");
//     }
//
//     output_->verbose(CALL_INFO, 10, 0, "Dumping application graph\n");
//     if( output_->getVerboseLevel() >= 10 ) {
//         applicationGraph_.printGraph();
//         applicationGraph_.printDot("llyr_app.dot");
//     }
//
//     output_->verbose(CALL_INFO, 10, 0, "Dumping mapping\n");
//     if( output_->getVerboseLevel() >= 10 ) {
//         mappedGraph_.printGraph();
//         mappedGraph_.printDot("llyr_mapped.dot");
//     }
}

void LlyrComponent::init( uint32_t phase )
{
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

        StandardMem::Request* initMemory = new StandardMem::Write(0, memInit.size(), memInit);
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
        vertex_map_->at(currentNode).getValue()->doSend();

        //send n responses from L/S unit to destination
        doLoadStoreOps(ls_entries_);

        //Let the PE decide whether or not it can do the compute
        vertex_map_->at(currentNode).getValue()->doCompute();
        compute_complete = compute_complete | vertex_map_->at(currentNode).getValue()->getPendingOp();

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

    // return false so we keep going
    if( ls_queue_->getNumEntries() > 0 ) {
        return false;
    } else if( compute_complete == 1 ){
        return false;
    } else {
        primaryComponentOKToEndSim();
        return true;
    }
}

void LlyrComponent::handleEvent(StandardMem::Request* req) {
    req->handle(mem_handlers_);
}

/* Handler for incoming Read requests */
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::Write* write) {
    out->verbose(CALL_INFO, 8, 0, "Handle Write.\n");

    /* Send response (ack) if needed */
    if (!(write->posted)) {
        llyr_->mem_interface_->send(write->makeResponse());
    }
    delete write;
}

/* Handler for incoming Read responses - should be a response to a Read we issued */
void LlyrComponent::LlyrMemHandlers::handle(StandardMem::ReadResp* resp) {

    for( auto &it : resp->data ) {
        std::cout << unsigned(it) << " ";
    }
    std::cout << std::endl;

    // Read request needs some special handling
    uint64_t addr = resp->pAddr;
    uint64_t memValue = 0;

    LlyrData testArg;
    for( auto &it : resp->data ) {
        testArg = it;
        std::cout << testArg << " ";
    }
    std::cout << std::endl;

    std::memcpy( std::addressof(memValue), std::addressof(resp->data[0]), sizeof(memValue) );

    testArg = memValue;
    std::cout << "*" << testArg << std::endl;

    out->verbose(CALL_INFO, 8, 0, "Response to a read, payload=%" PRIu64 ", for addr: %" PRIu64
    " to PE %" PRIu32 "\n", memValue, addr, ls_queue_->lookupEntry( resp->getID() ).second );

    ls_queue_->setEntryData( resp->getID(), testArg );
    ls_queue_->setEntryReady( resp->getID(), 1 );

    // Need to clean up the events coming back from the cache
    delete resp;
    out->verbose(CALL_INFO, 4, 0, "Complete cache response handling.\n");
}

/* Handler for incoming Write responses - should be a response to a Write we issued */
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
        std::uint64_t position;
        while( std::getline( inputStream, thisLine ) ) {
            output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());

            if( thisLine.find( "{" ) != std::string::npos || thisLine.find( "}" ) != std::string::npos) {
                continue;
            }

            //Ignore blank lines
            if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 ) {
                //First read all nodes
                //If all nodes read, must mean we're at edge list
                position = thisLine.find_first_of( "[" );
                if( position !=  std::string::npos ) {
                    uint32_t vertex = std::stoi( thisLine.substr( 0, position ) );

                    std::uint64_t posA = thisLine.find_first_of( "=" ) + 1;
                    std::uint64_t posB = thisLine.find_last_of( "]" );
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

                    hardwareGraph_.addEdge( std::stoi(edges[0]), std::stoi(edges[1]) );
                }
            }
        }

        inputStream.close();
    }
    else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open file\n");
        exit(0);
    }

}

void LlyrComponent::constructSoftwareGraph(std::string fileName)
{
    output_->verbose(CALL_INFO, 1, 0, "Constructing Application Graph From: %s\n", fileName.c_str());

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {
        std::string thisLine;
        std::uint64_t position;

        std::getline( inputStream, thisLine );
        position = thisLine.find( "ModuleID" );

        output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());
        std::cout << ::std::endl;
        if( position !=  std::string::npos ) {
            constructSoftwareGraphIR(inputStream);
        } else {
            constructSoftwareGraphApp(inputStream);
        }

        inputStream.close();
    } else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open file\n");
        exit(0);
    }
}

void LlyrComponent::constructSoftwareGraphApp(std::ifstream& inputStream)
{
    std::string thisLine;
    std::uint64_t position;

    inputStream.seekg (0, inputStream.beg);
    while( std::getline( inputStream, thisLine ) ) {
        output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());

        //Ignore blank lines
        if( std::all_of(thisLine.begin(), thisLine.end(), isspace) == 0 ) {
            //First read all nodes
            //If all nodes read, must mean we're at edge list
            position = thisLine.find_first_of( "[" );
            if( position !=  std::string::npos ) {
                AppNode tempNode;
                uint32_t vertex = std::stoi( thisLine.substr( 0, position ) );

                std::uint64_t posA = thisLine.find_first_of( "=" ) + 1;
                std::uint64_t posB = thisLine.find_last_of( "," );
                if( posB !=  std::string::npos ) {
                    std::uint64_t posC = thisLine.find_last_of( "]" );
                    tempNode.constant_val_ = thisLine.substr( posB + 1, posC - posB - 1 );
//                     std::cout << "CONSTANT " << tempNode.constant_val_ << std::endl;
                } else {
                    posB = thisLine.find_last_of( "]" );
                }

                std::string op = thisLine.substr( posA, posB-posA );
                opType operation = getOptype(op);
                tempNode.optype_ = operation;
                output_->verbose(CALL_INFO, 10, 0, "OpString:  %s\t\t%" PRIu32 "\n", op.c_str(), tempNode.optype_);

                applicationGraph_.addVertex( vertex, tempNode );
            } else {

                std::regex delimiter( "\\--" );

                std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                std::sregex_token_iterator iterB;
                std::vector< std::string > edges( iterA, iterB );

                edges[0].erase(remove_if(edges[0].begin(), edges[0].end(), isspace), edges[0].end());
                edges[1].erase(remove_if(edges[1].begin(), edges[1].end(), isspace), edges[1].end());

                output_->verbose(CALL_INFO, 10, 0, "Edges %s--%s\n", edges[0].c_str(), edges[1].c_str());

                applicationGraph_.addEdge( std::stoi(edges[0]), std::stoi(edges[1]) );
            }
        }
    }
}

void LlyrComponent::constructSoftwareGraphIR(std::ifstream& inputStream)
{
    std::string thisLine;

    output_->verbose(CALL_INFO, 15, 0, "Sending to LLVM parser\n");
    std::cout << ::std::endl;
    inputStream.seekg (0, inputStream.beg);
    std::string irString( (std::istreambuf_iterator< char >( inputStream )),
                          (std::istreambuf_iterator< char >() ));
    Parser parser(irString, output_);
    parser.generateAppGraph("offload_");

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
        output_->fatal(CALL_INFO, -1, "Error: Unable to open file\n");
        exit(0);
    }

    return tempVector;
}

opType LlyrComponent::getOptype(std::string &opString) const
{
    opType operation;

    //transform to make opString case insensitive
    std::transform(opString.begin(), opString.end(), opString.begin(),
                   [](unsigned char c){ return std::toupper(c); }
                  );

    if( opString == "ANY" )
        operation = ANY;
    else if( opString == "ANY_MEM" )
        operation = ANY_MEM;
    else if( opString == "LD" )
        operation = LD;
    else if( opString == "LDADDR" )
        operation = LDADDR;
    else if( opString == "LD_ST" )
        operation = LD_ST;
    else if( opString == "ST" )
        operation = ST;
    else if( opString == "STADDR" )
        operation = STADDR;
    else if( opString == "ANY_LOGIC" )
        operation = ANY_LOGIC;
    else if( opString == "AND" )
        operation = AND;
    else if( opString == "OR" )
        operation = OR;
    else if( opString == "XOR" )
        operation = XOR;
    else if( opString == "NOT" )
        operation = NOT;
    else if( opString == "SLL" )
        operation = SLL;
    else if( opString == "SLR" )
        operation = SLR;
    else if( opString == "ROL" )
        operation = ROL;
    else if( opString == "ROR" )
        operation = ROR;
    else if( opString == "EQ" )
        operation = EQ;
    else if( opString == "NE" )
        operation = NE;
    else if( opString == "UGT" )
        operation = UGT;
    else if( opString == "UGE" )
        operation = UGE;
    else if( opString == "SGT" )
        operation = SGT;
    else if( opString == "SGE" )
        operation = SGE;
    else if( opString == "ULT" )
        operation = ULT;
    else if( opString == "ULE" )
        operation = ULE;
    else if( opString == "SLT" )
        operation = SLT;
    else if( opString == "SLE" )
        operation = SLE;
    else if( opString == "ANY_INT" )
        operation = ANY_INT;
    else if( opString == "ADD" )
        operation = ADD;
    else if( opString == "SUB" )
        operation = SUB;
    else if( opString == "MUL" )
        operation = MUL;
    else if( opString == "DIV" )
        operation = DIV;
    else if( opString == "ADDCONST" )
        operation = ADDCONST;
    else if( opString == "SUBCONST" )
        operation = SUBCONST;
    else if( opString == "MULCONST" )
        operation = MULCONST;
    else if( opString == "DIVCONST" )
        operation = DIVCONST;
    else if( opString == "ANY_FP" )
        operation = ANY_FP;
    else if( opString == "FPADD" )
        operation = FPADD;
    else if( opString == "FPSUB" )
        operation = FPSUB;
    else if( opString == "FPMUL" )
        operation = FPMUL;
    else if( opString == "FPDIV" )
        operation = FPDIV;
    else if( opString == "FPMATMUL" )
        operation = FPMATMUL;
    else if( opString == "ANY_CP" )
        operation = ANY_CP;
    else if( opString == "TSIN" )
        operation = TSIN;
    else if( opString == "TCOS" )
        operation = TCOS;
    else if( opString == "DUMMY" )
        operation = DUMMY;
    else if( opString == "BUFFER" )
        operation = BUFFER;
    else if( opString == "SEL" )
        operation = SEL;
    else
        operation = OTHER;

    return operation;
}

} // namespace llyr
} // namespace SST


