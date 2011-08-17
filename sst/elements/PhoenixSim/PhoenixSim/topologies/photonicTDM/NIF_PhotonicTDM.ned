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

package topologies.photonicTDM;

import processingPlane.interfaces.NIF;


simple NIF_PhotonicTDM like NIF {
  	parameters:
       int numNetPorts;
        int numOfNodesX;
        int numOfNodesY;
        
           int id;
        
        double tdmSlotPeriod;
        double pseSetupTime;
        
         string cntrlFileName;
        double guardTimeGap;
        
      double O_frequency_cntrl;
      double O_frequency_data;
      double O_frequency_proc;

        int electronicChannelWidth;
        int maxPacketSize;
        int numOfWavelengthChannels;
        
        int processorConcentration;
        
   	bool useIOplane;
      
      	int routerBufferSize;
      	int routerVirtualChannels;
       
        string networkName;
        
         string addressLevel = default("PROC");
       
       
         int DRAM_arrays;
        int DRAM_cols;
        int DRAM_rows;
        
        double DRAM_freq;

		int DRAM_tCAS; //in clk cycles
		int DRAM_tCL;
		int DRAM_tRP;
		
		 int DRAM_DIMMsPerGW;
        int DRAM_chipsPerDIMM;
        int DRAM_banksPerChip;
		

        @display("i=block/ifcard");
    gates:
         input portIn[numNetPorts];
        output portOut[numNetPorts];

        inout procReq;
        input fromProc;
        output toProc;
    
};


simple NIF_PhotonicETDM like NIF {
  	parameters:
       int numNetPorts;
        int numOfNodesX;
        int numOfNodesY;
        
           int id;
        
        double tdmSlotPeriod;
        double pseSetupTime;
        
         string cntrlFileName;
        double guardTimeGap;
        
      double O_frequency_cntrl;
      double O_frequency_data;
      double O_frequency_proc;

        int electronicChannelWidth;
        int maxPacketSize;
        int numOfWavelengthChannels;
        
        int processorConcentration;
        
     bool useIOplane;
      
      	int routerBufferSize;
      	int routerVirtualChannels;
       
        string networkName;
        
         string addressLevel = default("PROC");
       
       
         int DRAM_arrays;
        int DRAM_cols;
        int DRAM_rows;
        
        double DRAM_freq;

		int DRAM_tCAS; //in clk cycles
		int DRAM_tCL;
		int DRAM_tRP;
		
		 int DRAM_DIMMsPerGW;
        int DRAM_chipsPerDIMM;
        int DRAM_banksPerChip;
		

        @display("i=block/ifcard");
    gates:
         input portIn[numNetPorts];
        output portOut[numNetPorts];

        inout procReq;
        input fromProc;
        output toProc;
    
};