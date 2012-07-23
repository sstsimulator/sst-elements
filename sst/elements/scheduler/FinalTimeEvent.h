// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                               

/*
 * Class that tells the scheduler there are no more events at this time
 * (otherwise the scheduler handles each event individually when they come,
 * even if they are at the same time) 
 * Has high priority value so that SST puts it at the end of its event queue
 */

#ifndef __FINALTIMEEVENT_H__
#define __FINALTIMEEVENT_H__

class FinalTimeEvent : public SST::Event {
 public:
  FinalTimeEvent() : SST::Event() {
    setPriority(98); // one less than the event that says everything is done
    //(we want this to be after everything except that)
  }

  friend class boost::serialization::access;
  template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
    }
};

#endif

