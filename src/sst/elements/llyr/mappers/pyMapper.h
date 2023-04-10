// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _PY_MAPPER_H
#define _PY_MAPPER_H

#include <map>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <Python.h>

#include "mappers/llyrMapper.h"
#include "csvParser.h"

namespace SST {
namespace Llyr {

typedef struct alignas(uint64_t) {
    std::vector< uint32_t >* adjacency_list_;
    std::vector< std::string >* state_list_;
    std::vector< std::pair< uint32_t, uint32_t >* >* forward_list_;
} NodeAttributes;

typedef struct alignas(uint64_t) {
    std::string* routing_arg_;
    ProcessingElement* src_node_;
    ProcessingElement* dst_node_;
} RoutingFixUp;

class PyMapper : public LlyrMapper
{

public:
    explicit PyMapper(Params& params) :
        LlyrMapper() {}
    ~PyMapper() { }

    SST_ELI_REGISTER_MODULE_DERIVED(
        PyMapper,
        "llyr",
        "mapper.py",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "App to HW",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                  LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

private:

    void runMappingTool( std::string );
    void getAdjacencyList( std::string, std::vector< uint32_t >* );
    void getStateList( std::string, std::vector< std::string >* );
    void printDot( std::string, LlyrGraph< ProcessingElement* >* ) const;

};

void PyMapper::runMappingTool(std::string mapping_tool)
{
    FILE* fp;

    Py_Initialize();

    fp = fopen(mapping_tool.c_str(), "r");
    PyRun_SimpleFile(fp, mapping_tool.c_str());

    Py_Finalize();
}

void PyMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    TraceFunction trace(CALL_INFO_LONG);
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][pyMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    if( llyr_config->mapping_tool_ == "" ) {
        output_->fatal( CALL_INFO, -1, "This mapper requires a pre-processor, defined as mapper_tool in the configuration.\n" );
        exit(0);
    }

    char tmp[256];
    if( getcwd(tmp, 256) == nullptr ) {
        output_->fatal( CALL_INFO, -1, "Unable to find current working directory.\n" );
        exit(0);
    } else {
        std::cout << "Current working directory: " << tmp << std::endl;
    }
    runMappingTool(llyr_config->mapping_tool_);

//     std::string fileName = "deepmind/strassen2x2_clay.csv";
//     std::string fileName = "deepmind/strassen_6x7_rect_gap1.csv";
    std::string fileName = "deepmind/generic_solution.csv";
    output_->verbose(CALL_INFO, 1, 0, "Mapping Application Using: %s\n", fileName.c_str());

    std::list< HardwareNode* > node_list;
    std::list< PairEdge* > edge_list;

    CSVParser csvData(fileName, '|');

    const auto& data = csvData.get_data();

    for( const auto& row : data ) {
        if( row[0] == "node" ) {
            node_list.push_back( process_node_row(row) );
        } else {
            edge_list.push_back( process_edge_row(row) );
        }
    }

    // add the nodes from the mapper to the hardware graph
    for( auto it = node_list.begin(); it != node_list.end(); ++it ) {
        printHardwareNode(*it, std::cout);
        std::cout << std::endl;

        // target node in fabric
        uint32_t hardwareVertex = std::stoul( (*it)->pe_id_ );
        output_->verbose(CALL_INFO, 15, 0, "Target Graph Node:  %u\n", hardwareVertex);

        // operation
        if( (*it)->op_ != "" ) {
            opType op = getOptype( (*it)->op_ );
            output_->verbose(CALL_INFO, 15, 0, "Operation:  %u -- %s\n", op, getOpString(op).c_str());

            // get constants
            QueueArgMap* arguments = new QueueArgMap;
            for( auto val_it = (*it)->const_list_->begin(); val_it != (*it)->const_list_->end(); ++val_it ) {
                if( val_it->size() > 0 ) {
                    uint64_t posA = val_it->find_first_of( ":" );

                    std::cout << " Consts(" << val_it->size() << " -- " << (*it)->const_list_->size() << ")\n";
                    std::cout << *val_it << " -- " << val_it->substr(0, posA) << std::flush;
                    std::cout << " -- " << val_it->substr(posA + 1) << std::flush;
                    std::cout << std::endl;

                    Arg some_arg = val_it->substr(0, posA);
                    uint32_t queue_id = std::stoll(val_it->substr(posA + 1));
                    arguments->emplace( queue_id, some_arg );
                }
            }

            // encode node in hardware graph
            if( op == ADDCONST || op == SUBCONST || op == MULCONST || op == DIVCONST || op == REMCONST ) {
                addNode( op, arguments, hardwareVertex, graphOut, llyr_config );
            } else if( op == INC || op == ACC ) {
                addNode( op, arguments, hardwareVertex, graphOut, llyr_config );
            } else if( op == LDADDR || op == STREAM_LD || op == STADDR || op == STREAM_ST ) {
                addNode( op, arguments, hardwareVertex, graphOut, llyr_config );
            } else if( op == OR_IMM || op == AND_IMM ) {
                addNode( op, arguments, hardwareVertex, graphOut, llyr_config );
            } else if( op == UGT_IMM || op == UGE_IMM || op == SGT_IMM || op == SLT_IMM ) {
                addNode( op, arguments, hardwareVertex, graphOut, llyr_config );
            } else {
                addNode( op, hardwareVertex, graphOut, llyr_config );
            }
        }
    }

    // add the edges, ignore queue bindings
    for( auto it = edge_list.begin(); it != edge_list.end(); ++it ) {
        uint32_t source_pe = std::stoul( (*it)->first );
        uint32_t dest_pe = std::stoul( (*it)->second );

        graphOut.addEdge( source_pe, dest_pe );
    }

    // should probably just check nodes in adj list
    // add the edges, instantiating the input/output queues
    std::cout << "------------------------------------------------------\n";
    std::cout << "\tBinding Nodes\n";
    std::cout << "------------------------------------------------------\n" << std::endl;
    ProcessingElement* srcNode;
    ProcessingElement* dstNode;
    std::list< RoutingFixUp* > routing_fix_list;
    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map = graphOut.getVertexMap();
    for( auto vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator ) {
        std::cout << "num input queues: " << vertexIterator->second.getValue()->getNumInputQueues() << std::endl;
        vertexIterator->second.getValue()->inputQueueInit();
        std::cout << "num input queues: " << vertexIterator->second.getValue()->getNumInputQueues() << std::endl;
        std::cout << std::endl;
    }

    for( auto vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator ) {
        uint32_t current_node = vertexIterator->first;

        // bind the queues -- there is now explicit binding based on arguments
        // find the current node in the hardware list
        auto src_node_iter = std::find_if(node_list.begin(), node_list.end(), [&current_node]
                (const HardwareNode* some_node){ return std::stoul(some_node->pe_id_) == current_node;} );

        std::cout << "** Binding Node " << (*src_node_iter)->pe_id_ << " **" << std::endl;
        std::cout << "\t** Output Queue " << (*src_node_iter)->pe_id_ << " **" << std::endl;
        uint32_t input_offset = 0;
        uint32_t output_queue_num = 0;
        for( auto output_iter = (*src_node_iter)->output_list_->begin(); output_iter != (*src_node_iter)->output_list_->end(); ++output_iter ) {

            std::string arg = output_iter->first;
            uint32_t dst_node = output_iter->second;
            std::cout << "\targ: " << arg << " --> " << dst_node << std::endl;

            auto dst_node_iter = std::find_if(node_list.begin(), node_list.end(), [&dst_node]
                (const HardwareNode* some_node){ return std::stoul(some_node->pe_id_) == dst_node;} );

            // check input list at destination for match
            bool is_also_route = 0;
            for( auto input_iter = (*dst_node_iter)->input_list_->begin(); input_iter != (*dst_node_iter)->input_list_->end(); ++input_iter ) {
                std::cout << "\t\t" << input_iter->first << " -- " << input_iter->second << " (offset " << input_offset << ")" << std::endl;

                std::string* routing_arg = new std::string("");
                if( arg == input_iter->first ) {

                    // check to see if this variable is also being routed
                    for( auto route_iter = (*dst_node_iter)->route_list_->begin(); route_iter != (*dst_node_iter)->route_list_->end(); ++route_iter ) {
                        std::cout << "\t" << std::get<0>(*route_iter) << " ++ " << std::get<1>(*route_iter);
                        std::cout << " ++ " << std::get<2>(*route_iter) << std::endl;

                        if( input_iter->first == std::get<0>(*route_iter) ) {
                            is_also_route = 1;
                            *routing_arg = std::get<0>(*route_iter);
                            break;
                        }
                    }

                    srcNode = vertex_map->at(current_node).getValue();
                    dstNode = vertex_map->at(dst_node).getValue();

                    uint32_t input_queue_num = (*dst_node_iter)->const_list_->size();
                    std::cout << "input num-0 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                    if( dstNode->getNumInputQueues() > 0 ) {
                        while( dstNode->getProcInputQueueBinding(input_queue_num) != NULL ) {
                            input_queue_num = input_queue_num + 1;
                            std::cout << "input num-1 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                        }
                    }

                    if( is_also_route == 1 ) {
                        srcNode->bindOutputQueue(dstNode, output_queue_num);
                        dstNode->bindInputQueue(srcNode, input_queue_num, 1, routing_arg);
                        std::cout << "AAA: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                        std::cout << " <" << *routing_arg << ">";
                        std::cout << std::endl;
                    } else {
                        srcNode->bindOutputQueue(dstNode, output_queue_num);
                        dstNode->bindInputQueue(srcNode, input_queue_num, 0);
                        std::cout << "BBB: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                        std::cout << std::endl;
                    }

                    break;
                }
            }// dst for

            // check route list at destination for match
            for( auto route_iter = (*dst_node_iter)->route_list_->begin(); route_iter != (*dst_node_iter)->route_list_->end(); ++route_iter ) {
                if( arg == std::get<0>(*route_iter) ) {
                    if( is_also_route == 1 ) {
                        continue;
                    }

                    std::cout << "\t\t" << arg << " ++ " << std::get<0>(*route_iter) << " " << (*dst_node_iter)->pe_id_ << " (offset " << input_offset << ")" << std::endl;

                    std::string* routing_arg = new std::string(arg);

                    srcNode = vertex_map->at(current_node).getValue();
                    dstNode = vertex_map->at(dst_node).getValue();

                    uint32_t input_queue_num = (*dst_node_iter)->const_list_->size();
                    std::cout << "input num-0 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                    if( dstNode->getNumInputQueues() > 0 ) {
                        while( dstNode->getProcInputQueueBinding(input_queue_num) != NULL ) {
                            input_queue_num = input_queue_num + 1;
                            std::cout << "input num-1 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                        }
                    }

                    srcNode->bindOutputQueue(dstNode, output_queue_num);
                    dstNode->bindInputQueue(srcNode, input_queue_num, -1, routing_arg);
                    std::cout << "CCC: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                    std::cout << " <" << *routing_arg << ">";
                    std::cout << std::endl;
                }

                // FIXME -- This is hack-y ^-^
                // If this item is also on the input list, don't increment
                bool dual_list = 0;
                for( auto input_iter = (*dst_node_iter)->input_list_->begin(); input_iter != (*dst_node_iter)->input_list_->end(); ++input_iter ) {
                    std::cout << "\t\tx " << input_iter->first << " -- " << std::get<0>(*route_iter) << " (offset " << input_offset << ")" << std::endl;

                    if( std::get<0>(*route_iter) == input_iter->first ) {
                        dual_list = 1;
                        break;
                    }
                }

                if( dual_list == 0 ) {
                    input_offset = input_offset + 1;
                }
            }

            output_queue_num = output_queue_num + 1;
        }// binding output queues

        std::cout << "\t** Route Queue " << (*src_node_iter)->pe_id_ << " **" << std::endl;
        for( auto route_iter = (*src_node_iter)->route_list_->begin(); route_iter != (*src_node_iter)->route_list_->end(); ++route_iter ) {
            std::string arg = std::get<0>(*route_iter);
            uint32_t dst_node = std::get<2>(*route_iter);

            std::cout << "\t src: " << (*src_node_iter)->pe_id_;
            std::cout << "   dst: " << dst_node;
            std::cout << std::endl;

            auto dst_node_iter = std::find_if(node_list.begin(), node_list.end(), [&dst_node]
                (const HardwareNode* some_node){ return std::stoul(some_node->pe_id_) == dst_node;} );

            // check input list at destination for match
            bool is_input = 0;
            uint32_t input_offset = 0;
            std::cout << "\t\tChecking input list at dst..." << std::endl;
            for( auto input_iter = (*dst_node_iter)->input_list_->begin(); input_iter != (*dst_node_iter)->input_list_->end(); ++input_iter ) {
                std::string* routing_arg = new std::string("");
                if( arg == input_iter->first ) {

                    // check to see if this variable is also being routed
                    bool is_also_route = 0;
                    for( auto route_iter = (*dst_node_iter)->route_list_->begin(); route_iter != (*dst_node_iter)->route_list_->end(); ++route_iter ) {
                        std::cout << "\t" << std::get<0>(*route_iter) << " ++ " << std::get<1>(*route_iter);
                        std::cout << " ++ " << std::get<2>(*route_iter) << std::endl;

                        if( input_iter->first == std::get<0>(*route_iter) ) {
                            is_also_route = 1;
                            *routing_arg = std::get<0>(*route_iter);
                            break;
                        }
                    }

                    srcNode = vertex_map->at(current_node).getValue();
                    dstNode = vertex_map->at(dst_node).getValue();

                    uint32_t input_queue_num = (*dst_node_iter)->const_list_->size();
                    std::cout << "input num-0 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                    if( dstNode->getNumInputQueues() > 0 ) {
                        while( dstNode->getProcInputQueueBinding(input_queue_num) != NULL ) {
                            input_queue_num = input_queue_num + 1;
                            std::cout << "input num-1 " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                        }
                    }

                    if( is_also_route == 1 ) {
                        srcNode->bindOutputQueue(dstNode, output_queue_num);
                        dstNode->bindInputQueue(srcNode, input_queue_num, 1, routing_arg);
                        std::cout << "DDD: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                        std::cout << " <" << *routing_arg << ">";
                        std::cout << std::endl;
                    } else {
                        srcNode->bindOutputQueue(dstNode, output_queue_num);
                        dstNode->bindInputQueue(srcNode, input_queue_num, 0);
                        std::cout << "EEE: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                        std::cout << std::endl;
                    }

                    output_queue_num = output_queue_num + 1;
                    is_input = 1;
                    break;
                }
            }

            //if not an input, check routing queue (node could route-to-route)
            std::cout << "\t\tChecking route list at dst..." << std::endl;
            if( is_input == 0 ) {
                for( auto dst_route_iter = (*dst_node_iter)->route_list_->begin(); dst_route_iter != (*dst_node_iter)->route_list_->end(); ++dst_route_iter ) {
                    std::string* routing_arg = new std::string(arg);
                    std::cout << "\t\t\t" << std::get<0>(*dst_route_iter) << std::endl;
                    if( arg == std::get<0>(*dst_route_iter) ) {
                        std::cout << "FOUND " << arg << std::endl;

                        srcNode = vertex_map->at(current_node).getValue();
                        dstNode = vertex_map->at(dst_node).getValue();

                        //find which input queue -- const list size + something
                        uint32_t input_queue_num = (*dst_node_iter)->const_list_->size();
                        std::cout << "input num " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                        if( dstNode->getNumInputQueues() > 0 ) {
                            while( dstNode->getProcInputQueueBinding(input_queue_num) != NULL ) {
                                std::cout << "input num " << input_queue_num << "(" << dstNode->getNumInputQueues() << ")" << std::endl;
                                input_queue_num = input_queue_num + 1;
                            }
                        }

std::cout << "xxxxx: " << (*dst_node_iter)->const_list_->size() << "  " << (*dst_node_iter)->input_list_->size() << " <> " << input_queue_num << std::endl;

                        srcNode->bindOutputQueue(dstNode, output_queue_num);
                        dstNode->bindInputQueue(srcNode, input_queue_num, -1, routing_arg);
                        std::cout << "FFF: " << current_node << " -> " << dst_node << " :: " << input_queue_num << " -> " << output_queue_num;
                        std::cout << " <" << *routing_arg << ">";
                        std::cout << std::endl;

                        output_queue_num = output_queue_num + 1;
                        is_input = 1;
                        break;
                    }
                }

                input_offset = input_offset + 1;
            }

        }// binding route queue
    }

