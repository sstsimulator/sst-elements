// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Johnnie Chan 2008                           *
// *                                                                       *
// * file: torustile.ned			                                       *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package topologies.blockingTorus;


import photonic.devices.LineBidirectional;
import photonic.devices.BendBidirectional;
import photonic.devices.CrossBidirectional;
import topologies.blockingTorus.*;
import photonic.devices.CrossDouble;
import photonic.devices.Line;

import electronicComponents.ElectronicChannel;

import chipComponents.HybridRouter;


module TorusTileNW
{
    parameters:
        int nodeId;

        int level;
        string networkPrefix;

        int numX;
        int numY;

        bool useIOplane;

        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;
        double BarDelay_2x2;

        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Bend;
        double Latency_Cross;

        double coreSizeX;
        double coreSizeY;

        volatile double disp_Width;

        int numOfWavelengthChannels;



        @display("bgb=660,660");
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

        inout photonicNLeftInjectionIn;
        inout photonicNRightInjectionIn;
        inout photonicSLeftInjectionIn;
        inout photonicSRightInjectionIn;
        inout photonicWEjectionIn;
        inout photonicNLeftInjectionOut;
        inout photonicNRightInjectionOut;
        inout photonicSLeftInjectionOut;
        inout photonicSRightInjectionOut;
        inout photonicWEjectionOut;

        inout photonicGatewayIn;
        inout photonicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout source;


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

        input electronicNLeftInjectionIn;
        input electronicNRightInjectionIn;
        input electronicSLeftInjectionIn;
        input electronicSRightInjectionIn;
        input electronicWTopEjectionIn;
        input electronicWBottomEjectionIn;
        output electronicNLeftInjectionOut;
        output electronicNRightInjectionOut;
        output electronicSLeftInjectionOut;
        output electronicSRightInjectionOut;
        output electronicWTopEjectionOut;
        output electronicWBottomEjectionOut;

        input electronicGatewayIn;
        output electronicGatewayOut;

        input electronicMemoryIn;
        output electronicMemoryOut;






