#ifndef IRIS_NIC_ARBITER_H
#define IRIS_NIC_ARBITER_H

#include <sst/elements/iris/SST_interface.h>

class irisNicArbiter
{
    public:
        irisNicArbiter (unsigned nch);
        virtual ~irisNicArbiter();
        bool is_requested(unsigned ch);
        bool is_empty();
        virtual void request( unsigned ch);
        virtual void clear_request( unsigned ch);
        virtual unsigned pick_winner() = 0;

        const unsigned no_channels;
        std::vector < bool > requested;
};



//! Round-robin arbiter
class RRirisNicArbiter : public irisNicArbiter {
public:
    RRirisNicArbiter(unsigned nch);

    virtual unsigned pick_winner();

private:
    unsigned last_winner;
};



//! First-come-first-serve arbiter
class FCFSirisNicArbiter : public irisNicArbiter {
public:
    FCFSirisNicArbiter(unsigned nch);

    virtual void request(unsigned ch);
    virtual unsigned pick_winner();

private:
    std::list<unsigned> m_requesters; //vcs making request
};



#endif  /*IRIS_NIC_ARBITER_H*/
