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

import electronicComponents.ElectronicRouter;
import photonic.PhotonicSwitch;


module HybridRouter {
 	parameters:
 		string optSwitch;
 		int elRouter;
 		
 		string id;
 		string level;
 		
 		int numX = default(1);
 		int numY = default(1);
 		
 		
        double dimensionX = default(430);
        double dimensionY = default(430);
 		
 	
   
 	gates:
        inout phIn[15];
 		inout phOut[15];
 		
 		input elIn[15];
 		output elOut[15];
        
        
    submodules:
        
         ph : <optSwitch> like PhotonicSwitch {
        
            
        }
        
        el : ElectronicRouter {
         	parameters:
         	    type = elRouter;
         	  
            	numPorts = ph.numPorts;
        		numPSEports = ph.numPSE;
        		
                id = id;
                level = level;
                
                numX = numX;
                numY = numY;
                
                switchVariant = ph.variant;
               
        }
        
       
        
    connections allowunconnected:
         
         for i=0..ph.numPorts-1 {
             el.portIn[i] <-- elIn[i];
             el.portOut[i] --> elOut[i];
             
             
             ph.in[i] <--> phIn[i];
             ph.out[i] <--> phOut[i];

         }
         
         for i=0..ph.numPSE-1 {
             el.toPSE[i] --> ph.cntrl[i];
         }   
            
    
}
