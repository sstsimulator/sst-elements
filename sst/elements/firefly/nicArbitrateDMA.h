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

class ArbitrateDMA {

    struct XX {
        long         avail;
        SimTime_t   lastTime;
        bool        blocked;
    };

  public:
    enum Type { Write, Read };
    ArbitrateDMA( Nic& nic, Output& dbg, float GBs, float contentionMult, 
                                    int bufferSize ) : 
        m_nic(nic), m_dbg(dbg), m_GBs(GBs), m_contentionMult( contentionMult),
        m_bufferSize( bufferSize ),
        m_xx(2) 
    {
        for ( unsigned i = 0; i < m_xx.size(); i++ ) {
            m_xx[i].avail = m_bufferSize;
            m_xx[i].lastTime = 0;
            m_xx[i].blocked = false;
        }
    } 

    bool canIWrite( int bytes ) {

        m_dbg.verbose(CALL_INFO,1,0,"bytes=%d\n",bytes);
        uint64_t delay = foo( m_xx[Write], m_xx[Read], bytes  );
        if ( delay ) {
            schedWakeup( Write, delay );
        }
        return ( 0 ==  delay); 
    }

    bool canIRead( int bytes ) {
        m_dbg.verbose(CALL_INFO,1,0,"bytes=%d\n",bytes);
        uint64_t delay = foo( m_xx[Read], m_xx[Write], bytes  );
        if ( delay ) {
            schedWakeup( Read, delay );
        }
        return ( 0 ==  delay); 
    }

  private:
    void updateAvail( XX& xx ) {
        SimTime_t delta = m_nic.getCurrentSimTimeNano() - xx.lastTime; 
        xx.lastTime = m_nic.getCurrentSimTimeNano();
        long add = (float) delta * m_GBs;  
        xx.avail += add; 

        if ( xx.avail > m_bufferSize ) {
            xx.avail = m_bufferSize;
        }

        m_dbg.verbose(CALL_INFO,1,0,"avail Bytes %ld, delta %lu ns, "
            "added %ld \n", xx.avail, delta, add ); 
		//assert( xx.avail >= 0 );
    }

    void schedWakeup( Type type, uint64_t delay) {
        SelfEvent* event = new SelfEvent;
        m_dbg.verbose(CALL_INFO,1,0,"wakeup %s in %lu ns\n",
                    Read == type ? "Send":"Recv", delay);
        if ( Read == type ) {
            event->type = SelfEvent::RunSendMachine;
        } else {
            event->type = SelfEvent::RunRecvMachine;
        }
    
        m_nic.schedEvent( event, delay );
    }

    uint64_t foo( XX& me, XX& other, int bytes ) {

        updateAvail( me );

        adjustOther( other, bytes ); 

        uint64_t delay = 0;
        if ( me.avail - bytes < 0 ) {
            me.blocked = true;
            delay = (m_bufferSize/2) / m_GBs;
        } else {
            me.avail -= bytes;
        }

        return delay;
    }

    void adjustOther( XX& xx, int bytes ) {
        xx.avail -= (float) bytes * m_contentionMult;
    }

    Nic&        m_nic;
    Output&     m_dbg;
    float       m_GBs;
    float       m_contentionMult;
    int         m_bufferSize;

    std::vector<XX> m_xx;
};
