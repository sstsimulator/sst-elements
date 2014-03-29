/*
 * File:   MESITopCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef __SST__MOESITopCoherenceController__
#define __SST__MOESITopCoherenceController__

#include <iostream>
#include "MESITopCoherenceController.h"


namespace SST { namespace MemHierarchy {



/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MOESI protocol, should be use by any cache that is not L1
 * keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MOESITopCC : public MESITopCC{
private:




public:
    MOESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines, uint _lineSize, uint64_t _accessLatency, vector<Link*>* _childrenLinks) :
    MESITopCC(_cache, _dbg, _protocol, _numLines, _lineSize, _accessLatency, _childrenLinks){}
    
    bool handleEviction(int lineIndex, BCC_MESIState _state);
    void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd);
    bool handleAccess(MemEvent* event, CacheLine* cacheLine);
    void handleInvalidate(int lineIndex, Command cmd);
    void handleInvAck(MemEvent* event, CCLine* ccLine);

};

}}


#endif /* defined(__SST__MOESITopCoherenceController__) */
