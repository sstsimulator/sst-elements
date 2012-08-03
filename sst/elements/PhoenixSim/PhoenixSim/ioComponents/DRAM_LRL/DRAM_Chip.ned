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

package ioComponents.DRAM_LRL;

import electronicComponents.mux;
import electronicComponents.demux;


module DRAM_Chip {
 	parameters:
 	    int DRAM_banksPerChip;
 	gates:
 		input cntrlIn;
 		input dataIn;
 		output dataOut;    
 		
 		
 	submodules:       
 		
 		banks[DRAM_banksPerChip] : DRAM_Bank{
 		    parameters:
 		        id = index;
 		}	    
 		
 		de : demux {
 		    parameters:
 		        numOut = DRAM_banksPerChip;
 		}
 		
 		m : mux { 
 		    gates:
 		        in[DRAM_banksPerChip];
 		}
 		
 		cde : demux {
 		      parameters:
 		        numOut = DRAM_banksPerChip;
 		}
 		
 	
 		
 		
 	connections:
 		
 		for i=0..DRAM_banksPerChip-1 {
 		 	banks[i].dataIn <-- de.out[i];
 		 	banks[i].dataOut --> m.in[i];
 		 	
 		 	banks[i].cntrlIn <-- cde.out[i];
 		}
 		
 		dataOut <-- m.out;	
 		dataIn --> de.in;
 		
 		cntrlIn --> cde.in;
    
}



