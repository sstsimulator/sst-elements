#ifndef IRIS_NIC_H
#define IRIS_NIC_H

#include "irisNicBuffer.h"
#include "irisNicArbiter.h"
#include "simpleCache/cache_messages.h"

#include "RtrIF.h"


#include <iostream>
#include <assert.h>



/* *********** the Iris NIC start here ************ */
//! Class T must support get_dst() which returns destination ID.
//! Class T must be a child class of SST::Event
//! Data structures used:
//!
//!       |    ...    |
//!       |-----------|  input_pkt_buffer; no size limit;
//!       |-----------|  holds packets from terminal
//!
//!      -----------  proc_out_buffer: a vector;
//!     |  |  |  |  | one element for a VC; each               proc_in_buffer is similar.
//!      -----------  element holds a FlitLevelPacket.
//!
//!      -----------  router_out_buffer: a vector; one
//!     |  |  |  |  | element for a VC; each a irisNicBuffer   router_in_buffer is similar.
//!      -----------  holding flits.
//!
//!
template<typename T>
class irisNic : public SST::RtrIF
{
    public:
        //the enums that specifies the ports
//        enum { DATAOUT=0, DATATOTERMINAL};
//        enum { DATAIN=0, DATAFROMTERMINAL};
        SST::Link * terminal_link;
        
        //irisNic () {} 
        irisNic (SST::ComponentId_t cid, SST::Component::Params_t &params);
        ~irisNic (); 

        /* ====================  Event handlers  (unsynchronized)   ======================================= */
        void handle_terminal(SST::Event * ev); //modified

        /* ====================  Clocked funtions  ======================================= */
        virtual bool tick (SST::Cycle_t cycle);

    	unsigned get_id() { return m_id; }
	    virtual void print_stats(std::ostream& out);

        void setup() {  
            std::cout << "irisNic " << m_id << " starts!" << std::endl;
        }
        
        void finish() { 
            std::cout << "irisNic " << m_id << " finished!" << std::endl;
            print_stats(std::cout);
        }

    private:

         /* ====================  convert the pkt to flits(or inversely)  ======================================= */
        void to_flit_level_packet(FlitLevelPacket* flp, uint lw, T* data);
        T* from_flit_level_packet(FlitLevelPacket* flp);

        unsigned no_vcs; //# of virtual channel
//        unsigned credits; //# credits for each channel
        unsigned link_width; //the link bandwidth in bits, fixed to 64 in iris
        unsigned last_inpkt_winner; //This is the input VC from router where we pulled in a complete packet.
	                            //This is used so the VCs are processed in a round-robin manner.

        //the infinite input buffer for the input from terminal
        std::list<T*> input_pkt_buffer;
        
        std::vector<FlitLevelPacket> proc_out_buffer; //A buffer where a packet is turned into flits; one per VC.

        irisNicBuffer * router_out_buffer; //Buffer for outgoing flits; from here flits enter the router.

        std::vector<bool> is_proc_out_buffer_free; //whether a slot of the proc_out_buffer is available.
	                                           //Note this is set to true when the whole packet has left the
						   //interface, NOT when the whole packet has been moved from
						   //proc_out_buffer to router_out_buffer.

//        std::vector < int > downstream_credits; //the credit correspond for link to network

        irisNicBuffer * router_in_buffer; //input buffer for flits coming in from router.

        std::vector<FlitLevelPacket> proc_in_buffer; //A buffer where flits for the same packet are assembled.
	                                             //One per VC.
        
        //the arbiter used in interface
        irisNicArbiter* arbiter; 
    
        //stats
        uint64_t stat_packets_in_from_router; 
        uint64_t stat_packets_out_to_router;
    	uint64_t stat_packets_in_from_terminal;
    	uint64_t stat_packets_out_to_terminal;

        //pointer of next packet to router
        irisRtrEvent * next_to_router;

};  /* -----  end of class generic interface  ----- */

