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

import photonic.devices.*;
import topologies.torusNX.*;

import electronicComponents.ElectronicChannel;
import chipComponents.HybridRouter;

//
// TODO auto-generated module
//



module TorusNXTileNE_Regular
{
    parameters:
        int nodeId;

        int numX;
        int numY;
        double coreSizeX;
        double coreSizeY;
        volatile double disp_Width;
        double PathSeparation;
        
        double PositionLeftX = default((nodeId%numX)*coreSizeX);
        double PositionBottomY = default((nodeId/numX)*coreSizeY);
    gates:
        inout photonicNLeftIn;
        inout photonicNRightIn;
        inout photonicSLeftIn;
        inout photonicSRightIn;
        inout photonicETopIn;
        inout photonicEBottomIn;
        inout photonicWTopIn;
        inout photonicWBottomIn;
        inout photonicNLeftOut;
        inout photonicNRightOut;
        inout photonicSLeftOut;
        inout photonicSRightOut;
        inout photonicETopOut;
        inout photonicEBottomOut;
        inout photonicWTopOut;
        inout photonicWBottomOut;

        inout photonicGatewayIn;
        inout photonicGatewayOut;
        inout photonicMemoryIn;
        inout photonicMemoryOut;

        input electronicNLeftIn;
        input electronicNRightIn;
        input electronicSLeftIn;
        input electronicSRightIn;
        input electronicETopIn;
        input electronicEBottomIn;
        input electronicWTopIn;
        input electronicWBottomIn;
        output electronicNLeftOut;
        output electronicNRightOut;
        output electronicSLeftOut;
        output electronicSRightOut;
        output electronicETopOut;
        output electronicEBottomOut;
        output electronicWTopOut;
        output electronicWBottomOut;

        input electronicGatewayIn;
        output electronicGatewayOut;
        input electronicMemoryIn;
        output electronicMemoryOut;


