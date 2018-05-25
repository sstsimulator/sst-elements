// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_TIMING_TRANSACTION
#define _H_SST_MEMH_TIMING_TRANSACTION

#include <sst/core/subcomponent.h>

namespace SST {
namespace MemHierarchy {
namespace TimingDRAM_NS {

typedef uint64_t ReqId;

struct Transaction {
    Transaction( SimTime_t _createTime, ReqId id, Addr addr, bool isWrite, unsigned numBytes, unsigned _bank, unsigned _row) :
        createTime(_createTime), id(id), addr(addr), isWrite(isWrite), numBytes(numBytes),
	bank(_bank), row(_row), retired(false) 
    {}

    void setRetired() { retired = true; }
    bool isRetired() { return retired; }

    SimTime_t createTime;
    ReqId id;
    Addr addr;
    bool isWrite;
    unsigned numBytes;
    unsigned bank;
    unsigned row;
    bool retired;
};

class TransactionQ : public SST::SubComponent {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(TransactionQ, "memHierarchy", "fifoTransactionQ", SST_ELI_ELEMENT_VERSION(1,0,0),
            "fifo transaction queue", "SST::MemHierarchy::TransactionQ")

/* Begin class definition */
    TransactionQ( Component* owner, Params& params ) : SubComponent( owner)  {}

    virtual void push( Transaction* trans ) {
        m_transQ.push_back( trans );
    }

    virtual Transaction* pop( unsigned row ) {
        Transaction* trans = NULL; 
        if ( ! m_transQ.empty() ) {
            trans = m_transQ.front();
            m_transQ.pop_front();
        }
        return trans;
    }

  protected:
    std::list<Transaction*> m_transQ;
};

class ReorderTransactionQ : public TransactionQ {

  public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(ReorderTransactionQ, "memHierarchy", "reorderTransactionQ", SST_ELI_ELEMENT_VERSION(1,0,0),
            "reorder transaction queue", "SST::MemHierarchy::TransactionQ")

    SST_ELI_DOCUMENT_PARAMS( {"windowCycles", "Reorder window in cycles", "10" } )

/* Begin class definition */
    ReorderTransactionQ( Component* owner, Params& params ) : TransactionQ( owner, params ) {
        windowCycles = params.find<unsigned int>("windowCycles", 10);
    }

    virtual Transaction* pop( unsigned row ) {

        size_t numTrans = m_transQ.size();

        // the most comman condition is empty
        if ( 0 == numTrans ) { 
            return NULL;
        }

        // the second most condition is size of 1
        if ( 1 == numTrans ) { 
            Transaction* trans = m_transQ.front();
            m_transQ.pop_front();    
            return trans; 

        }

        std::list<Transaction*>::iterator one = m_transQ.begin();

        if ( (*one)->row == row ) {
            Transaction* trans = (*one);
            m_transQ.erase(one);    
            return trans; 
        }

        std::list<Transaction*>::iterator two = std::next(one,1);

        // size > 2 is rare so didn't try to craft generic reorder code
        // see if we can swap position 1 and 2
        if ( (*two)->row == row && 
             //(*one)->col != (*two)->col && 
               (*one)->createTime + windowCycles > (*two)->createTime ) 
        {
            Transaction* trans = (*two);
            m_transQ.erase(two);    
            return trans;
        }
        Transaction* trans = m_transQ.front();
        m_transQ.pop_front();    
        return trans; 
    }
  private:

    unsigned  windowCycles;
};

}
}
}

#endif