    // add routing arguments for output queues
    for( auto vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator ) {

        uint32_t current_node = vertexIterator->first;
        auto node_iter = std::find_if(node_list.begin(), node_list.end(), [&current_node]
                (const HardwareNode* some_node){ return std::stoul(some_node->pe_id_) == current_node;} );

        std::cout << "\nFixing route for node " << current_node << std::endl;
        for( auto route_iter = (*node_iter)->route_list_->begin(); route_iter != (*node_iter)->route_list_->end(); ++route_iter ) {
            std::string* routing_arg = new std::string(std::get<0>(*route_iter));
            uint32_t dst_node = std::get<2>(*route_iter);

            srcNode = vertex_map->at(current_node).getValue();
            dstNode = vertex_map->at(dst_node).getValue();

            uint32_t queue_id_x = srcNode->getQueueOutputProcBinding(dstNode);
            std::cout << "Queue Id = " << queue_id_x << std::endl;

            std::cout << "Updatating Queue Id = " << queue_id_x;
            std::cout << " With " << *routing_arg << std::endl;
            srcNode->setOutputQueueRoute(queue_id_x, routing_arg);
        }
    }

    // insert dummy as node 0 to make BFS easier
    addNode( DUMMY, 0, graphOut, llyr_config );

    std::cout << "Doing fixup for Node-0..." << std::endl;
    // fixup for node-0; would be nice to get rid of this one day
    typename std::map< uint32_t, Vertex< ProcessingElement* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
        uint32_t current_node = vertexIterator->first;
        auto node_iter = std::find_if(node_list.begin(), node_list.end(), [&current_node]
                (const HardwareNode* some_node){ return std::stoul(some_node->pe_id_) == current_node;} );

