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


import ioComponents.DRAM_LRL.NIF_MemGW;
import ioComponents.DRAM_LRL.MemoryControl;
import ioComponents.DRAM_LRL.MemoryControl_E;

import photonic.devices.Modulator;
import electronicComponents.Parallelizer;

import electronicComponents.RouterCrossbar;

import chipComponents.InoutConverter;

import topologies.ModulatorBank;

module MemoryGateway_Photonic
{
    parameters:
        int DRAM_DIMMsPerGW;
        int id;

      

    gates:
        inout ph_in_net;
        inout ph_out_net;

        inout ph_in_dimm[DRAM_DIMMsPerGW];
        inout ph_out_dimm[DRAM_DIMMsPerGW];

        input elIn;
        output elOut;


    submodules:


        phSwitch: MemGWSwitch;


        nif: NIF_MemGW {
         	id = id;   
        }

        cntrl: MemoryControl;

        ModBank : ModulatorBank {
        }
        
           modConv : InoutConverter {
 	        
 	    }

    connections allowunconnected:

        ph_in_net <--> phSwitch.in[0];
        ph_out_net <--> phSwitch.out[0];



        for i=0..DRAM_DIMMsPerGW-1 {
            phSwitch.in[i+2] <--> ph_in_dimm[i];
            phSwitch.out[i+1] <--> ph_out_dimm[i];
        }

        for i=0..(DRAM_DIMMsPerGW-1)*2 {
            cntrl.switchCntrl[i] --> phSwitch.cntrl[i];
        }

        cntrl.fromNIF <-- nif.toProc;
        cntrl.toNIF --> nif.fromProc;
        cntrl.nifReq <--> nif.procReq;

        nif.portIn[0] <-- elIn;
        nif.portOut[0] --> elOut;

       
        phSwitch.in[1] <--> ModBank.networkOut;

         ModBank.electronicIn <--> modConv.bi;
         modConv.uniIn <-- cntrl.toDIMMs;


}

import electronicComponents.mux;
import electronicComponents.inoutConverter;

module MemoryGateway_ElectronicCC
{
    parameters:
        int DRAM_DIMMsPerGW;
        int id;
        
        double O_frequency_data;

    gates:
        input data_in_net;
        output data_out_net;

        input data_in_dimm[DRAM_DIMMsPerGW];
        output data_out_dimm[DRAM_DIMMsPerGW];

        input cIn;
        output cOut;


    submodules:




        nif: NIF_MemGW{
            id = id;
        }

        cntrl: MemoryControl_E;

        xbar: RouterCrossbar {
            parameters:
                autounblock = false;
                numPorts = DRAM_DIMMsPerGW + 2;
                clockRate = O_frequency_data;
        }
		
		conv : inoutConverter {
		    
		}
		
		conv2 : inoutConverter {
		    
		}

    connections allowunconnected:

        cIn --> nif.portIn[0];
        cOut <-- nif.portOut[0];

        nif.procReq <--> cntrl.nifReq;

        nif.fromProc <-- cntrl.toNIF;
        nif.toProc --> cntrl.fromNIF;

      
        cntrl.switchCntrl[0] --> conv.in;
        conv.io_out <--> xbar.cntrl;
        
        for i=0..DRAM_DIMMsPerGW-1 {
         	xbar.in[2+i] <-- data_in_dimm[i];
         	xbar.out[2+i] --> data_out_dimm[i];   
        }
        
        xbar.in[0] <-- data_in_net;
        xbar.out[0] --> data_out_net;
        
        xbar.in[1] <--  cntrl.toDIMMs;
        
        
      
}
