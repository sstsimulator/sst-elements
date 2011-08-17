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
simple PSE1x2NXMultiRing
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
        

        bool useRingModel = default(false);

        double thermalRingTuningPower;
        double thermalTemperatureDeviation;


        double ringDiameter = default(50);
        double ringDiameter0 = default(ringDiameter); 
        double ringDiameter1 = default(ringDiameter); 
        double ringDiameter2 = default(ringDiameter);
        double ringDiameter3 = default(ringDiameter); 
        double ringDiameter4 = default(ringDiameter); 
        double ringDiameter5 = default(ringDiameter);
        double ringDiameter6 = default(ringDiameter); 
        double ringDiameter7 = default(ringDiameter); 
        double ringDiameter8 = default(ringDiameter);
        double ringDiameter9 = default(ringDiameter); 

		double filterBaseWavelength0;
        int numOfResonantWavelengths0;
        double filterWavelengthSpacing0;        
		double filterBaseWavelength1;
        int numOfResonantWavelengths1;
        double filterWavelengthSpacing1;
		double filterBaseWavelength2;
        int numOfResonantWavelengths2;
        double filterWavelengthSpacing2;
		double filterBaseWavelength3;
        int numOfResonantWavelengths3;
        double filterWavelengthSpacing3;        
		double filterBaseWavelength4;
        int numOfResonantWavelengths4;
        double filterWavelengthSpacing4;
		double filterBaseWavelength5;
        int numOfResonantWavelengths5;
        double filterWavelengthSpacing5;
		double filterBaseWavelength6;
        int numOfResonantWavelengths6;
        double filterWavelengthSpacing6;        
		double filterBaseWavelength7;
        int numOfResonantWavelengths7;
        double filterWavelengthSpacing7;
		double filterBaseWavelength8;
        int numOfResonantWavelengths8;
        double filterWavelengthSpacing8;
		double filterBaseWavelength9;
        int numOfResonantWavelengths9;
        double filterWavelengthSpacing9;  
        
        int numOfRingSets = default(3);
        int numOfRings = numOfRingSets;
        string groupLabel = default("PSE1x2NXMultiRing");
        int numOfPorts = 4;
        int numOfStates = pow(2,numOfRingSets);
        @display("i=components/pse1x2nxr3");
    gates:
        inout photonicInA;
        inout photonicInB;
        input fromRouter;
		inout photonicOutA;
		inout photonicOutB;
}
