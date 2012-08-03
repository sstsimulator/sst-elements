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

//
// TODO auto-generated module
//
module SiNCrossing
{
    parameters:
        double switchSizeX;
        double switchSizeY;
        double SiN_CrosspointSizeX;
        double SiN_CrosspointSizeY;

	gates:
        inout horizontalIn[switchSizeY];
        inout horizontalOut[switchSizeY];
        inout verticalIn[switchSizeX];
        inout verticalOut[switchSizeX];
       
	submodules:
	    lineVertical[switchSizeX]: Line
	    {
	        parameters:
	            Length_Line = switchSizeY*SiN_CrosspointSizeY;
	    }
	    lineHorizontal[switchSizeY]: Line
	    {
	        parameters:
	            Length_Line = switchSizeX*SiN_CrosspointSizeX;
	    }
	    
	connections:
	    for i = 0..switchSizeX - 1
	    {
	        verticalIn[i] <--> lineVertical[i].photonicIn;
	        verticalOut[i] <--> lineVertical[i].photonicOut;
	    }
	    
	    for i = 0..switchSizeY - 1
	    {
	        horizontalIn[i] <--> lineHorizontal[i].photonicIn;
	        horizontalOut[i] <--> lineHorizontal[i].photonicOut;
	    }
	    
}
