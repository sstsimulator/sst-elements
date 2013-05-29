#include "packetstat.h"

using namespace std;

PacketStat::PacketStat()
{
	usedPhotonic = false;
	isNoise = false;

	propagationLoss = 0;
	crossingLoss = 0;
	passByRingLoss = 0;
	dropIntoRingLoss = 0;
	bendingLoss = 0;

	next = NULL;
	prev = NULL;

	currInsertionLoss = 0;

	laserNoisePower = 0;
	messageNoisePower = 0;
}

PacketStat::PacketStat(PacketStat *copy)
{

	usedPhotonic = copy->usedPhotonic;
	isNoise = copy->isNoise;

	propagationLoss = copy->propagationLoss;
	crossingLoss = copy->crossingLoss;
	passByRingLoss = copy->passByRingLoss;
	dropIntoRingLoss = copy->dropIntoRingLoss;
	bendingLoss = copy->bendingLoss;

	wavelength = copy->wavelength;

	numWDMChannels = copy->numWDMChannels;
	signalPower = copy->signalPower;

	next = copy->next;
	prev = copy->prev;

	currInsertionLoss = copy->currInsertionLoss;

	timestamp = copy->timestamp;

	laserNoisePower = copy->laserNoisePower;
	messageNoisePower = copy->messageNoisePower;
	wavelengthNoisePower = copy->wavelengthNoisePower;


}

PacketStat::~PacketStat()
{
}

void PacketStat::addWavelengthNoisePower(double wavelength, double value)
{
	map<double,double>::iterator it;

	it = wavelengthNoisePower.find(wavelength);

	if(it != wavelengthNoisePower.end())
	{
		// found
		it->second += value;
	}
	else
	{
		// not found, new noise type
		wavelengthNoisePower.insert(pair<double,double>(wavelength,value));
	}
}

bool PacketStat::resetWavelengthNoisePower(double wavelengh)
{
	map<double,double>::iterator it;

	it = wavelengthNoisePower.find(wavelength);

	if(it != wavelengthNoisePower.end())
	{
		wavelengthNoisePower.erase(it);
	}

	return false; // Added to avoid Compile Warning
}

double PacketStat::getWavelengthNoisePower(double wavelength)
{
	map<double,double>::iterator it;

	it = wavelengthNoisePower.find(wavelength);

	if(it != wavelengthNoisePower.end())
	{
		// found
		return it->second;
	}
	else
	{
		return 0;
	}
}

double PacketStat::getTotalWavelengthNoisePower()
{
	map<double,double>::iterator it;
	double totalNoisePower = 0;
	for(it = wavelengthNoisePower.begin(); it != wavelengthNoisePower.end(); it++)
	{
		totalNoisePower += it->second;
	}
	return totalNoisePower;
}

void PacketStat::setTimestamp(string name, simtime_t time)
{
	timestamp[name] = time;
}


