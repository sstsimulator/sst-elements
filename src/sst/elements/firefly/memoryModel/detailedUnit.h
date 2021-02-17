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

class DetailedUnit : public Unit {

  public:

	DetailedUnit( SimpleMemoryModel& model, Output& dbg, int id ) : Unit( model, dbg )
    {
		m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::DetailedUnit::@p():@l ";
	}

	void setDetailedInterface( SST::Firefly::DetailedInterface* detailed ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"\n");
		m_detailed = detailed;
		m_detailed->setCallback( std::bind( &DetailedUnit::callback,this,std::placeholders::_1) );
		m_detailed->setResume( std::bind( &DetailedUnit::resume,this,std::placeholders::_1 ) );
	}


	void callback( Callback* callback) {
		m_model.schedCallback(0, callback );
	}
	void resume( PTR* ptr ) {
		m_model.schedResume( 0, (UnitBase*)ptr, this );
	}

    bool store( UnitBase* src, MemReq* req ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);
		return m_detailed->store( (PTR*)src, req );
    }

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
		return m_detailed->load( (PTR*)src, req, callback );
	}
	void init(unsigned int phase) {}

  private:

	SST::Firefly::DetailedInterface* m_detailed;
};
