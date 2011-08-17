// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: nonblockingswitch4x4another.ned                                 *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************

package photonic.switches;
import photonic.devices.PSE2x2;
import photonic.devices.PSE1x2;
import photonic.devices.Line;
import photonic.devices.Bend;
import photonic.devices.Cross;
import photonic.PhotonicSwitch;


//Straight Path
module NonBlockingSwitch4x4_SP like PhotonicSwitch
{
    parameters:
        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;
        double BarDelay_2x2;

        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;
        int numPorts = 4;
        int numPSE = 6;
        
        int variant = 1;
        double pse1x2Width;
        double pse2x2Width;
        double crossWidth;
        
        double dimX = ((pse1x2Width) + crossWidth + (pse2x2Width)) * 2;
        double dimY = dimX;
        
        @display("bgb=800,806");

    gates:
        //0-N
        //1-E
        //2-S
        //3-W
        inout in[numPorts];
        inout out[numPorts];

        input cntrl[numPSE];
    submodules:
        PSE2x2_NE: PSE2x2 {
            parameters:
                RingOn_ER_2x2 = RingOn_ER_2x2;
                RingOff_ER_2x2 = RingOff_ER_2x2;
                CrossDelay_2x2 = CrossDelay_2x2;
                BarDelay_2x2 = BarDelay_2x2;
                @display("p=589,276;b=54,54,,,,1");
        }
        PSE2x2_SW: PSE2x2 {
            parameters:
                RingOn_ER_2x2 = RingOn_ER_2x2;
                RingOff_ER_2x2 = RingOff_ER_2x2;
                CrossDelay_2x2 = CrossDelay_2x2;
                BarDelay_2x2 = BarDelay_2x2;
                @display("p=269,594;b=54,54,,,,1");
        }
        PSE1x2_N: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=589,176;b=42,42,,,,1");
        }
        PSE1x2_S: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=269,675;b=42,42,,,,1");
        }
        PSE1x2_E: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=689,276;b=42,42,,,,1");
        }
        PSE1x2_W: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=175,594;b=42,42,,,,1");
        }
        Cross_1: Cross {
            parameters:
                @display("p=689,176;b=24,24,,,,1");
        }
        Cross_2: Cross {
            parameters:
                @display("p=175,675;b=24,24,,,,1");
        }
        Cross_3: Cross {
            parameters:
                @display("p=490,276;b=24,24,,,,1");
        }
        Cross_4: Cross {
            parameters:
                @display("p=385,594;b=24,24,,,,1");
        }
        Cross_5: Cross {
            parameters:
                @display("p=589,376;b=24,24,,,,1");
        }
        Cross_6: Cross {
            parameters:
                @display("p=269,483;b=24,24,,,,1");
        }
        Cross_7: Cross {
            parameters:
                @display("p=385,376;b=24,24,,,,1");
        }
        Cross_8: Cross {
            parameters:
                @display("p=490,376;b=24,24,,,,1");
        }
        Cross_9: Cross {
            parameters:
                @display("p=385,483;b=24,24,,,,1");
        }
        Cross_10: Cross {
            parameters:
                @display("p=490,483;b=24,24,,,,1");
        }
        Bend_1: Bend {
            parameters:
                @display("p=490,176;b=10,10,,,,1");
        }
        Bend_2: Bend {
            parameters:
                @display("p=385,675;b=10,10,,,,1");
        }
        Bend_3: Bend {
            parameters:
                @display("p=689,376;b=10,10,,,,1");
        }
        Bend_4: Bend {
            parameters:
                @display("p=175,483;b=10,10,,,,1");
        }
        Bend_5: Bend {
            parameters:
                @display("p=385,276;b=10,10,,,,1");
        }
        Bend_6: Bend {
            parameters:
                @display("p=490,594;b=10,10,,,,1");
        }
        Bend_7: Bend {
            parameters:
                @display("p=589,483;b=10,10,,,,1");
        }
        Bend_8: Bend {
            parameters:
                @display("p=269,376;b=10,10,,,,1");
        }
        Line_20_1: Line {
            parameters:
                Length_Line = 20;
                @display("p=443,276;b=40,4,,#408080,#000000,1");
        }
        Line_20_2: Line {
            parameters:
                Length_Line = 20;
                @display("p=443,594;b=40,4,,#408080,#000000,1");
        }
        Line_20_3: Line {
            parameters:
                Length_Line = 20;
                @display("p=589,438;b=4,40,,#408080,#000000,1");
        }
        Line_20_4: Line {
            parameters:
                Length_Line = 20;
                @display("p=269,423;b=4,40,,#408080,#000000,1");
        }
        Line_30_1: Line {
            parameters:
                Length_Line = 30;
                @display("p=689,126;b=4,40,,#408080,#000000,1");
        }
        Line_30_2: Line {
            parameters:
                Length_Line = 30;
                @display("p=175,752;b=4,40,,#408080,#000000,1");
        }
        Line_30_3: Line {
            parameters:
                Length_Line = 30;
                @display("p=732,176;b=40,4,,#408080,#000000,1");
        }
        Line_30_4: Line {
            parameters:
                Length_Line = 30;
                @display("p=103,675;b=40,4,,#408080,#000000,1");
        }
        Line_30_5: Line {
            parameters:
                Length_Line = 30;
                @display("p=490,330;b=4,40,,#408080,#000000,1");
        }
        Line_30_6: Line {
            parameters:
                Length_Line = 30;
                @display("p=385,538;b=4,40,,#408080,#000000,1");
        }
        Line_30_7: Line {
            parameters:
                Length_Line = 30;
                @display("p=543,376;b=40,4,,#408080,#000000,1");
        }
        Line_30_8: Line {
            parameters:
                Length_Line = 30;
                @display("p=324,483;b=40,4,,#408080,#000000,1");
        }
        Line_50_1: Line {
            parameters:
                Length_Line = 50;
                @display("p=543,176;b=40,4,,#408080,#000000,1");
        }
        Line_50_2: Line {
            parameters:
                Length_Line = 50;
                @display("p=335,675;b=40,4,,#408080,#000000,1");
        }
        Line_50_3: Line {
            parameters:
                Length_Line = 50;
                @display("p=689,335;b=4,40,,#408080,#000000,1");
        }
        Line_50_4: Line {
            parameters:
                Length_Line = 50;
                @display("p=175,538;b=4,40,,#408080,#000000,1");
        }
        Line_50_5: Line {
            parameters:
                Length_Line = 50;
                @display("p=490,219;b=4,40,,#408080,#000000,1");
        }
        Line_50_6: Line {
            parameters:
                Length_Line = 50;
                @display("p=385,648;b=4,40,,#408080,#000000,1");
        }
        Line_50_7: Line {
            parameters:
                Length_Line = 50;
                @display("p=641,376;b=40,4,,#408080,#000000,1");
        }
        Line_50_8: Line {
            parameters:
                Length_Line = 50;
                @display("p=223,483;b=40,4,,#408080,#000000,1");
        }
        Line_50_9: Line {
            parameters:
                Length_Line = 50;
                @display("p=385,317;b=4,40,,#408080,#000000,1");
        }
        Line_50_10: Line {
            parameters:
                Length_Line = 50;
                @display("p=490,538;b=4,40,,#408080,#000000,1");
        }
        Line_50_11: Line {
            parameters:
                Length_Line = 50;
                @display("p=543,483;b=40,4,,#408080,#000000,1");
        }
        Line_50_12: Line {
            parameters:
                Length_Line = 50;
                @display("p=324,376;b=40,4,,#408080,#000000,1");

        }
    connections allowunconnected:
        cntrl[0] --> { @display("ls=,0"); } --> PSE2x2_NE.fromRouter;
        cntrl[1] --> { @display("ls=,0"); } --> PSE2x2_SW.fromRouter;
        cntrl[2] --> { @display("ls=,0"); } --> PSE1x2_N.fromRouter;
        cntrl[4] --> { @display("ls=,0"); } --> PSE1x2_S.fromRouter;
        cntrl[3] --> { @display("ls=,0"); } --> PSE1x2_E.fromRouter;
        cntrl[5] --> { @display("ls=,0"); } --> PSE1x2_W.fromRouter;


        in[0] <--> {@display("m=n");} <--> Line_30_1.photonicIn;
        in[2] <--> {@display("m=s");} <--> Line_30_2.photonicIn;
        in[1] <--> {@display("m=e");} <--> Line_30_3.photonicIn;
        in[3] <--> {@display("m=w");} <--> Line_30_4.photonicIn;

        Line_30_1.photonicOut <--> Cross_1.photonicVertIn;
        Line_30_2.photonicOut <--> Cross_2.photonicVertIn;
        Line_30_3.photonicOut <--> Cross_1.photonicHorizIn;
        Line_30_4.photonicOut <--> Cross_2.photonicHorizIn;

        Cross_1.photonicVertOut <--> PSE1x2_E.photonicHorizIn;
        Cross_2.photonicVertOut <--> PSE1x2_W.photonicHorizIn;
        Cross_1.photonicHorizOut <--> PSE1x2_N.photonicHorizIn;
        Cross_2.photonicHorizOut <--> PSE1x2_S.photonicHorizIn;

        PSE1x2_N.photonicHorizOut <--> Line_50_1.photonicIn;
        PSE1x2_S.photonicHorizOut <--> Line_50_2.photonicIn;
        PSE1x2_E.photonicHorizOut <--> Line_50_3.photonicIn;
        PSE1x2_W.photonicHorizOut <--> Line_50_4.photonicIn;

        Line_50_1.photonicOut <--> Bend_1.photonicIn;
        Line_50_2.photonicOut <--> Bend_2.photonicIn;
        Line_50_3.photonicOut <--> Bend_3.photonicIn;
        Line_50_4.photonicOut <--> Bend_4.photonicIn;

        Bend_1.photonicOut <--> Line_50_5.photonicIn;
        Bend_2.photonicOut <--> Line_50_6.photonicIn;
        Bend_3.photonicOut <--> Line_50_7.photonicIn;
        Bend_4.photonicOut <--> Line_50_8.photonicIn;

        Line_50_5.photonicOut <--> Cross_3.photonicVertIn;
        Line_50_6.photonicOut <--> Cross_4.photonicVertIn;
        Line_50_7.photonicOut <--> Cross_5.photonicHorizIn;
        Line_50_8.photonicOut <--> Cross_6.photonicHorizIn;

        Cross_3.photonicVertOut <--> Line_30_5.photonicIn;
        Cross_4.photonicVertOut <--> Line_30_6.photonicIn;
        Cross_5.photonicHorizOut <--> Line_30_7.photonicIn;
        Cross_6.photonicHorizOut <--> Line_30_8.photonicIn;

        Cross_3.photonicHorizOut <--> PSE2x2_NE.photonicHorizIn;
        Cross_4.photonicHorizOut <--> PSE2x2_SW.photonicHorizIn;
        Cross_5.photonicVertOut <--> PSE2x2_NE.photonicVertIn;
        Cross_6.photonicVertOut <--> PSE2x2_SW.photonicVertIn;

        Line_20_1.photonicOut <--> Cross_3.photonicHorizIn;
        Line_20_2.photonicOut <--> Cross_4.photonicHorizIn;
        Line_20_3.photonicOut <--> Cross_5.photonicVertIn;
        Line_20_4.photonicOut <--> Cross_6.photonicVertIn;

        Line_50_9.photonicOut <--> Bend_5.photonicIn;
        Line_50_10.photonicOut <--> Bend_6.photonicIn;
        Line_50_11.photonicOut <--> Bend_7.photonicIn;
        Line_50_12.photonicOut <--> Bend_8.photonicIn;

        Bend_5.photonicOut <--> Line_20_1.photonicIn;
        Bend_6.photonicOut <--> Line_20_2.photonicIn;
        Bend_7.photonicOut <--> Line_20_3.photonicIn;
        Bend_8.photonicOut <--> Line_20_4.photonicIn;

        Line_30_7.photonicOut <--> Cross_8.photonicHorizIn;
        Cross_8.photonicHorizOut <--> Cross_7.photonicHorizIn;
        Cross_7.photonicHorizOut <--> Line_50_12.photonicIn;

        Line_30_8.photonicOut <--> Cross_9.photonicHorizIn;
        Cross_9.photonicHorizOut <--> Cross_10.photonicHorizIn;
        Cross_10.photonicHorizOut <--> Line_50_11.photonicIn;

        Line_30_5.photonicOut <--> Cross_8.photonicVertIn;
        Cross_8.photonicVertOut <--> Cross_10.photonicVertIn;
        Cross_10.photonicVertOut <--> Line_50_10.photonicIn;

        Line_30_6.photonicOut <--> Cross_9.photonicVertIn;
        Cross_9.photonicVertOut <--> Cross_7.photonicVertIn;
        Cross_7.photonicVertOut <--> Line_50_9.photonicIn;

        PSE2x2_NE.photonicVertOut <--> PSE1x2_N.photonicVertIn;
        PSE2x2_NE.photonicHorizOut <--> PSE1x2_E.photonicVertIn;
        PSE2x2_SW.photonicVertOut <--> PSE1x2_S.photonicVertIn;
        PSE2x2_SW.photonicHorizOut <--> PSE1x2_W.photonicVertIn;


        PSE1x2_N.photonicVertOut <--> out[0];
        PSE1x2_S.photonicVertOut <--> out[2];
        PSE1x2_E.photonicVertOut <--> out[1];
        PSE1x2_W.photonicVertOut <--> out[3];

}

