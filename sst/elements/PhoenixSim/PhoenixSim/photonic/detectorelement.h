/*
 * detectorelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef DETECTORELEMENT_H_
#define DETECTORELEMENT_H_

#include <vector>
#include "ringelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"
#include "thermalmodel.h"

class DetectorElement : public RingElement
{
	public:
		DetectorElement();
		virtual ~DetectorElement();
	protected:
		int portIdDetector;
		double photonicBitRate;
		StatObject* DynamicDetectorEnergyStat;
		StatObject* StaticDetectorEnergyStat;
		StatObject* StaticDetectorDummyEnergyStat;
		double temperature;

		StatObject* IL_prop;
		StatObject* IL_bend;
		StatObject* IL_cross;
		StatObject* IL_pass;
		StatObject* IL_drop;
		StatObject* IL_total;

		StatObject* PHY_SNR_EL;
		StatObject* PHY_SNR_OPT;
		StatObject* LaserNoisePowerStat;
		StatObject* InterMessageNoisePowerStat;
		StatObject* IntraMessageNoisePowerStat;
		StatObject* TotalNoisePowerStat;
		StatObject* TotalNoisePowerdBStat;
		StatObject* SignalPowerStat;
		StatObject* JohnsonNoisePowerStat;
		StatObject* ShotNoisePowerStat;
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		void InitializeDetectorElement();

	private:
		bool CheckAndHandleDetectorMessage(cMessage *msg);
		void HandleDetectorMessage(PhotonicMessage *pmsg, int indexIn, int indexOut);
		virtual double AddDetectionEnergyDissipation(double time) = 0;

};

#endif /* DETECTORELEMENT_H_ */
