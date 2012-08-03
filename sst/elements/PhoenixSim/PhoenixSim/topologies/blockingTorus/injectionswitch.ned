// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: injectionswitch.ned				                               *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package topologies.blockingTorus;

import photonic.devices.PSE1x2;
import photonic.devices.Line;

module InjectionSwitch
{
    parameters:
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double LatencyRate_Line;

        @display("bgb=386,391");
    gates:
        inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;
        
        input controlPSE1x2_NE;
        input controlPSE1x2_NW;
        input controlPSE1x2_SE;
        input controlPSE1x2_SW;
        
        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;

    submodules:
        PSE1x2_NE: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=266,141;i=components/pse1x2");
            //display: "o=,,1;p=266,141;b=42,42";
        }
        PSE1x2_NW: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=141,141;i=components/pse1x2");
            //display: "o=,,1;p=141,141;b=42,42";
        }
        PSE1x2_SE: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=266,266;i=components/pse1x2");
            //display: "o=,,1;p=266,266;b=42,42";
        }
        PSE1x2_SW: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=141,266;i=components/pse1x2");
            //display: "o=,,1;p=141,266;b=42,42";
        }
        Line_NE: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=341,141;b=40,4,,#408080,#000000,1");
        }
        Line_NW: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=141,66;b=4,40,,#408080,#000000,1");
        }
        Line_SE: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=266,341;b=4,40,,#408080,#000000,1");
        }
        Line_SW: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=66,266;b=40,4,,#408080,#000000,1");
        }
    connections allowunconnected:
        controlPSE1x2_NE --> { @display("ls=,0"); } --> PSE1x2_NE.fromRouter;
        controlPSE1x2_NW --> { @display("ls=,0"); } --> PSE1x2_NW.fromRouter;
        controlPSE1x2_SE --> { @display("ls=,0"); } --> PSE1x2_SE.fromRouter;
        controlPSE1x2_SW --> { @display("ls=,0"); } --> PSE1x2_SW.fromRouter;


        photonicInN <--> { @display("m=n"); } <--> PSE1x2_NE.photonicHorizIn;
        photonicInS <--> { @display("m=s"); } <--> PSE1x2_SW.photonicHorizIn;
        photonicInE <--> { @display("m=e"); } <--> Line_NE.photonicIn;
        photonicInW <--> { @display("m=w"); } <--> Line_SW.photonicIn;

        Line_NE.photonicOut <--> PSE1x2_NE.photonicVertIn;
        Line_SW.photonicOut <--> PSE1x2_SW.photonicVertIn;

        PSE1x2_NE.photonicHorizOut <--> PSE1x2_SE.photonicHorizIn;
        PSE1x2_NE.photonicVertOut <--> PSE1x2_NW.photonicVertIn;
        PSE1x2_SW.photonicHorizOut <--> PSE1x2_NW.photonicHorizIn;
        PSE1x2_SW.photonicVertOut <--> PSE1x2_SE.photonicVertIn;

        PSE1x2_SE.photonicVertOut <--> { @display("m=e"); } <--> photonicOutE;
        PSE1x2_NW.photonicVertOut <--> { @display("ls=,1;m=w"); } <--> photonicOutW;
        PSE1x2_SE.photonicHorizOut <--> Line_SE.photonicIn;
        PSE1x2_NW.photonicHorizOut <--> Line_NW.photonicIn;
        Line_SE.photonicOut <--> { @display("m=s"); } <--> photonicOutS;
        Line_NW.photonicOut <--> { @display("m=n"); } <--> photonicOutN;
}


