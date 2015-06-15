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

/*
 * Classes representing system events
 */

#ifndef SST_SCHEDULER_MPIEVENT_H__
#define SST_SCHEDULER_MPIEVENT_H__

namespace SST {
    namespace Scheduler {

	enum MPITypes {START, FINISH};
        class MPIEvent : public SST::Event {
            public:

                MPIEvent(enum MPITypes type, int src_node) : SST::Event() {
		    this -> MPItype = type;
		    this -> src_node = src_node; 
               }

                MPIEvent(enum MPITypes type, unsigned long time, int jobNum, int num_msg, int expected_recv_count, int msg_size_in_bits, int src_node, int dest_node) : SST::Event() {
		    this -> MPItype = type;
                    this -> time = time;
                    this -> jobNum = jobNum;
		    this -> num_msg = num_msg;
		    this -> expected_recv_count = expected_recv_count;
                    this -> msg_size_in_bits = msg_size_in_bits;
                    this -> src_node = src_node;
		    this -> dest_node = dest_node;
                }

		enum MPITypes MPItype;
                unsigned long time;   //the length of the started job
                int jobNum;
		int num_msg;
		int expected_recv_count;
		int msg_size_in_bits;
		int src_node;
		int dest_node;

            private:

                MPIEvent() { }  // for serialization only

                friend class boost::serialization::access;
                template<class Archive>
                    void serialize(Archive & ar, const unsigned int version )
                    {
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                        ar & BOOST_SERIALIZATION_NVP(time);
                        ar & BOOST_SERIALIZATION_NVP(jobNum);
                    }
        };

    }
}
#endif