    submodules:
        CrossNW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                PositionLeftX = default((nodeId%numX)*coreSizeX);
        		PositionBottomY = default((nodeId/numX)*coreSizeY);
                @display("p=110,100;b=48,48,,,,1");
        }

        GatewaySE: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "TorusNXGateway";
              
                elRouter = 71;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=427,377");
        }

        CrossSW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=110,377;b=48,48,,,,1");
        }
        Sw4x4NE: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "NonBlockingSwitch4x4Regular";
               
                elRouter = 70;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=397,143");
        }



        LineVertTopW: LineBidirectional {
            parameters:
                //Length_Line = 110;
                Length_Line = 190;
				PositionLeftX = default(0);
        		PositionBottomY = default(0);
                @display("p=110,29");
        }
        LineVertMiddleW: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 190 + 190;
                @display("p=110,250");
        }
        LineVertBottomW: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=110,452");
        }

        LineVertTopE: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=427,29");
        }
        LineVertMiddleE: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 100;
                @display("p=427,266");
        }
        LineVertBottomE: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=427,444");
        }

        LineHorizLeftN: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=24,100");
        }
        LineHorizMiddleN: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 190;
                @display("p=270,100");
        }
        LineHorizRightN: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=506,100");
        }

        LineHorizLeftS: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=24,377");
        }
        LineHorizMiddleS: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 190 + 100;
                @display("p=270,377");
        }
        LineHorizRightS: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=506,377");
        }

    connections allowunconnected:
        //Direction convention
        // Up = A
        // Down = B
        // Left = A
        // Right = B
        Sw4x4NE.phIn[0] <--> LineVertTopE.photonicOutB;
        Sw4x4NE.phIn[2] <--> LineVertMiddleE.photonicOutA;
        Sw4x4NE.phIn[1] <--> LineHorizRightN.photonicOutA;
        Sw4x4NE.phIn[3] <--> LineHorizMiddleN.photonicOutB;

        Sw4x4NE.phOut[0] <--> LineVertTopE.photonicInA;
        Sw4x4NE.phOut[2] <--> LineVertMiddleE.photonicInB;
        Sw4x4NE.phOut[1] <--> LineHorizRightN.photonicInB;
        Sw4x4NE.phOut[3] <--> LineHorizMiddleN.photonicInA;

        CrossNW.photonicInN <--> LineVertTopW.photonicOutB;
        CrossNW.photonicInS <--> LineVertMiddleW.photonicOutA;
        CrossNW.photonicInE <--> LineHorizMiddleN.photonicOutA;
        CrossNW.photonicInW <--> LineHorizLeftN.photonicOutB;

        CrossNW.photonicOutN <--> LineVertTopW.photonicInA;
        CrossNW.photonicOutS <--> LineVertMiddleW.photonicInB;
        CrossNW.photonicOutE <--> LineHorizMiddleN.photonicInB;
        CrossNW.photonicOutW <--> LineHorizLeftN.photonicInA;

        GatewaySE.phIn[0] <--> LineVertMiddleE.photonicOutB;
        GatewaySE.phIn[2] <--> LineVertBottomE.photonicOutA;
        GatewaySE.phIn[1] <--> LineHorizRightS.photonicOutA;
        GatewaySE.phIn[3] <--> LineHorizMiddleS.photonicOutB;

        GatewaySE.phOut[0] <--> LineVertMiddleE.photonicInA;
        GatewaySE.phOut[2] <--> LineVertBottomE.photonicInB;
        GatewaySE.phOut[1] <--> LineHorizRightS.photonicInB;
        GatewaySE.phOut[3] <--> LineHorizMiddleS.photonicInA;

        CrossSW.photonicInN <--> LineVertMiddleW.photonicOutB;
        CrossSW.photonicInS <--> LineVertBottomW.photonicOutA;
        CrossSW.photonicInE <--> LineHorizMiddleS.photonicOutA;
        CrossSW.photonicInW <--> LineHorizLeftS.photonicOutB;

        CrossSW.photonicOutN <--> LineVertMiddleW.photonicInA;
        CrossSW.photonicOutS <--> LineVertBottomW.photonicInB;
        CrossSW.photonicOutE <--> LineHorizMiddleS.photonicInB;
        CrossSW.photonicOutW <--> LineHorizLeftS.photonicInA;

        GatewaySE.phOut[4] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> GatewaySE.phIn[4];

        photonicNLeftIn <--> LineVertTopW.photonicInB;
        photonicNRightIn <--> LineVertTopE.photonicInB;
        photonicSLeftIn <--> LineVertBottomW.photonicInA;
        photonicSRightIn <--> LineVertBottomE.photonicInA;
        photonicETopIn <--> LineHorizRightN.photonicInA;
        photonicEBottomIn <--> LineHorizRightS.photonicInA;
        photonicWTopIn <--> LineHorizLeftN.photonicInB;
        photonicWBottomIn <--> LineHorizLeftS.photonicInB;
        photonicNLeftOut <--> LineVertTopW.photonicOutA;
        photonicNRightOut <--> LineVertTopE.photonicOutA;
        photonicSLeftOut <--> LineVertBottomW.photonicOutB;
        photonicSRightOut <--> LineVertBottomE.photonicOutB;
        photonicETopOut <--> LineHorizRightN.photonicOutB;
        photonicEBottomOut <--> LineHorizRightS.photonicOutB;
        photonicWTopOut <--> LineHorizLeftN.photonicOutA;
        photonicWBottomOut <--> LineHorizLeftS.photonicOutA;


        // Electronic Connections

        Sw4x4NE.elIn[0] <-- electronicNRightIn;
        Sw4x4NE.elIn[1] <-- electronicETopIn;
        Sw4x4NE.elIn[2] <-- ElectronicChannel <-- GatewaySE.elOut[0];
        Sw4x4NE.elIn[3] <-- electronicWTopIn;

        Sw4x4NE.elOut[0] --> electronicNRightOut;
        Sw4x4NE.elOut[1] --> electronicETopOut;
        Sw4x4NE.elOut[2] --> ElectronicChannel --> GatewaySE.elIn[0];
        Sw4x4NE.elOut[3] --> electronicWTopOut;

        GatewaySE.elIn[1] <-- electronicEBottomIn;
        GatewaySE.elIn[2] <-- electronicSRightIn;
        GatewaySE.elIn[3] <-- electronicWBottomIn;

        GatewaySE.elOut[1] --> electronicEBottomOut;
        GatewaySE.elOut[2] --> electronicSRightOut;
        GatewaySE.elOut[3] --> electronicWBottomOut;
        
        electronicNLeftIn --> electronicSLeftOut;
        electronicSLeftIn --> electronicNLeftOut;


        electronicGatewayIn --> { @display("ls=blue,0"); } --> GatewaySE.elIn[4];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- GatewaySE.elOut[4];
}

