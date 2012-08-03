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

package chipComponents;

import photonic.devices.Laser;
import photonic.devices.Facet;

import photonic.devices.StarCoupler;

module LaserPlane {
    parameters:
        
        int numOfWavelengthChannels;
      
      	double firstWavelength;
      	double wavelengthSpacing;
      	
      	double IL_Facet_Vertical;
      	
      	int numOfNodesX;
      	int numOfNodesY;
  	
  	gates:
  	    inout signal[];
  	
  	
  	submodules:
      
        
    	las[numOfWavelengthChannels] : Laser {
    	    
    	    parameters:
    	        Wavelength = firstWavelength + index*wavelengthSpacing;
    	}    
    	
    	
    	vertCoup[numOfWavelengthChannels * numOfNodesX * numOfNodesY] : Facet {
    	    parameters:
    	        IL_Facet = IL_Facet_Vertical;
    	        
    	} 
    	
    	
    	star [numOfWavelengthChannels] : StarCoupler {
    	 	parameters:
    	 	    numOuts = numOfWavelengthChannels;
    	 	    
    	 	gates:
    	 	    outs[numOfWavelengthChannels];   
    	    
    	}
	
	
	connections allowunconnected:
	    
	    for i = 0..numOfWavelengthChannels-1 {
	    	las[i].photonicOut <--> star[i].theIn;
	    
	    }
	    
	    for i = 0..numOfWavelengthChannels-1, for j=0..numOfNodesX*numOfNodesY {
	    	star[i].outs[j] <--> vertCoup[j*numOfWavelengthChannels + i].photonicIn;
	    	vertCoup[j*numOfWavelengthChannels + i].photonicOut <--> 	signal[j*numOfWavelengthChannels + i];
	    }
	

    
    
};