    submodules:
        CrossA: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=376,352;b=48,48,,,,1");
        }
        CrossB: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=536,352;b=48,48,,,,1");
        }
        CrossC: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=376,512;b=48,48,,,,1");
        }
        CrossGAPX: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=80,352;b=48,48,,,,1");
        }
        CrossGAPY: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=288,512;b=48,48,,,,1");
        }
        CrossGAPZ: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=80,512;b=48,48,,,,1");
        }
        Sw4x4: BlockingTorusNodeRouter {
            numX = numX;
            numY = numY;
            nodeId = nodeId;

            networkPrefix = networkPrefix;

            type = 13;
            @display("i=components/4x4old;p=496,461");
        }
        GatewaySw: BlockingTorusGatewayRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            @display("p=81,144;i=components/gw_sw");
        }
        Gateway: HybridRouter {
            parameters:
                id = networkPrefix + string(nodeId) + ".";

                optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");

                elRouter = 1;

                level = "NET";

                numX = numX;
                numY = numY;

                @display("p=182,144");
        }
        Injection: BlockingTorusInjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 11;
            @display("p=288,352;i=components/in_sw");
        }
        Ejection: BlockingTorusEjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 10;
            @display("p=376,264;i=components/ej_sw");
        }
        LineMainVertA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+325;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=376,130;b=24,166,,,,1");
        }
        LineMainVertB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+435;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=536,178;b=24,262,,,,1");
        }
        LineMainHorizA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=182,352;b=110,24,,,,1");
        }
        LineMainHorizB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+145;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,512;b=118,24,,,,1");
        }
        LinePartA: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=376,432;b=24,72,,,,1");
        }
        LinePartB: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=456,352;b=72,24,,,,1");
        }
        LineSmallA: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=615,352;b=60,24,,,,1");
        }
        LineSmallB: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=376,591;b=24,60,,,,1");
        }
        LineExtraSmallA: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=611,472;b=24,30,,,,1");
        }
        LineExtraSmallB: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=496,587;b=30,24,,,,1");
        }
        LineGAPInject: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+355;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=288,135;b=24,188,,,,1");
        }
        LineGAPGateway: LineBidirectional {
            parameters:
                Length_Line = 195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=80,211;b=24,36,,,,1");
        }
        LineGAPGatewayShort: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=80,307;b=24,20,,,,1");
        }
        LineGAPGatewayExtend: LineBidirectional {
            parameters:
                Length_Line = coreSizeY-1000;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=80,67;b=24,52,,,,1");
        }
        LineGAPInjectShort: LineBidirectional {
            parameters:
                Length_Line = 60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=288,296;b=24,12,,,,1");
        }
        LineGAPGatewayConnect: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=80,431;b=24,76,,,,1");
        }
        LineGAPInjectConnect: LineBidirectional {
            parameters:
                Length_Line = 160;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=288,439;b=24,60,,,,1");
        }
        LineGAPHorizShortB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=332,511;b=16,24,,,,1");
        }
        LineGAPGatewaySmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=80,591;b=24,60,,,,1");
        }
        LineGAPInjectSmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=288,591;b=24,60,,,,1");
        }
        LineGAPUniA: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=330,264;b=12,10,,,,1");
        }
        LineGAPUniB: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+145;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,264;b=128,8,,,,1");
        }
        CrossDoubleA: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=80,264;b=48,32,,,,1");
        }
        CrossDoubleB: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=288,264;b=48,32,,,,1");
        }
        TurnA: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=611,422;b=40,40,,,,1");
        }
        TurnB: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=611,522;b=40,40,,,,1");
        }
        TurnC: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=446,587;b=40,40,,,,1");
        }
        TurnD: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=546,587;b=40,40,,,,1");
        }
    connections allowunconnected:

        //
        // Photonic Connections
        //



        Sw4x4.photonicOutN <--> CrossB.photonicInS;
        Sw4x4.photonicOutS <--> TurnC.photonicInnerBendIn;
        Sw4x4.photonicOutE <--> TurnA.photonicInnerBendIn;
        Sw4x4.photonicOutW <--> CrossC.photonicInE;

        CrossB.photonicOutS <--> Sw4x4.photonicInN;
        TurnC.photonicOuterBendOut <--> Sw4x4.photonicInS;
        TurnA.photonicOuterBendOut <--> Sw4x4.photonicInE;
        CrossC.photonicOutE <--> Sw4x4.photonicInW;

        CrossA.photonicOutN <--> Ejection.photonicInS;
        CrossA.photonicOutS <--> LinePartA.photonicInB;
        CrossA.photonicOutE <--> LinePartB.photonicInB;
        CrossA.photonicOutW <--> Injection.photonicInE;

        Ejection.photonicOutS <--> CrossA.photonicInN;
        LinePartA.photonicOutA <--> CrossA.photonicInS;
        LinePartB.photonicOutA <--> CrossA.photonicInE;
        Injection.photonicOutE <--> CrossA.photonicInW;

        CrossB.photonicOutN <--> LineMainVertB.photonicInA;
        CrossB.photonicOutE <--> LineSmallA.photonicInB;
        CrossB.photonicOutW <--> LinePartB.photonicInA;

        LineMainVertB.photonicOutB <--> CrossB.photonicInN;
        LineSmallA.photonicOutA <--> CrossB.photonicInE;
        LinePartB.photonicOutB <--> CrossB.photonicInW;

        CrossC.photonicOutN <--> LinePartA.photonicInA;
        CrossC.photonicOutS <--> LineSmallB.photonicInB;
        CrossC.photonicOutW <--> LineGAPHorizShortB.photonicInA;

        LinePartA.photonicOutB <--> CrossC.photonicInN;
        LineSmallB.photonicOutA <--> CrossC.photonicInS;
        LineGAPHorizShortB.photonicOutB <--> CrossC.photonicInW;

        TurnA.photonicInnerBendOut <--> LineExtraSmallA.photonicInB;
        LineExtraSmallA.photonicOutB <--> TurnB.photonicOuterBendIn;
        TurnB.photonicOuterBendOut <--> photonicEBottomOut;
        photonicEBottomIn <--> TurnB.photonicInnerBendIn;
        TurnB.photonicInnerBendOut <--> LineExtraSmallA.photonicInA;
        LineExtraSmallA.photonicOutA <--> TurnA.photonicOuterBendIn;

        TurnC.photonicInnerBendOut <--> LineExtraSmallB.photonicInB;
        LineExtraSmallB.photonicOutB <--> TurnD.photonicOuterBendIn;
        TurnD.photonicOuterBendOut <--> photonicSRightOut;
        photonicSRightIn <--> TurnD.photonicInnerBendIn;
        TurnD.photonicInnerBendOut <--> LineExtraSmallB.photonicInA;
        LineExtraSmallB.photonicOutA <--> TurnC.photonicOuterBendIn;

        Injection.photonicOutN <--> LineGAPInjectShort.photonicInA;
        Injection.photonicOutS <--> LineGAPInjectConnect.photonicInB;
        Injection.photonicOutW <--> LineMainHorizA.photonicInA;

        LineGAPInjectShort.photonicOutB <--> Injection.photonicInN;
        LineGAPInjectConnect.photonicOutA <--> Injection.photonicInS;
        LineMainHorizA.photonicOutB <--> Injection.photonicInW;

        Ejection.photonicOutGateway <--> LineGAPUniA.photonicIn;
        Ejection.photonicOutN <--> LineMainVertA.photonicInA;

        LineMainVertA.photonicOutB <--> Ejection.photonicInN;

        GatewaySw.photonicOutInjectionN <--> { @display("m=n"); } <--> LineGAPGatewayExtend.photonicInA;
        GatewaySw.photonicOutInjectionS <--> LineGAPGateway.photonicInB;
        GatewaySw.photonicOutGateway <--> Gateway.phIn[1];




        Gateway.phOut[0] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> Gateway.phIn[0];

        Gateway.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut if useIOplane;
        photonicMemoryIn <--> { @display("ls=,0"); } <--> Gateway.phIn[2] if useIOplane;

        GatewaySw.electronicOutGateway --> ElectronicChannel --> Gateway.elIn[1];
        GatewaySw.electronicInGateway <-- ElectronicChannel <-- Gateway.elOut[1];

        electronicGatewayIn --> { @display("ls=blue,0"); } --> Gateway.elIn[0];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[0];


        electronicMemoryIn --> { @display("ls=blue,0"); } --> Gateway.elIn[2] if useIOplane;
        electronicMemoryOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[2] if useIOplane;



        LineGAPGatewayExtend.photonicOutB <--> { @display("m=n"); } <--> GatewaySw.photonicInInjectionN;
        LineGAPGateway.photonicOutA <--> GatewaySw.photonicInInjectionS;

        Gateway.phOut[1] <--> { @display("ls=,0"); } <--> GatewaySw.photonicInGateway;
        photonicWEjectionIn <--> { @display("m=w"); } <--> GatewaySw.photonicInEjection;

        photonicNLeftInjectionIn <--> LineGAPGatewayExtend.photonicInB;
        LineGAPGatewayExtend.photonicOutA <--> photonicNLeftInjectionOut;

        CrossGAPX.photonicOutN <--> LineGAPGatewayShort.photonicInA;
        CrossGAPX.photonicOutS <--> LineGAPGatewayConnect.photonicInB;
        CrossGAPX.photonicOutE <--> LineMainHorizA.photonicInB;
        CrossGAPX.photonicOutW <--> photonicWTopOut;

        LineGAPGatewayShort.photonicOutB <--> CrossGAPX.photonicInN;
        LineGAPGatewayConnect.photonicOutA <--> CrossGAPX.photonicInS;
        LineMainHorizA.photonicOutA <--> CrossGAPX.photonicInE;
        photonicWTopIn <--> CrossGAPX.photonicInW;

        CrossGAPY.photonicOutN <--> LineGAPInjectConnect.photonicInA;
        CrossGAPY.photonicOutS <--> LineGAPInjectSmall.photonicInB;
        CrossGAPY.photonicOutE <--> LineGAPHorizShortB.photonicInB;
        CrossGAPY.photonicOutW <--> LineMainHorizB.photonicInA;

        LineGAPInjectConnect.photonicOutB <--> CrossGAPY.photonicInN;
        LineGAPInjectSmall.photonicOutA <--> CrossGAPY.photonicInS;
        LineGAPHorizShortB.photonicOutA <--> CrossGAPY.photonicInE;
        LineMainHorizB.photonicOutB <--> CrossGAPY.photonicInW;

        CrossGAPZ.photonicOutN <--> LineGAPGatewayConnect.photonicInA;
        CrossGAPZ.photonicOutS <--> LineGAPGatewaySmall.photonicInB;
        CrossGAPZ.photonicOutE <--> LineMainHorizB.photonicInB;
        CrossGAPZ.photonicOutW <--> photonicWBottomOut;

        LineGAPGatewayConnect.photonicOutB <--> CrossGAPZ.photonicInN;
        LineGAPGatewaySmall.photonicOutA <--> CrossGAPZ.photonicInS;
        LineMainHorizB.photonicOutA <--> CrossGAPZ.photonicInE;
        photonicWBottomIn <--> CrossGAPZ.photonicInW;

        CrossDoubleA.photonicOutA <--> LineGAPGateway.photonicInA;
        CrossDoubleA.photonicOutB <--> LineGAPGatewayShort.photonicInB;
        CrossDoubleA.photonicOutSide <--> photonicWEjectionOut;

        LineGAPGatewayShort.photonicOutA <--> CrossDoubleA.photonicInA;
        LineGAPGateway.photonicOutB <--> CrossDoubleA.photonicInB;
        LineGAPUniB.photonicOut <--> CrossDoubleA.photonicInSide;

        CrossDoubleB.photonicOutA <--> LineGAPInject.photonicInA;
        CrossDoubleB.photonicOutB <--> LineGAPInjectShort.photonicInB;
        CrossDoubleB.photonicOutSide <--> LineGAPUniB.photonicIn;

        LineGAPInjectShort.photonicOutA <--> CrossDoubleB.photonicInA;
        LineGAPInject.photonicOutB <--> CrossDoubleB.photonicInB;
        LineGAPUniA.photonicOut <--> CrossDoubleB.photonicInSide;


        LineMainVertA.photonicOutA <--> photonicNLeftOut;
        photonicNLeftIn <--> LineMainVertA.photonicInB;

        LineMainVertB.photonicOutA <--> photonicNRightOut;
        photonicNRightIn <--> LineMainVertB.photonicInB;

        LineSmallA.photonicOutB <--> photonicETopOut;
        photonicETopIn <--> LineSmallA.photonicInA;

        LineSmallB.photonicOutB <--> photonicSLeftOut;
        photonicSLeftIn <--> LineSmallB.photonicInA;

        LineGAPInject.photonicOutA <--> photonicNRightInjectionOut;
        photonicNRightInjectionIn <--> LineGAPInject.photonicInB;

        LineGAPGatewaySmall.photonicOutB <--> { @display("m=s"); } <--> photonicSLeftInjectionOut;
        photonicSLeftInjectionIn <--> { @display("m=s"); } <--> LineGAPGatewaySmall.photonicInA;

        LineGAPInjectSmall.photonicOutB <--> photonicSRightInjectionOut;
        photonicSRightInjectionIn <--> LineGAPInjectSmall.photonicInA;

        //
        // Electronic Connections
        //





        electronicNLeftIn --> { @display("m=n;ls=red,0"); } --> Ejection.electronicInN;
        electronicNRightIn --> { @display("m=n;ls=red,0"); } --> Sw4x4.electronicInN;
        electronicSLeftIn --> { @display("m=s;ls=red,0"); } --> Ejection.electronicInS;
        electronicSRightIn --> { @display("m=s;ls=red,0"); } --> Sw4x4.electronicInS;
        electronicETopIn --> { @display("m=e;ls=red,0"); } --> Injection.electronicInE;
        electronicEBottomIn --> { @display("m=e;ls=red,0"); } --> Sw4x4.electronicInE;
        electronicWTopIn --> { @display("m=w;ls=red,0"); } --> Injection.electronicInW;
        electronicWBottomIn --> { @display("m=w;ls=red,0"); } --> Sw4x4.electronicInW;

        electronicNLeftOut <-- { @display("m=n;ls=red,0"); } <-- Ejection.electronicOutN;
        electronicNRightOut <-- { @display("m=n;ls=red,0"); } <-- Sw4x4.electronicOutN;
        electronicSLeftOut <-- { @display("m=s;ls=red,0"); } <-- Ejection.electronicOutS;
        electronicSRightOut <-- { @display("m=s;ls=red,0"); } <-- Sw4x4.electronicOutS;
        electronicETopOut <-- { @display("m=e;ls=red,0"); } <-- Injection.electronicOutE;
        electronicEBottomOut <-- { @display("m=e;ls=red,0"); } <-- Sw4x4.electronicOutE;
        electronicWTopOut <-- { @display("m=w;ls=red,0"); } <-- Injection.electronicOutW;
        electronicWBottomOut <-- { @display("m=w;ls=red,0"); } <-- Sw4x4.electronicOutW;

        electronicNLeftInjectionIn --> { @display("m=n;ls=red,0"); } --> GatewaySw.electronicInInjectionN;
        electronicNRightInjectionIn --> { @display("m=n;ls=red,0"); } --> Injection.electronicInN;
        electronicSLeftInjectionIn --> { @display("m=s;ls=red,0"); } --> GatewaySw.electronicInInjectionS;
        electronicSRightInjectionIn --> { @display("m=s;ls=red,0"); } --> Injection.electronicInS;
        electronicWTopEjectionIn --> { @display("m=w;ls=red,0"); } --> GatewaySw.electronicInEjection;
        electronicWBottomEjectionIn --> { @display("m=w;ls=red,0"); } --> Ejection.electronicInGateway;
        electronicNLeftInjectionOut <-- { @display("m=n;ls=red,0"); } <-- GatewaySw.electronicOutInjectionN;
        electronicNRightInjectionOut <-- { @display("m=n;ls=red,0"); } <-- Injection.electronicOutN;
        electronicSLeftInjectionOut <-- { @display("m=s;ls=red,0"); } <-- GatewaySw.electronicOutInjectionS;
        electronicSRightInjectionOut <-- { @display("m=s;ls=red,0"); } <-- Injection.electronicOutS;
        electronicWTopEjectionOut <-- { @display("m=w;ls=red,0"); } <-- GatewaySw.electronicOutEjection;
        electronicWBottomEjectionOut <-- { @display("m=w;ls=red,0"); } <-- Ejection.electronicOutGateway;




}

module TorusTileSE
{
    parameters:
        int nodeId;

        int level;
        string networkPrefix;

        int numX;
        int numY;

        bool useIOplane;


        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;
        double BarDelay_2x2;

        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Bend;
        double Latency_Cross;

        double coreSizeX;
        double coreSizeY;

        volatile double disp_Width;

        int numOfWavelengthChannels;



        @display("bgb=660,660");
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

        inout photonicNLeftInjectionIn;
        inout photonicNRightInjectionIn;
        inout photonicSLeftInjectionIn;
        inout photonicSRightInjectionIn;
        inout photonicEEjectionIn;
        inout photonicNLeftInjectionOut;
        inout photonicNRightInjectionOut;
        inout photonicSLeftInjectionOut;
        inout photonicSRightInjectionOut;
        inout photonicEEjectionOut;

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

        input electronicNLeftInjectionIn;
        input electronicNRightInjectionIn;
        input electronicSLeftInjectionIn;
        input electronicSRightInjectionIn;
        input electronicETopEjectionIn;
        input electronicEBottomEjectionIn;
        output electronicNLeftInjectionOut;
        output electronicNRightInjectionOut;
        output electronicSLeftInjectionOut;
        output electronicSRightInjectionOut;
        output electronicETopEjectionOut;
        output electronicEBottomEjectionOut;

