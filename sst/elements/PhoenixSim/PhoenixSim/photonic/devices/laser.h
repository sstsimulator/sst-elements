#ifndef _LASER_H_
#define _LASER_H_

#include <omnetpp.h>

#include "sim_includes.h"
#include "messages_m.h"

#include "packetstat.h"


class Laser : public cSimpleModule
{
private:
    double Power_Laser;
    double Wavelength;
    double Efficiency;


public:
	Laser();
	virtual ~Laser();
	virtual void initialize ();
	virtual void handleMessage (cMessage *msg);
};

#endif /* _LASER_H_ */
