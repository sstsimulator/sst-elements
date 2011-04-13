#ifndef _barrier_h
#define _barrier_h

#include <sst/core/event.h>
#include <sst/core/simulation.h>
#include <sst/core/action.h>
#include <boost/mpi.hpp>

class Barrier {

    typedef std::deque< SST::Event::HandlerBase* > queue_t; 

    class action : public SST::Action {
      public:
        action( Barrier* obj) : Action(), m_obj(obj) {}

        void execute( ) {
            boost::mpi::communicator world;
            world.barrier(); 
            m_obj->finish();
        }
      private:
        Barrier* m_obj;
    };
   
  public:
    Barrier() : 
        m_count(0),
        m_sim( SST::Simulation::getSimulation() )
    {
    }

    void add( SST::Event::HandlerBase* handler ) {
        m_queue.push_back( handler );
    }

    void enter( ) {
        SST::Event* ptr;

        ++m_count;
        if ( m_count == m_queue.size() ) {
            m_sim->insertActivity( m_sim->getCurrentSimCycle(), 
                                                new action(this) ); 
        }
    }

    void finish( ) {
        queue_t::iterator it = m_queue.begin(); 
        while( it != m_queue.end() ) {
            (**it)(NULL);
            ++it;
        }
        m_count = 0;
    }

  private:

    queue_t             m_queue; 
    int                 m_count;
    SST::Simulation    *m_sim;
};

#endif