        if( vertexIterator->first > 0 ) {
            std::cout << vertexIterator->first << std::flush;
            std::cout << ": " << (*node_iter)->input_list_->size();
//             std::cout << " --> " << vertexIterator->second.getValue->getNumInputQueues();
            std::cout << std::endl;

            if( (*node_iter)->input_list_->size() == 0 ) {
                std::cout << "Adding edge between 0 and " << vertexIterator->first << std::endl;
                graphOut.addEdge( 0, vertexIterator->first );
            }
        }

    }

    //FIXME Need to use a fake init on some PEs for
//     for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
//         opType tempOp = vertexIterator->second.getValue()->getOpBinding();
// //         if( tempOp == ST ) {
// //             vertexIterator->second.getValue()->inputQueueInit();
// //         } else if( tempOp == LDADDR || tempOp == STADDR ) {
// //             vertexIterator->second.getValue()->inputQueueInit();
// //         } else if( tempOp == STREAM_LD || tempOp == STREAM_ST ) {
// //             vertexIterator->second.getValue()->inputQueueInit();
// //         } else if( tempOp == INC || tempOp == ACC ) {
// //             vertexIterator->second.getValue()->inputQueueInit();
// //         } else if( tempOp == ALLOCA ) {
// //             vertexIterator->second.getValue()->outputQueueInit();
// //         }
//
//         if( tempOp == ADDCONST || tempOp == SUBCONST || tempOp == MULCONST || tempOp == DIVCONST || tempOp == REMCONST ) {
//             vertexIterator->second.getValue()->inputQueueInit();
//         } else if( tempOp == INC || tempOp == ACC ) {
//             vertexIterator->second.getValue()->inputQueueInit();
//         } else if( tempOp == LDADDR || tempOp == STREAM_LD || tempOp == STADDR || tempOp == STREAM_ST ) {
//             vertexIterator->second.getValue()->inputQueueInit();
//         }
//     }

//     //FIXME Fake init for now, need to read values from stack
//     //Initialize any L/S PEs at the top of the graph (unless already init'd)
//     std::vector< Edge* >* rootAdjacencyList = vertex_map->at(0).getAdjacencyList();
//     for( auto it = rootAdjacencyList->begin(); it != rootAdjacencyList->end(); it++ ) {
//         uint32_t destinationVertex = (*it)->getDestination();
//
//         opType tempOp = vertex_map->at(destinationVertex).getValue()->getOpBinding();
//         if( tempOp == ADDCONST || tempOp == SUBCONST || tempOp == MULCONST || tempOp == DIVCONST || tempOp == REMCONST ) {
//         } else if( tempOp == INC || tempOp == ACC ) {
//         } else if( tempOp == LDADDR || tempOp == STREAM_LD || tempOp == STADDR || tempOp == STREAM_ST ) {
//         } else {
//             vertex_map->at(destinationVertex).getValue()->inputQueueInit();
//         }
//     }

}

