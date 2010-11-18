#ifndef _SCHEDEVENT_H
#define _SCHEDEVENT_H

//#include <sst/core/compEvent.h>
#include <string>

class ComppEvent : public SST::Event {
  public: ComppEvent() : Event() { /*std::cout << "New compevent " << this << '\n'; */}
          ~ComppEvent() {}
};

class failEvent : public SST::Event {
public:
    failEvent(std::string c, int t) : comp(c), tick(t){ /*std::cout << "New failevent!\n";*/ }
    std::string comp;  
		int tick;						//in number of cycles for a job

private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<Event>(*this);
        ar & BOOST_SERIALIZATION_NVP(comp);
				ar & BOOST_SERIALIZATION_NVP(tick);
    }
}; 

class finishEvent : public SST::Event {
public:
		finishEvent(int j) : jobid(j) { /*std::cout << "New finnish(lol) event!\n";*/ }

		int jobid;  //job id of the job which is finishing


private: 

		friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<Event>(*this);
        ar & BOOST_SERIALIZATION_NVP(jobid);
    }
};


/*class endEvent : public SST::Event {
public:
		endEvent(int j, int d) : jobid(j), duration(d) {}
		virtual void cause_action()=0;
		virtual ~endEvent(){}
protected:
    int jobid, duration;

};


class killEvent : public endEvent{
public:
		killEvent(int j, int d) : endEvent(j, d) { }
		virtual void cause_action(schedule *c);

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<CompEvent>(*this);
        ar & BOOST_SERIALIZATION_NVP(duration);
    }

};
    
*/

#endif


/*----------------------------------------------------------------------------
class my_evt_t: public SST::Event {
	virtual void do_shit(my_component *c)=0;
	virtual ~my_evt_t(){}
}

class killeventthing_evt_t{
public:
	taskid
	cycles
	virtual void do_shit(my_component *c){
		c->killeverything();
	}	


in code:

event_handler (event *e){
	(static_cast <my_evt_t *> (e))->do_shit(this);
}

delete e;*/
