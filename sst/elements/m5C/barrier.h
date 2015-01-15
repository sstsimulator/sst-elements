// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _barrier_h
#define _barrier_h

#include <sst/core/action.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/timeConverter.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include <boost/filesystem.hpp>
#include "sst/core/debug.h"

#if 0 
#define BA_DBG(fmt,args...) \
  fprintf(stderr,"%d:BarrierAction::%s() "fmt, world.rank(), __func__, ##args)
#else
#define BA_DBG(fmt,args...)
#endif

namespace SST {
namespace M5 {

class BarrierAction : public SST::Action 
{
     boost::mpi::communicator world; 

  public:

    class HandlerBase {
      public:
        virtual void operator()() = 0;
        virtual ~HandlerBase() {}
    };

    template <typename classT>
    class Handler : public HandlerBase {
      private:
        typedef void (classT::*PtrMember)();
        const PtrMember member;
        classT* object;

      public:
        Handler( classT* const object, PtrMember member ) :
          member(member),
          object(object)
        {}

        void operator()() {
            (object->*member)();
        }
    };
    
    ~BarrierAction(){
    /*
        try{
            close(m_readFd);
            if(!m_readFilename.empty() && fileExists(m_readFilename)) remove(m_readFilename.c_str());
            for(unsigned int i = 0; i < m_writeFds.size(); i++){
                close(m_writeFds[i]);
            }
            
            for(unsigned int i = 0; i < m_writeFilenames.size(); i++){
                 //boost::filesystem::exists(m_writeFilenames[i].c_str());
                if(m_writeFilenames[i].empty()) continue;
                if(fileExists(m_writeFilenames[i])) remove(m_writeFilenames[i].c_str());
            }
        }
        catch(const std::exception& e){}
    */
    }


    BarrierAction( int numPerRank ) :m_numReporting( 0 ), m_opened( false ){
        BA_DBG("\n");
        setPriority(BARRIERPRIORITY);
        char buf[100];
        m_nRanks = SST::Simulation::getSimulation()->getNumRanks();

        SST::Simulation::getSimulation()->insertActivity(
            SST::Simulation::getSimulation()->getTimeLord()->
                getTimeConverter("1us")->getFactor(),this);
        
        
        sprintf( buf, "/tmp/sst-barrier.%d", getpid() );
        BA_DBG("filename=`%s`\n",buf);
        std::string name(buf);
        m_readFilename = name;
        int rc = mkfifo( buf, 0666 );
        if(rc != 0) _abort(BarrierAction, "Unable to create temp file named: %s", buf);
        m_readFd = open( buf, O_RDONLY | O_NONBLOCK );
        assert( m_readFd > -1 );

        m_writeFds.resize( numPerRank );
        m_writeFilenames.resize(numPerRank);

        for ( unsigned int i = 0; i < m_writeFds.size(); i++ ) {
            sprintf( buf, "/tmp/sst-barrier-app-%u.%d", i, getpid() );
            rc = mkfifo( buf, 0666);
            BA_DBG("filename=`%s`\n",buf);
            std::string name(buf);
            m_writeFilenames.push_back(name);
            if(rc != 0) _abort(BarrierAction, "Unable to create temp file named: %s", buf);
        }
    }

    void execute() {
        SST::Simulation *sim = SST::Simulation::getSimulation();

        int buf;
        while ( read( m_readFd, &buf, sizeof( buf) ) == sizeof(buf) ) {
            BA_DBG("%d entered barrier\n",buf);
            ++m_numReporting;
        }

        int value = m_writeFds.size() - m_numReporting ? 0 : 1 ;
        int out;
        
        all_reduce( world, &value, 1, &out, std::plus<int>() );

        if ( out == m_nRanks ) {
            BA_DBG("everyone is here\n");  

            if ( ! m_opened ) {
                openWriteFds();
                m_opened = true;
            }

            for ( unsigned int i = 0; i < m_writeFds.size(); i++ ) {
                int rc = write( m_writeFds[i], &buf, sizeof(buf) ); 
                assert( rc == sizeof(buf) );
            }

            m_numReporting = 0;
        }  

        SST::SimTime_t next = sim->getCurrentSimCycle() +
        sim->getTimeLord()->getTimeConverter("1us")->getFactor();
        sim->insertActivity( next, this );
    }

    void openWriteFds() {
        for ( int unsigned i = 0; i < m_writeFds.size(); i++ ) {
            char buf[100];
            sprintf( buf, "/tmp/sst-barrier-app-%u.%d", i, getpid() );
            m_writeFds[i] = open( buf, O_WRONLY ); 
            assert( m_writeFds[i] > -1 );
        }
    }

  private:
    int m_nRanks;
    int m_numReporting;
    int m_opened;
    int m_readFd;
    std::string m_readFilename;
    std::vector<std::string> m_writeFilenames;
    std::vector< int > m_writeFds;
    
    inline bool fileExists(const std::string& name) {
        struct stat buffer;
        return (stat (name.c_str(), &buffer) == 0);
    }
};

}}
#endif
