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

package photonic.devices;

//
// TODO auto-generated module
//
simple Filter1x2
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
        
        double filterBaseWavelength = default(0);
        double filterWavelengthSpacing = default(1);
        int numOfResonantWavelengths = default(1);
        
        double RingThrough_ER_Filter1x2;
        double RingDrop_ER_Filter1x2;
        double ThroughDelay_Filter1x2;
        double DropDelay_Filter1x2;
        
		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

        int dispValueX;
        int dispValueY;
        int dispValueZ;
        double ringDiameter = default(3); // Assumes a 3 um circular ring
        bool useRingModel = default(false);
        int numOfRings = default(1);
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        
        double SizeWidth = default(60);
        double SizeHeight = default(60);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
        string groupLabel = default("Filter1x2");
        int numOfPorts = 4;
        @display("i=components/filter1x2");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;

        inout photonicHorizOut;
        inout photonicVertOut;
}
