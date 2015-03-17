// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ADDR_HISTOGRAMMER
#define _H_SST_ADDR_HISTOGRAMMER

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/stats/histo/histo.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

class AddrHistogrammer : public SST::MemHierarchy::CacheListener {
    public:
	AddrHistogrammer(Params& params);
        ~AddrHistogrammer();

	void setOwningComponent(const SST::Component* owner);
        void notifyAccess(const NotifyAccessType notifyType,
		const NotifyResultType notifyResType,
		const Addr addr, const uint32_t size);
	void registerResponseCallback(Event::HandlerBase *handler);
	void printStats(Output& out);

    private:
	const SST::Component* owner;
	std::vector<Event::HandlerBase*> registeredCallbacks;
	uint32_t blockSize;
	uint32_t binWidth;
    uint32_t verbosity;
    Statistics::Histogram<uint32_t, uint32_t>* rdHisto;
    Statistics::Histogram<uint32_t, uint32_t>* wrHisto;
    Output *output;
    void printHistogram(Statistics::Histogram<uint32_t, uint32_t> *, std::string);
};

}
}

#endif
