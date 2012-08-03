#include "laser.h"

Define_Module(Laser);

Laser::Laser()
{
}

Laser::~Laser()
{
}

void Laser::initialize()
{
	Power_Laser = par("LaserPower");
	Wavelength = par("Wavelength");
    Efficiency = par("LaserEfficiency");

    PhotonicMessage* level = new PhotonicMessage();
    PacketStat *stat = new PacketStat();

	stat->signalPower = Power_Laser;
	stat->laserNoisePower =  0;
	stat->wavelength = Wavelength;
	stat->numWDMChannels = 1;
	stat->passByRingLoss = 0;
	stat->dropIntoRingLoss = 0;

	level->setPacketStat(long(stat));

	level->setMsgType(Llevel);

    send(level, "photonicOut");

}

void Laser::handleMessage(cMessage *msg)
{

}
