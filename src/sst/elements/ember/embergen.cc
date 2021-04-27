// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

EmberGenerator::EmberGenerator( ComponentId_t id, Params& params, std::string name ) :
    SubComponent(id),
    m_detailedCompute( NULL ),
    m_dataMode( NoBacking ),
    m_motifName( name ),
    m_ee(NULL),
    m_curVirtAddr( 0x1000 )
{
    m_primary = params.find<bool>("primary",true);
    m_motifNum = params.find<int>( "_motifNum", -1 );
    m_jobId = params.find<int>( "_jobId", -1 );
    uint64_t parentPtr = params.find<uint64_t>("_enginePtr",0 );
    assert( parentPtr != 0 );

    setEngine( reinterpret_cast< EmberEngine* >( parentPtr ) );

    setVerbosePrefix();

    Params distribParams = params.get_scoped_params("distribParams");
    std::string distribModule = params.find<std::string>("distribModule", "ember.ConstDistrib");

	m_computeDistrib = dynamic_cast<EmberComputeDistribution*>( loadModule(distribModule, distribParams) );

    if(NULL == m_computeDistrib) {
        std::cerr << "Error: Unable to load compute distribution: \'"
                                    << distribModule << "\'" << std::endl;
        exit(-1);
    }
}


void EmberGenerator::setEngine( EmberEngine* ee ) {

	m_ee = ee;
    m_output = m_ee->getOutput();
    m_nodePerf = m_ee->getNodePerf();
    m_detailedCompute = m_ee->getDetailedCompute();
	m_memHeapLink = m_ee->getMemHeapLink();
}

EmberLib* EmberGenerator::getLib(std::string name )
{
    return m_ee->getLib( name );
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


