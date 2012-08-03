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

simple OCM_Control {
 	parameters:
 	    double DRAM_freq;
 	   
 	    int DRAM_arrays;
 	    
 	    int DRAM_chipsPerDIMM;
 	    
 	    double O_frequency_data;
 	    int numOfWavelengthChannels;
 	    
 	    int DRAM_tCAS;   //in clk cycles
		int DRAM_tCL;
		int DRAM_tRP;
		
		
		 
		 
 	gates:
     	input fromChip;
     	output toChip;
     	
     	input fromDRAM;
     	output toDRAM;
 	    output cntrl;
    
}
