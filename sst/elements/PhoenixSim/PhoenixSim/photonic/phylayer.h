/* Physical Layer Library
 *
 * Ref: Saleh and Teich, Fund. of Photonics, 1st ed.
 */
#ifndef _PHYLAYER_H_
#define _PHYLAYER_H_

#include <iostream>
#include <string>
#include <math.h>

using namespace std;

namespace PhyLayer
{
	const double c = 2.9979e8;		// Speed of light in freespace (m/s)
	const double kB = 1.3807e-23;	// Boltzmann's constant (J/K)
	const double h = 6.6262e-34; 	// Planck's constant(J-s)
	const double e = 1.6022e-19;	// Electron charge(C)
	const double pi = 3.141592654; 	// Pi

	// Assumes conversion is done in terms of power dB=10*log10(x)
	// For voltage or current, make sure to account for 2x factor
	double dBtoLinear(double val);
	double LineartodB(double val);

	double AbsorptionCoefficient(double wavelength, string material);


	// Speed of light in the specified material
	double SpeedOfLight(string material);


	double Wavelength(double fs_wavelength, string material);


	double FrequencyToWavelength(double frequency, string material);

	// Energy of an electron
	double PhotonEnergy(double frequency);


	/* Refractive index of a material
	 * Valid material strings: "Si, Ge, [Al,Ga,In][P,As,Sb]
	 *
	 * Assumes T=300K, with photon energy near bandgap energy of the material
	 *
	 * Ref: Saleh and Teich, Table 15.2-1, Fund. of Photonics, 1st ed., pg 588.
	 */
	double RefractiveIndex(string material);


	// Thermal (or Johnson or Nyquist) noise in a resistor
	double ThermalNoiseVariance(double temperature, double bandwidth, double resistance);


	// Shot noise (photocurrent) noise variance
	double ShotNoiseVariance(double gain, double current, double bandwidth, double excessnoisefactor);


	double Photocurrent(double gain, double efficiency, double power, double frequency);


	// Frequency spacing of adjacent resonator modes
	// Ref: Saleh and Teich, Eq. 9.1-4, Fund. of Photonics, 1st ed., pg 313.
	double FreeSpectralRange(double distance, string material);


	// Finesse of a resonator
	// Ref: Saleh and Teich, Eq. 9.1-10, Fund. of Photonics, 1st ed., pg 316.
	double Finesse(double attenuationfactor);


	// Spectral response of the Fabry-Perot resonator
	double FabryPerotResonanceProfile(double freq, double imax, double finesse, double fsr);

}
#endif /* _PHYLAYER_H_ */
