// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __JOBKILLEVENT_H__
#define __JOBKILLEVENT_H__

namespace SST {
    namespace Scheduler {
        
            class JobKillEvent : public SST::Event{
                public:
                       
                           JobKillEvent( int jobNumber ) : SST::Event(){
                               this->jobNum= jobNumber;
                           }
                       
                           
                           JobKillEvent * copy(){
                               JobKillEvent * newKill = new JobKillEvent( jobNum);
                                   newKill->jobNum= jobNum;
                                   
                                   return newKill;
                           }
                       
                           
                           int jobNum;
                           
                private:
                        JobKillEvent();
                            
                            friend class boost::serialization::access;
                            template<class Archive>
                            void serialize( Archive & ar, const unsigned int version ){
                                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
                                    ar & BOOST_SERIALIZATION_NVP( jobNum);
                            }
            };
        
    }
}
#endif

