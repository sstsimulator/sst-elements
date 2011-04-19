#ifndef _barrier_h
#define _barrier_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>
#include <debug.h>

class Barrier {

  public:
    class BarrierEvent : public SST::Event {
      public:
         BarrierEvent() {}
      private: 
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {   
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        }
    };

    typedef std::deque< SST::Event::HandlerBase* > queue_t;

  public:
    Barrier( SST::Component* comp, SST::Params params ) :
        m_count( 0 ),
        m_parent( NULL ),
        m_phase( Arrival )
    {
        DBGX(1,"\n");

        params.print_all_params(std::cout);
        
        std::string linkName = params.find_string("parent");

        if ( ! linkName.empty() ) {
            DBGX(1,"configure parent link `%s`\n",linkName.c_str());
            m_parent = comp->configureLink( linkName, "1ns",
                            new SST::Event::Handler<Barrier>(
                                        this, &Barrier::barrierEvent ) );

            assert( m_parent );
        }

        int num = 0;
        while ( 1 ) {

            std::stringstream tmp;
            tmp << "child." << num;
            std::string linkName = params.find_string( tmp.str() );
            if ( linkName.empty() ) {
                break;
            }

            ++num;

            DBGX(1,"configure %s link `%s`\n", tmp.str().c_str(), 
                                                    linkName.c_str());
            m_children.push_back( comp->configureLink( linkName, "1ns",
                    new SST::Event::Handler<Barrier>(
                                this, &Barrier::barrierEvent ) ) );

            assert( m_children.back() );
        } 
    }

    void add( SST::Event::HandlerBase* handler ) {
        DBGX(1,"\n");
        m_queue.push_back( handler );
    }

    void barrierEvent( SST::Event* e )
    {
        delete e;
        DBGX(1,"%s\n", m_phase == Arrival ? "Arrival" : "Departure");
        if ( m_phase == Arrival )  {
            arrival();
        }
        if ( m_phase == Departure ) {
            departure();
        }
    }

    void arrival( ) {
        ++m_count;
        DBGX(1,"m_count=%d need=%d\n",m_count, 
                            m_queue.size() + m_children.size());
        if ( m_count == m_queue.size() + m_children.size() ) {
            DBGX(1,"set to Departure\n");
            m_phase = Departure;
            if ( m_parent ) {
                DBGX(1,"send to parent\n");
                m_parent->Send( new BarrierEvent );
            }  else {
                departure();
            }
        } 
    }

    void enter( ) {
        arrival();
    }

    void departure( ) {
        queue_t::iterator it = m_queue.begin(); 

        while( it != m_queue.end() ) {
            (**it)(NULL);
            ++it;
        }

        sendChildren();

        m_phase = Arrival;
        m_count = 0;
    }

    void sendChildren() {
        std::deque<SST::Link*>::iterator it = m_children.begin(); 
        while( it != m_children.end() ) {
            DBGX(1,"send to child\n");
            (*it)->Send( new BarrierEvent );
            ++it;
        }
    }

  private:

    enum Phase { Arrival, Departure };

    int         m_count;
    queue_t     m_queue; 
    SST::Link*  m_parent;
    Phase       m_phase;
    std::deque<SST::Link*>  m_children;
};

#endif
