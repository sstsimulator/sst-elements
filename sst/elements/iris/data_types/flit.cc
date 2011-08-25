/*
 * =====================================================================================
//       Filename:  flit.cc
 * =====================================================================================
 */

#ifndef  _FLIT_CC_INC
#define  _FLIT_CC_INC

#include	"flit.h"

/*
* Flit class implementation
*/
Flit::Flit(): type (UNK), virtual_channel(-1), pkt_length(0) 
{
}

Flit::Flit(flit_type t): type (t), virtual_channel(-1), pkt_length(0)
{
}

Flit::~Flit()
{

}

std::string
Flit::toString ( ) const
{
    std::stringstream str;
    str <<
        " type: " << type <<
        " vc: " << virtual_channel <<
        " pkt_length: " << pkt_length 
        << "\n";
    return str.str();
} /* ----- end of method Flit::toString ----- */

/*
 * HeadFlit class implementation
 */
HeadFlit::HeadFlit(): Flit(HEAD) //,src_node_id(-1), dst_node_id(-1),dst_component_id(-1)
{
}

HeadFlit::~HeadFlit()
{
}

std::string
HeadFlit::toString () const
{
    std::stringstream str;
    str << " HEAD"
        << " src_node_id: " << src_node_id 
        << " dst_node_id: " << dst_node_id
        << " dst_compid: " << dst_component_id 
        << " mem_address: 0x" << std::hex << mem_address << std::dec 
        << " mclass: " << mclass 
                          ;
    str << static_cast<const Flit*>(this)->toString();
    return str.str();
} /* ----- end of method HeadFlit::toString ----- */

void
HeadFlit::populate_head_flit(void)
{
    /* May want to construct control bit streams etc here */
}

/*
 * BodyFlit class implementation
 BodyFlit::BodyFlit()
 {
 type =BODY;
 }

 BodyFlit::~BodyFlit()
 {
 }

 string
 BodyFlit::toString () const
 {
 std::stringstream str;
 std::str << " BODY "
 << static_cast<const Flit*>(this)->toString();
 << std::endl;
 return str.str();
 }

 void
 BodyFlit::populate_body_flit(void)
 {
 }

 */

/*
 * TailFlit class implementation
 */
TailFlit::TailFlit():Flit(TAIL)
{
}

TailFlit::~TailFlit()
{
}

std::string
TailFlit::toString () const
{
    std::stringstream str;
    str << " TAIL"
        << " packet_originated_time: " << packet_originated_time
        << " packet_arrival_time: " << packet_arrival_time
        ;
    str << static_cast<const Flit*>(this)->toString();
    return str.str();
} /* ----- end of method TailFlit::toString ----- */

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

void
FlitLevelPacket::add ( Flit* f )
{
    switch ( f->type ) {
        case HEAD:
            {
                this->pkt_length = f->pkt_length;
                this->virtual_channel = f->virtual_channel;

                HeadFlit* hf = static_cast< HeadFlit* >(f);
                this->src_node_id = hf->src_node_id;
                this->dst_node_id = hf->dst_node_id;
                this->dst_component_id = hf->dst_component_id;
                this->address = hf->mem_address;
                this->mc = hf->mclass;

                delete hf;
                break;
            }

        case BODY:
            {
                // No specific type defined for this and no payload simulated.
                // Just delete the flit
                delete f;
                break;
            }

        case TAIL:
            {
                TailFlit* tf = static_cast< TailFlit* >(f);
                this->packet_originated_time = tf->packet_originated_time;
                this->packet_arrival_time = tf->packet_arrival_time;
                delete tf;
                break;
            }

        default:
            std::cerr << " ERROR: Flit type unk " << std::endl;
            exit(1);
            break;
    }
    return ;
} /* ----- end of method FlitLevelPacket::add ----- */

Flit*
FlitLevelPacket::get_next_flit ( void )
{
    Flit* np = flits.front();
    flits.pop_front();
    return np;
}

uint16_t
FlitLevelPacket::size ( void )
{
    return flits.size() ;
} /* ----- end of method FlitLevelPacket::size ----- */

bool
FlitLevelPacket::valid_packet ( void )
{
    if ( flits.size() )
        return ( flits.size() == flits[0]->pkt_length);
    else
        return false ;
} /* ----- end of method FlitLevelPacket::valid_packet ----- */


#endif   /* ----- #ifndef _FLIT_CC_INC  ----- */
