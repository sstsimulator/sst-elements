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

import topologies.ModulatorBank;
import photonic.devices.Line;
//
// TODO auto-generated module
//
module SiNInjection
{
	parameters:
        int numLines;
        int modulatorRow;
        int length;
    gates:
        inout dataIn;
        
        inout portA[numLines];
        inout portB[numLines]; // Direction of Modulator Output
    submodules:
        line[numLines - 1] : Line
        {
            parameters:
                Length_Line = length;
        }
		ModBank: ModulatorBank {
        }
    connections:
        for i = 0 .. numLines - 2
        {
            portA[i + (i>=modulatorRow?1:0)] <--> line[i].photonicIn;
            line[i].photonicOut <--> portB[i + (i>=modulatorRow?1:0)];
        }
        
        portA[modulatorRow] <--> ModBank.sourceIn;
        ModBank.networkOut <--> portB[modulatorRow];
        
        dataIn <--> ModBank.electronicIn;
        
}
