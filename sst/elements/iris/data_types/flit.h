/*
 * =====================================================================================
// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 * =====================================================================================
 */

#ifndef  _FLIT_H_INC
#define  _FLIT_H_INC

#include	"../interfaces/genericHeader.h"

enum flit_type {UNK=0, HEAD, BODY, TAIL };

/*
 * *
 * =====================================================================================
 * * Class: Flit
 * * Description: This object defines a single flow control unit.
 * * FROM TEXT: Flow control is a synchronization protocol for transmitting and
 * * recieving a unit of information. This unit of flow control refers to that
 * * portion of the message whoose transfer must be syncronized. A packet is
 * * divided into flits and flow control between routers is at the flit level.
 * * It can be multicycle depending on the physical channel width. 
 * ( TODO:Implement Phits too!!)
 * *
 * =====================================================================================
 * */
class Flit
{
    public:
        Flit ();
        Flit (flit_type);
        ~Flit ();

        flit_type type;
        uint16_t virtual_channel;
        uint16_t pkt_length;
        std::string toString() const;

        // Its easier to debug if there is some unique way of identifying flits.
        // Addr in the HEAD flit can be used

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_NVP(type);
                ar & BOOST_SERIALIZATION_NVP(virtual_channel);
                ar & BOOST_SERIALIZATION_NVP(pkt_length);
            }

}; /*  ----- end of class Flit ----- */

/*
 * *
 * =====================================================================================
 * * Class: HeadFlit
 * * Description: All pkt stat variables are passed within the head flit for
 * * simulation purposes. Not passing it in tail as some pkt types may not need
 * * more than one flit.
 * *
 * =====================================================================================
 * */
class HeadFlit: public Flit
{
    public:
        HeadFlit ();
        ~HeadFlit ();
        uint16_t src_node_id;
        uint16_t dst_node_id;
        uint16_t dst_component_id;
        message_class_t mclass;
        uint64_t mem_address;

        void populate_head_flit(void);
        std::string toString() const;

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Flit);
                ar & BOOST_SERIALIZATION_NVP(src_node_id);
                ar & BOOST_SERIALIZATION_NVP(dst_node_id);
                ar & BOOST_SERIALIZATION_NVP(dst_component_id);
                ar & BOOST_SERIALIZATION_NVP(mclass);
                ar & BOOST_SERIALIZATION_NVP(mem_address);
            }

}; /*  ----- end of class HeadFlit ----- */

/*
 * *
 * =====================================================================================
 * * Class: TailFlit
 * * Description: flits of type TAIL. Has the pkt sent time.
 * *
 * =====================================================================================
 * */
class TailFlit : public Flit
{
    public:
        TailFlit ();
        ~TailFlit ();

        void populate_tail_flit();
        std::string toString() const;

        /*  stats */
        uint64_t packet_originated_time;
        uint64_t packet_arrival_time;

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Flit);
                ar & BOOST_SERIALIZATION_NVP(packet_originated_time);
                ar & BOOST_SERIALIZATION_NVP(packet_arrival_time);
            }

}; /*  ----- end of class TailFlit ----- */

/*
 * *
 * =====================================================================================
 * * Class: FlitLevelPacket
 * * Description: This is the desription of the lower link layer protocol
 * * class. It translates from the higher message protocol definition of the
 * * packet to flits. Phits are further handled for the physical layer within
 * * this definition.
 * *
 * =====================================================================================
 * */
class FlitLevelPacket
{
    public:
        FlitLevelPacket ();
        ~FlitLevelPacket ();

        uint16_t src_node_id;
        uint16_t dst_node_id;
        uint16_t dst_component_id;
        uint64_t address;
        message_class_t mc;
        uint16_t pkt_length;
        uint16_t virtual_channel;
        uint64_t packet_originated_time;
        uint64_t packet_arrival_time;



        void add ( Flit* ptr ); /*  Adds an incoming flit to the pkt. */
        Flit* get_next_flit(); /*  This will pop the flit from the queue as well. */
        uint16_t size(); /*  Return the length of the pkt in flits. */
        bool valid_packet(); /*  Checks the length of the pkt against the flits vector size. */
        std::string toString () const;

        std::deque<Flit*> flits;

    protected:

    private:

}; /*  ----- end of class FlitLevelPacket ----- */


#endif   /* ----- #ifndef _FLIT_H_INC  ----- */
