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

package topologies.nonblockingSiliconNitride;


import topologies.DetectorBank;
import photonic.devices.Line;
//
// TODO auto-generated module
//
module SiNEjection
{
	parameters:
        int numLines;
        int detectorColumn;
        int length;
    gates:
        inout dataOut;
        
        inout portA[numLines];  // Direction of signal coming in to be detected
        inout portB[numLines];
    submodules:
        line[numLines-1] : Line
        {
            parameters:
                Length_Line = length;
        }
        DetBank: DetectorBank {
        }
    connections:
        for i = 0 .. numLines - 2
        {
            portA[i + (i>=detectorColumn?1:0)] <--> line[i].photonicIn;
            line[i].photonicOut <--> portB[i + (i>=detectorColumn?1:0)];
        }
        
        portA[detectorColumn] <--> DetBank.networkIn;
        DetBank.dontcareOut <--> portB[detectorColumn];
        
        dataOut <--> DetBank.electronicOut;
}
