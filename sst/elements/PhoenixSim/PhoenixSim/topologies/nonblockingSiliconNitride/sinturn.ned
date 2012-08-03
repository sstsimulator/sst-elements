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

import photonic.devices.Line;
import photonic.devices.Bend;
// Shape like this:
//   *****
//   *   *
module SiNTurn
{
    parameters:
        int numTurns;
        int firstLength;
        int firstHeight;
        int spaceA;
        int spaceB;
        int spaceSide;
        
        int disp_Width;
    gates:
        inout portA[numTurns];
        inout portB[numTurns];
    submodules:
        lineA[numTurns] : Line
        {
            parameters:
                Length_Line = firstHeight + index * spaceSide;
        }
        lineB[numTurns] : Line
        {
            parameters:
                Length_Line = firstHeight + index * spaceSide;
        }
        lineSide[numTurns] : Line
        {
            parameters:
                Length_Line = firstLength + index * (spaceA + spaceB);
        }
        
        bend[numTurns*2] : Bend
        {
            parameters:
                Angle_Bend = 90;
        }
    connections:
        for i = 0 .. numTurns - 1
        {
            portA[i] <--> lineA[i].photonicIn;
            lineA[i].photonicOut <--> bend[i].photonicIn;
            bend[i].photonicOut <--> lineSide[i].photonicIn;
            lineSide[i].photonicOut <--> bend[numTurns + i].photonicIn;
            bend[numTurns + i].photonicOut <--> lineB[i].photonicIn;
            lineB[i].photonicOut <--> portB[i];
        }
}
