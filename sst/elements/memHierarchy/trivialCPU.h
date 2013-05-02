// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _TRIVIALCPU_H
#define _TRIVIALCPU_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/memEvent.h>
using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class trivialCPU : public SST::Component {
public:

	trivialCPU(SST::ComponentId_t id, SST::Component::Params_t& params);
	void init();
	void finish() { 
		printf("TrivialCPU Finished after %lu issued reads, %lu returned\n",
				num_reads_issued, num_reads_returned);
//		return 0;
	}

private:
	trivialCPU();  // for serialization only
	trivialCPU(const trivialCPU&); // do not implement
	void operator=(const trivialCPU&); // do not implement
	void init(unsigned int phase);

	void handleEvent( SST::Event *ev );
	virtual bool clockTic( SST::Cycle_t );

    int numLS;
	int workPerCycle;
	int commFreq;
	bool do_write;
	uint32_t maxAddr;
	uint64_t num_reads_issued, num_reads_returned;

	std::map<MemEvent::id_type, SimTime_t> requests;

	SST::Link* mem_link;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
		ar & BOOST_SERIALIZATION_NVP(workPerCycle);
		ar & BOOST_SERIALIZATION_NVP(commFreq);
		ar & BOOST_SERIALIZATION_NVP(maxAddr);
		ar & BOOST_SERIALIZATION_NVP(mem_link);
	}

	template<class Archive>
	void load(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
		ar & BOOST_SERIALIZATION_NVP(workPerCycle);
		ar & BOOST_SERIALIZATION_NVP(commFreq);
		ar & BOOST_SERIALIZATION_NVP(maxAddr);
		ar & BOOST_SERIALIZATION_NVP(mem_link);
		//resture links
		mem_link->setFunctor(new SST::Event::Handler<trivialCPU>(this,&trivialCPU::handleEvent));
	}

	BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}
#endif /* _TRIVIALCPU_H */
