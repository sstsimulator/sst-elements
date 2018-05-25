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


class UnitPool {
    enum { RoundRobin, PerContext } m_nicUnitAllocPolicy;
  public:
    UnitPool( Output& output, std::string policy, int numRecvUnits, int numSendUnits, int numAckUnits, int numCtx ) :
        m_dbg(output),
        m_numRecvUnits(numRecvUnits),
        m_numSendUnits(numSendUnits),
        m_numAckUnits(numAckUnits),
        m_numCtx(numCtx),
        m_curSendUnit(0)
    {
        m_dbg.debug(CALL_INFO,3,1,"numRecvUnits=%d  numSendUnits=%d policy=%s\n",
                numRecvUnits, numSendUnits, policy.c_str());
        if ( 0 == policy.compare("RoundRobin") ) {
            m_nicUnitAllocPolicy = RoundRobin;
            m_curRecvUnit.resize(1,0);
        } else if ( 0 == policy.compare("PerContext") ) {
            assert( numRecvUnits % numCtx == 0 ); 
            m_curRecvUnit.resize(numCtx,0);
            m_nicUnitAllocPolicy = PerContext;
        } else {
            assert(0);
        }
    } 
    int allocRecvUnit( int pid ) {
        int unit;
        switch ( m_nicUnitAllocPolicy ) {
          case RoundRobin:
            unit = m_curRecvUnit[0]++ % m_numRecvUnits;
            break;
          case PerContext:
            unit = pid * m_numRecvUnits/m_numCtx;
            unit += m_curRecvUnit[pid]++ % m_numRecvUnits/m_numCtx;
            break;
          default:
            assert(0);
        }
        m_dbg.debug(CALL_INFO,3,1,"pid=%d unit=%d\n",pid, unit);
        return unit;
    }
    int allocSendUnit( ) {
        return m_numRecvUnits + (m_curSendUnit++ % m_numSendUnits );
    }
    int allocAckUnit() {
        return m_numRecvUnits + m_numSendUnits;
    }

    int getTotal() {
        return m_numRecvUnits + m_numSendUnits + m_numAckUnits;
    }
  private:
    Output& m_dbg;
    int m_numNicUnitsPerCtx;
    int m_numRecvUnits;
    int m_numSendUnits;
    int m_numAckUnits;
    std::vector<int> m_curRecvUnit;
    int m_curSendUnit;
    int m_numCtx;
};