module TorusNXTileNW_Regular
{
    parameters:
        int nodeId;

        int numX;
        int numY;
        double coreSizeX;
        double coreSizeY;
        volatile double disp_Width;
        double PathSeparation;
        
        double PositionLeftX = default((nodeId%numX)*coreSizeX);
        double PositionBottomY = default((nodeId/numX)*coreSizeY);
    gates:
        inout photonicNLeftIn;
        inout photonicNRightIn;
        inout photonicSLeftIn;
        inout photonicSRightIn;
        inout photonicETopIn;
        inout photonicEBottomIn;
        inout photonicWTopIn;
        inout photonicWBottomIn;
        inout photonicNLeftOut;
        inout photonicNRightOut;
        inout photonicSLeftOut;
        inout photonicSRightOut;
        inout photonicETopOut;
        inout photonicEBottomOut;
        inout photonicWTopOut;
        inout photonicWBottomOut;

        inout photonicGatewayIn;
        inout photonicGatewayOut;
        inout photonicMemoryIn;
        inout photonicMemoryOut;

        input electronicNLeftIn;
        input electronicNRightIn;
        input electronicSLeftIn;
        input electronicSRightIn;
        input electronicETopIn;
        input electronicEBottomIn;
        input electronicWTopIn;
        input electronicWBottomIn;
        output electronicNLeftOut;
        output electronicNRightOut;
        output electronicSLeftOut;
        output electronicSRightOut;
        output electronicETopOut;
        output electronicEBottomOut;
        output electronicWTopOut;
        output electronicWBottomOut;

        input electronicGatewayIn;
        output electronicGatewayOut;
        input electronicMemoryIn;
        output electronicMemoryOut;


