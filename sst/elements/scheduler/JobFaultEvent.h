
#ifndef __JOBFAULTEVENT_H__
#define __JOBFAULTEVENT_H__

#include <string>

class JobFaultEvent : public SST::Event{
  public:

    JobFaultEvent( std::string faultType ) : SST::Event(){
      this->faultType = faultType;
      
      jobNum= -1;
      nodeNumber = -1;
      /* jobNumber and nodeNumber are assigned by the leaf node */
    }
    
    
    JobFaultEvent * copy(){
      JobFaultEvent * newFault = new JobFaultEvent( faultType );
      newFault->jobNum= jobNum;
      newFault->nodeNumber = nodeNumber;
      
      return newFault;
    }
    
    
    int jobNum;
    int nodeNumber;
    std::string faultType;

  private:
    JobFaultEvent();

    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive & ar, const unsigned int version ){
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
      ar & BOOST_SERIALIZATION_NVP( jobNum );
      ar & BOOST_SERIALIZATION_NVP( nodeNumber );
      ar & BOOST_SERIALIZATION_NVP( faultType );
    }
};

#endif

