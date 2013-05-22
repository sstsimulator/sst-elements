#include	"irisNicFlit.h"

using namespace std;

/* 
 * Flit class implementation
 */
Flit::Flit()
{
    type = UNK;
    virtual_channel = -1;
    pkt_length = 0;
}

Flit::~Flit()
{

}

string
Flit::toString ( ) const
{
    stringstream str;
    switch(type) {
        case HEAD:
	    { const HeadFlit* hf = static_cast<const HeadFlit*>(this);
	      str << " HEAD " << hf->toString();
	    }
	    break;
        case BODY:
	    {
	      str << " BODY ";
	    }
	    break;
        case TAIL:
	    { //const TailFlit* tf = static_cast<const TailFlit*>(this);
	      str << " TAIL ";
	    }
	    break;
	default:
	    break;
    }
    /*
    */

    str <<
        " vc: " << virtual_channel <<
        //" no_phits: " << no_phits <<
        " pkt_length: " << pkt_length <<
        " ";
    return str.str();
}		/* -----  end of method Flit::toString  ----- */

/* 
 * HeadFlit class implementation
 */
HeadFlit::HeadFlit()
{
    type =HEAD;
}

HeadFlit::~HeadFlit()
{
}


// Do not call base class's toString() because it calls subclass's toString().
string
HeadFlit::toString () const
{
    stringstream str;
    str <<
        " src: " << src_id <<
        " dst: " << dst_id <<
        //" dst_compid: " << dst_component_id <<
        //" addr: " << hex << address << dec <<
        //" class: " << mclass <<
        " ";
    return str.str();
}		/* -----  end of method HeadFlit::toString  ----- */


void
HeadFlit::populate_head_flit(void)
{
    /* May want to construct control bit streams etc here */
}

/* 
 * BodyFlit class implementation
 */
BodyFlit::BodyFlit()
{
    type =BODY;
}

BodyFlit::~BodyFlit()
{
}


void
BodyFlit::populate_body_flit(void)
{
    /* May want to construct control bit streams etc here */
}

/* 
 * TailFlit class implementation
 */
TailFlit::TailFlit()
{
    type =TAIL ;
}

TailFlit::~TailFlit()
{
}


void
TailFlit::populate_tail_flit(void)
{
    /* May want to construct control bit streams etc here */
}

/* 
 * FlitLevelPacket class implementation
 */
FlitLevelPacket::FlitLevelPacket()
{
}

FlitLevelPacket::~FlitLevelPacket()
{
}

void FlitLevelPacket :: set_pkt_length(unsigned len)
{
    //This function should only be called to preset the pkt_length when there are
    //no flits yet. It doesn't make sense to change pkt_length at other times.
    assert(flits.size() == 0);
    pkt_length = len;
}


//! Append a flit.
void
FlitLevelPacket::add ( Flit* f )
{   
    switch ( f->type  ) {
        case HEAD:	
            {
		assert(flits.size() == 0);

                this->pkt_length = f->pkt_length;
                this->virtual_channel = f->virtual_channel;

                HeadFlit* hf = static_cast< HeadFlit* >(f);
                this->src_id = hf->src_id;
                this->dst_id = hf->dst_id;
                //this->dst_component_id = hf->dst_component_id;
                //this->address = hf->address;
                //this->mc = hf->mclass;
                break;
            }

        case BODY:	
            {
		assert(flits.size() > 0 && flits.size() < pkt_length);
	        //nothing to do.
                break;
            }

        case TAIL:	
            {
		assert(flits.size() > 0);
		assert(flits.size() + 1 == pkt_length);

                TailFlit* tf = static_cast< TailFlit* >(f);
                this->enter_network_time = tf->enter_network_time;
                break;
            }

        default:	
            cerr << " ERROR: Flit type unk " << endl;
            exit(1);
            break;
    }

    flits.push_back(f);
}		/* -----  end of method FlitLevelPacket::add  ----- */



Flit* 
FlitLevelPacket::pop_next_flit ( void )
{
    assert(flits.size() > 0);
    Flit* np = flits.front();
    flits.pop_front();
    return np;
}


uint
FlitLevelPacket::size ( void )
{
    return flits.size() ;
}		/* -----  end of method FlitLevelPacket::size  ----- */


bool
FlitLevelPacket::has_whole_packet ( void )
{
    if ( flits.size() > 0 )
        return ( flits.size() == flits[0]->pkt_length);
    else
        return false ;
}



