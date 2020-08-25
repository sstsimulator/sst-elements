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
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "llyr.h"
#include "processingElement.h"
#include "mappers/simpleMapper.h"

namespace SST {
namespace Llyr {

LlyrComponent::LlyrComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    const int32_t verbosity = params.find< uint32_t >("verbose", 0);
    const std::string clock_rate = params.find< std::string >("clock", "1.0GHz");
    clock_count = params.find< int64_t >("clockcount", 10);

    std::cout << "Clock is configured for: " << clock_rate << std::endl;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our Main Clock
    clockTickHandler = new Clock::Handler<LlyrComponent>(this, &LlyrComponent::tick);
    registerClock(clock_rate, clockTickHandler);

    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][%s]: ", getName().c_str());
    output = new SST::Output(prefix, verbosity, 0, Output::STDOUT);

    //construct hardware graph
    std::string const& hwFileName = params.find< std::string >("hardwareGraph", "grid.cfg");
    constructHardwareGraph(hwFileName);

    std::string const& swFileName = params.find< std::string >("hardwareGraph", "app.in");
    constructSoftwareGraph(swFileName);

    //do the mapping
    Params mapperParams;    //empty but needed for loadModule API
    std::string mapperName = params.find<std::string>("mapper", "llyr.simpleMapper");
    llyrMapper = dynamic_cast<LlyrMapper*>( loadModule(mapperName, mapperParams) );
    llyrMapper->mapGraph(hardwareGraph, applicationGraph, mappedGraph);
}

LlyrComponent::~LlyrComponent()
{
    output->verbose(CALL_INFO, 1, 0, "Llyr destructor fired, closing down.\n");
//     hardwareGraph.printGraph();
    hardwareGraph.printDot("hdwr.dot");

//     applicationGraph.printGraph();
    applicationGraph.printDot("app.dot");

    mappedGraph.printDot("mapp.dot");
}

LlyrComponent::LlyrComponent() :
    Component(-1)
{
    // for serialization only
}

void LlyrComponent::constructHardwareGraph(std::string fileName)
{
    std::cout << "Constructing Hardware Graph From " << fileName << "\n";
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
                    hardwareGraph.addVertex( vertex, operation );
                }
                else
                {
//                     std::cout << "\t*Parse " << thisLine << std::endl;
                    std::regex delimiter( "\\->" );

                    std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                    std::sregex_token_iterator iterB;
                    std::vector<std::string> edges( iterA, iterB );

                    hardwareGraph.addEdge( std::stoi(edges[0].substr(0, edges[0].find(" "))), std::stoi(edges[1].substr(0, edges[1].find(" "))) );
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
    std::cout << "Constructing Software Graph From " << fileName << "\n";
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
                    applicationGraph.addVertex( vertex, operation );
                }
                else
                {
                    std::cout << "\t*Parse " << thisLine << std::endl;
                    std::regex delimiter( "\\->" );

                    std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
                    std::sregex_token_iterator iterB;
                    std::vector<std::string> edges( iterA, iterB );

                    applicationGraph.addEdge( std::stoi(edges[0].substr(0, edges[0].find(" "))), std::stoi(edges[1].substr(0, edges[1].find(" "))) );
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

opType LlyrComponent::getOptype(std::string opString)
{
    opType operation;

    if( opString == "ANY" )
        operation = ANY;
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

bool LlyrComponent::tick( Cycle_t )
{
    clock_count--;

    // return false so we keep going
    if(clock_count == 0) {
    primaryComponentOKToEndSim();
        return true;
    } else {
        return false;
    }
}

// Serialization
} // namespace llyr
} // namespace SST


