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

#ifndef  _IRIS_COMPONENT_H_INC
#define  _IRIS_COMPONENT_H_INC

#include	<sst/core/event.h>
#include	<sst/core/sst_types.h>
#include	<sst/core/component.h>
#include	<sst/core/link.h>
#include	<sst/core/timeConverter.h>
#include	"../interfaces/genericHeader.h"

/*
 * =====================================================================================
 *        Class:  Iris
 *  Description: This is a wrapper for the main within the original IRIS. 
 *  Main Tasks:
 *  1) Construct the nodes and assign node ids
 *  2) Pass configuration parameters to nodes
 *  3) Control Simulation start and end
 * =====================================================================================
 */

class Iris
{
    public:
        Iris ();
        ~Iris ();


    protected:

    private:


}; /* -----  end of class Iris  ----- */

enum irisEvent_type {
    UNK_EVENT=0,
    NEW_CREDIT,
    NEW_FLIT,
};

/*
 * =====================================================================================
 *        Class:  IrisEvent
 *  Description:  Inherits from SST::Event. Used within Iris to pass flits and
 *  credits 
 * =====================================================================================
 */
class IrisEvent: pusblic SST::Event
{
    public:
        IrisEvent (): SST::Event() {}
        ~IrisEvent ();

        irisEvent_type etype;
        uint32_t src_node_id;
        uint32_t dst_node_id;
        uint16_t channel;

    private:
        friend class boost::serialization::acess;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                ar & BOOST_SERIALIZATION_NVP(etype);
                ar & BOOST_SERIALIZATION_NVP(src_node_id);
                ar & BOOST_SERIALIZATION_NVP(dst_node_id);
                ar & BOOST_SERIALIZATION_NVP(channel);
            }
}; /* -----  end of class IrisEvent  ----- */

class IrisEvent: pusblic SST::Event
{
    public:
        IrisEvent (): SST::Event() {}
        ~IrisEvent ();

        irisEvent_type etype;
        uint32_t src_node_id;
        uint32_t dst_node_id;
        uint16_t channel;

    private:
        friend class boost::serialization::acess;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                ar & BOOST_SERIALIZATION_NVP(etype);
                ar & BOOST_SERIALIZATION_NVP(src_node_id);
                ar & BOOST_SERIALIZATION_NVP(dst_node_id);
                ar & BOOST_SERIALIZATION_NVP(channel);
            }
}; /* -----  end of class IrisEvent  ----- */


class IrisNewFlitEvent : public IrisEvent
{
    public:
        IrisNewFlitEvent (): IrisEvent() {}
        ~IrisNewFlitEvent()
        {
            flit = NULL;
        }

        Flit flit = NULL;
    private:
        friend class boost::serialization::acess;
        template< class Archive>
            void
            Serialize( Archive &ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IrisEvent);
                switch ( flit->type ) {
                    case HEAD:	
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(HeadFlit);
                        break;

                    case BODY:	
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Flit);
                        break;

                    case TAIL:	
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(TailFlit);
                        break;

                    default:	
                        _abort(" Flit type unknown\n");
                        break;
                }				/* -----  end switch  ----- */
            }

}; /* -----  end of class IrisNewFlitEvent  ----- */

#endif   /* ----- #ifndef _IRIS_COMPONENT_H_INC  ----- */