    submodules:
        CrossNE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=427,100;b=48,48,,,,1");
        }

        GatewaySW: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "TorusNXGateway";
             
                elRouter = 71;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=110,377");
        }

        CrossSE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=427,377;b=48,48,,,,1");
        }
        Sw4x4NW: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "NonBlockingSwitch4x4Regular";
              
                elRouter = 70;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=149,143");
        }



        LineVertTopW: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=110,29");
        }
        LineVertMiddleW: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 100;
                @display("p=110,281");
        }
        LineVertBottomW: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=110,452");
        }

        LineVertTopE: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=427,29");
        }
        LineVertMiddleE: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 190 + 190;
                @display("p=427,266");
        }
        LineVertBottomE: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=427,444");
        }

        LineHorizLeftN: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=24,100");
        }
        LineHorizMiddleN: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 190;
                @display("p=270,100");
        }
        LineHorizRightN: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=506,100");
        }

        LineHorizLeftS: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=24,377");
        }
        LineHorizMiddleS: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 100 + 190;
                @display("p=270,377");
        }
        LineHorizRightS: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=506,377");
        }

    connections allowunconnected:
        //Direction convention
        // Up = A
        // Down = B
        // Left = A
        // Right = B
        CrossNE.photonicInN <--> LineVertTopE.photonicOutB;
        CrossNE.photonicInS <--> LineVertMiddleE.photonicOutA;
        CrossNE.photonicInE <--> LineHorizRightN.photonicOutA;
        CrossNE.photonicInW <--> LineHorizMiddleN.photonicOutB;

        CrossNE.photonicOutN <--> LineVertTopE.photonicInA;
        CrossNE.photonicOutS <--> LineVertMiddleE.photonicInB;
        CrossNE.photonicOutE <--> LineHorizRightN.photonicInB;
        CrossNE.photonicOutW <--> LineHorizMiddleN.photonicInA;

        Sw4x4NW.phIn[0] <--> LineVertTopW.photonicOutB;
        Sw4x4NW.phIn[2] <--> LineVertMiddleW.photonicOutA;
        Sw4x4NW.phIn[1] <--> LineHorizMiddleN.photonicOutA;
        Sw4x4NW.phIn[3] <--> LineHorizLeftN.photonicOutB;

        Sw4x4NW.phOut[0] <--> LineVertTopW.photonicInA;
        Sw4x4NW.phOut[2] <--> LineVertMiddleW.photonicInB;
        Sw4x4NW.phOut[1] <--> LineHorizMiddleN.photonicInB;
        Sw4x4NW.phOut[3] <--> LineHorizLeftN.photonicInA;

        CrossSE.photonicInN <--> LineVertMiddleE.photonicOutB;
        CrossSE.photonicInS <--> LineVertBottomE.photonicOutA;
        CrossSE.photonicInE <--> LineHorizRightS.photonicOutA;
        CrossSE.photonicInW <--> LineHorizMiddleS.photonicOutB;

        CrossSE.photonicOutN <--> LineVertMiddleE.photonicInA;
        CrossSE.photonicOutS <--> LineVertBottomE.photonicInB;
        CrossSE.photonicOutE <--> LineHorizRightS.photonicInB;
        CrossSE.photonicOutW <--> LineHorizMiddleS.photonicInA;

        GatewaySW.phIn[0] <--> LineVertMiddleW.photonicOutB;
        GatewaySW.phIn[2] <--> LineVertBottomW.photonicOutA;
        GatewaySW.phIn[1] <--> LineHorizMiddleS.photonicOutA;
        GatewaySW.phIn[3] <--> LineHorizLeftS.photonicOutB;

        GatewaySW.phOut[0] <--> LineVertMiddleW.photonicInA;
        GatewaySW.phOut[2] <--> LineVertBottomW.photonicInB;
        GatewaySW.phOut[1] <--> LineHorizMiddleS.photonicInB;
        GatewaySW.phOut[3] <--> LineHorizLeftS.photonicInA;

        GatewaySW.phOut[4] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> GatewaySW.phIn[4];

        photonicNLeftIn <--> LineVertTopW.photonicInB;
        photonicNRightIn <--> LineVertTopE.photonicInB;
        photonicSLeftIn <--> LineVertBottomW.photonicInA;
        photonicSRightIn <--> LineVertBottomE.photonicInA;
        photonicETopIn <--> LineHorizRightN.photonicInA;
        photonicEBottomIn <--> LineHorizRightS.photonicInA;
        photonicWTopIn <--> LineHorizLeftN.photonicInB;
        photonicWBottomIn <--> LineHorizLeftS.photonicInB;
        photonicNLeftOut <--> LineVertTopW.photonicOutA;
        photonicNRightOut <--> LineVertTopE.photonicOutA;
        photonicSLeftOut <--> LineVertBottomW.photonicOutB;
        photonicSRightOut <--> LineVertBottomE.photonicOutB;
        photonicETopOut <--> LineHorizRightN.photonicOutB;
        photonicEBottomOut <--> LineHorizRightS.photonicOutB;
        photonicWTopOut <--> LineHorizLeftN.photonicOutA;
        photonicWBottomOut <--> LineHorizLeftS.photonicOutA;


        // Electronic Connections

        Sw4x4NW.elIn[0] <-- electronicNLeftIn;
        Sw4x4NW.elIn[1] <-- electronicETopIn;
        Sw4x4NW.elIn[2] <-- ElectronicChannel <-- GatewaySW.elOut[0];
        Sw4x4NW.elIn[3] <-- electronicWTopIn;

        Sw4x4NW.elOut[0] --> electronicNLeftOut;
        Sw4x4NW.elOut[1] --> electronicETopOut;
        Sw4x4NW.elOut[2] --> ElectronicChannel --> GatewaySW.elIn[0];
        Sw4x4NW.elOut[3] --> electronicWTopOut;

        GatewaySW.elIn[1] <-- electronicEBottomIn;
        GatewaySW.elIn[2] <-- electronicSLeftIn;
        GatewaySW.elIn[3] <-- electronicWBottomIn;

        GatewaySW.elOut[1] --> electronicEBottomOut;
        GatewaySW.elOut[2] --> electronicSLeftOut;
        GatewaySW.elOut[3] --> electronicWBottomOut;
        
        electronicNRightIn --> electronicSRightOut;
        electronicSRightIn --> electronicNRightOut;

        electronicGatewayIn --> { @display("ls=blue,0"); } --> GatewaySW.elIn[4];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- GatewaySW.elOut[4];
}

