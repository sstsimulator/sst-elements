//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package photonic.devices.multiring;

//
// TODO auto-generated module
//
simple PSE1x2NXR3
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
        
        double RingOn_ER_1x2NX;
        double RingOff_ER_1x2NX;
        double ThroughDelay_1x2NX;
		double DropDelay_1x2NX;
		
		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

        int dispValueX;
        int dispValueY;
        int dispValueZ;
        
        
        double ringDiameter0 = default(17); // Assumes a 50 um circular ring
        double ringDiameter1 = default(17); // Assumes a 50 um circular ring
        double ringDiameter2 = default(17); // Assumes a 50 um circular ring
        
        bool useRingModel = default(false);
        int numOfRingSets = 3;
		int numOfRings = default(3);
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
		double filterBaseWavelength0;
        int numOfResonantWavelengths0;
        double filterWavelengthSpacing0;        
		double filterBaseWavelength1;
        int numOfResonantWavelengths1;
        double filterWavelengthSpacing1;
		double filterBaseWavelength2;
        int numOfResonantWavelengths2;
        double filterWavelengthSpacing2;
        
        string groupLabel = default("PSE1x2NXR3");
        int numOfPorts = 4;
        int numOfStates = 8;
        @display("i=components/pse1x2nxr3");
    gates:
        inout photonicInA;
        inout photonicInB;
        input fromRouter;
		inout photonicOutA;
		inout photonicOutB;
}
