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

import electronicComponents.Parallelizer;
import electronicComponents.Serializer;


module SDRAM_DIMM {
 	parameters:
 	  
 	    int DRAM_chipsPerDIMM;
 	gates:
 		input cntrlIn;
 		input dataIn;
 		output dataOut;  
 		
 	submodules:
 		
 		chips[DRAM_chipsPerDIMM] : DRAM_Chip{
 		    
 		}	    
 		
 		de : demux {
 		}
 		
 		m : mux { 
 		}
 		
 		cde : demux {
 		}
 		
 		
 	connections:
 		
 		for i=0..DRAM_chipsPerDIMM-1 {
 		 	chips[i].dataIn <-- de.out[i];
 		 	chips[i].dataOut --> m.in[i];
 		 	
 		 	chips[i].cntrlIn <-- cde.out[i];
 		}
 		
 		dataOut <-- m.out;	
 		dataIn --> de.in;       
    
    	cntrlIn --> cde.in;
}


module DRAM_OCM {
 	parameters:
 	  
 	    int DRAM_chipsPerDIMM;
 	gates:
 		inout dataIn;
 		inout dataOut;  
 		
 	submodules:
 		
 		chips[DRAM_chipsPerDIMM] : DRAM_Chip{
 		    
 		}	    
 		
 		de : demux {
 		    parameters:
 		    	numOut = DRAM_chipsPerDIMM;
 		}
 		
 		m : mux { 
 		 	gates:
 		    	in[DRAM_chipsPerDIMM];
 		}
 		
 		cde : demux {
 		     parameters:
 		    	numOut = DRAM_chipsPerDIMM;
 		}
 		
 		tx : OCM_Transceiver {
 		    
 		}
 		
 		   
     
        
 		
 	connections allowunconnected:
 		
 		for i=0..DRAM_chipsPerDIMM-1 {
 		 	chips[i].dataIn <-- de.out[i];
 		 	chips[i].dataOut --> m.in[i];
 		 	
 		 	chips[i].cntrlIn <-- cde.out[i];
 		}
 		
 		tx.eldataIn <-- m.out;	
 		tx.eldataOut --> de.in;       
    
    	tx.cntrl --> cde.in;
    	
    	dataIn <--> tx.phdataIn;
    	dataOut <--> tx.phdataOut;
    	
    	
}


module DRAM_OCM_E {
 	parameters:
 	  
 	    int DRAM_chipsPerDIMM;
 	gates:
 		input dataIn;
 		output dataOut;  
 		
 	submodules:
 		
 		chips[DRAM_chipsPerDIMM] : DRAM_Chip{
 		    
 		}	    
 		
 		
 		tx : OCM_Transceiver_E {
 		    
 		}
 		
 			de : demux {
 		    parameters:
 		    	numOut = DRAM_chipsPerDIMM;
 		}
 		
 		m : mux { 
 		 	gates:
 		    	in[DRAM_chipsPerDIMM];
 		}
 		
 		cde : demux {
 		     parameters:
 		    	numOut = DRAM_chipsPerDIMM;
 		}
 		
 		
 	connections allowunconnected:
 		
 		for i=0..DRAM_chipsPerDIMM-1 {
 		 	chips[i].dataIn <-- de.out[i];
 		 	chips[i].dataOut --> m.in[i];
 		 	
 		 	chips[i].cntrlIn <-- cde.out[i];
 		}
 		
 		tx.dramdataIn <-- m.out;	
 		tx.dramdataOut --> de.in;       
    
    	tx.cntrl --> cde.in;
    	
    	dataIn --> tx.chipdataIn;
    	dataOut <-- tx.chipdataOut;
    	
    	
}