        input electronicGatewayIn;
        output electronicGatewayOut;

        input electronicMemoryIn;
        output electronicMemoryOut;

    submodules:
        CrossA: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=308,308;b=48,48,,,,1");
        }
        CrossB: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=148,308;b=48,48,,,,1");
        }
        CrossC: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=308,148;b=48,48,,,,1");
        }
        CrossGAPX: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=588,308;b=48,48,,,,1");
        }
        CrossGAPY: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=396,148;b=48,48,,,,1");
        }
        CrossGAPZ: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=588,148;b=48,48,,,,1");
        }
        Sw4x4: BlockingTorusNodeRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 13;
            @display("i=components/4x4old;p=193,188");
        }
        GatewaySw: BlockingTorusGatewayRouterRight {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            @display("p=588,511;i=components/gw_sw");
        }
        Gateway: HybridRouter {
            parameters:
                id = networkPrefix + string(nodeId) + ".";

                optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");

                elRouter = 1;

                level = "NET";

                numX = numX;
                numY = numY;

        }
        Injection: BlockingTorusInjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 11;
            @display("p=396,316;i=components/in_sw");
        }
        Ejection: BlockingTorusEjectionRouterRight {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 10;
            @display("p=308,396;i=components/ej_sw");
        }
        LineMainVertA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+435;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=148,486;b=24,270,,,,1");
        }
        LineMainVertB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+325;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=308,534;b=24,182,,,,1");
        }
        LineMainHorizA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+145;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=492,148;b=102,24,,,,1");
        }
        LineMainHorizB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=494,308;b=94,24,,,,1");
        }
        LinePartA: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=308,228;b=24,72,,,,1");
        }
        LinePartB: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=228,308;b=72,24,,,,1");
        }
        LineSmallA: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=69,308;b=60,24,,,,1");
        }
        LineSmallB: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=308,69;b=24,60,,,,1");
        }
        LineExtraSmallA: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=73,188;b=24,30,,,,1");
        }
        LineExtraSmallB: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=188,73;b=30,24,,,,1");
        }
        LineGAPInject: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+355;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=396,529;b=24,204,,,,1");
        }
        LineGAPGateway: LineBidirectional {
            parameters:
                Length_Line = 195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=588,449;b=24,44,,,,1");
        }
        LineGAPGatewayShort: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=588,353;b=24,12,,,,1");
        }
        LineGAPInjectShort: LineBidirectional {
            parameters:
                Length_Line = 60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=396,364;b=24,12,,,,1");
        }
        LineGAPGatewayExtend: LineBidirectional {
            parameters:
                Length_Line = coreSizeY-1000;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=584,595;b=24,52,,,,1");
        }
        LineGAPGatewayConnect: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=588,229;b=24,60,,,,1");
        }
        LineGAPInjectConnect: LineBidirectional {
            parameters:
                Length_Line = 160;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=396,221;b=24,76,,,,1");
        }
        LineGAPHorizShortA: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=352,149;b=16,24,,,,1");
        }
        LineGAPGatewaySmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=588,69;b=24,60,,,,1");
        }
        LineGAPInjectSmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=396,69;b=24,60,,,,1");
        }
        LineGAPUniA: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=354,396;b=12,8,,,,1");
        }
        LineGAPUniB: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+145;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=492,396;b=112,8,,,,1");
        }
        CrossDoubleA: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=588,396;b=48,32,,,,1");
        }
        CrossDoubleB: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=396,396;b=48,32,,,,1");
        }
        TurnA: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=73,238;b=40,40,,,,1");
        }
        TurnB: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=73,138;b=40,40,,,,1");
        }
        TurnC: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=238,73;b=40,40,,,,1");
        }
        TurnD: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=138,73;b=40,40,,,,1");
        }
    connections allowunconnected:
        Sw4x4.photonicOutN <--> TurnC.photonicInnerBendIn;
        Sw4x4.photonicOutS <--> CrossB.photonicInN;
        Sw4x4.photonicOutE <--> CrossC.photonicInW;
        Sw4x4.photonicOutW <--> TurnB.photonicInnerBendIn;

        TurnC.photonicOuterBendOut <--> Sw4x4.photonicInN;
        CrossB.photonicOutN <--> Sw4x4.photonicInS;
        CrossC.photonicOutW <--> Sw4x4.photonicInE;
        TurnB.photonicOuterBendOut <--> Sw4x4.photonicInW;

        CrossA.photonicOutN <--> LinePartA.photonicInA;
        CrossA.photonicOutS <--> Ejection.photonicInN;
        CrossA.photonicOutE <--> Injection.photonicInW;
        CrossA.photonicOutW <--> LinePartB.photonicInA;

        LinePartA.photonicOutB <--> CrossA.photonicInN;
        Ejection.photonicOutN <--> CrossA.photonicInS;
        Injection.photonicOutW <--> CrossA.photonicInE;
        LinePartB.photonicOutB <--> CrossA.photonicInW;

        CrossB.photonicOutS <--> LineMainVertA.photonicInB;
        CrossB.photonicOutE <--> LinePartB.photonicInB;
        CrossB.photonicOutW <--> LineSmallA.photonicInA;

        LineMainVertA.photonicOutA <--> CrossB.photonicInS;
        LinePartB.photonicOutA <--> CrossB.photonicInE;
        LineSmallA.photonicOutB <--> CrossB.photonicInW;

        CrossC.photonicOutN <--> LineSmallB.photonicInA;
        CrossC.photonicOutS <--> LinePartA.photonicInB;
        CrossC.photonicOutE <--> LineGAPHorizShortA.photonicInB;

        LineSmallB.photonicOutB <--> CrossC.photonicInN;
        LinePartA.photonicOutA <--> CrossC.photonicInS;
        LineGAPHorizShortA.photonicOutA <--> CrossC.photonicInE;


        photonicWTopIn <--> TurnA.photonicInnerBendIn;
        TurnA.photonicInnerBendOut <--> LineExtraSmallA.photonicInB;
        LineExtraSmallA.photonicOutB <--> TurnB.photonicOuterBendIn;
        TurnB.photonicInnerBendOut <--> LineExtraSmallA.photonicInA;
        LineExtraSmallA.photonicOutA <--> TurnA.photonicOuterBendIn;
        TurnA.photonicOuterBendOut <--> photonicWTopOut;

        photonicNLeftIn <--> TurnD.photonicInnerBendIn;
        TurnD.photonicInnerBendOut <--> LineExtraSmallB.photonicInB;
        LineExtraSmallB.photonicOutB <--> TurnC.photonicOuterBendIn;
        TurnC.photonicInnerBendOut <--> LineExtraSmallB.photonicInA;
        LineExtraSmallB.photonicOutA <--> TurnD.photonicOuterBendIn;
        TurnD.photonicOuterBendOut <--> photonicNLeftOut;

        Injection.photonicOutN <--> LineGAPInjectConnect.photonicInA;
        Injection.photonicOutS <--> LineGAPInjectShort.photonicInB;
        Injection.photonicOutE <--> LineMainHorizB.photonicInB;

        LineGAPInjectConnect.photonicOutB <--> Injection.photonicInN;
        LineGAPInjectShort.photonicOutA <--> Injection.photonicInS;
        LineMainHorizB.photonicOutA <--> Injection.photonicInE;

        Ejection.photonicOutS <--> LineMainVertB.photonicInB;
        Ejection.photonicOutGateway <--> LineGAPUniA.photonicIn;

        LineMainVertB.photonicOutA <--> Ejection.photonicInS;

        GatewaySw.photonicOutInjectionN <--> LineGAPGateway.photonicInA;
        GatewaySw.photonicOutInjectionS <--> LineGAPGatewayExtend.photonicInB;

        GatewaySw.photonicOutGateway <--> Gateway.phIn[1];




        Gateway.phOut[0] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> { @display("ls=,0"); } <--> Gateway.phIn[0];

        Gateway.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut if useIOplane;
        photonicMemoryIn <--> { @display("ls=,0"); } <--> Gateway.phIn[2] if useIOplane;

        GatewaySw.electronicOutGateway --> ElectronicChannel --> Gateway.elIn[1];
        GatewaySw.electronicInGateway <-- ElectronicChannel <-- Gateway.elOut[1];

        electronicGatewayIn --> { @display("ls=blue,0"); } --> Gateway.elIn[0];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[0];


        electronicMemoryIn --> { @display("ls=blue,0"); } --> Gateway.elIn[2] if useIOplane;
        electronicMemoryOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[2] if useIOplane;




        LineGAPGateway.photonicOutB <--> GatewaySw.photonicInInjectionN;
        LineGAPGatewayExtend.photonicOutA <--> GatewaySw.photonicInInjectionS;
        photonicEEjectionIn <--> GatewaySw.photonicInEjection;

        Gateway.phOut[1] <--> { @display("ls=,0"); } <--> GatewaySw.photonicInGateway;


        LineGAPGatewayExtend.photonicOutB <--> photonicSRightInjectionOut;
        photonicSRightInjectionIn <--> LineGAPGatewayExtend.photonicInA;

        CrossGAPX.photonicOutN <--> LineGAPGatewayConnect.photonicInA;
        CrossGAPX.photonicOutS <--> LineGAPGatewayShort.photonicInB;
        CrossGAPX.photonicOutE <--> photonicEBottomOut;
        CrossGAPX.photonicOutW <--> LineMainHorizB.photonicInA;

        LineGAPGatewayConnect.photonicOutB <--> CrossGAPX.photonicInN;
        LineGAPGatewayShort.photonicOutA <--> CrossGAPX.photonicInS;
        photonicEBottomIn <--> CrossGAPX.photonicInE;
        LineMainHorizB.photonicOutB <--> CrossGAPX.photonicInW;

        CrossGAPY.photonicOutN <--> LineGAPInjectSmall.photonicInA;
        CrossGAPY.photonicOutS <--> LineGAPInjectConnect.photonicInB;
        CrossGAPY.photonicOutE <--> LineMainHorizA.photonicInB;
        CrossGAPY.photonicOutW <--> LineGAPHorizShortA.photonicInA;

        LineGAPInjectConnect.photonicOutA <--> CrossGAPY.photonicInS;
        LineGAPInjectSmall.photonicOutB <--> CrossGAPY.photonicInN;
        LineMainHorizA.photonicOutA <--> CrossGAPY.photonicInE;
        LineGAPHorizShortA.photonicOutB <--> CrossGAPY.photonicInW;

        CrossGAPZ.photonicOutN <--> LineGAPGatewaySmall.photonicInA;
        CrossGAPZ.photonicOutS <--> LineGAPGatewayConnect.photonicInB;
        CrossGAPZ.photonicOutE <--> photonicETopOut;
        CrossGAPZ.photonicOutW <--> LineMainHorizA.photonicInA;

        LineGAPGatewaySmall.photonicOutB <--> CrossGAPZ.photonicInN;
        LineGAPGatewayConnect.photonicOutA <--> CrossGAPZ.photonicInS;
        photonicETopIn <--> CrossGAPZ.photonicInE;
        LineMainHorizA.photonicOutB <--> CrossGAPZ.photonicInW;

        CrossDoubleA.photonicOutA <--> LineGAPGatewayShort.photonicInA;
        CrossDoubleA.photonicOutB <--> LineGAPGateway.photonicInB;
        CrossDoubleA.photonicOutSide <--> photonicEEjectionOut;

        LineGAPGatewayShort.photonicOutB <--> CrossDoubleA.photonicInB;
        LineGAPGateway.photonicOutA <--> CrossDoubleA.photonicInA;
        LineGAPUniB.photonicOut <--> CrossDoubleA.photonicInSide;

        CrossDoubleB.photonicOutA <--> LineGAPInjectShort.photonicInA;
        CrossDoubleB.photonicOutB <--> LineGAPInject.photonicInB;
        CrossDoubleB.photonicOutSide <--> LineGAPUniB.photonicIn;

        LineGAPInject.photonicOutA <--> CrossDoubleB.photonicInA;
        LineGAPInjectShort.photonicOutB <--> CrossDoubleB.photonicInB;
        LineGAPUniA.photonicOut <--> CrossDoubleB.photonicInSide;


        LineMainVertA.photonicOutB <--> photonicSLeftOut;
        photonicSLeftIn <--> LineMainVertA.photonicInA;

        LineMainVertB.photonicOutB <--> photonicSRightOut;
        photonicSRightIn <--> LineMainVertB.photonicInA;

        LineSmallA.photonicOutA <--> photonicWBottomOut;
        photonicWBottomIn <--> LineSmallA.photonicInB;

        LineSmallB.photonicOutA <--> photonicNRightOut;
        photonicNRightIn <--> LineSmallB.photonicInB;

        LineGAPInject.photonicOutB <--> photonicSLeftInjectionOut;
        photonicSLeftInjectionIn <--> LineGAPInject.photonicInA;

        LineGAPGatewaySmall.photonicOutA <--> photonicNRightInjectionOut;
        photonicNRightInjectionIn <--> LineGAPGatewaySmall.photonicInB;

        LineGAPInjectSmall.photonicOutA <--> photonicNLeftInjectionOut;
        photonicNLeftInjectionIn <--> LineGAPInjectSmall.photonicInB;




        electronicNLeftIn --> { @display("m=n;ls=red,0"); } --> Sw4x4.electronicInN;
        electronicNRightIn --> { @display("m=n;ls=red,0"); } --> Ejection.electronicInN;
        electronicSLeftIn --> { @display("m=s;ls=red,0"); } --> Sw4x4.electronicInS;
        electronicSRightIn --> { @display("m=s;ls=red,0"); } --> Ejection.electronicInS;
        electronicETopIn --> { @display("m=e;ls=red,0"); } --> Sw4x4.electronicInE;
        electronicEBottomIn --> { @display("m=e;ls=red,0"); } --> Injection.electronicInE;
        electronicWTopIn --> { @display("m=w;ls=red,0"); } --> Sw4x4.electronicInW;
        electronicWBottomIn --> { @display("m=w;ls=red,0"); } --> Injection.electronicInW;

        electronicNLeftOut <-- { @display("m=n;ls=red,0"); } <-- Sw4x4.electronicOutN;
        electronicNRightOut <-- { @display("m=n;ls=red,0"); } <-- Ejection.electronicOutN;
        electronicSLeftOut <-- { @display("m=s;ls=red,0"); } <-- Sw4x4.electronicOutS;
        electronicSRightOut <-- { @display("m=s;ls=red,0"); } <-- Ejection.electronicOutS;
        electronicETopOut <-- { @display("m=e;ls=red,0"); } <-- Sw4x4.electronicOutE;
        electronicEBottomOut <-- { @display("m=e;ls=red,0"); } <-- Injection.electronicOutE;
        electronicWTopOut <-- { @display("m=w;ls=red,0"); } <-- Sw4x4.electronicOutW;
        electronicWBottomOut <-- { @display("m=w;ls=red,0"); } <-- Injection.electronicOutW;

        electronicNLeftInjectionIn --> { @display("m=n;ls=red,0"); } --> Injection.electronicInN;
        electronicNRightInjectionIn --> { @display("m=n;ls=red,0"); } --> GatewaySw.electronicInInjectionN;
        electronicSLeftInjectionIn --> { @display("m=s;ls=red,0"); } --> Injection.electronicInS;
        electronicSRightInjectionIn --> { @display("m=s;ls=red,0"); } --> GatewaySw.electronicInInjectionS;
        electronicETopEjectionIn --> { @display("m=e;ls=red,0"); } --> Ejection.electronicInGateway;
        electronicEBottomEjectionIn --> { @display("m=e;ls=red,0"); } --> GatewaySw.electronicInEjection;
        electronicNLeftInjectionOut <-- { @display("m=n;ls=red,0"); } <-- Injection.electronicOutN;
        electronicNRightInjectionOut <-- { @display("m=n;ls=red,0"); } <-- GatewaySw.electronicOutInjectionN;
        electronicSLeftInjectionOut <-- { @display("m=s;ls=red,0"); } <-- Injection.electronicOutS;
        electronicSRightInjectionOut <-- { @display("m=s;ls=red,0"); } <-- GatewaySw.electronicOutInjectionS;
        electronicETopEjectionOut <-- { @display("m=e;ls=red,0"); } <-- Ejection.electronicOutGateway;
        electronicEBottomEjectionOut <-- { @display("m=e;ls=red,0"); } <-- GatewaySw.electronicOutEjection;



}

