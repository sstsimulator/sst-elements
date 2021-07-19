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

#ifndef _H_THORNHILL_SINGLE_THREAD
#define _H_THORNHILL_SINGLE_THREAD

#include <queue>
#include "detailedCompute.h"

namespace SST {
namespace Thornhill {

class SingleThread : public DetailedCompute {

  public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SingleThread,
        "thornhill",
        "SingleThread",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
       	SST::Thornhill::SingleThread
    )
	SST_ELI_DOCUMENT_PARAMS( 
		{"portName","Sets the portname of the detailed compute model","detailed0"}
	)

	struct Entry {
      Entry( std::function<int()>& _finiHandler ) : finiHandler( _finiHandler ) {}
    	std::function<int()> finiHandler;
	};

  public:

    SingleThread( ComponentId_t id, Params& params );

    ~SingleThread(){};

	struct Pending {
		Pending( std::deque< std::pair< std::string, SST::Params > >& work,
                 std::function<int()> retHandler, std::function<int()> finiHandler) : work(work), retHandler(retHandler), finiHandler(finiHandler) {}
    	std::deque< std::pair< std::string, SST::Params > > work;
        std::function<int()> retHandler;
		std::function<int()> finiHandler;
	};
    virtual void start( std::deque< std::pair< std::string, SST::Params > >& work,
                 std::function<int()> retHandler, std::function<int()> finiHandler )
	{
		if ( ! m_busy ) {
			start2( work, retHandler, finiHandler );
		} else {
			m_pendingQ.push( Pending( work, retHandler, finiHandler ) );
		}
	}
    virtual bool isConnected() { return ( m_link ); }

	virtual std::string getModelName() {
		return "thornhill.SingleThread";
	}

  private:
	bool m_busy;
	std::queue<Pending> m_pendingQ;
    void eventHandler( SST::Event* ev );
    void start2( const std::deque< std::pair< std::string, SST::Params > >&,
                 std::function<int()>, std::function<int()> );
    Link*  m_link;
};


}
}
#endif
