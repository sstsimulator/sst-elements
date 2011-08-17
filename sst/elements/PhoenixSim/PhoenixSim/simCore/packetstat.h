#ifndef _PACKETSTAT_H_
#define _PACKETSTAT_H_

#include <omnetpp.h>
#include <map>
#include <string>

class PacketStat
{
	public:
		PacketStat();
		PacketStat(PacketStat*);
		~PacketStat();


		double wavelength;
		int numWDMChannels;
		double signalPower;

		double propagationLoss;
		double crossingLoss;
		double passByRingLoss;
		double dropIntoRingLoss;
		double bendingLoss;

		//double crossTalkPower;

		bool isNoise;
		bool usedPhotonic;

		// used to track chain
		PacketStat* next;
		PacketStat* prev;
		double currInsertionLoss;

		//simtime_t timeCreated;
		//simtime_t timeStart;
		//simtime_t timeStartPhotonic;
		//simtime_t timeEnd;
		//simtime_t timeEndPhotonic;

		void setTimestamp(std::string name, simtime_t time);
		void addWavelengthNoisePower(double wavelength, double value);
		bool resetWavelengthNoisePower(double wavelength);
		double getWavelengthNoisePower(double wavelength);
		double getTotalWavelengthNoisePower();
		std::map <std::string, simtime_t> timestamp;

		double laserNoisePower;		// intensity noise at laser
		double messageNoisePower;	// in band noise
		std::map <double, double> wavelengthNoisePower; // out of band noise

		//double length;
};

#endif
