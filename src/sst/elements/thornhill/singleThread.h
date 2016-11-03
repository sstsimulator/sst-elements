// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

#include "detailedCompute.h"

namespace SST {
namespace Thornhill {

class SingleThread : public DetailedCompute {

	struct Entry {
      Entry( std::function<int()>& _finiHandler ) : finiHandler( _finiHandler ) {}
    	std::function<int()> finiHandler; 
	};

  public:

    SingleThread( Component* owner, Params& params );

    ~SingleThread(){};

    virtual void start( const std::deque< 
						std::pair< std::string, SST::Params > >&, 
                 std::function<int()> );
    virtual bool isConnected() { return ( m_link ); }
	
	virtual std::string getModelName() {
		return "thornhill.SingleThread";
	}

  private:
    void eventHandler( SST::Event* ev ); 
    Link*  m_link;
	Entry* m_entry;
};


}
}
#endif
