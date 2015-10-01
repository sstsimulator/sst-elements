/*
 * =====================================================================================
// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 * =====================================================================================
 */

#ifndef  SST_INTERFACE_H_INC
#define  SST_INTERFACE_H_INC

#include        <sst_stdint.h>
#include	"genericHeader.h"
#include	<sst/core/event.h>

namespace SST {
namespace Iris {

const uint32_t HDR_SIZE =  8/sizeof(int);
const uint32_t PKT_SIZE =  64/sizeof(int);
/*
 * =====================================================================================
 *        Class:  irisNPkt
 *  Description:  Piggybacks the additional parameters need by iris to the
 *  existing networkPacket in SST
 * =====================================================================================
 */

class irisNPkt
{
    public:
        irisNPkt (){}
        ~irisNPkt(){}

        /*  Use this if you need to configure multiple virtual networks */
        message_class_t mclass;
        int destNum;
        int srcNum;
        uint sizeInFlits;
        int vc;
        int link;
        uint64_t sending_time;

	// adding for macsim.. need to add to serialize if reqd
	void* req;   
	uint64_t address;

        uint32_t payload[HDR_SIZE+PKT_SIZE];

    private:
        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize(Archive & ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_NVP(destNum);
                ar & BOOST_SERIALIZATION_NVP(srcNum);
                ar & BOOST_SERIALIZATION_NVP(sizeInFlits);
                ar & BOOST_SERIALIZATION_NVP(vc);
                ar & BOOST_SERIALIZATION_NVP(link);
                ar & BOOST_SERIALIZATION_NVP(sending_time);
                ar & BOOST_SERIALIZATION_NVP(payload);
            }

}; /* -----  end of class irisNPkt  ----- */

/*
 * =====================================================================================
 *        Class:  RtrEvent in ../SS_router/SS_router
 *  Description:  Its the same as T 
 * =====================================================================================
 */
class irisRtrEvent : public SST::Event {
    public:
        ~irisRtrEvent()
        {
            // Do not delete the pkt with the event
            // Its explicitly deleted by the sender after
            // verification of the response
        }

        typedef enum { Credit, Packet } msgType_t;

        int             type;
        char            mesh_loc[3];
        irisNPkt*        packet;
        struct Credit {
            uint32_t        num;
            int             vc;

            friend class boost::serialization::access;
            template<class Archive>
                void
                serialize(Archive & ar, const unsigned int version )
                {
                    ar & BOOST_SERIALIZATION_NVP(num);
                    ar & BOOST_SERIALIZATION_NVP(vc);
                }
        } credit;

    private:
        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize(Archive & ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                ar & BOOST_SERIALIZATION_NVP(type);
                ar & BOOST_SERIALIZATION_NVP(credit);
                ar & BOOST_SERIALIZATION_NVP(packet);
            }
};

}
}

#endif   /* ----- #ifndef SST_INTERFACE_H_INC  ----- */