module TorusTileSW
{
    parameters:
        int nodeId;

        int level;
        string networkPrefix;


        int numX;
        int numY;

        bool useIOplane;

        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;
        double BarDelay_2x2;

        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Bend;
        double Latency_Cross;

        double coreSizeX;
        double coreSizeY;

        volatile double disp_Width;


        int numOfWavelengthChannels;



        @display("bgb=660,660");
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

        inout photonicNLeftInjectionIn;
        inout photonicNRightInjectionIn;
        inout photonicSLeftInjectionIn;
        inout photonicSRightInjectionIn;
        inout photonicWEjectionIn;
        inout photonicNLeftInjectionOut;
        inout photonicNRightInjectionOut;
        inout photonicSLeftInjectionOut;
        inout photonicSRightInjectionOut;
        inout photonicWEjectionOut;

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

        input electronicNLeftInjectionIn;
        input electronicNRightInjectionIn;
        input electronicSLeftInjectionIn;
        input electronicSRightInjectionIn;
        input electronicWTopEjectionIn;
        input electronicWBottomEjectionIn;
        output electronicNLeftInjectionOut;
        output electronicNRightInjectionOut;
        output electronicSLeftInjectionOut;
        output electronicSRightInjectionOut;
        output electronicWTopEjectionOut;
        output electronicWBottomEjectionOut;

        input electronicGatewayIn;
        output electronicGatewayOut;

        input electronicMemoryIn;
        output electronicMemoryOut;



