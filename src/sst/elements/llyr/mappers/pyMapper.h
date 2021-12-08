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

#ifndef _PY_MAPPER_H
#define _PY_MAPPER_H

#include <stack>
#include <iostream>

#include "mappers/llyrMapper.h"

namespace SST {
namespace Llyr {

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


};

void PyMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][pyMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    std::string fileName = "00_sst.in";
    output_->verbose(CALL_INFO, 1, 0, "Constructing Hardware Graph From: %s\n", fileName.c_str());

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {
        std::string thisLine;

        bool routing = 0;
        while( std::getline( inputStream, thisLine ) ) {
            output_->verbose(CALL_INFO, 15, 0, "Parsing:  %s\n", thisLine.c_str());

            // need to seperate the compute from the routing
            if( thisLine.find( "-1" ) != std::string::npos ) {
                routing = 1;
            }

            // first read compute graph
            std::uint64_t posA = thisLine.find_first_of( "(" ) + 1;
            std::uint64_t posB = thisLine.find_last_of( "," );
            std::string op = thisLine.substr( posA, posB-posA );

            uint32_t vertex = std::stoi( thisLine.substr( posA, posB ) );
            output_->verbose(CALL_INFO, 15, 0, "App Graph Node:  %u", vertex);;

//             else {
//                 //edge delimiter
//                 std::regex delimiter( "\\--" );
//
//                 std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
//                 std::sregex_token_iterator iterB;
//                 std::vector<std::string> edges( iterA, iterB );
//
//                 edges[0].erase(remove_if(edges[0].begin(), edges[0].end(), isspace), edges[0].end());
//                 edges[1].erase(remove_if(edges[1].begin(), edges[1].end(), isspace), edges[1].end());
//
//                 output_->verbose(CALL_INFO, 10, 0, "Edges %s--%s\n", edges[0].c_str(), edges[1].c_str());
//
//                 hardwareGraph_.addEdge( std::stoi(edges[0]), std::stoi(edges[1]) );
//             }

        }

        inputStream.close();
    }
    else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to open file\n");
        exit(0);
    }

}// mapGraph

}// namespace Llyr
}// namespace SST

#endif // _PY_MAPPER_H