//! the constructor
//! initializing all the vector to given size and assign initial value to them
//! @param \c i_p  # of virtual channels, # of credits for each channel, the link bandwidth in bits
template<typename T>
irisNic<T>::irisNic (SST::ComponentId_t cid, SST::Component::Params_t & params) :
    SST::RtrIF(cid, params),
    /*credits(i_p->num_credits),*/ link_width(PKT_SIZE * sizeof(int) * 8),/*always PKT_SIZE defined in iris router*/
	/*router_out_buffer(i_p->num_vc, 6*i_p->num_credits),*/
	/*router_in_buffer(i_p->num_vc, 6*i_p->num_credits),*/
    next_to_router(NULL)
{

    
    if ( params.find( "num_vc" ) != params.end() ) {
        no_vcs = params.find_integer("num_vc");
    }
    else
        _abort(iris_nic, "no virtual channel number found! use \"num_vc\" to specify!\n");

    if ( params.find("clock") == params.end() )
        _abort(iris_nic, "no clock freq found! use \"clock\" to specify!\n");

    if ( params.find( "Node2RouterQSize_flits" ) == params.end() )
        _abort(iris_nic, "no token/credit number found! use \"Node2RouterQSize_flits\" to specify!\n");

    registerClock( params["clock"], new SST::Clock::Handler< irisNic >(this, &irisNic::tick) );

    terminal_link = configureLink( "terminal", new SST::Event::Handler< irisNic >(this,&irisNic::handle_terminal) );
     
 
    assert(link_width % 8 == 0); //link_width must be multiple of 8

    //initializing vectors to given size
    router_out_buffer = new irisNicBuffer(no_vcs); 
    proc_out_buffer.resize ( no_vcs );
    is_proc_out_buffer_free.resize(no_vcs);
    router_in_buffer = new irisNicBuffer(no_vcs);
    proc_in_buffer.resize(no_vcs);
    //downstream_credits.resize( no_vcs );
    //router_ob_packet_complete.resize(no_vcs);

    arbiter = new FCFSirisNicArbiter(no_vcs);

    last_inpkt_winner = -1; //set to -1 so the very first time VC 0 has highest priority.

    //initializing the values for each vector
    for ( uint i=0; i<no_vcs; i++)
    {
        is_proc_out_buffer_free[i] = true;
        //router_ob_packet_complete[i] = false;
        //downstream_credits[i] = credits;
    }


    /* Init stats */
    stat_packets_in_from_terminal = 0;
    stat_packets_out_to_terminal = 0;
    stat_packets_in_from_router = 0;
    stat_packets_out_to_router = 0;
}


//! the deconstructor
template<typename T>
irisNic<T>::~irisNic ()
{
    delete arbiter;
    delete router_in_buffer;
    delete router_out_buffer;
}

//! response for putting flits into buffer and add the downstream credit
//! @param \c port  The port # where the data come from
//! @param \c data  The link data come from network which can be two types: credit and flit
#if 0
template<typename T>
void
irisNic::handle_router (int port, LinkData* data )
{   
    switch ( data->type ) {
        case FLIT:
            {
#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " got flit from router: " << data->f->toString() << std::endl;
#endif

                if ( data->f->type == TAIL )
                    stat_packets_in_from_router++;
                
                //push the incoming flit to the router in buffer 
                router_in_buffer->push(data->vc, data->f);
                
                if ( data->f->type != TAIL && data->f->pkt_length != 1)
                {
                    //generate credit and send back to data source
                    LinkData* ld =  new LinkData();
                    ld->type = CREDIT;
                    ld->src = id;
                    ld->vc = data->vc;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND Head/Body CREDIT vc " << ld->vc << std::endl;
#endif
                    //send credit to router
                    send(DATAOUT, ld); 
                }
                break;
            }

        case CREDIT:
            {
                downstream_credits[data->vc]++;
                assert(downstream_credits[data->vc] <= (int)credits);
                break;
            }

        default:	
            assert(0);
            break;
    }	
    // delete ld.. if ld contains a flit it is not deleted. see destructor
    delete data;
}
#endif

//! putting packets into the input buffer for terminal
//! @param \c port  The port # where the data come from
//! @param \c data  The packet come from terminal
template<typename T>
void
irisNic<T>::handle_terminal (SST::Event * ev)
{ 
    T * data = dynamic_cast<T *>(ev);  
//    assert(port == DATAFROMTERMINAL);
    input_pkt_buffer.push_back(data);
    stat_packets_in_from_terminal++;
}