    submodules:
        CrossA: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=528,172;b=48,48,,,,1");
        }
        CrossB: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=460,172;b=48,48,,,,1");
        }
        CrossC: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=528,240;b=48,48,,,,1");
        }
        CrossGAPX: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=280,336;b=48,48,,,,1");
        }
        CrossGAPY: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=120,336;b=48,48,,,,1");
        }
        CrossGAPZ: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=280,172;b=48,48,,,,1");
        }
        Sw4x4: BlockingTorusNodeRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 13;
            @display("i=components/4x4old;p=415,290");
        }
        GatewaySw: BlockingTorusGatewayRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            @display("p=280,415;i=components/gw_sw");
        }
        Gateway: HybridRouter {
            parameters:
                id = networkPrefix + string(nodeId) + ".";

                optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");

                elRouter = 1;

                level = "NET";

                numX = numX;
                numY = numY;

        }
        Injection: BlockingTorusInjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 11;
            @display("p=120,172;i=components/in_sw");
        }
        Ejection: BlockingTorusEjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 10;
            @display("p=528,520;i=components/ej_sw");
        }
        LineMainVertA: LineBidirectional {
            parameters:
                Length_Line = 595;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=528,386;b=24,186,,,,1");
        }
        LineMainVertB: LineBidirectional {
            parameters:
                Length_Line = 405;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=368,434;b=24,102,,,,1");
        }
        LineMainVertShortB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+405;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=368,594;b=24,86,,,,1");
        }
        LineMainHorizA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+595;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=206,172;b=66,24,,,,1");
        }
        LineMainHorizShortA: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=370,172;b=90,24,,,,1");
        }
        LineMainHorizB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+405;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=202,335;b=70,24,,,,1");
        }
        LineSmallA: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=617,172;b=60,24,,,,1");
        }
        LineSmallB: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=528,97;b=24,60,,,,1");
        }
        LineExtraSmallA: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=625,290;b=24,30,,,,1");
        }
        LineExtraSmallB: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=410,88;b=30,24,,,,1");
        }
        LineStubA: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=460,129;b=24,8,,,,1");
        }
        LineStubB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=494,172;b=8,24,,,,1");
        }
        LineStubC: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=528,206;b=24,8,,,,1");
        }
        LineStubD: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=581,240;b=8,24,,,,1");
        }
        LineGAPGateway: LineBidirectional {
            parameters:
                Length_Line = 195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,467;b=24,28,,,,1");
        }
        LineGAPInject: LineBidirectional {
            parameters:
                Length_Line = 305;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=120,467;b=24,28,,,,1");
        }
        LineGAPShort: LineBidirectional {
            parameters:
                Length_Line = 0;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=120,375;b=24,20,,,,1");
        }
        LineGAPInjectExtraSmall: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=120,595;b=24,92,,,,1");
        }
        LineGAPGatewayConnect: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,259;b=24,84,,,,1");
        }
        LineGAPInjectConnect: LineBidirectional {
            parameters:
                Length_Line = 160;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=120,267;b=24,68,,,,1");
        }
        LineGAPEjectionExtend: LineBidirectional {
            parameters:
                Length_Line = coreSizeY-1000;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=528,603;b=32,72,,,,1");
        }
        LineMainHorizShortB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=324,335;b=16,24,,,,1");
        }
        LineMainHorizExtraShortB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=68,335;b=32,24,,,,1");
        }
        LineGAPGatewaySmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,87;b=24,92,,,,1");
        }
        LineGAPGatewayExtraSmall: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,595;b=24,84,,,,1");
        }
        LineGAPInjectSmall: LineBidirectional {
            parameters:
                Length_Line = 85;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=120,83;b=24,84,,,,1");
        }
        LineGAPUniA: Line {
            parameters:
                Length_Line = 55;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=446,519;b=84,8,,,,1");
        }
        LineGAPUniB: Line {
            parameters:
                Length_Line = 60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=328,519;b=16,8,,,,1");
        }
        LineGAPUniC: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=204,520;b=72,8,,,,1");
        }
        LineGAPUniD: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=64,520;b=32,8,,,,1");
        }
        LineGAPUniE: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=64,415;b=32,8,,,,1");
        }
        LineGAPUniF: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=204,416;b=72,8,,,,1");
        }
        CrossDoubleA: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=368,520;b=48,32,,,,1");
        }
        CrossDoubleB: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=280,520;b=48,32,,,,1");
        }
        CrossDoubleC: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=120,520;b=48,32,,,,1");
        }
        CrossDoubleD: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=120,416;b=48,32,,,,1");
        }
        TurnA: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=625,340;b=40,40,,,,1");
        }
        TurnB: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=625,240;b=40,40,,,,1");
        }
        TurnC: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=460,88;b=40,40,,,,1");
        }
        TurnD: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=360,88;b=40,40,,,,1");
        }
    connections allowunconnected:
        Sw4x4.photonicOutN <--> CrossB.photonicInS;
        Sw4x4.photonicOutS <--> LineMainVertB.photonicInB;
        Sw4x4.photonicOutE <--> CrossC.photonicInW;
        Sw4x4.photonicOutW <--> LineMainHorizShortB.photonicInA;

        CrossB.photonicOutS <--> Sw4x4.photonicInN;
        LineMainVertB.photonicOutA <--> Sw4x4.photonicInS;
        CrossC.photonicOutW <--> Sw4x4.photonicInE;
        LineMainHorizShortB.photonicOutB <--> Sw4x4.photonicInW;

        CrossA.photonicOutN <--> LineSmallB.photonicInA;
        CrossA.photonicOutS <--> LineStubC.photonicInB;
        CrossA.photonicOutE <--> LineSmallA.photonicInB;
        CrossA.photonicOutW <--> LineStubB.photonicInA;

        LineSmallB.photonicOutB <--> CrossA.photonicInN;
        LineStubC.photonicOutA <--> CrossA.photonicInS;
        LineSmallA.photonicOutA <--> CrossA.photonicInE;
        LineStubB.photonicOutB <--> CrossA.photonicInW;

        CrossB.photonicOutN <--> LineStubA.photonicInA;
        CrossB.photonicOutE <--> LineStubB.photonicInB;
        CrossB.photonicOutW <--> LineMainHorizShortA.photonicInA;

        LineStubA.photonicOutB <--> CrossB.photonicInN;
        LineStubB.photonicOutA <--> CrossB.photonicInE;
        LineMainHorizShortA.photonicOutB <--> CrossB.photonicInW;

        CrossC.photonicOutN <--> LineStubC.photonicInA;
        CrossC.photonicOutS <--> LineMainVertA.photonicInB;
        CrossC.photonicOutE <--> LineStubD.photonicInB;

        LineStubC.photonicOutB <--> CrossC.photonicInN;
        LineMainVertA.photonicOutA <--> CrossC.photonicInS;
        LineStubD.photonicOutA <--> CrossC.photonicInE;

        LineStubD.photonicOutB <--> TurnB.photonicInnerBendIn;
        TurnB.photonicInnerBendOut <--> LineExtraSmallA.photonicInB;
        LineExtraSmallA.photonicOutB <--> TurnA.photonicOuterBendIn;
        TurnA.photonicOuterBendOut <--> photonicEBottomOut;
        photonicEBottomIn <--> TurnA.photonicInnerBendIn;
        TurnA.photonicInnerBendOut <--> LineExtraSmallA.photonicInA;
        LineExtraSmallA.photonicOutA <--> TurnB.photonicOuterBendIn;
        TurnB.photonicOuterBendOut <--> LineStubD.photonicInA;

        photonicNLeftIn <--> TurnD.photonicInnerBendIn;
        TurnD.photonicInnerBendOut <--> LineExtraSmallB.photonicInB;
        LineExtraSmallB.photonicOutB <--> TurnC.photonicOuterBendIn;
        TurnC.photonicOuterBendOut <--> LineStubA.photonicInB;
        LineStubA.photonicOutA <--> TurnC.photonicInnerBendIn;
        TurnC.photonicInnerBendOut <--> LineExtraSmallB.photonicInA;
        LineExtraSmallB.photonicOutA <--> TurnD.photonicOuterBendIn;
        TurnD.photonicOuterBendOut <--> photonicNLeftOut;

        CrossGAPX.photonicOutN <--> LineGAPGatewayConnect.photonicInA;
        CrossGAPX.photonicOutS <--> GatewaySw.photonicInInjectionN;
        CrossGAPX.photonicOutE <--> LineMainHorizShortB.photonicInB;
        CrossGAPX.photonicOutW <--> LineMainHorizB.photonicInA;

        LineGAPGatewayConnect.photonicOutB <--> CrossGAPX.photonicInN;
        GatewaySw.photonicOutInjectionN <--> CrossGAPX.photonicInS;
        LineMainHorizShortB.photonicOutA <--> CrossGAPX.photonicInE;
        LineMainHorizB.photonicOutB <--> CrossGAPX.photonicInW;

        CrossGAPY.photonicOutN <--> LineGAPInjectConnect.photonicInA;
        CrossGAPY.photonicOutS <--> LineGAPShort.photonicInB;
        CrossGAPY.photonicOutE <--> LineMainHorizB.photonicInB;
        CrossGAPY.photonicOutW <--> LineMainHorizExtraShortB.photonicInA;

        LineGAPInjectConnect.photonicOutB <--> CrossGAPY.photonicInN;
        LineGAPShort.photonicOutA <--> CrossGAPY.photonicInS;
        LineMainHorizB.photonicOutA <--> CrossGAPY.photonicInE;
        LineMainHorizExtraShortB.photonicOutB <--> CrossGAPY.photonicInW;

        CrossGAPZ.photonicOutN <--> LineGAPGatewaySmall.photonicInA;
        CrossGAPZ.photonicOutS <--> LineGAPGatewayConnect.photonicInB;
        CrossGAPZ.photonicOutE <--> LineMainHorizShortA.photonicInB;
        CrossGAPZ.photonicOutW <--> LineMainHorizA.photonicInA;

        LineGAPGatewaySmall.photonicOutB <--> CrossGAPZ.photonicInN;
        LineGAPGatewayConnect.photonicOutA <--> CrossGAPZ.photonicInS;
        LineMainHorizShortA.photonicOutA <--> CrossGAPZ.photonicInE;
        LineMainHorizA.photonicOutB <--> CrossGAPZ.photonicInW;



        Gateway.phOut[0] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> Gateway.phIn[0];

        GatewaySw.photonicOutInjectionS <--> LineGAPGateway.photonicInB;
        GatewaySw.photonicOutGateway <--> Gateway.phIn[1];

        GatewaySw.electronicOutGateway --> ElectronicChannel --> Gateway.elIn[1];
        GatewaySw.electronicInGateway <-- ElectronicChannel <-- Gateway.elOut[1];

        electronicGatewayIn --> { @display("ls=blue,0"); } --> Gateway.elIn[0];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[0];


        electronicMemoryIn --> { @display("ls=blue,0"); } --> Gateway.elIn[2] if useIOplane;
        electronicMemoryOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[2] if useIOplane;


        LineGAPGateway.photonicOutA <--> GatewaySw.photonicInInjectionS;
        LineGAPUniF.photonicOut <--> GatewaySw.photonicInEjection;

        Gateway.phOut[1] <--> { @display("ls=,0"); } <--> GatewaySw.photonicInGateway;


        Gateway.phOut[2] <--> photonicMemoryOut if useIOplane;
        photonicMemoryIn <--> Gateway.phIn[2] if useIOplane;

        Injection.photonicOutN <--> LineGAPInjectSmall.photonicInA;
        Injection.photonicOutS <--> LineGAPInjectConnect.photonicInB;
        Injection.photonicOutE <--> LineMainHorizA.photonicInB;
        Injection.photonicOutW <--> photonicWTopOut;

        LineGAPInjectSmall.photonicOutB <--> Injection.photonicInN;
        LineGAPInjectConnect.photonicOutA <--> Injection.photonicInS;
        LineMainHorizA.photonicOutA <--> Injection.photonicInE;
        photonicWTopIn <--> Injection.photonicInW;

        Ejection.photonicOutN <--> LineMainVertA.photonicInA;
        Ejection.photonicOutS <--> LineGAPEjectionExtend.photonicInB;
        Ejection.photonicOutGateway <--> LineGAPUniA.photonicIn;

        LineMainVertA.photonicOutB <--> Ejection.photonicInN;
        LineGAPEjectionExtend.photonicOutA <--> Ejection.photonicInS;

        LineGAPEjectionExtend.photonicOutB <--> photonicSRightOut;
        photonicSRightIn <--> LineGAPEjectionExtend.photonicInA;

        CrossDoubleA.photonicOutSide <--> LineGAPUniB.photonicIn;
        CrossDoubleA.photonicOutA <--> LineMainVertB.photonicInA;
        CrossDoubleA.photonicOutB <--> LineMainVertShortB.photonicInB;

        LineGAPUniA.photonicOut <--> CrossDoubleA.photonicInSide;
        LineMainVertB.photonicOutB <--> CrossDoubleA.photonicInB;
        LineMainVertShortB.photonicOutA <--> CrossDoubleA.photonicInA;

        CrossDoubleB.photonicOutSide <--> LineGAPUniC.photonicIn;
        CrossDoubleB.photonicOutA <--> LineGAPGateway.photonicInA;
        CrossDoubleB.photonicOutB <--> LineGAPGatewayExtraSmall.photonicInB;

        LineGAPUniB.photonicOut <--> CrossDoubleB.photonicInSide;
        LineGAPGateway.photonicOutB <--> CrossDoubleB.photonicInB;
        LineGAPGatewayExtraSmall.photonicOutA <--> CrossDoubleB.photonicInA;

        CrossDoubleC.photonicOutSide <--> LineGAPUniD.photonicIn;
        CrossDoubleC.photonicOutA <--> LineGAPInject.photonicInA;
        CrossDoubleC.photonicOutB <--> LineGAPInjectExtraSmall.photonicInB;

        LineGAPUniC.photonicOut <--> CrossDoubleC.photonicInSide;
        LineGAPInject.photonicOutB <--> CrossDoubleC.photonicInB;
        LineGAPInjectExtraSmall.photonicOutA <--> CrossDoubleC.photonicInA;

        CrossDoubleD.photonicOutSide <--> LineGAPUniF.photonicIn;
        CrossDoubleD.photonicOutA <--> LineGAPShort.photonicInA;
        CrossDoubleD.photonicOutB <--> LineGAPInject.photonicInB;

        LineGAPUniE.photonicOut <--> CrossDoubleD.photonicInSide;
        LineGAPShort.photonicOutB <--> CrossDoubleD.photonicInB;
        LineGAPInject.photonicOutA <--> CrossDoubleD.photonicInA;

        photonicNRightIn <--> LineSmallB.photonicInB;
        LineSmallB.photonicOutA <--> photonicNRightOut;

        photonicSLeftIn <--> LineMainVertShortB.photonicInA;
        LineMainVertShortB.photonicOutB <--> photonicSLeftOut;

        photonicETopIn <--> LineSmallA.photonicInA;
        LineSmallA.photonicOutB <--> photonicETopOut;

        photonicWBottomIn <--> LineMainHorizExtraShortB.photonicInB;
        LineMainHorizExtraShortB.photonicOutA <--> photonicWBottomOut;

        photonicNLeftInjectionIn <--> LineGAPInjectSmall.photonicInB;
        photonicNRightInjectionIn <--> LineGAPGatewaySmall.photonicInB;
        photonicSLeftInjectionIn <--> LineGAPInjectExtraSmall.photonicInA;
        photonicSRightInjectionIn <--> LineGAPGatewayExtraSmall.photonicInA;

        LineGAPInjectSmall.photonicOutA <--> photonicNLeftInjectionOut;
        LineGAPGatewaySmall.photonicOutA <--> photonicNRightInjectionOut;
        LineGAPInjectExtraSmall.photonicOutB <--> photonicSLeftInjectionOut;
        LineGAPGatewayExtraSmall.photonicOutB <--> photonicSRightInjectionOut;

        LineGAPUniD.photonicOut <--> photonicWEjectionOut;
        photonicWEjectionIn <--> LineGAPUniE.photonicIn;

        //
        // Electronic Connections
        //





        electronicNLeftIn --> { @display("m=n;ls=red,0"); } --> Sw4x4.electronicInN;
        electronicNRightIn --> { @display("m=n;ls=red,0"); } --> Ejection.electronicInN;
        electronicSLeftIn --> { @display("m=s;ls=red,0"); } --> Sw4x4.electronicInS;
        electronicSRightIn --> { @display("m=s;ls=red,0"); } --> Ejection.electronicInS;
        electronicETopIn --> { @display("m=e;ls=red,0"); } --> Injection.electronicInE;
        electronicEBottomIn --> { @display("m=e;ls=red,0"); } --> Sw4x4.electronicInE;
        electronicWTopIn --> { @display("m=w;ls=red,0"); } --> Injection.electronicInW;
        electronicWBottomIn --> { @display("m=w;ls=red,0"); } --> Sw4x4.electronicInW;

        electronicNLeftOut <-- { @display("m=n;ls=red,0"); } <-- Sw4x4.electronicOutN;
        electronicNRightOut <-- { @display("m=n;ls=red,0"); } <-- Ejection.electronicOutN;
        electronicSLeftOut <-- { @display("m=s;ls=red,0"); } <-- Sw4x4.electronicOutS;
        electronicSRightOut <-- { @display("m=s;ls=red,0"); } <-- Ejection.electronicOutS;
        electronicETopOut <-- { @display("m=e;ls=red,0"); } <-- Injection.electronicOutE;
        electronicEBottomOut <-- { @display("m=e;ls=red,0"); } <-- Sw4x4.electronicOutE;
        electronicWTopOut <-- { @display("m=w;ls=red,0"); } <-- Injection.electronicOutW;
        electronicWBottomOut <-- { @display("m=w;ls=red,0"); } <-- Sw4x4.electronicOutW;

        electronicNLeftInjectionIn --> { @display("m=n;ls=red,0"); } --> Injection.electronicInN;
        electronicNRightInjectionIn --> { @display("m=n;ls=red,0"); } --> GatewaySw.electronicInInjectionN;
        electronicSLeftInjectionIn --> { @display("m=s;ls=red,0"); } --> Injection.electronicInS;
        electronicSRightInjectionIn --> { @display("m=s;ls=red,0"); } --> GatewaySw.electronicInInjectionS;
        electronicWTopEjectionIn --> { @display("m=w;ls=red,0"); } --> GatewaySw.electronicInEjection;
        electronicWBottomEjectionIn --> { @display("m=w;ls=red,0"); } --> Ejection.electronicInGateway;
        electronicNLeftInjectionOut <-- { @display("m=n;ls=red,0"); } <-- Injection.electronicOutN;
        electronicNRightInjectionOut <-- { @display("m=n;ls=red,0"); } <-- GatewaySw.electronicOutInjectionN;
        electronicSLeftInjectionOut <-- { @display("m=s;ls=red,0"); } <-- Injection.electronicOutS;
        electronicSRightInjectionOut <-- { @display("m=s;ls=red,0"); } <-- GatewaySw.electronicOutInjectionS;
        electronicWTopEjectionOut <-- { @display("m=w;ls=red,0"); } <-- GatewaySw.electronicOutEjection;
        electronicWBottomEjectionOut <-- { @display("m=w;ls=red,0"); } <-- Ejection.electronicOutGateway;

}

