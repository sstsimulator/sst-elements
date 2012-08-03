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

package ioComponents;

import photonic.devices.PSE1x2;
import photonic.devices.Line;
import photonic.devices.Bend;
import photonic.PhotonicSwitch;

module MemGWSwitch {
    
     parameters:
        int DRAM_DIMMsPerGW;
      


        double pse1x2Width;
        double pse2x2Width;
        double crossWidth;
        
        double dimensionX = default(430);
        double dimensionY = default(430);
        @display("bgb=800,806");

    gates:
    
        inout in[DRAM_DIMMsPerGW + 2];
        inout out[DRAM_DIMMsPerGW + 1];
        input cntrl[(DRAM_DIMMsPerGW-1)*2 + 1];
        
        
    submodules:
        
        inports[DRAM_DIMMsPerGW-1] : PSE1x2 {
        }
        
    	outports[DRAM_DIMMsPerGW-1] : PSE1x2 {
        }
        
        memCntrlIn : PSE1x2 {
            
        }
        
        inExitLine : Line {
         	parameters:
         	    Length_Line = (DRAM_DIMMsPerGW + 1) * pse1x2Width + 10;  
        }
        
        incomingLine : Line {
         	parameters:
         	    Length_Line = pse1x2Width * 2;   
        }
        
        inBend : Bend {
            
        }
        
        outgoingLine : Line {
         	parameters:
         	    Length_Line = pse1x2Width;   
        }
        
        outBend : Bend {
            
        }
        
        
    connections allowunconnected:
        
        in[2] <--> incomingLine.photonicIn;
        incomingLine.photonicOut <--> inBend.photonicIn;
        inBend.photonicOut <--> inports[0].photonicVertIn if (DRAM_DIMMsPerGW > 1);
        inBend.photonicOut <--> inExitLine.photonicIn if (DRAM_DIMMsPerGW <= 1);
        
        for i=0..DRAM_DIMMsPerGW-3 {
         	inports[i].photonicHorizIn <--> in[i+3];
         	inports[i].photonicVertOut <--> inports[i+1].photonicVertIn;
        }
        
        inports[DRAM_DIMMsPerGW-2].photonicVertOut <--> inExitLine.photonicIn if (DRAM_DIMMsPerGW > 1);
        inports[DRAM_DIMMsPerGW-2].photonicHorizIn <--> in[DRAM_DIMMsPerGW+1] if (DRAM_DIMMsPerGW > 1);
        inExitLine.photonicOut <--> out[0];
        
        for i=0..DRAM_DIMMsPerGW-2 {
             cntrl[i] --> inports[i].fromRouter;
             cntrl[DRAM_DIMMsPerGW - 1 + i] --> outports[i].fromRouter;
        }
        cntrl[(DRAM_DIMMsPerGW-1)*2] --> memCntrlIn.fromRouter;
        
        
        out[1] <--> outgoingLine.photonicOut;
        outgoingLine.photonicIn <--> outBend.photonicOut;
        outBend.photonicIn <--> outports[0].photonicHorizOut if (DRAM_DIMMsPerGW > 1);
        outBend.photonicIn <--> memCntrlIn.photonicVertOut if (DRAM_DIMMsPerGW <= 1);
        
        for i=0..DRAM_DIMMsPerGW-3 {
         	outports[i].photonicVertOut <--> out[i+2];
         	outports[i].photonicHorizIn <--> outports[i+1].photonicHorizOut;
        }

        
      	in[1] <--> memCntrlIn.photonicHorizIn;
      	memCntrlIn.photonicVertIn <--> in[0];
      	memCntrlIn.photonicVertOut <--> outports[DRAM_DIMMsPerGW-2].photonicHorizIn if (DRAM_DIMMsPerGW > 1);
      	outports[DRAM_DIMMsPerGW-2].photonicVertOut <--> out[DRAM_DIMMsPerGW] if (DRAM_DIMMsPerGW > 1);
        
}

