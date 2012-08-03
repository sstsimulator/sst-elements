package processingPlane;

import photonic.devices.SwitchingFilter;
import photonic.devices.attenuator;
import photonic.devices.Modulator;


module GatewaySignalActuator {
 
 	int numOfWavelengthChannels;
 	double wavelengthSpacing;
 	double firstWavelength;
 
 	gates:
 	    inout phIn[];
 	    inout phOut;
 	    
 	    input elInProc;
 	    output elOutProc;
 	    
 	    input elInNet;
 	    output elOutNet;
 	   
 	    
 	submodules:
 	    
 	    selectors[numOfWavelengthChannels] : SwitchingFilter {
 	        parameters:
 	        	filterBaseWavelength = firstWavelength + index*wavelengthSpacing;
 	        
 	    }
    
    	attenuator : attenuator {
    	    
    	}
    	
    	
    	LBCmod : Modulator {
    	    filterBaseWavelength = firstWavelength + wavelengthSpacing*numOfWavelengthChannels;
    	}
    	
    	cntrl : GatewaySignalControl {
    	 	   gates:
    	 	       toSelectors[numOfWavelengthChannels];
    	}
    	 
    connections allowunconnected:
        
        for i=0..numOfWavelengthChannels-2 {
          	  selectors[i].Out <--> selectors[i+1].thruIn;
          	  cntrl.toSelectors[i] --> selectors[i].cntrl;
          	  
          	  selectors[i].In <--> phIn[i];
        }
        
        selectors[numOfWavelengthChannels-1].In <--> phIn[numOfWavelengthChannels-1];
        selectors[numOfWavelengthChannels-1].Out <--> attenuator.phIn;
        phOut <--> attenuator.phOut;
       
        cntrl.elInProc <-- elInProc;
        cntrl.elOutProc --> elOutProc;
        
        cntrl.elInNet <-- elInNet;
        cntrl.elOutNet --> elOutNet;
        
        cntrl.toLBC --> LBCmod.cntrl;
        cntrl.toAttenuator --> attenuator.cntrl;
        
        
}