module TorusTileNE
{
    parameters:
        int nodeId;

        int level;
        string networkPrefix;


        int numX;
        int numY;

        bool useIOplane;


        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;
        double BarDelay_2x2;

        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Bend;
        double Latency_Cross;

        double coreSizeX;
        double coreSizeY;

        volatile double disp_Width;


        int numOfWavelengthChannels;



        @display("bgb=660,660");
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

        inout photonicNLeftInjectionIn;
        inout photonicNRightInjectionIn;
        inout photonicSLeftInjectionIn;
        inout photonicSRightInjectionIn;
        inout photonicEEjectionIn;
        inout photonicNLeftInjectionOut;
        inout photonicNRightInjectionOut;
        inout photonicSLeftInjectionOut;
        inout photonicSRightInjectionOut;
        inout photonicEEjectionOut;

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

        input electronicNLeftInjectionIn;
        input electronicNRightInjectionIn;
        input electronicSLeftInjectionIn;
        input electronicSRightInjectionIn;
        input electronicETopEjectionIn;
        input electronicEBottomEjectionIn;
        output electronicNLeftInjectionOut;
        output electronicNRightInjectionOut;
        output electronicSLeftInjectionOut;
        output electronicSRightInjectionOut;
        output electronicETopEjectionOut;
        output electronicEBottomEjectionOut;

        input electronicGatewayIn;
        output electronicGatewayOut;

        input electronicMemoryIn;
        output electronicMemoryOut;