//! Generate a new packet when all the flits in a packet are received; flits are deleted.
//! @param \c flp  Pointer to a FlitLevelPacket, which is a container that holds all flits for a packet.
//! @return  Pointer to a packet.
template<typename T>
T*
irisNic<T>::from_flit_level_packet(FlitLevelPacket* flp)
{
    T* message = new T;
    uint8_t* data = new uint8_t[message->get_pack_size()];
    uint8_t *ptr = data;
    uint byte_count = 0;
    
    //for (unsigned i = 0; i < flp->size(); i++)
    while(flp->size() > 0)
    {
        Flit* f = flp->pop_next_flit ();
        if (f->type == BODY)
        {
//            uint8_t * data = static_cast<BodyFlit*>(f)->data;
        	assert(byte_count < (uint)message->get_pack_size());
            for (unsigned j = 0; j < link_width/8; j++)
            {
                if(byte_count < (uint)message->get_pack_size()) {
    	    	    *ptr = static_cast<BodyFlit*>(f)->data[j];
	        	    ptr++;
		            byte_count++;
		        }
            }
        } 
	delete f;
    }

    message->unpack_data(data);
    delete[] data;
    
    return message;
}


//! 1. For each VC, move one flit from proc_out_buffer to router_out_buffer, if available.
//! 2. Make arbitration for each VC with a flit to send. The winner leaves the interface and enter the router.
//! 3. Check proc_in_buffer to see if entire packets have been received; if so, deliver packets to terminal.
//! 4. For each VC, move one flit from router_in_buffer to proc_in_buffer, if possible.
//!
template<typename T>
bool
irisNic<T>::tick (SST::Cycle_t cycle)
{
    // For each VC, move one flit from the proc_out_buffer (which holds FlitLevelPkt) to the
    // router_out_buffer (which holds individual flits)
    for ( uint i=0 ; i<no_vcs ; i++ )
    {
        if ( proc_out_buffer[i].size()>0 )
        {
            //router_out_buffer -> flit level buffer
            Flit* f = proc_out_buffer[i].pop_next_flit();
            f->virtual_channel = i;
            router_out_buffer->push(i, f);
        }
    }
   
        //Arbitration request. Only make a request when there's credit for sending.
        for ( uint i=0; i<no_vcs ; i++ ) {
            if( /*downstream_credits[i]>0 &&*/ router_out_buffer->get_occupancy(i)>0)
            {
    //            Flit* f= router_out_buffer->peek(i);
    //            if((f->type != HEAD)||( f->type == HEAD  && downstream_credits[i] == (int)credits))
                {
    		if(!arbiter->is_requested(i))
    		    arbiter->request(i);
                }
            }
        }
    
    if(next_to_router == NULL) {//pick winner only when last packet is sent succesfully
        // Pick a winner for the arbiter
        uint winner = arbiter->pick_winner();
        if(winner < no_vcs) {
            arbiter->clear_request(winner);
    
            // send out a flit as credits were already checked earlier this cycle
            Flit* f = router_out_buffer->pull(winner);
    
            //reduce the local downstream credit
            //downstream_credits[winner]--;
    
            //if this is the last flit the proc out buffer will available again
            if ( f->type == TAIL || f->pkt_length == 1 )
            {
                if( f->type == TAIL )
                    static_cast<TailFlit*>(f)->enter_network_time = getCurrentSimTime();
                //router_ob_packet_complete[winner] = false;
    	    //mark the proc_out_buffer slot as free
                is_proc_out_buffer_free[winner] = true;
    	        stat_packets_out_to_router++;
            }
    
    #if 0
            //send the flit over the winner channel
            LinkData* ld =  new LinkData();
            ld->type = FLIT;
            ld->src = this->id; //ld->src only useful for debug
            ld->f = f;
            ld->vc = winner;
    
    #ifdef DEBUG_IRIS_INTERFACE
    std::cout << "Interface " << id << " SEND FLIT to router on VC " << winner << " Flit is " << f->toString() << std::endl;
    #endif    
            //send data to router
            send(DATAOUT, ld); 
    #endif
    
            next_to_router = new irisRtrEvent(); 
            next_to_router->type = irisRtrEvent::Packet;
            next_to_router->packet = new irisNPkt();
            next_to_router->packet->vc = 0;
            assert(f->src_id == (unsigned int)m_id);
            next_to_router->packet->srcNum = f->src_id;
            next_to_router->packet->destNum = f->dst_id;
            next_to_router->packet->sizeInFlits = f->pkt_length;
            next_to_router->packet->mclass = PROC_REQ; //FIX ME, what actually is message class?
    
    
            next_to_router->packet->payload[0] = f->type ;
            assert(winner == f->virtual_channel);
            next_to_router->packet->payload[1] = winner;
    
            if(f->type == BODY)
                memcpy(&next_to_router->packet->payload[2], 
                    (static_cast<BodyFlit *>(f))->data, PKT_SIZE*sizeof(int));
            else
                memset(&next_to_router->packet->payload[2], 0, PKT_SIZE*sizeof(int));
            delete f;

        }
    }

    if(next_to_router != NULL) {
        if(send2Rtr(next_to_router)) {//if success, set to NULL, else wait for next cycle
            next_to_router = NULL;
        }
    }

    // Take packets from input_pkt_buffer, convert them into flits, and store in free slots of proc_out_buffer
    for ( uint i=0; i < no_vcs; i++ )
    {
        if (is_proc_out_buffer_free[i] /*&& downstream_credits[i] == (int)credits*/ && !input_pkt_buffer.empty())
        {  
           //convert the T to flits and copy it to out buffer
    	   T* pkt = input_pkt_buffer.front();
           to_flit_level_packet( &proc_out_buffer[i], link_width, pkt);

           //assign virtual channel
           proc_out_buffer[i].virtual_channel = i;
               
           is_proc_out_buffer_free[i] = false;
           assert( proc_out_buffer[i].size() != 0 );
            
           //remove the element in pkt buffer
    	   delete pkt;
           input_pkt_buffer.pop_front();
        }
    }


    //Now process inputs from router
    for (uint i=0; i<no_vcs; i++) {
        while(!toNicQ_empty(i)) {
            irisRtrEvent * event = toNicQ_front(i);
            toNicQ_pop(i);

            if (event->packet->payload[0] == HEAD) {
                HeadFlit * hf = new HeadFlit();
                hf->src_id = event->packet->srcNum;
                hf->dst_id = event->packet->destNum;
                hf->virtual_channel = event->packet->payload[1];
                hf->pkt_length = event->packet->sizeInFlits;
                //push the incoming flit to the router in buffer 
                router_in_buffer->push(hf->virtual_channel, hf);
            }
            else if(event->packet->payload[0] == BODY) {
                BodyFlit * bf = new BodyFlit();
                bf->src_id = event->packet->srcNum;
                bf->dst_id = event->packet->destNum;
                bf->virtual_channel = event->packet->payload[1];
                bf->pkt_length = event->packet->sizeInFlits;
                memcpy(bf->data, &event->packet->payload[2], PKT_SIZE*sizeof(int));
                //push the incoming flit to the router in buffer 
                router_in_buffer->push(bf->virtual_channel, bf);
            }
            else if(event->packet->payload[0] == TAIL) {
                TailFlit * tf = new TailFlit();
                tf->src_id = event->packet->srcNum;
                tf->dst_id = event->packet->destNum;
                tf->virtual_channel = event->packet->payload[1];
                tf->pkt_length = event->packet->sizeInFlits;
                tf->sent_time = event->packet->sending_time;
                stat_packets_in_from_router++;
                //push the incoming flit to the router in buffer 
                router_in_buffer->push(tf->virtual_channel, tf);
            }
            else
                assert(false && "event->packet->payload[0] invalid");
            
            delete event;
        }
    }

                
    //Pull entire packets from proc_in_buffer in a round-robin manner.
    bool found = false;
    //decide which vc to use under the FIFO principal
    const unsigned start_ch = last_inpkt_winner + 1;
    for ( uint i=0; i<no_vcs ; i++) {
        unsigned ch = start_ch + i;
	if(ch >= no_vcs) //wrap around
	    ch -= no_vcs;
	if(proc_in_buffer[ch].has_whole_packet())
	{
	    //a whole packet has been received.
            last_inpkt_winner= ch;
            found = true;
            break;
        }
    }

    //find the finished flit level pkt convert the filts to T and send it back to cache
    if ( found )
    {  
        //send a new packet to the terminal if received tail pkt.
        T* np = from_flit_level_packet(&proc_in_buffer[last_inpkt_winner]);
    	assert(proc_in_buffer[last_inpkt_winner].size() == 0);

//        send(DATATOTERMINAL, np); 
        terminal_link->send(np); 
        
	    stat_packets_out_to_terminal++;
          
#if 0            
        /*  Send the tail credit back. All other flit credits were sent
         *  as soon as flit was got in link-arrival */
        LinkData* ld =  new LinkData();
        ld->type = CREDIT;

        ld->src = this->id;
        ld->vc = last_inpkt_winner;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND CREDIT to router on vc " << ld->vc << " after delivering packet to terminal." << std::endl;
#endif
        //send credit for the tail flit to router
        send(DATAOUT,ld); 
#endif

    }



    //handle receiving flits: push flits coming in from router in buffer to the in buffer
    for ( uint i=0; i<no_vcs ; i++ )
    {
        Flit *ptr;
        bool more_flits = true;
        while( router_in_buffer->get_occupancy(i) > 0 && more_flits &&
               proc_in_buffer[i].has_whole_packet() == false)
        {
            if(proc_in_buffer[i].size() > 0) {//already has flits
                uint current_src = proc_in_buffer[i].src_id;
                Flit* ptr = router_in_buffer->pull(i, current_src);
                if(ptr) //exist flit with same src id
                    proc_in_buffer[i].add(ptr);
                else
                    more_flits = false;
            }
            else {
                ptr = router_in_buffer->pull(i);
                proc_in_buffer[i].add(ptr);
            }
        }
    }

    return false;
}



