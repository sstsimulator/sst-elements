/*
 * modulatorelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef MODULATORELEMENT_H_
#define MODULATORELEMENT_H_

#include <vector>
#include "ringelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"
#include "thermalmodel.h"

class ModulatorElement : public RingElement
{
	public:
		ModulatorElement();
		virtual ~ModulatorElement();
	protected:
		int portIdModulator;
		PacketStat *currPacketStat;
		StatObject* DynamicModulatorEnergyStat;

        double NoiseBandwidth;
        double ModulatorExtinctionRatio;
		double LaserRelativeIntensityNoise;

		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		void InitializeModulatorElement();
		virtual double GetModulatorLevel() = 0;
		virtual double GetModulatorWavelength() = 0;
		virtual void HandleModulatorMessage(PhotonicMessage *pmsg, int indexIn);


	private:
		bool CheckAndHandleModulatorMessage(cMessage *msg);
		virtual double AddModulationEnergyDissipation(double time) = 0;
};

#endif /* MODULATORELEMENT_H_ */
