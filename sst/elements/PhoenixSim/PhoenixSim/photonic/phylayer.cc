/*
 * phylayer.cc
 *
 *  Created on: Mar 25, 2009
 *      Author: johnnie
 */

#include "phylayer.h"

using namespace PhyLayer;


double PhyLayer::dBtoLinear(double val)
{
	return pow(10,val/10);
}
double PhyLayer::LineartodB(double val)
{
	return 10*log10(val);
}

double PhyLayer::AbsorptionCoefficient(double wavelength, string material)
{

}


double PhyLayer::SpeedOfLight(string material)
{
	return c/RefractiveIndex(material);
}

double PhyLayer::Wavelength(double fs_wavelength, string material)
{
	return fs_wavelength/RefractiveIndex(material);
}

double PhyLayer::FrequencyToWavelength(double frequency, string material)
{
	return SpeedOfLight(material)/frequency;
}

double PhyLayer::PhotonEnergy(double frequency)
{
	return h*frequency;
}

double PhyLayer::Finesse(double attenuationfactor)
{
	return (pi*sqrt(attenuationfactor))/(1-attenuationfactor);
}

double PhyLayer::RefractiveIndex(string material)
{
	if(material == "Si")
	{
		return 3.43;
	}
	if(material == "SiGI")
	{
		return 4.4617; // group index based on Barrios, Optics Letters 2008, and Lee PTL 2008
	}
	if(material == "Ge")
	{
		return 4.0;
	}
	if(material == "AlP")
	{
		return 3.0;
	}
	if(material == "AlAs")
	{
		return 3.2;
	}
	if(material == "AlSb")
	{
		return 3.8;
	}
	if(material == "GaP")
	{
		return 3.3;
	}
	if(material == "GaAs")
	{
		return 3.6;
	}
	if(material == "GaSb")
	{
		return 4.0;
	}
	if(material == "InP")
	{
		return 3.5;
	}
	if(material == "InAs")
	{
		return 3.8;
	}
	if(material == "InSb")
	{
		return 4.2;
	}
	else
	{
		std::cout<<"Warning: Unknown material specified, using refractive index of 1."<<endl;
		return 1;
	}
}

double PhyLayer::ThermalNoiseVariance(double temperature, double bandwidth, double resistance)
// S&T: 17.5-26
{
	return 4*kB*temperature*bandwidth/resistance;
}


double PhyLayer::ShotNoiseVariance(double gain, double current, double bandwidth, double excessnoisefactor)
{
	return 2*e*gain*current*bandwidth*excessnoisefactor;
}


double PhyLayer::Photocurrent(double gain, double efficiency, double power, double frequency)
{
	return (gain*efficiency*e*power)/(h*frequency);
}


double PhyLayer::FreeSpectralRange(double distance, string material)
{
	return SpeedOfLight(material)/(2*distance);
}


double PhyLayer::FabryPerotResonanceProfile(double freq, double imax, double finesse, double fsr)
{
	return imax/(1+pow(2*finesse/pi,2)*pow(sin(pi*freq/fsr),2));
}
