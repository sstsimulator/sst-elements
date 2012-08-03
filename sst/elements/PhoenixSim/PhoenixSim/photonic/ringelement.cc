/*
 * ringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "ringelement.h"

RingElement::RingElement()
{
}

RingElement::~RingElement()
{
}

void RingElement::initialize()
{
	InitializeBasicElement();
	InitializeRingElement();
	Setup();
}

void RingElement::InitializeRingElement()
{
	numOfRings = par("numOfRings");
	if(hasPar("filterBaseWavelength"))
	{
		filterBaseWavelength = par("filterBaseWavelength");
		isBroadband = false;
	}
	else
	{
		isBroadband = true;
	}

	if(hasPar("numOfResonantWavelengths"))
	{
		for(int i = 0; i < par("numOfResonantWavelengths").longValue() ; i++)
		{
			resonantWavelengths.insert(filterBaseWavelength + i*(par("filterWavelengthSpacing").doubleValue()));
		}
	}
	else
	{
		resonantWavelengths.insert(filterBaseWavelength);
	}


	useRingModel = par("useRingModel").boolValue();

	ringDiameter = par("ringDiameter").doubleValue();

	if(useRingModel)
	{
		ringLength = PhyLayer::pi*(par("ringDiameter").doubleValue());
		ringFinesse = 20000*1/1550e-9;
	}

	routingTableOffResonance.resize(numOfPorts,-1);

	thermalRingTuningPower = par("thermalRingTuningPower");
	thermalTemperatureDeviation = par("thermalTemperatureDeviation");

	E_thermal = Statistics::registerStat(GetName(), StatObject::ENERGY_STATIC, "photonic");
	E_thermal->track(thermalTemperatureDeviation*thermalRingTuningPower*numOfRings);

	SetOffResonanceRoutingTable();

}


int RingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	if(IsInResonance(ps->wavelength))
	{
		return Route(index);
	}
	else
	{
		return RouteOffResonance(index);
	}
}

bool RingElement::IsInResonance(double wavelength)
{
	//double n = PhyLayer::RefractiveIndex("Si");
	double K, vf;
	int near1, near2;
	if(useRingModel)
	{
		//K = 2*n*ringLength/wavelength;
		vf = PhyLayer::SpeedOfLight("SiGI")/ringLength;
		K = wavelength/vf;



		near1 = (int)ceil(K);
		near2 = (int)floor(K);

		//if(fabs(diff1 - wavelength) < (diff1-diff2)/ringFinesse || fabs(diff2 - wavelength) < (diff1-diff2)/ringFinesse)
		if(fabs(K - near1) < .0001 || fabs(K - near2) < .0001)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return isBroadband || resonantWavelengths.find(wavelength) != resonantWavelengths.end();
	}
}

int RingElement::RouteOffResonance(int index)
{
	return routingTableOffResonance[index];
}

void RingElement::ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength)
{
	PacketStat *frontps = localps->next;
	BasicElement::ApplyInsertionLoss(localps, indexIn, indexOut, wavelength);
	frontps->passByRingLoss += GetPassByRingLoss(indexIn, indexOut, wavelength);
	frontps->dropIntoRingLoss += GetDropIntoRingLoss(indexIn, indexOut, wavelength);
}

double RingElement::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double RingElement::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double RingElement::GetInsertionLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	IL += GetPropagationLoss(indexIn, indexOut, wavelength);
	IL += GetCrossingLoss(indexIn, indexOut, wavelength);
	IL += GetBendingLoss(indexIn, indexOut, wavelength);
	IL += GetDropIntoRingLoss(indexIn, indexOut, wavelength);
	IL += GetPassByRingLoss(indexIn, indexOut, wavelength);
	IL += GetGain(indexIn, indexOut, wavelength);
	return IL;
}