//! Take an input packet and create a FlitLevelPacket.
//! @param \c lw  The link bandwidth in bits
//! @param \c data  The packet from terminal
//! @param \c flp  The point of outbound proc_out_buffer
template<typename T>
void
irisNic<T>::to_flit_level_packet(FlitLevelPacket* flp, uint lw, T* data)
{
    assert(flp->size() == 0);

    /* find the number of flits */
    uint extra_flits = 2; //head and tail
    
    //calculate # of body flit needed to send a pkt
    uint no_bf = uint ((data->get_pack_size()*8)/lw);
    double compare =double (data->get_pack_size()*8)/ lw;
    
    if (compare > no_bf)
       no_bf ++;
    
    //generate the flit level packet
    uint tot_flits = extra_flits + no_bf;  
    flp->set_pkt_length(tot_flits);

    //convert to network address
    flp->dst_id = /*term_ni_mapping->terminal_to_net*/(data->get_dst());
    flp->src_id = m_id;
    
    //generate the header flit
    HeadFlit* hf = new HeadFlit();
    hf->pkt_length = tot_flits;
    hf->src_id = m_id;
    hf->dst_id = flp->dst_id;
    //hf->dst_component_id = 0;
    //hf->address = this->id;

    flp->add(hf);

    //generate all the body flits
    uint8_t* ptr = (uint8_t*)data->get_pack_data();
    uint8_t * tmpptr = ptr;
    
    //the flits count
    uint byte_count = 0;

    for ( uint i=0; i<no_bf; i++)
    {
        BodyFlit* bf = new BodyFlit();
        bf->type = BODY;
        bf->pkt_length = tot_flits;
        bf->src_id = m_id;
        bf->dst_id = flp->dst_id;

        flp->add(bf);
        
        for (uint j = 0; j < link_width/8; j++)
        {
            if (byte_count < (uint)data->get_pack_size())
            {
                bf->data[j] = *ptr;
                ptr++;
                byte_count++;
            }
        }

    }
    delete[] tmpptr;

    //generate tail flits
    TailFlit* tf = new TailFlit();
    tf->type = TAIL;
    tf->pkt_length = tot_flits;
    tf->src_id = m_id;
    tf->dst_id = flp->dst_id;
    //record down the time that the packet intered network
    tf->enter_network_time = getCurrentSimTime();
    flp->add(tf);

    assert( flp->has_whole_packet());
}

template<typename T>
void irisNic<T> :: print_stats(std::ostream& out)
{
    out << "Interface " << m_id << ":\n";
    out << "  Packets in from terminal: " << stat_packets_in_from_terminal << "\n";
    out << "  Packets out to terminal:  " << stat_packets_out_to_terminal << "\n";
    out << "  Packets in from router: " << stat_packets_in_from_router << "\n";
    out << "  Packets out to router:  " << stat_packets_out_to_router << "\n";
}


#endif // IRIS_NIC_H
