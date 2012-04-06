#ifndef _barrier_h
#define _barrier_h

#include <sst/core/action.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if 0 
#define BA_DBG(fmt,args...) \
  fprintf(stderr,"%d:BarrierAction::%s() "fmt, world.rank(), __func__, ##args)
#else
#define BA_DBG(fmt,args...)
#endif

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


    BarrierAction( int numPerRank ) :
        m_numReporting( 0 ),
        m_opened( false )
    {
        BA_DBG("\n");
        setPriority(75);
        m_nRanks = SST::Simulation::getSimulation()->getNumRanks();

        SST::Simulation::getSimulation()->insertActivity(
            SST::Simulation::getSimulation()->getTimeLord()->
                getTimeConverter("1us")->getFactor(),this);

        char buf[100];
        sprintf( buf, "/tmp/sst-barrier.%d", getpid() );
        int rc = mkfifo( buf, 0666 );
        assert( rc == 0 );

        m_readFd = open( buf, O_RDONLY | O_NONBLOCK ); 
        assert( m_readFd > -1 );

        m_writeFds.resize( numPerRank );

        for ( int i = 0; i < m_writeFds.size(); i++ ) {
            sprintf( buf, "/tmp/sst-barrier-app-%d.%d", i, getpid() );
            rc = mkfifo( buf, 0666);
            assert( rc == 0 );
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

            for ( int i = 0; i < m_writeFds.size(); i++ ) {
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
        for ( int i = 0; i < m_writeFds.size(); i++ ) {
            char buf[100];
            sprintf( buf, "/tmp/sst-barrier-app-%d.%d", i, getpid() );
            m_writeFds[i] = open( buf, O_WRONLY ); 
            assert( m_writeFds[i] > -1 );
        }
    }

  private:
    int m_opened;
    int m_nRanks;
    int m_numReporting;
    int m_readFd;
    std::vector< int > m_writeFds;
};

#endif
