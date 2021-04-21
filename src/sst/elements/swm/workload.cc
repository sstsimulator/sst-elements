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

#include <map>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <mutex>

#include "sst_config.h"

#include <boost/property_tree/json_parser.hpp>
#include <swm-include.h>
#include "sst/elements/swm/workload.h"

using namespace SST;
using namespace SST::Swm;

boost::property_tree::ptree root;

Workload::Workload( Convert* convert, std::string path, std::string name, int rank, uint32_t verboseLevel, uint32_t verboseMask) :
	m_convert(convert), m_rank(rank), m_readConfig(true) 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:Workload::@p():@l ",m_rank);
    m_output.init(buffer, verboseLevel, verboseMask, Output::STDOUT);

    void** generic_ptrs;
    int array_len = 1;
    generic_ptrs = (void**)calloc(array_len,  sizeof(void*));
    generic_ptrs[0] = (void*)&rank;

	m_output.debug( CALL_INFO, 1, 0, "path=%s workload=%s\n",path.c_str(),name.c_str());

    static std::mutex boostMtx;
    boostMtx.lock();

	if ( m_readConfig ) {
    	try {
        	std::ifstream jsonFile(path.c_str());
        	boost::property_tree::json_parser::read_json(jsonFile, root);
        	jsonFile.close();

        	m_numRanks = root.get<uint32_t>("jobs.size", 1);
        	m_cpuFreq = root.get<double>("jobs.cfg.cpu_freq") / 1e9;
    	}
    	catch(std::exception & e)
    	{
			throw;
    	}
    	m_readConfig = false;
	}
    // ***********************************
    // THIS IS a problem with an exception 
    // ***********************************
    boostMtx.unlock();

    if( name.compare( "lammps") == 0)
    {
        m_type = Lammps;
        m_lammps = new LAMMPS_SWM(root, generic_ptrs);
    }
    else if ( name.compare( "nekbone") == 0)
    {
        m_type = Nekbone;
        m_nekbone = new NEKBONESWMUserCode(root, generic_ptrs);
    }
    else if ( name.compare( "nearest_neighbor") == 0)
    {
        m_type = NN;
        m_nn = new NearestNeighborSWMUserCode(root, generic_ptrs);
    }
    else if ( name.compare( "many_to_many") == 0)
    {
        m_type = MM;
        m_mm = new ManyToManySWMUserCode(root, generic_ptrs);
    }
    else if ( name.compare( "milc") == 0)
    {
        m_type = MILC;
        m_milc = new MilcSWMUserCode(root, generic_ptrs);
    }
    else if ( name.compare( "incast") == 0 || name.compare( "incast1") == 0 || name.compare( "incast2") == 0)
    {
        m_type = Incast;
        m_incast = new AllToOneSWMUserCode(root, generic_ptrs);
    } else {
		throw "Unknown workload: " + name;
	}	
}

static std::mutex _threadMapMtx;
static std::map<std::thread::id,Workload*> _threadMap;

static void workloadThread( Workload* workload ) {
	_threadMapMtx.lock();
	_threadMap[std::this_thread::get_id()] = workload;
	_threadMapMtx.unlock();

	workload->convert().init( );

	workload->call();	

    WorkloadDBG(workload->rank(), "call() returned\n");
}

static Workload* getWorkload() {
	_threadMapMtx.lock();
	Workload* workload = _threadMap[std::this_thread::get_id()];
	_threadMapMtx.unlock();
	return workload;	
}

void Workload::start() { 
	m_output.debug( CALL_INFO, 1, 0, "start thread\n");
	m_thread = std::thread( workloadThread, this ); 
    m_convert->waitForWork();
}

void Workload::stop() { 
	m_output.debug( CALL_INFO, 1, 0, "stop thread\n");
	m_thread.join(); 
}

void SWM_Init() 
{
    // Some workload don't call this but we need it so we call it when the thread is started
}

void SWM_Send(SWM_PEER peer,
              SWM_COMM_ID comm_id,
              SWM_TAG tag,
              SWM_VC reqvc,
              SWM_VC rspvc,
              SWM_BUF buf,
              SWM_BYTES bytes,
              SWM_BYTES pktrspbytes,
              SWM_ROUTING_TYPE reqrt,
              SWM_ROUTING_TYPE rsprt)
{
	getWorkload()->convert().send( peer, comm_id, tag, reqvc, rspvc, buf, bytes, pktrspbytes, reqrt, rsprt );
}

