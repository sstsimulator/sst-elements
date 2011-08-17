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

package processingPlane.interfaces;

simple NIF_Electronic like NIF {
  	parameters:
        
       	int numNetPorts = default(1);
       	
        double O_frequency_cntrl;

        int electronicChannelWidth;
        int routerBufferSize;
        int maxPacketSize;
  
        int routerVirtualChannels;
        int id;
        int processorConcentration;
        
        @display("i=block/ifcard");
    gates:
        input portIn[numNetPorts];
        output portOut[numNetPorts];
        
        inout procReq;
        input fromProc;
        output toProc;

    
};

simple NIF_ElectronicCC like NIF {
  	parameters:
        
       int numNetPorts;
        int numOfNodesX;
        int numOfNodesY;
        
        int id;
        
         string NIFselection = default("none");
        double NIFthreshold = default(0);
        
        double guardTimeGap;
      double O_frequency_cntrl;
      double O_frequency_data;

        int electronicChannelWidth;
        int maxPacketSize;
        int numOfWavelengthChannels;
        
        int processorConcentration;
        
        double DetectorSensitivity;
      
      	int routerBufferSize;
      	int routerVirtualChannels;
       
        string networkName;
       
        string addressLevel = default("PROC");
        
        @display("i=block/ifcard");
    gates:
         input portIn[numNetPorts];
        output portOut[numNetPorts];

        inout procReq;
        input fromProc;
        output toProc;

    
};