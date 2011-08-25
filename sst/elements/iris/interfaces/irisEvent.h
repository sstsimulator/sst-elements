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


#ifndef  _IRISEVENT_H_INC
#define  _IRISEVENT_H_INC

#include <sst/core/event.h>
#include	"../iris/interfaces/genericHeader.h"


enum irisEvent_type {
    UNK_EVENT=0,
    CREDIT,
    FLIT,
    NEW_CREDIT,
    NEW_HEAD_FLIT_EVENT,
    NEW_TAIL_FLIT_EVENT,
    NEW_BODY_FLIT_EVENT
};

/*
 * =====================================================================================
 *        Class:  IrisEvent
 *  Description:  Inherits from the DES kernel event (see genericHeader for which 
 *  kernel is used). Used within Iris to pass flits and
 *  credits 
 * =====================================================================================
 */
class IrisEvent: public DES_Event
{
    public:
        IrisEvent (): DES_Event(), etype(UNK_EVENT) {}
        IrisEvent (irisEvent_type type): DES_Event(), etype(type) {}
        ~IrisEvent () {}

        irisEvent_type etype;
        uint32_t src_node_id;
        uint32_t dst_node_id;
        uint16_t channel;
        uint16_t pkt_length;

        std::string toString() const
        {
            std::stringstream str;
            str 
                << " etype: " << etype
                << " src_node_id: " << src_node_id
                << " dst_node_id: " << dst_node_id
                << " channel: " << channel
                << " pkt_length: " << pkt_length
                << "\n";

            return str.str();
        }

    private:
        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                ar & BOOST_SERIALIZATION_NVP(etype);
                ar & BOOST_SERIALIZATION_NVP(src_node_id);
                ar & BOOST_SERIALIZATION_NVP(dst_node_id);
                ar & BOOST_SERIALIZATION_NVP(channel);
                ar & BOOST_SERIALIZATION_NVP(pkt_length);
            }
}; /* -----  end of class IrisEvent  ----- */

class IrisNewHeadFlitEvent : public IrisEvent
{
    public:
        IrisNewHeadFlitEvent (): IrisEvent(NEW_HEAD_FLIT_EVENT) {}
        ~IrisNewHeadFlitEvent() {}

        uint16_t dst_component_id;
        message_class_t mclass;
        uint64_t mem_address;

        std::string toString() const
        {
            std::stringstream str;
            str << " HEAD flit: "
                << " dst_component_id: " << dst_component_id
                << " mclass: " << mclass
                << " mem_address: " << std::hex << mem_address << std::dec;
            str << static_cast<const IrisEvent*>(this)->toString();
                                  ;
            return str.str();
        }

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IrisEvent);
                ar & BOOST_SERIALIZATION_NVP(dst_component_id);
                ar & BOOST_SERIALIZATION_NVP(mclass);
                ar & BOOST_SERIALIZATION_NVP(mem_address);
            }

}; /* -----  end of class IrisNewHeadFlitEvent  ----- */


class IrisNewBodyFlitEvent : public IrisEvent
{
    public:
        IrisNewBodyFlitEvent (): IrisEvent(NEW_BODY_FLIT_EVENT) {}
        ~IrisNewBodyFlitEvent() {}

        std::string toString() const
        {
            std::stringstream str;
            str << " BODY flit: ";
            str << static_cast<const IrisEvent*>(this)->toString();

            return str.str();
        }

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IrisEvent);
            }

};  /* -----  end of class IrisNewBodyFlitEvent  ----- */

class IrisNewTailFlitEvent : public IrisEvent
{
    public:
        IrisNewTailFlitEvent (): IrisEvent(NEW_TAIL_FLIT_EVENT) {}
        ~IrisNewTailFlitEvent() {}

        //  stats 
        uint64_t packet_originated_time;
        uint64_t packet_arrival_time;

        std::string toString() const
        {
            std::stringstream str;
            str << " TAILflit: "
                << " packet_originated_time: " << packet_originated_time
                << " packet_arrival_time: " << packet_arrival_time
                ;
            str << static_cast<const IrisEvent*>(this)->toString();

            return str.str();
        }

    private:
        friend class boost::serialization::access;
        template< class Archive>
            void
            serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IrisEvent);
                ar & BOOST_SERIALIZATION_NVP(packet_originated_time);
                ar & BOOST_SERIALIZATION_NVP(packet_arrival_time);

            }

}; /* -----  end of class IrisNewFlitEvent  ----- */


#endif   /* ----- #ifndef _IRISEVENT_H_INC  ----- */
