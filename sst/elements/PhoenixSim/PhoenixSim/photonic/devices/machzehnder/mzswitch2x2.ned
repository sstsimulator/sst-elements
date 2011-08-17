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

package photonic.devices.machzehnder;

//
// TODO auto-generated module
//
simple MZSwitch2x2
{
	parameters:

        double PropagationLoss;
        double PropagationDelay;
        
        int dispValueX;
        int dispValueY;
        int dispValueZ;
        
        string groupLabel = default("MZSwitch2x2");
        int numOfPorts = 4;
        int numOfStates = 2;
        //@display("i=components/pse1x2nx");
    gates:
        inout photonicInA;
        inout photonicInB;
        input fromRouter;
		inout photonicOutA;
		inout photonicOutB;
}
