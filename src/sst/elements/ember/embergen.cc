// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include "emberengine.h"
#include "embergen.h"

using namespace SST::Ember;

EmberGenerator::EmberGenerator( Component* owner, Params& params,
		std::string name ) :
    SubComponent(owner),
    m_detailedCompute( NULL ),
    m_dataMode( NoBacking ),
    m_motifName( name )
{
	EmberEngine* ee = static_cast<EmberEngine*>(owner);
    m_output = ee->getOutput();
    m_nodePerf = ee->getNodePerf();
    m_api = ee->getAPI( params.find<std::string>("_apiName") );

    m_detailedCompute = ee->getDetailedCompute();
	m_memHeapLink = ee->getMemHeapLink();
	m_famAddrMapper = ee->getFamAddrMapper();

    m_motifNum = params.find<int>( "_motifNum", -1 );	
    m_jobId = params.find<int>( "_jobId", -1 );	

    m_verbosePrefix << "@t:" << getJobId() << ":" << rank() << 
						":EmberEngine:MPI:" << name << ":@p:@l: ";
    //std::cout << "Job:" << getJobId() << " Rank:" << rank() << std::endl;//NetworkSim
    
    Params distribParams = params.find_prefix_params("distribParams.");
    std::string distribModule = params.find<std::string>("distribModule",
                                                "ember.ConstDistrib");

    m_computeDistrib = dynamic_cast<EmberComputeDistribution*>(
        owner->loadModuleWithComponent(distribModule, owner, distribParams));

    if(NULL == m_computeDistrib) {
        std::cerr << "Error: Unable to load compute distribution: \'"
                                    << distribModule << "\'" << std::endl;
        exit(-1);
    } 
}

EmberLib* EmberGenerator::getLib(std::string name )
{
    return static_cast<EmberEngine*>(parent)->getLib( name );
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif

void EmberGenerator::fatal(uint32_t line, const char* file, const char* func,
               uint32_t exit_code,
               const char* format, ...)    const
{
	char buf[500];
	va_list arg;
	va_start(arg, format);
	vsnprintf( buf, 500, format, arg);
    va_end(arg);
	m_output->fatal( line, file, func, exit_code, buf ); 
}

void EmberGenerator::output(const char* format, ...) const
{
	char buf[500];
	va_list arg;
	va_start(arg, format);
	vsnprintf( buf, 500, format, arg);
    va_end(arg);
	m_output->output( buf ); 
}
    
void EmberGenerator::verbose(uint32_t line, const char* file, const char* func,
                 uint32_t output_level, uint32_t output_bits,
                 const char* format, ...) const
{
	char buf[500];
	va_list arg;
	va_start(arg, format);
	vsnprintf( buf, 500, format, arg);
    va_end(arg);
	m_output->verbosePrefix( m_verbosePrefix.str().c_str(), line, file, func,
											output_level, output_bits, buf ); 
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

void* EmberGenerator::memAlloc( size_t size )
{
    void *ret = NULL;
    switch ( m_dataMode  ) {
      case Backing:
        ret = malloc( size );
        break;
      case BackingZeroed: 
        ret = malloc( size );
        memset( ret, 0, size ); 
        break;
      case NoBacking:
        break;
    } 
    return ret;
}

void EmberGenerator::memFree( void* ptr )
{
    switch ( m_dataMode  ) {
      case Backing:
      case BackingZeroed: 
        free( ptr );
        break;
      case NoBacking:
        break;
    } 
}