void PyMapper::getAdjacencyList( std::string opList, std::vector< uint32_t >* vecIn )
{
    std::cout << "--- Getting Adjacency List --- " << std::endl;

    // clean the input string
    opList.erase(remove(opList.begin(), opList.end(), '('), opList.end());
    opList.erase(remove(opList.begin(), opList.end(), ')'), opList.end());
    opList.erase(remove(opList.begin(), opList.end(), '['), opList.end());
    opList.erase(remove(opList.begin(), opList.end(), ']'), opList.end());
    opList.erase(opList.find("operation"), std::string("operation").length());

    // split into tokens
    std::regex delimiter( "," );
    std::sregex_token_iterator iterA(opList.begin(), opList.end(), delimiter, -1);
    std::sregex_token_iterator iterB;
    std::vector<std::string> tokenizedSring( iterA, iterB );

    std::cout << "***Len of vector " << tokenizedSring.size() << std::endl;
    //
    if( tokenizedSring.size() > 1 ) {
        for( uint32_t i = 0; i < tokenizedSring.size(); i++ ) {
            std::cout << "+++ " << tokenizedSring[i] << std::endl;
            if( i % 2 == 1 ) {
                std::cout << "-- " << tokenizedSring[i] << std::endl;
                vecIn->push_back(std::stoul(tokenizedSring[i]));
            }
        }
    }
}

