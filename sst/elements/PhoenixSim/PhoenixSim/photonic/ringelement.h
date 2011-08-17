/*
 * ringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef RINGELEMENT_H_
#define RINGELEMENT_H_

#include <vector>
#include "basicelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"

class RingElement : public virtual BasicElement
{
	public:
		RingElement();
		virtual ~RingElement();
	protected:
		vector<int> routingTableOffResonance;
		double ringFSR;
		double filterBaseWavelength;
		double filterWavelengthSpacing;
		int  numOfResonantWavelengths;
		int numOfRings;
		bool isBroadband;
		bool useRingModel;
		double ringLength;
		double ringDiameter;
		double ringFinesse;

		double thermalRingTuningPower;
		double thermalTemperatureDeviation;
		set <double> resonantWavelengths;

		virtual void initialize();
		void InitializeRingElement();
		virtual int AccessRoutingTable(int index, PacketStat *ps);
		int RouteOffResonance(int index);
		virtual void SetRoutingTable() = 0;
		virtual void SetOffResonanceRoutingTable() = 0;
		bool IsInResonance(double wavelength);

		virtual double GetInsertionLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength = 0);

		StatObject* E_thermal;
};

#endif /* RINGELEMENT_H_ */