    submodules:
        CrossA: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=164,504;b=48,48,,,,1");
        }
        CrossB: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=232,504;b=48,48,,,,1");
        }
        CrossC: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=164,436;b=48,48,,,,1");
        }
        CrossGAPX: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=412,340;b=48,48,,,,1");
        }
        CrossGAPY: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=580,340;b=48,48,,,,1");
        }
        CrossGAPZ: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=412,504;b=48,48,,,,1");
        }
        Sw4x4: BlockingTorusNodeRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 13;
            @display("i=components/4x4old;p=276,391");
        }
        GatewaySw: BlockingTorusGatewayRouterRight {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            @display("p=412,261;i=components/gw_sw");
        }
        Gateway: HybridRouter {
            parameters:
                id = networkPrefix + string(nodeId) + ".";

                optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");

                elRouter = 1;

                level = "NET";

                numX = numX;
                numY = numY;

        }
        Injection: BlockingTorusInjectionRouter {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 11;
            @display("p=580,504;i=components/in_sw");
        }
        Ejection: BlockingTorusEjectionRouterRight {
            nodeId = nodeId;

            networkPrefix = networkPrefix;
            numX = numX;
            numY = numY;
            type = 10;
            @display("p=164,156;i=components/ej_sw");
        }
        LineMainVertA: LineBidirectional {
            parameters:
                Length_Line = 595;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=164,290;b=24,186,,,,1");
        }
        LineMainVertB: LineBidirectional {
            parameters:
                Length_Line = 405;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=324,242;b=24,102,,,,1");
        }
        LineMainVertShortB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+405;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=324,82;b=24,86,,,,1");
        }
        LineMainHorizA: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=486,504;b=66,24,,,,1");
        }
        LineMainHorizShortA: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=322,504;b=90,24,,,,1");
        }
        LineMainHorizB: LineBidirectional {
            parameters:
                Length_Line = (coreSizeX-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=494,341;b=78,24,,,,1");
        }
        LineSmallA: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=75,504;b=60,24,,,,1");
        }
        LineSmallB: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=164,579;b=24,60,,,,1");
        }
        LineExtraSmallA: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=67,386;b=24,30,,,,1");
        }
        LineExtraSmallB: LineBidirectional {
            parameters:
                Length_Line = 70;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=282,588;b=30,24,,,,1");
        }
        LineStubA: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=232,547;b=24,8,,,,1");
        }
        LineStubB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=198,504;b=8,24,,,,1");
        }
        LineStubC: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=164,470;b=24,8,,,,1");
        }
        LineStubD: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=111,436;b=8,24,,,,1");
        }
        LineGAPGateway: LineBidirectional {
            parameters:
                Length_Line = 195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=412,209;b=24,28,,,,1");
        }
        LineGAPInject: LineBidirectional {
            parameters:
                Length_Line = 305;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=580,209;b=24,28,,,,1");
        }
        LineGAPEjectionExtend: LineBidirectional {
            parameters:
                Length_Line = coreSizeY-1000;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=160,75;b=32,72,,,,1");
        }
        LineGAPShort: LineBidirectional {
            parameters:
                Length_Line = 0;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=580,301;b=24,20,,,,1");
        }
        LineGAPInjectExtraSmall: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+195;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=580,85;b=24,84,,,,1");
        }
        LineGAPGatewayConnect: LineBidirectional {
            parameters:
                Length_Line = 190;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=412,417;b=24,84,,,,1");
        }
        LineGAPInjectConnect: LineBidirectional {
            parameters:
                Length_Line = 160;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=580,409;b=24,68,,,,1");
        }
        LineMainHorizShortB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=368,341;b=16,24,,,,1");
        }
        LineMainHorizExtraShortB: LineBidirectional {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=624,341;b=16,24,,,,1");
        }
        LineGAPGatewaySmall: LineBidirectional {
            parameters:
                Length_Line = 115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=412,589;b=24,92,,,,1");
        }
        LineGAPGatewayExtraSmall: LineBidirectional {
            parameters:
                Length_Line = (coreSizeY-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=412,85;b=24,76,,,,1");
        }
        LineGAPInjectSmall: LineBidirectional {
            parameters:
                Length_Line = 85;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=580,593;b=24,84,,,,1");
        }
        LineGAPUniA: Line {
            parameters:
                Length_Line = 55;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=246,157;b=84,8,,,,1");
        }
        LineGAPUniB: Line {
            parameters:
                Length_Line = 60;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=364,157;b=16,8,,,,1");
        }
        LineGAPUniC: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=496,156;b=88,8,,,,1");
        }
        LineGAPUniD: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=628,156;b=16,8,,,,1");
        }
        LineGAPUniE: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=628,261;b=16,8,,,,1");
        }
        LineGAPUniF: Line {
            parameters:
                Length_Line = (coreSizeX-1000)+115;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=496,260;b=88,8,,,,1");
        }
        CrossDoubleA: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=324,156;b=48,32,,,,1");
        }
        CrossDoubleB: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=412,156;b=48,32,,,,1");
        }
        CrossDoubleC: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=580,156;b=48,32,,,,1");
        }
        CrossDoubleD: CrossDouble {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Cross = Latency_Cross;
                @display("p=580,260;b=48,32,,,,1");
        }
        TurnA: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=67,336;b=40,40,,,,1");
        }
        TurnB: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=67,436;b=40,40,,,,1");
        }
        TurnC: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=232,588;b=40,40,,,,1");
        }
        TurnD: BendBidirectional {
            parameters:
                PathSeparation = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=332,588;b=40,40,,,,1");
        }
    connections allowunconnected:


        Sw4x4.photonicOutN <--> LineMainVertB.photonicInA;
        Sw4x4.photonicOutS <--> CrossB.photonicInN;
        Sw4x4.photonicOutE <--> LineMainHorizShortB.photonicInB;
        Sw4x4.photonicOutW <--> CrossC.photonicInE;

        LineMainVertB.photonicOutB <--> Sw4x4.photonicInN;
        CrossB.photonicOutN <--> Sw4x4.photonicInS;
        LineMainHorizShortB.photonicOutA <--> Sw4x4.photonicInE;
        CrossC.photonicOutE <--> Sw4x4.photonicInW;

        CrossA.photonicOutN <--> LineStubC.photonicInA;
        CrossA.photonicOutS <--> LineSmallB.photonicInB;
        CrossA.photonicOutE <--> LineStubB.photonicInB;
        CrossA.photonicOutW <--> LineSmallA.photonicInA;

        LineStubC.photonicOutB <--> CrossA.photonicInN;
        LineSmallB.photonicOutA <--> CrossA.photonicInS;
        LineStubB.photonicOutA <--> CrossA.photonicInE;
        LineSmallA.photonicOutB <--> CrossA.photonicInW;

        CrossB.photonicOutS <--> LineStubA.photonicInB;
        CrossB.photonicOutE <--> LineMainHorizShortA.photonicInB;
        CrossB.photonicOutW <--> LineStubB.photonicInA;

        LineStubA.photonicOutA <--> CrossB.photonicInS;
        LineMainHorizShortA.photonicOutA <--> CrossB.photonicInE;
        LineStubB.photonicOutB <--> CrossB.photonicInW;

        CrossC.photonicOutN <--> LineMainVertA.photonicInA;
        CrossC.photonicOutS <--> LineStubC.photonicInB;
        CrossC.photonicOutW <--> LineStubD.photonicInA;

        LineMainVertA.photonicOutB <--> CrossC.photonicInN;
        LineStubC.photonicOutA <--> CrossC.photonicInS;
        LineStubD.photonicOutB <--> CrossC.photonicInW;

        photonicWTopIn <--> TurnA.photonicInnerBendIn;
        TurnA.photonicInnerBendOut <--> LineExtraSmallA.photonicInB;
        LineExtraSmallA.photonicOutB <--> TurnB.photonicOuterBendIn;
        TurnB.photonicOuterBendOut <--> LineStubD.photonicInB;
        LineStubD.photonicOutA <--> TurnB.photonicInnerBendIn;
        LineExtraSmallA.photonicOutA <--> TurnA.photonicOuterBendIn;
        TurnB.photonicInnerBendOut <--> LineExtraSmallA.photonicInA;
        TurnA.photonicOuterBendOut <--> photonicWTopOut;

        LineStubA.photonicOutB <--> TurnC.photonicInnerBendIn;
        TurnC.photonicInnerBendOut <--> LineExtraSmallB.photonicInB;
        LineExtraSmallB.photonicOutB <--> TurnD.photonicOuterBendIn;
        TurnD.photonicOuterBendOut <--> photonicSRightOut;
        photonicSRightIn <--> TurnD.photonicInnerBendIn;
        TurnD.photonicInnerBendOut <--> LineExtraSmallB.photonicInA;
        LineExtraSmallB.photonicOutA <--> TurnC.photonicOuterBendIn;
        TurnC.photonicOuterBendOut <--> LineStubA.photonicInA;

        CrossGAPX.photonicOutN <--> GatewaySw.photonicInInjectionS;
        CrossGAPX.photonicOutS <--> LineGAPGatewayConnect.photonicInB;
        CrossGAPX.photonicOutE <--> LineMainHorizB.photonicInB;
        CrossGAPX.photonicOutW <--> LineMainHorizShortB.photonicInA;

        GatewaySw.photonicOutInjectionS <--> CrossGAPX.photonicInN;
        LineGAPGatewayConnect.photonicOutA <--> CrossGAPX.photonicInS;
        LineMainHorizB.photonicOutA <--> CrossGAPX.photonicInE;
        LineMainHorizShortB.photonicOutB <--> CrossGAPX.photonicInW;

        CrossGAPY.photonicOutN <--> LineGAPShort.photonicInA;
        CrossGAPY.photonicOutS <--> LineGAPInjectConnect.photonicInB;
        CrossGAPY.photonicOutE <--> LineMainHorizExtraShortB.photonicInB;
        CrossGAPY.photonicOutW <--> LineMainHorizB.photonicInA;

        LineGAPShort.photonicOutB <--> CrossGAPY.photonicInN;
        LineGAPInjectConnect.photonicOutA <--> CrossGAPY.photonicInS;
        LineMainHorizExtraShortB.photonicOutA <--> CrossGAPY.photonicInE;
        LineMainHorizB.photonicOutB <--> CrossGAPY.photonicInW;

        CrossGAPZ.photonicOutN <--> LineGAPGatewayConnect.photonicInA;
        CrossGAPZ.photonicOutS <--> LineGAPGatewaySmall.photonicInB;
        CrossGAPZ.photonicOutE <--> LineMainHorizA.photonicInB;
        CrossGAPZ.photonicOutW <--> LineMainHorizShortA.photonicInA;

        LineGAPGatewayConnect.photonicOutB <--> CrossGAPZ.photonicInN;
        LineGAPGatewaySmall.photonicOutA <--> CrossGAPZ.photonicInS;
        LineMainHorizA.photonicOutA <--> CrossGAPZ.photonicInE;
        LineMainHorizShortA.photonicOutB <--> CrossGAPZ.photonicInW;


        Gateway.phOut[0] <--> { @display("ls=,0"); } <--> photonicGatewayOut;
        photonicGatewayIn <--> Gateway.phIn[0];

        GatewaySw.photonicOutInjectionN <--> LineGAPGateway.photonicInA;
        GatewaySw.photonicOutGateway <--> Gateway.phIn[1];

        GatewaySw.electronicOutGateway --> ElectronicChannel --> Gateway.elIn[1];
        GatewaySw.electronicInGateway <-- ElectronicChannel <-- Gateway.elOut[1];

        electronicGatewayIn --> { @display("ls=blue,0"); } --> Gateway.elIn[0];
        electronicGatewayOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[0];


        electronicMemoryIn --> { @display("ls=blue,0"); } --> Gateway.elIn[2] if useIOplane;
        electronicMemoryOut <-- { @display("ls=blue,0"); } <-- Gateway.elOut[2] if useIOplane;


        LineGAPGateway.photonicOutB <--> GatewaySw.photonicInInjectionN;
        LineGAPUniF.photonicOut <--> GatewaySw.photonicInEjection;

        Gateway.phOut[1] <--> { @display("ls=,0"); } <--> GatewaySw.photonicInGateway;

        Gateway.phOut[2] <--> photonicMemoryOut if useIOplane;
        photonicMemoryIn <--> Gateway.phIn[2] if useIOplane;

        Injection.photonicOutN <--> LineGAPInjectConnect.photonicInA;
        Injection.photonicOutS <--> LineGAPInjectSmall.photonicInB;
        Injection.photonicOutE <--> photonicEBottomOut;
        Injection.photonicOutW <--> LineMainHorizA.photonicInA;

        LineGAPInjectConnect.photonicOutB <--> Injection.photonicInN;
        LineGAPInjectSmall.photonicOutA <--> Injection.photonicInS;
        photonicEBottomIn <--> Injection.photonicInE;
        LineMainHorizA.photonicOutB <--> Injection.photonicInW;

        Ejection.photonicOutN <--> LineGAPEjectionExtend.photonicInA;
        Ejection.photonicOutS <--> LineMainVertA.photonicInB;
        Ejection.photonicOutGateway <--> LineGAPUniA.photonicIn;

        LineGAPEjectionExtend.photonicOutB <--> Ejection.photonicInN;
        LineMainVertA.photonicOutA <--> Ejection.photonicInS;

        LineGAPEjectionExtend.photonicOutA <--> photonicNLeftOut;
        photonicNLeftIn <--> LineGAPEjectionExtend.photonicInB;

        CrossDoubleA.photonicOutSide <--> LineGAPUniB.photonicIn;
        CrossDoubleA.photonicOutA <--> LineMainVertShortB.photonicInA;
        CrossDoubleA.photonicOutB <--> LineMainVertB.photonicInB;

        LineGAPUniA.photonicOut <--> CrossDoubleA.photonicInSide;
        LineMainVertB.photonicOutA <--> CrossDoubleA.photonicInA;
        LineMainVertShortB.photonicOutB <--> CrossDoubleA.photonicInB;

        CrossDoubleB.photonicOutSide <--> LineGAPUniC.photonicIn;
        CrossDoubleB.photonicOutA <--> LineGAPGatewayExtraSmall.photonicInA;
        CrossDoubleB.photonicOutB <--> LineGAPGateway.photonicInB;

        LineGAPUniB.photonicOut <--> CrossDoubleB.photonicInSide;
        LineGAPGateway.photonicOutA <--> CrossDoubleB.photonicInA;
        LineGAPGatewayExtraSmall.photonicOutB <--> CrossDoubleB.photonicInB;

        CrossDoubleC.photonicOutSide <--> LineGAPUniD.photonicIn;
        CrossDoubleC.photonicOutA <--> LineGAPInjectExtraSmall.photonicInA;
        CrossDoubleC.photonicOutB <--> LineGAPInject.photonicInB;

        LineGAPUniC.photonicOut <--> CrossDoubleC.photonicInSide;
        LineGAPInject.photonicOutA <--> CrossDoubleC.photonicInA;
        LineGAPInjectExtraSmall.photonicOutB <--> CrossDoubleC.photonicInB;

        CrossDoubleD.photonicOutSide <--> LineGAPUniF.photonicIn;
        CrossDoubleD.photonicOutA <--> LineGAPInject.photonicInA;
        CrossDoubleD.photonicOutB <--> LineGAPShort.photonicInB;

        LineGAPUniE.photonicOut <--> CrossDoubleD.photonicInSide;
        LineGAPShort.photonicOutA <--> CrossDoubleD.photonicInA;
        LineGAPInject.photonicOutB <--> CrossDoubleD.photonicInB;


        photonicNRightIn <--> LineMainVertShortB.photonicInB;
        LineMainVertShortB.photonicOutA <--> photonicNRightOut;

        photonicSLeftIn <--> LineSmallB.photonicInA;
        LineSmallB.photonicOutB <--> photonicSLeftOut;

        photonicETopIn <--> LineMainHorizExtraShortB.photonicInA;
        LineMainHorizExtraShortB.photonicOutB <--> photonicETopOut;

        photonicWBottomIn <--> LineSmallA.photonicInB;
        LineSmallA.photonicOutA <--> photonicWBottomOut;

        photonicNLeftInjectionIn <--> LineGAPGatewayExtraSmall.photonicInB;
        photonicNRightInjectionIn <--> LineGAPInjectExtraSmall.photonicInB;
        photonicSLeftInjectionIn <--> LineGAPGatewaySmall.photonicInA;
        photonicSRightInjectionIn <--> LineGAPInjectSmall.photonicInA;
        photonicEEjectionIn <--> LineGAPUniE.photonicIn;

        LineGAPGatewayExtraSmall.photonicOutA <--> photonicNLeftInjectionOut;
        LineGAPInjectExtraSmall.photonicOutA <--> photonicNRightInjectionOut;
        LineGAPGatewaySmall.photonicOutB <--> photonicSLeftInjectionOut;
        LineGAPInjectSmall.photonicOutB <--> photonicSRightInjectionOut;
        LineGAPUniD.photonicOut <--> photonicEEjectionOut;

        //
        // Electronic Connections
        //



        electronicNLeftIn --> { @display("m=n;ls=red,0"); } --> Ejection.electronicInN;
        electronicNRightIn --> { @display("m=n;ls=red,0"); } --> Sw4x4.electronicInN;
        electronicSLeftIn --> { @display("m=s;ls=red,0"); } --> Ejection.electronicInS;
        electronicSRightIn --> { @display("m=s;ls=red,0"); } --> Sw4x4.electronicInS;
        electronicETopIn --> { @display("m=e;ls=red,0"); } --> Sw4x4.electronicInE;
        electronicEBottomIn --> { @display("m=e;ls=red,0"); } --> Injection.electronicInE;
        electronicWTopIn --> { @display("m=w;ls=red,0"); } --> Sw4x4.electronicInW;
        electronicWBottomIn --> { @display("m=w;ls=red,0"); } --> Injection.electronicInW;

        electronicNLeftOut <-- { @display("m=n;ls=red,0"); } <-- Ejection.electronicOutN;
        electronicNRightOut <-- { @display("m=n;ls=red,0"); } <-- Sw4x4.electronicOutN;
        electronicSLeftOut <-- { @display("m=s;ls=red,0"); } <-- Ejection.electronicOutS;
        electronicSRightOut <-- { @display("m=s;ls=red,0"); } <-- Sw4x4.electronicOutS;
        electronicETopOut <-- { @display("m=e;ls=red,0"); } <-- Sw4x4.electronicOutE;
        electronicEBottomOut <-- { @display("m=e;ls=red,0"); } <-- Injection.electronicOutE;
        electronicWTopOut <-- { @display("m=w;ls=red,0"); } <-- Sw4x4.electronicOutW;
        electronicWBottomOut <-- { @display("m=w;ls=red,0"); } <-- Injection.electronicOutW;

        electronicNLeftInjectionIn --> { @display("m=n;ls=red,0"); } --> GatewaySw.electronicInInjectionN;
        electronicNRightInjectionIn --> { @display("m=n;ls=red,0"); } --> Injection.electronicInN;
        electronicSLeftInjectionIn --> { @display("m=s;ls=red,0"); } --> GatewaySw.electronicInInjectionS;
        electronicSRightInjectionIn --> { @display("m=s;ls=red,0"); } --> Injection.electronicInS;
        electronicETopEjectionIn --> { @display("m=e;ls=red,0"); } --> Ejection.electronicInGateway;
        electronicEBottomEjectionIn --> { @display("m=e;ls=red,0"); } --> GatewaySw.electronicInEjection;
        electronicNLeftInjectionOut <-- { @display("m=n;ls=red,0"); } <-- GatewaySw.electronicOutInjectionN;
        electronicNRightInjectionOut <-- { @display("m=n;ls=red,0"); } <-- Injection.electronicOutN;
        electronicSLeftInjectionOut <-- { @display("m=s;ls=red,0"); } <-- GatewaySw.electronicOutInjectionS;
        electronicSRightInjectionOut <-- { @display("m=s;ls=red,0"); } <-- Injection.electronicOutS;
        electronicETopEjectionOut <-- { @display("m=e;ls=red,0"); } <-- Ejection.electronicOutGateway;
        electronicEBottomEjectionOut <-- { @display("m=e;ls=red,0"); } <-- GatewaySw.electronicOutEjection;


}