module TorusNXTileSE_Regular
{
    parameters:
        int nodeId;

        int numX;
        int numY;
        double coreSizeX;
        double coreSizeY;
        volatile double disp_Width;
        double PathSeparation;
        
        double PositionLeftX = default((nodeId%numX)*coreSizeX);
        double PositionBottomY = default((nodeId/numX)*coreSizeY);
    gates:
        inout photonicNLeftIn;
        inout photonicNRightIn;
        inout photonicSLeftIn;
        inout photonicSRightIn;
        inout photonicETopIn;
        inout photonicEBottomIn;
        inout photonicWTopIn;
        inout photonicWBottomIn;
        inout photonicNLeftOut;
        inout photonicNRightOut;
        inout photonicSLeftOut;
        inout photonicSRightOut;
        inout photonicETopOut;
        inout photonicEBottomOut;
        inout photonicWTopOut;
        inout photonicWBottomOut;

        inout photonicGatewayIn;
        inout photonicGatewayOut;
        inout photonicMemoryIn;
        inout photonicMemoryOut;

        input electronicNLeftIn;
        input electronicNRightIn;
        input electronicSLeftIn;
        input electronicSRightIn;
        input electronicETopIn;
        input electronicEBottomIn;
        input electronicWTopIn;
        input electronicWBottomIn;
        output electronicNLeftOut;
        output electronicNRightOut;
        output electronicSLeftOut;
        output electronicSRightOut;
        output electronicETopOut;
        output electronicEBottomOut;
        output electronicWTopOut;
        output electronicWBottomOut;

        input electronicGatewayIn;
        output electronicGatewayOut;
        input electronicMemoryIn;
        output electronicMemoryOut;


