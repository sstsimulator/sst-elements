
#ifndef COMPONENTS_FIREFLY_MERLINEVENT_H
#define COMPONENTS_FIREFLY_MERLINEVENT_H

namespace SST {
namespace Firefly {


class MerlinFireflyEvent : public Merlin::RtrEvent {

    static const int BufLen = 56;

  public:
    uint16_t        seq;
    std::string     buf;

    MerlinFireflyEvent() {}

    MerlinFireflyEvent(const MerlinFireflyEvent *me) :
        Merlin::RtrEvent()
    {
        buf = me->buf;
        seq = me->seq;
    }

    MerlinFireflyEvent(const MerlinFireflyEvent &me) :
        Merlin::RtrEvent()
    {
        buf = me.buf;
        seq = me.seq;
    }

    virtual RtrEvent* clone(void)
    {
        return new MerlinFireflyEvent(*this);
    }

    void setNumFlits( size_t len ) {
        size_in_flits = len / 8;
        if ( len % 8 )
        ++size_in_flits;

        // add flit for 8 bytes of packet header info 
        ++size_in_flits;
    }

    void setDest( int _dest ) {
        dest = _dest;
    }

    void setSrc( int _src ) {
        src = _src;
    }

    size_t getMaxLength(){
        return BufLen;
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RtrEvent);
        ar & BOOST_SERIALIZATION_NVP(seq);
        ar & BOOST_SERIALIZATION_NVP(buf);
    }
};

}
}

#endif
