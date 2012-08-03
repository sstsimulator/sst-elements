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
simple PSE1x2NX
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
        
        
        double ringDiameter = default(50); // Assumes a 50 um circular ring
        bool useRingModel = default(false);
		int numOfRings = default(1);
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        
        string groupLabel = default("PSE1x2NX");
        int numOfPorts = 4;
        int numOfStates = 2;
        
        double SizeWidth = default(60);
        double SizeHeight = default(60);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("i=components/pse1x2nx");
    gates:
        inout photonicInA;
        inout photonicInB;
        input fromRouter;
		inout photonicOutA;
		inout photonicOutB;
}