    submodules:
        CrossNW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=110,100;b=48,48,,,,1");
        }

        GatewayNE: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "TorusNXGateway";
               
                elRouter = 71;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=427,100");
        }

        CrossSW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=110,377;b=48,48,,,,1");
        }
        Sw4x4SE: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "NonBlockingSwitch4x4Regular";
               
                elRouter = 70;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=394,326");
        }



        LineVertTopW: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=110,29");
        }
        LineVertMiddleW: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 190 + 190;
                @display("p=110,250");
        }
        LineVertBottomW: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=110,452");
        }

        LineVertTopE: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=427,29");
        }
        LineVertMiddleE: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 100;
                @display("p=427,205");
        }
        LineVertBottomE: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=427,444");
        }

        LineHorizLeftN: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=24,100");
        }
        LineHorizMiddleN: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 100 + 190;
                @display("p=270,100");
        }
        LineHorizRightN: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=506,100");
        }

        LineHorizLeftS: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=24,377");
        }
        LineHorizMiddleS: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 190;
                @display("p=270,377");
        }
        LineHorizRightS: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=506,377");
        }

    connections allowunconnected:
        //Direction convention
        // Up = A
        // Down = B
        // Left = A
        // Right = B
        GatewayNE.phIn[0] <--> LineVertTopE.photonicOutB;
        GatewayNE.phIn[2] <--> LineVertMiddleE.photonicOutA;
        GatewayNE.phIn[1] <--> LineHorizRightN.photonicOutA;
        GatewayNE.phIn[3] <--> LineHorizMiddleN.photonicOutB;

        GatewayNE.phOut[0] <--> LineVertTopE.photonicInA;
        GatewayNE.phOut[2] <--> LineVertMiddleE.photonicInB;
        GatewayNE.phOut[1] <--> LineHorizRightN.photonicInB;
        GatewayNE.phOut[3] <--> LineHorizMiddleN.photonicInA;

        CrossNW.photonicInN <--> LineVertTopW.photonicOutB;
        CrossNW.photonicInS <--> LineVertMiddleW.photonicOutA;
        CrossNW.photonicInE <--> LineHorizMiddleN.photonicOutA;
        CrossNW.photonicInW <--> LineHorizLeftN.photonicOutB;

        CrossNW.photonicOutN <--> LineVertTopW.photonicInA;
        CrossNW.photonicOutS <--> LineVertMiddleW.photonicInB;
        CrossNW.photonicOutE <--> LineHorizMiddleN.photonicInB;
        CrossNW.photonicOutW <--> LineHorizLeftN.photonicInA;

        Sw4x4SE.phIn[0] <--> LineVertMiddleE.photonicOutB;
        Sw4x4SE.phIn[2] <--> LineVertBottomE.photonicOutA;
        Sw4x4SE.phIn[1] <--> LineHorizRightS.photonicOutA;
        Sw4x4SE.phIn[3] <--> LineHorizMiddleS.photonicOutB;

        Sw4x4SE.phOut[0] <--> LineVertMiddleE.photonicInA;
        Sw4x4SE.phOut[2] <--> LineVertBottomE.photonicInB;
        Sw4x4SE.phOut[1] <--> LineHorizRightS.photonicInB;
        Sw4x4SE.phOut[3] <--> LineHorizMiddleS.photonicInA;

        CrossSW.photonicInN <--> LineVertMiddleW.photonicOutB;
        CrossSW.photonicInS <--> LineVertBottomW.photonicOutA;
        CrossSW.photonicInE <--> LineHorizMiddleS.photonicOutA;
        CrossSW.photonicInW <--> LineHorizLeftS.photonicOutB;

        CrossSW.photonicOutN <--> LineVertMiddleW.photonicInA;
        CrossSW.photonicOutS <--> LineVertBottomW.photonicInB;
        CrossSW.photonicOutE <--> LineHorizMiddleS.photonicInB;
        CrossSW.photonicOutW <--> LineHorizLeftS.photonicInA;

        GatewayNE.phOut[4] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> GatewayNE.phIn[4];

        photonicNLeftIn <--> LineVertTopW.photonicInB;
        photonicNRightIn <--> LineVertTopE.photonicInB;
        photonicSLeftIn <--> LineVertBottomW.photonicInA;
        photonicSRightIn <--> LineVertBottomE.photonicInA;
        photonicETopIn <--> LineHorizRightN.photonicInA;
        photonicEBottomIn <--> LineHorizRightS.photonicInA;
        photonicWTopIn <--> LineHorizLeftN.photonicInB;
        photonicWBottomIn <--> LineHorizLeftS.photonicInB;
        photonicNLeftOut <--> LineVertTopW.photonicOutA;
        photonicNRightOut <--> LineVertTopE.photonicOutA;
        photonicSLeftOut <--> LineVertBottomW.photonicOutB;
        photonicSRightOut <--> LineVertBottomE.photonicOutB;
        photonicETopOut <--> LineHorizRightN.photonicOutB;
        photonicEBottomOut <--> LineHorizRightS.photonicOutB;
        photonicWTopOut <--> LineHorizLeftN.photonicOutA;
        photonicWBottomOut <--> LineHorizLeftS.photonicOutA;


        // Electronic Connections

        GatewayNE.elIn[0] <-- electronicNRightIn;
        GatewayNE.elIn[1] <-- electronicETopIn;
        GatewayNE.elIn[2] <-- ElectronicChannel <-- Sw4x4SE.elOut[0];
        GatewayNE.elIn[3] <-- electronicWTopIn;

        GatewayNE.elOut[0] --> electronicNRightOut;
        GatewayNE.elOut[1] --> electronicETopOut;
        GatewayNE.elOut[2] --> ElectronicChannel --> Sw4x4SE.elIn[0];
        GatewayNE.elOut[3] --> electronicWTopOut;

        Sw4x4SE.elIn[1] <-- electronicEBottomIn;
        Sw4x4SE.elIn[2] <-- electronicSRightIn;
        Sw4x4SE.elIn[3] <-- electronicWBottomIn;

        Sw4x4SE.elOut[1] --> electronicEBottomOut;
        Sw4x4SE.elOut[2] --> electronicSRightOut;
        Sw4x4SE.elOut[3] --> electronicWBottomOut;

        electronicNLeftIn --> electronicSLeftOut;
        electronicSLeftIn --> electronicNLeftOut;

        electronicGatewayIn --> { @display("ls=blue,0"); } --> GatewayNE.elIn[4];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- GatewayNE.elOut[4];
}

module TorusNXTileSW_Regular
{
    parameters:
        int nodeId;

