#ifndef  IRIS_NIC_FLIT_H
#define  IRIS_NIC_FLIT_H

#include <sst/elements/iris/SST_interface.h>

enum flit_type {UNK, HEAD, BODY, TAIL };


/*
 * =====================================================================================
 *        Class:  Flit
 *  Description:  This object defines a single flow control unit. 
 *  FROM TEXT: Flow control is a synchronization protocol for transmitting and
 *  recieving a unit of information. This unit of flow control refers to that
 *  portion of the message whoose transfer must be syncronized. A packet is
 *  divided into flits and flow control between routers is at the flit level. It
 *  can be multicycle depending on the physical channel width.
 * =====================================================================================
 */
// Do not create virtual functions as flit might be sent across LP boundaries.
class Flit
{
    public:
        Flit ();
        ~Flit ();

        uint src_id; //id of src and dst of network interface.
        uint dst_id;

        flit_type type;
        uint virtual_channel;
        uint pkt_length;
        std::string toString() const;

    protected:

    private:

}; /* -----  end of class Flit  ----- */

/*
 * =====================================================================================
 *        Class:  HeadFlit
 *  Description:  All pkt stat variables are passed within the head flit for
 *  simulation purposes. Not passing it in tail as some pkt types may not need
 *  more than one flit. 
 * =====================================================================================
 */
class HeadFlit: public Flit
{
    public:
        HeadFlit ();  
        ~HeadFlit ();  

        void populate_head_flit(void);
        std::string toString() const;

    protected:

    private:

}; /* -----  end of class HeadFlit  ----- */

/*
 * =====================================================================================
 *        Class:  BodyFlit
 *  Description:  flits of type body 
 * =====================================================================================
 */
class BodyFlit : public Flit
{
    public:
        
        BodyFlit (); 
        ~BodyFlit (); 

        std::string toString() const;
        void populate_body_flit();
        
        uint8_t data[PKT_SIZE*sizeof(int)]; //payload data


    protected:

    private:

}; /* -----  end of class BodyFlit  ----- */

/*
 * =====================================================================================
 *        Class:  TailFlit
 *  Description:  flits of type TAIL. Has the pkt sent time. 
 * =====================================================================================
 */
class TailFlit : public Flit
{
    public:
        TailFlit ();
        ~TailFlit ();

        void populate_tail_flit();
        u_int64_t sent_time;
        u_int64_t enter_network_time;
        std::string toString() const;

    protected:

    private:

}; /* -----  end of class TailFlit  ----- */

/*
 * =====================================================================================
 *        Class:  FlitLevelPacket
 *  Description:  This is the desription of the lower link layer protocol
 *    class. It translates from the higher message protocol definition of the
 *    packet to flits. Phits are further handled for the physical layer within
 *    this definition. 
 * =====================================================================================
 */

class InvalidAddException {};

class FlitLevelPacket
{
    public:
        FlitLevelPacket ();
        ~FlitLevelPacket ();

    	unsigned get_pkt_length() { return pkt_length; }
	    void set_pkt_length(unsigned len);

        uint src_id; //src and dst interface id; only useful for debugging
        uint dst_id;
        //uint dst_component_id;
        //u_int64_t address;
        //message_class mc;
        uint virtual_channel;
        uint64_t enter_network_time;

        void add ( Flit* ptr);  /* Appends an incoming flit to the pkt. */
        Flit* pop_next_flit();  /* This will pop the flit from the queue as well. */
        unsigned size();            /* Return the length of the pkt in flits. */
        bool has_whole_packet();    //true if it contains all flits of a single packet.
        std::string toString () const;

        std::deque<Flit*> flits;
        uint pkt_length; //number of flits in the packet

}; /* -----  end of class FlitLevelPacket  ----- */


#endif   //IRIS_NIC_FLIT_H