void SWM_Isend(SWM_PEER peer,
              SWM_COMM_ID comm_id,
              SWM_TAG tag,
              SWM_VC reqvc,
              SWM_VC rspvc,
              SWM_BUF buf,
              SWM_BYTES bytes,
              SWM_BYTES pktrspbytes,
              uint32_t * handle,
              SWM_ROUTING_TYPE reqrt,
              SWM_ROUTING_TYPE rsprt)
{
	getWorkload()->convert().isend( peer, comm_id, tag, reqvc, rspvc, buf, bytes, pktrspbytes, handle, reqrt, rsprt );
}

void SWM_Barrier(
        SWM_COMM_ID comm_id,
        SWM_VC reqvc,
        SWM_VC rspvc,
        SWM_BUF buf,
        SWM_UNKNOWN auto1,
        SWM_UNKNOWN2 auto2,
        SWM_ROUTING_TYPE reqrt,
        SWM_ROUTING_TYPE rsprt)
{
	getWorkload()->convert().barrier( comm_id, reqvc, rspvc, buf, auto1, auto2, reqrt, rsprt );
}

void SWM_Recv(SWM_PEER peer,
        SWM_COMM_ID comm_id,
        SWM_TAG tag,
        SWM_BUF buf,
        SWM_BYTES bytes)
{
	getWorkload()->convert().recv( peer, comm_id, tag, buf, bytes );
}

void SWM_Irecv(SWM_PEER peer,
        SWM_COMM_ID comm_id,
        SWM_TAG tag,
        SWM_BUF buf,
        SWM_BYTES bytes,
        uint32_t* handle)
{
	getWorkload()->convert().irecv( peer, comm_id, tag, buf, bytes, handle );
}


void SWM_Compute(long cycle_count)
{
	getWorkload()->convert().compute( getWorkload()->calcComputeTime(cycle_count ) );
}

void SWM_Wait(uint32_t req_id)
{
	getWorkload()->convert().wait( req_id );
}

void SWM_Waitall(int len, uint32_t * req_ids)
{
	getWorkload()->convert().waitall( len, req_ids );
}

void SWM_Sendrecv(
         SWM_COMM_ID comm_id,
         SWM_PEER sendpeer,
         SWM_TAG sendtag,
         SWM_VC sendreqvc,
         SWM_VC sendrspvc,
         SWM_BUF sendbuf,
         SWM_BYTES sendbytes,
         SWM_BYTES pktrspbytes,
         SWM_PEER recvpeer,
         SWM_TAG recvtag,
         SWM_BUF recvbuf,
         SWM_ROUTING_TYPE reqrt,
         SWM_ROUTING_TYPE rsprt )
{
	getWorkload()->convert().sendrecv( comm_id, sendpeer, sendtag, sendreqvc, sendrspvc, sendbuf, sendbytes, pktrspbytes, recvpeer, recvtag, recvbuf, reqrt, rsprt);
}

void SWM_Allreduce(
        SWM_BYTES bytes,
        SWM_BYTES rspbytes,
        SWM_COMM_ID comm_id,
        SWM_VC sendreqvc,
        SWM_VC sendrspvc,
        SWM_BUF sendbuf,
        SWM_BUF rcvbuf)
{
	getWorkload()->convert().allreduce( bytes, rspbytes, comm_id, sendreqvc, sendrspvc, sendbuf, rcvbuf, 0, 0, 0, 0 );
}

void SWM_Allreduce(
        SWM_BYTES bytes,
        SWM_BYTES rspbytes,
        SWM_COMM_ID comm_id,
        SWM_VC sendreqvc,
        SWM_VC sendrspvc,
        SWM_BUF sendbuf,
        SWM_BUF rcvbuf,
        SWM_UNKNOWN auto1,
        SWM_UNKNOWN2 auto2,
        SWM_ROUTING_TYPE reqrt,
        SWM_ROUTING_TYPE rsprt)
{
	getWorkload()->convert().allreduce( bytes, rspbytes, comm_id, sendreqvc, sendrspvc, sendbuf, rcvbuf, auto1, auto2, reqrt, rsprt );
}

void SWM_Finalize()
{
	getWorkload()->convert().finalize();
}