        int numX;
        int numY;
        double coreSizeX;
        double coreSizeY;
        volatile double disp_Width;
        double PathSeparation;
        
        double PositionLeftX = default((nodeId%numX)*coreSizeX);
        double PositionBottomY = default((nodeId/numX)*coreSizeY);
    gates:
        inout photonicNLeftIn;
        inout photonicNRightIn;
        inout photonicSLeftIn;
        inout photonicSRightIn;
        inout photonicETopIn;
        inout photonicEBottomIn;
        inout photonicWTopIn;
        inout photonicWBottomIn;
        inout photonicNLeftOut;
        inout photonicNRightOut;
        inout photonicSLeftOut;
        inout photonicSRightOut;
        inout photonicETopOut;
        inout photonicEBottomOut;
        inout photonicWTopOut;
        inout photonicWBottomOut;

        inout photonicGatewayIn;
        inout photonicGatewayOut;
        inout photonicMemoryIn;
        inout photonicMemoryOut;

        input electronicNLeftIn;
        input electronicNRightIn;
        input electronicSLeftIn;
        input electronicSRightIn;
        input electronicETopIn;
        input electronicEBottomIn;
        input electronicWTopIn;
        input electronicWBottomIn;
        output electronicNLeftOut;
        output electronicNRightOut;
        output electronicSLeftOut;
        output electronicSRightOut;
        output electronicETopOut;
        output electronicEBottomOut;
        output electronicWTopOut;
        output electronicWBottomOut;

        input electronicGatewayIn;
        output electronicGatewayOut;
        input electronicMemoryIn;
        output electronicMemoryOut;


