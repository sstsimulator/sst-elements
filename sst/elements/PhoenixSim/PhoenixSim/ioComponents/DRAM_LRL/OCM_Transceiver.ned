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

import photonic.devices.Facet;
import photonic.devices.Detector;
import photonic.devices.Modulator;

import electronicComponents.Serializer;
import electronicComponents.Parallelizer;
import electronicComponents.mux;
import electronicComponents.demux;

import chipComponents.InoutConverter;

import topologies.ModulatorBank;
import topologies.DetectorBank;

import ioComponents.DRAM_LRL.PhotonicToProcessorMsg;


module OCM_Transceiver {
 	parameters:
 	   double IL_Facet_Lateral;
 	
 	gates:
 	    inout phdataIn;  //photonic signals
 	    inout phdataOut;   
 	    
 	    input eldataIn;
 	    output eldataOut;
 	    output cntrl;
 	    
 	    inout source;
 	    
 	    
 	submodules:
 	    oc : OCM_Control {
 	        
 	    }
 	    
 	    procStrip : PhotonicToProcessorMsg {
 	        
 	    }
 	    
 	    ModBank : ModulatorBank {
 	        
 	    }
 	    
 	    DetBank : DetectorBank {
 	        
 	    }
        
        fIn: Facet {
            IL_Facet = IL_Facet_Lateral;
            @display("p=215,244");
        }
        fOut: Facet {
             IL_Facet = IL_Facet_Lateral;
            @display("p=215,88");
        }
 	    
 	    
 	
 	    
 	connections allowunconnected:
 	    
 	    phdataIn <--> fIn.photonicIn;
 	    phdataOut <--> fOut.photonicOut;
 	    
 	    
 	    fIn.photonicOut <--> DetBank.networkIn;
 	    fOut.photonicIn <--> ModBank.networkOut;
 	    
 	    oc.toChip --> procStrip.procIn;
 	    procStrip.photonicOut <--> ModBank.electronicIn;
 	   
      	
      	procStrip.photonicIn <--> DetBank.electronicOut;
      	procStrip.procOut --> oc.fromChip;
      	
      	oc.fromDRAM <-- eldataIn;
      	oc.toDRAM --> eldataOut;
      	oc.cntrl --> cntrl;
}


module OCM_Transceiver_E {
 	parameters:
 	  
 		double clockRate_io;
 		double DRAM_freq;
 		
 		int ioChannelWidth;
 		
 		int DRAM_chipsPerDIMM;
 		int DRAM_arrays;
 	    
 	
 	gates:
 	    input chipdataIn;  
 	    output chipdataOut;   
 	    
 	    input dramdataIn;
 	    output dramdataOut;
 	    output cntrl;
 	    
 	    inout source;
 	    
 	    
 	submodules:
 	    oc : OCM_Control {
 	        
 	    }
 	    
 	    procStrip : ElectronicToProcessorMsg {
 	        
 	    }
 	    
 	    
 	       serializer: Parallelizer {
            parameters:
               
             
                rateIn = DRAM_freq;
                rateOut = clockRate_io;
                
                numWiresOut = ioChannelWidth;
                numWiresIn = DRAM_arrays * DRAM_chipsPerDIMM;
               
            gates:
                out[1];
        }
         
         deserializer: Serializer {
            parameters:

                rateIn = clockRate_io;
                rateOut = DRAM_freq;
                
                numWiresIn = ioChannelWidth;
                numWiresOut = DRAM_arrays * DRAM_chipsPerDIMM;
                
            gates:
                in[1];
        }
 	    
 	connections allowunconnected:
 	    
      	procStrip.electronicIn <-- deserializer.out;
      	deserializer.in[0] <-- chipdataIn;
        procStrip.electronicOut --> serializer.in;
        serializer.out[0] --> chipdataOut;
      	procStrip.procIn <-- oc.toChip;
      	procStrip.procOut --> oc.fromChip;
      	
      	
      	oc.fromDRAM <-- dramdataIn;
      	oc.toDRAM --> dramdataOut;
      	oc.cntrl --> cntrl;
}