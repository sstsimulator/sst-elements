/*
 * ringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef MULTIRINGELEMENT_H_
#define MULTIRINGELEMENT_H_

#include <vector>
#include "basicelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"
#include <sstream>

class MultiRingElement : public virtual BasicElement
{
	public:
		MultiRingElement();
		virtual ~MultiRingElement();
	protected:
		vector<double> ringFSR;
		vector<double> ringLength;
		vector<double> ringDiameter;
		vector<double> ringFinesse;
		vector<double> filterBaseWavelength;
		vector<double> filterWavelengthSpacing;
		vector<int>  numOfResonantWavelengths;

		int numOfRings;
		int numOfRingSets;

		bool useRingModel;

		double thermalRingTuningPower;
		double thermalTemperatureDeviation;
		vector< set <double> > resonantWavelengths;

		virtual void initialize();
		void InitializeMultiRingElement();
		virtual int AccessRoutingTable(int index, PacketStat *ps);
		virtual void SetRoutingTable() = 0;	// note that this is the off resonance case (unlike the other models)
		virtual int GetMultiRingRoutingTable(int index, int ringSet) = 0;
		bool IsInResonance(double wavelength, int &ringSet);

		virtual double GetInsertionLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength = 0);

		StatObject* E_thermal;
};

#endif /* MULTIRINGELEMENT_H_ */
