
#ifndef __JOBKILLEVENT_H__
#define __JOBKILLEVENT_H__

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

#endif