    submodules:
        CrossNE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=427,100;b=48,48,,,,1");
        }

        Sw4x4SW: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "NonBlockingSwitch4x4Regular";
             
                elRouter = 70;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=149,319");
        }

        CrossSE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=427,377;b=48,48,,,,1");
        }
        GatewayNW: HybridRouter {
            parameters:
                id = string(nodeId) + ".";
                optSwitch = "TorusNXGateway";
              
                elRouter = 71;
                level = "NET";
                numX = numX;
                numY = numY;
                @display("p=110,100");
        }



        LineVertTopW: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=110,29");
        }
        LineVertMiddleW: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 100;
                @display("p=110,192");
        }
        LineVertBottomW: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=110,452");
        }

        LineVertTopE: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=427,29");
        }
        LineVertMiddleE: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY - 860) + 190 + 190;
                @display("p=427,266");
        }
        LineVertBottomE: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=427,444");
        }

        LineHorizLeftN: LineBidirectional {
            parameters:
                Length_Line = 100;
                @display("p=24,100");
        }
        LineHorizMiddleN: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 100 + 190;
                @display("p=270,100");
        }
        LineHorizRightN: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=506,100");
        }

        LineHorizLeftS: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=24,377");
        }
        LineHorizMiddleS: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX - 860) + 190;
                @display("p=270,377");
        }
        LineHorizRightS: LineBidirectional {
            parameters:
                Length_Line = 190;
                @display("p=506,377");
        }

    connections allowunconnected:
        //Direction convention
        // Up = A
        // Down = B
        // Left = A
        // Right = B
        CrossNE.photonicInN <--> LineVertTopE.photonicOutB;
        CrossNE.photonicInS <--> LineVertMiddleE.photonicOutA;
        CrossNE.photonicInE <--> LineHorizRightN.photonicOutA;
        CrossNE.photonicInW <--> LineHorizMiddleN.photonicOutB;

        CrossNE.photonicOutN <--> LineVertTopE.photonicInA;
        CrossNE.photonicOutS <--> LineVertMiddleE.photonicInB;
        CrossNE.photonicOutE <--> LineHorizRightN.photonicInB;
        CrossNE.photonicOutW <--> LineHorizMiddleN.photonicInA;

        GatewayNW.phIn[0] <--> LineVertTopW.photonicOutB;
        GatewayNW.phIn[2] <--> LineVertMiddleW.photonicOutA;
        GatewayNW.phIn[1] <--> LineHorizMiddleN.photonicOutA;
        GatewayNW.phIn[3] <--> LineHorizLeftN.photonicOutB;

        GatewayNW.phOut[0] <--> LineVertTopW.photonicInA;
        GatewayNW.phOut[2] <--> LineVertMiddleW.photonicInB;
        GatewayNW.phOut[1] <--> LineHorizMiddleN.photonicInB;
        GatewayNW.phOut[3] <--> LineHorizLeftN.photonicInA;

        CrossSE.photonicInN <--> LineVertMiddleE.photonicOutB;
        CrossSE.photonicInS <--> LineVertBottomE.photonicOutA;
        CrossSE.photonicInE <--> LineHorizRightS.photonicOutA;
        CrossSE.photonicInW <--> LineHorizMiddleS.photonicOutB;

        CrossSE.photonicOutN <--> LineVertMiddleE.photonicInA;
        CrossSE.photonicOutS <--> LineVertBottomE.photonicInB;
        CrossSE.photonicOutE <--> LineHorizRightS.photonicInB;
        CrossSE.photonicOutW <--> LineHorizMiddleS.photonicInA;

        Sw4x4SW.phIn[0] <--> LineVertMiddleW.photonicOutB;
        Sw4x4SW.phIn[2] <--> LineVertBottomW.photonicOutA;
        Sw4x4SW.phIn[1] <--> LineHorizMiddleS.photonicOutA;
        Sw4x4SW.phIn[3] <--> LineHorizLeftS.photonicOutB;

        Sw4x4SW.phOut[0] <--> LineVertMiddleW.photonicInA;
        Sw4x4SW.phOut[2] <--> LineVertBottomW.photonicInB;
        Sw4x4SW.phOut[1] <--> LineHorizMiddleS.photonicInB;
        Sw4x4SW.phOut[3] <--> LineHorizLeftS.photonicInA;

        GatewayNW.phOut[4] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> GatewayNW.phIn[4];

        photonicNLeftIn <--> LineVertTopW.photonicInB;
        photonicNRightIn <--> LineVertTopE.photonicInB;
        photonicSLeftIn <--> LineVertBottomW.photonicInA;
        photonicSRightIn <--> LineVertBottomE.photonicInA;
        photonicETopIn <--> LineHorizRightN.photonicInA;
        photonicEBottomIn <--> LineHorizRightS.photonicInA;
        photonicWTopIn <--> LineHorizLeftN.photonicInB;
        photonicWBottomIn <--> LineHorizLeftS.photonicInB;
        photonicNLeftOut <--> LineVertTopW.photonicOutA;
        photonicNRightOut <--> LineVertTopE.photonicOutA;
        photonicSLeftOut <--> LineVertBottomW.photonicOutB;
        photonicSRightOut <--> LineVertBottomE.photonicOutB;
        photonicETopOut <--> LineHorizRightN.photonicOutB;
        photonicEBottomOut <--> LineHorizRightS.photonicOutB;
        photonicWTopOut <--> LineHorizLeftN.photonicOutA;
        photonicWBottomOut <--> LineHorizLeftS.photonicOutA;


        // Electronic Connections

        GatewayNW.elIn[0] <-- electronicNLeftIn;
        GatewayNW.elIn[1] <-- electronicETopIn;
        GatewayNW.elIn[2] <-- ElectronicChannel <-- Sw4x4SW.elOut[0];
        GatewayNW.elIn[3] <-- electronicWTopIn;

        GatewayNW.elOut[0] --> electronicNLeftOut;
        GatewayNW.elOut[1] --> electronicETopOut;
        GatewayNW.elOut[2] --> ElectronicChannel --> Sw4x4SW.elIn[0];
        GatewayNW.elOut[3] --> electronicWTopOut;

        Sw4x4SW.elIn[1] <-- electronicEBottomIn;
        Sw4x4SW.elIn[2] <-- electronicSLeftIn;
        Sw4x4SW.elIn[3] <-- electronicWBottomIn;

        Sw4x4SW.elOut[1] --> electronicEBottomOut;
        Sw4x4SW.elOut[2] --> electronicSLeftOut;
        Sw4x4SW.elOut[3] --> electronicWBottomOut;

        electronicNRightIn --> electronicSRightOut;
        electronicSRightIn --> electronicNRightOut;

        electronicGatewayIn --> { @display("ls=blue,0"); } --> GatewayNW.elIn[4];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- GatewayNW.elOut[4];
}