void PyMapper::getStateList( std::string states, std::vector< std::string >* vecIn )
{
    std::cout << "--- Getting State List --- " << std::endl;
std::cout << states << std::endl;

    // clean the input string
    states.erase(remove(states.begin(), states.end(), '['), states.end());
    states.erase(remove(states.begin(), states.end(), ']'), states.end());
    states.erase(states.find("output"), std::string("output").length());
std::cout << states << std::endl;

    // split into tokens
    std::regex delimiter( "," );
    std::sregex_token_iterator iterA(states.begin(), states.end(), delimiter, -1);
    std::sregex_token_iterator iterB;
    std::vector<std::string> tokenizedSring( iterA, iterB );

    std::cout << "***Len of vector " << tokenizedSring.size() << std::endl;

    if( tokenizedSring.size() > 1 ) {
        for( uint32_t i = 0; i < tokenizedSring.size(); i++ ) {
            std::cout << "+++ " << tokenizedSring[i] << std::endl;
            if( i % 2 == 1 ) {
                std::cout << "-- " << tokenizedSring[i] << std::endl;
                vecIn->push_back(tokenizedSring[i]);
            }
        }
    }
}

void PyMapper::printDot( std::string fileName, LlyrGraph< ProcessingElement* >* graphIn ) const
{
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);         //open a file for writing (truncate the current contents)
    if ( !outputFile )                                                   //check to be sure file is open
        std::cerr << "Error opening file.";

    outputFile << "digraph G {" << "\n";

    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map = graphIn->getVertexMap();
    std::map< uint32_t, Vertex<ProcessingElement*> >::iterator vertexIterator;
    for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
        outputFile << vertexIterator->first << "[label=\"";
        opType moop = vertexIterator->second.getValue()->getOpBinding();
        outputFile << vertexIterator->first << " - " << getOpString(moop);
        outputFile << "\"];\n";
    }

    for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << (*it)->getDestination();
            outputFile << "\n";
        }
    }

    outputFile << "}";
    outputFile.close();
}

}// namespace Llyr
}// namespace SST

#endif // _PY_MAPPER_H

