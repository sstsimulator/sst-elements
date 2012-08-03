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

package topologies.torusNX;

import topologies.torusNX.TorusNXNetworkPlane;
import processingPlane.ProcessingPlane_ProcRoute;
import ioComponents.IOPlane_Photonic;

import statistics.Statistics;
import electronicComponents.VirtualChannelControl;

import addon.ThermalModel;


import electronicComponents.inoutConverter;
//
// TODO auto-generated module
//
network TorusNXNetwork
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;

        bool useThermalModel;
        bool useIOplane;

        @display("bgb=447,513");
    submodules:
        stat: Statistics;

        vc: VirtualChannelControl;

        prPlane: ProcessingPlane_ProcRoute {
            parameters:
                numNodesX = numOfNodesX;
                numNodesY = numOfNodesY;
                networkPrefix = "";
                numNetPorts = 2;

        }

        phPlane: TorusNXNetworkPlane;

        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery

        }
        conv[numOfNodesX*numOfNodesY]: inoutConverter;

        thermal[(useThermalModel?1:0)]: ThermalModel {
            @display("p=333,448");
        }


    connections allowunconnected:
        for i=0..numOfNodesX*numOfNodesY-1 {
            prPlane.dataIn[i*2+1] <-- conv[i].out;
            conv[i].io_in <--> phPlane.photonicGatewayOut[i];
            prPlane.dataOut[i*2+1] --> conv[i].in;
            conv[i].io_out <--> phPlane.photonicGatewayIn[i];

            prPlane.dataIn[i*2] <-- phPlane.electronicGatewayOut[i];
            prPlane.dataOut[i*2] --> phPlane.electronicGatewayIn[i];
        }


  		//going off chip
        for i=0..numOfNodesX-1 {
            //--top row
            phPlane.photonicMemoryIn[i] <--> ioPlane[0].phOut[i] if (useIOplane);
            phPlane.photonicMemoryOut[i] <--> ioPlane[0].phIn[i] if (useIOplane);

            phPlane.electronicMemoryIn[i] <-- ioPlane[0].elOut[i] if (useIOplane);
            phPlane.electronicMemoryOut[i] --> ioPlane[0].elIn[i] if (useIOplane);
        }

        for i=0..numOfNodesX-1 {
            //--bottom row
            phPlane.photonicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phOut[i+numOfNodesX] if (useIOplane);
            phPlane.photonicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phIn[i+numOfNodesX] if (useIOplane);

            phPlane.electronicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <-- ioPlane[0].elOut[i+numOfNodesX] if (useIOplane);
            phPlane.electronicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] --> ioPlane[0].elIn[i+numOfNodesX] if (useIOplane);
        }

        for j=1..numOfNodesY-2 {
            //--left side
            phPlane.photonicMemoryIn[numOfNodesX*j] <--> ioPlane[0].phOut[(j-1)+numOfNodesX*2] if (useIOplane);
            phPlane.photonicMemoryOut[numOfNodesX*j] <--> ioPlane[0].phIn[(j-1)+numOfNodesX*2] if (useIOplane);

            phPlane.electronicMemoryIn[numOfNodesX*j] <-- ioPlane[0].elOut[(j-1)+numOfNodesX*2] if (useIOplane);
            phPlane.electronicMemoryOut[numOfNodesX*j] --> ioPlane[0].elIn[(j-1)+numOfNodesX*2] if (useIOplane);
        }

        for j=1..numOfNodesY-2 {
            //--right side
            phPlane.photonicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
            phPlane.photonicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);

            phPlane.electronicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <-- ioPlane[0].elOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
            phPlane.electronicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] --> ioPlane[0].elIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
        }
}
