/*
 * ringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "multiringelement.h"

MultiRingElement::MultiRingElement()
{
}

MultiRingElement::~MultiRingElement()
{
}

void MultiRingElement::initialize()
{
	InitializeBasicElement();
	InitializeMultiRingElement();
	Setup();
}

void MultiRingElement::InitializeMultiRingElement()
{
	numOfRings = par("numOfRings");
	numOfRingSets = par("numOfRingSets");
	useRingModel = par("useRingModel").boolValue();

	char buf[10];
	string fullParamName, value;
	double wavelengthSpacing;

	filterBaseWavelength.resize(numOfRingSets);
	resonantWavelengths.resize(numOfRingSets);
	ringLength.resize(numOfRingSets);
	ringDiameter.resize(numOfRingSets);
	ringFinesse.resize(numOfRingSets);
	for(int i = 0 ; i < numOfRingSets ; i++)
	{
		std::ostringstream sin;
		sin << i;
		value = sin.str();


		fullParamName = string("filterBaseWavelength")+value;
		if(hasPar(fullParamName.c_str()))
		{
			filterBaseWavelength[i] = par(fullParamName.c_str());
		}
		else
		{
			throw cRuntimeError((string("The parameter '") + fullParamName + string("' is missing")).c_str());
		}

		fullParamName = string("filterWavelengthSpacing")+value;
		if(hasPar(fullParamName.c_str()))
		{
			wavelengthSpacing = par(fullParamName.c_str()).doubleValue();
		}
		else
		{
			throw cRuntimeError((string("The parameter '") + fullParamName + string("' is missing")).c_str());
		}

		fullParamName = string("numOfResonantWavelengths")+value;
		if(hasPar(fullParamName.c_str()))
		{
			for(int j = 0; j < par(fullParamName.c_str()).longValue() ; j++)
			{
				resonantWavelengths[i].insert(filterBaseWavelength[i] + j*wavelengthSpacing);
			}
		}
		else
		{
			throw cRuntimeError((string("The parameter '") + fullParamName + string("' is missing")).c_str());
		}



		fullParamName = string("ringDiameter")+value;
		if(hasPar(fullParamName.c_str()))
		{
			ringDiameter[i] = par(fullParamName.c_str()).doubleValue();
			ringLength[i] = PhyLayer::pi*(par(fullParamName.c_str()).doubleValue());
		}
		else
		{
			throw cRuntimeError((string("The parameter '") + fullParamName + string("' is missing")).c_str());
		}
		ringFinesse[i] = 10000;

	}


	thermalRingTuningPower = par("thermalRingTuningPower");
	thermalTemperatureDeviation = par("thermalTemperatureDeviation");

	E_thermal = Statistics::registerStat(GetName(), StatObject::ENERGY_STATIC, "photonic");
	E_thermal->track(thermalTemperatureDeviation*thermalRingTuningPower*numOfRings);

}


int MultiRingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	int ringSet;
	if(IsInResonance(ps->wavelength,ringSet))
	{
		return GetMultiRingRoutingTable(index, ringSet);
	}
	else
	{
		return Route(index);
	}

}

bool MultiRingElement::IsInResonance(double wavelength, int &ringSet)
{
	double n = PhyLayer::RefractiveIndex("Si");
	double K;
	int near1, near2;

	if(useRingModel)
	{
		throw cRuntimeError("Ring Model resonance calculation not implemented yet");
	}
	else
	{
		for(int i = 0 ; i < numOfRingSets ; i++)
		{
			if(resonantWavelengths[i].find(wavelength) != resonantWavelengths[i].end())
			{
				ringSet = i;
				return true;
			}
		}
	}
	return false;
}


void MultiRingElement::ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength)
{
	PacketStat *frontps = localps->next;
	BasicElement::ApplyInsertionLoss(localps, indexIn, indexOut, wavelength);
	frontps->passByRingLoss += GetPassByRingLoss(indexIn, indexOut, wavelength);
	frontps->dropIntoRingLoss += GetDropIntoRingLoss(indexIn, indexOut, wavelength);
}

double MultiRingElement::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double MultiRingElement::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double MultiRingElement::GetInsertionLoss(int indexIn, int indexOut, double wavelength)
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
