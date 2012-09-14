#ifndef __rawEvent_h
#define __rawEvent_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <debug.h>

class RawEvent : public SST::Event {
  public:
    typedef uint64_t type_t;
    RawEvent( void* data, size_t len) :
        m_data( static_cast<type_t*>(data) ),
        m_numElements( len / sizeof( type_t) )
    {
        DBGX(2,"numElements=%d sizeof(Element)=%d\n",len,len % sizeof(type_t));
        assert( ! ( len % sizeof(type_t) ) );
    }

    void * data() { return m_data; }
    size_t len()  { return m_numElements * sizeof(type_t); }

  private:
    type_t*  m_data;
    int      m_numElements;

    RawEvent() :
        m_data( NULL )
     { }

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP( m_numElements );

        DBGX(2,"%s() data=%p numElements=%d\n",__func__,m_data,m_numElements);

        for ( int i = 0; i < m_numElements; i++ ) {
            ar & m_data[i];
        }
        delete m_data;
    }
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP( m_numElements );

        DBGX(2,"%s() data=%p numElements=%d\n",__func__,m_data,m_numElements);

        m_data = new type_t[m_numElements];
        for ( int i = 0; i < m_numElements; i++ ) {
            ar & m_data[i];
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif
