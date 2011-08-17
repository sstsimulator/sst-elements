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
module NonBlockingSwitch4x4Regular like PhotonicSwitch
{
    parameters:
       
        int numPorts = 4;
        int numPSE = 6;

        int variant = 1;

        double pse1x2Width;
        double pse2x2Width;
        double crossWidth;
        
        double dimensionX = default(430);
        double dimensionY = default(430);
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
               
                @display("p=589,276;b=54,54,,,,1");
        }
        PSE2x2_SW: PSE2x2 {
            parameters:
              
                @display("p=269,594;b=54,54,,,,1");
        }
        PSE1x2_N: PSE1x2 {
            parameters:
              
                @display("p=589,176;b=42,42,,,,1");
        }
        PSE1x2_S: PSE1x2 {
            parameters:
              
                @display("p=269,675;b=42,42,,,,1");
        }
        PSE1x2_E: PSE1x2 {
            parameters:
              
                @display("p=689,276;b=42,42,,,,1");
        }
        PSE1x2_W: PSE1x2 {
            parameters:
             
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
                @display("p=385,376;b=24,24,,,,1");
        }
        Cross_4: Cross {
            parameters:
                @display("p=490,376;b=24,24,,,,1");
        }
        Cross_5: Cross {
            parameters:
                @display("p=385,483;b=24,24,,,,1");
        }
        Cross_6: Cross {
            parameters:
                @display("p=490,483;b=24,24,,,,1");
        }
        Bend_1: Bend {
            parameters:
                @display("p=490,276;b=10,10,,,,1");
        }
        Bend_2: Bend {
            parameters:
                @display("p=385,594;b=10,10,,,,1");
        }
        Bend_3: Bend {
            parameters:
                @display("p=589,376;b=10,10,,,,1");
        }
        Bend_4: Bend {
            parameters:
                @display("p=175,376;b=10,10,,,,1");
        }
        Bend_5: Bend {
            parameters:
                @display("p=385,176;b=10,10,,,,1");
        }
        Bend_6: Bend {
            parameters:
                @display("p=490,675;b=10,10,,,,1");
        }
        Bend_7: Bend {
            parameters:
                @display("p=689,483;b=10,10,,,,1");
        }
        Bend_8: Bend {
            parameters:
                @display("p=269,483;b=10,10,,,,1");
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
        Line_80_1: Line {
            parameters:
                Length_Line = 80;
                @display("p=443,176;b=40,4,,#408080,#000000,1");
        }
        Line_80_2: Line {
            parameters:
                Length_Line = 80;
                @display("p=443,675;b=40,4,,#408080,#000000,1");
        }
        Line_80_3: Line {
            parameters:
                Length_Line = 80;
                @display("p=689,438;b=4,40,,#408080,#000000,1");
        }
        Line_80_4: Line {
            parameters:
                Length_Line = 80;
                @display("p=175,538;b=4,40,,#408080,#000000,1");
        }
        Line_110_1: Line {
            parameters:
                Length_Line = 110;
                @display("p=223,376;b=40,4,,#408080,#000000,1");
        }
        Line_110_2: Line {
            parameters:
                Length_Line = 110;
                @display("p=385,317;b=4,40,,#408080,#000000,1");
        }
        Line_110_3: Line {
            parameters:
                Length_Line = 110;
                @display("p=490,538;b=4,40,,#408080,#000000,1");
        }
        Line_110_4: Line {
            parameters:
                Length_Line = 110;
                @display("p=543,483;b=40,4,,#408080,#000000,1");
        }
    connections allowunconnected:
        cntrl[0] --> { @display("ls=,0"); } --> PSE2x2_NE.fromRouter;
        cntrl[1] --> { @display("ls=,0"); } --> PSE2x2_SW.fromRouter;
        cntrl[2] --> { @display("ls=,0"); } --> PSE1x2_N.fromRouter;
        cntrl[3] --> { @display("ls=,0"); } --> PSE1x2_S.fromRouter;
        cntrl[4] --> { @display("ls=,0"); } --> PSE1x2_E.fromRouter;
        cntrl[5] --> { @display("ls=,0"); } --> PSE1x2_W.fromRouter;


        in[0] <--> { @display("m=n"); } <--> Line_30_1.photonicIn;
        in[2] <--> { @display("m=s"); } <--> Line_30_2.photonicIn;
        in[1] <--> { @display("m=e"); } <--> Line_30_3.photonicIn;
        in[3] <--> { @display("m=w"); } <--> Line_30_4.photonicIn;

        Line_30_1.photonicOut <--> Cross_1.photonicVertIn;
        Line_30_2.photonicOut <--> Cross_2.photonicVertIn;
        Line_30_3.photonicOut <--> Cross_1.photonicHorizIn;
        Line_30_4.photonicOut <--> Cross_2.photonicHorizIn;

        Cross_1.photonicVertOut <--> PSE1x2_E.photonicHorizIn;
        Cross_2.photonicVertOut <--> PSE1x2_W.photonicHorizIn;
        Cross_1.photonicHorizOut <--> PSE1x2_N.photonicHorizIn;
        Cross_2.photonicHorizOut <--> PSE1x2_S.photonicHorizIn;

		PSE1x2_N.photonicHorizOut <--> Line_80_1.photonicIn;
		Line_80_1.photonicOut <--> Bend_5.photonicIn;
		Bend_5.photonicOut <--> Line_110_2.photonicIn;
		Line_110_2.photonicOut <--> Cross_3.photonicVertIn;
		
		PSE2x2_NE.photonicHorizIn <--> Bend_1.photonicOut;
		Bend_1.photonicIn <--> Line_30_5.photonicOut;
		Line_30_5.photonicIn <--> Cross_4.photonicVertIn;
		
		PSE1x2_E.photonicHorizOut <--> Line_80_3.photonicIn;
		Line_80_3.photonicOut <--> Bend_7.photonicIn;
		Bend_7.photonicOut <--> Line_110_4.photonicIn;
		Line_110_4.photonicOut <--> Cross_6.photonicHorizOut;
		
		PSE2x2_NE.photonicVertIn <--> Bend_3.photonicOut;
		Bend_3.photonicIn <--> Line_30_7.photonicOut;
		Line_30_7.photonicIn <--> Cross_4.photonicHorizOut;
		
		PSE1x2_S.photonicHorizOut <--> Line_80_2.photonicIn;
		Line_80_2.photonicOut <--> Bend_6.photonicIn;
		Bend_6.photonicOut <--> Line_110_3.photonicIn;
		Line_110_3.photonicOut <--> Cross_6.photonicVertOut;
		
		PSE2x2_SW.photonicHorizIn <--> Bend_2.photonicOut;
		Bend_2.photonicIn <--> Line_30_6.photonicOut;
		Line_30_6.photonicIn <--> Cross_5.photonicVertOut;
		
		PSE1x2_W.photonicHorizOut <--> Line_80_4.photonicIn;
		Line_80_4.photonicOut <--> Bend_4.photonicIn;
		Bend_4.photonicOut <--> Line_110_1.photonicIn;
		Line_110_1.photonicOut <--> Cross_3.photonicHorizIn;
		
		PSE2x2_SW.photonicVertIn <--> Bend_8.photonicOut;
		Bend_8.photonicIn <--> Line_30_8.photonicOut;
		Line_30_8.photonicIn <--> Cross_5.photonicHorizIn;
        
		Cross_3.photonicVertOut <--> Cross_5.photonicVertIn;
		Cross_3.photonicHorizOut <--> Cross_4.photonicHorizIn;
		Cross_6.photonicVertIn <--> Cross_4.photonicVertOut;
		Cross_6.photonicHorizIn <--> Cross_5.photonicHorizOut;

        PSE2x2_NE.photonicVertOut <--> PSE1x2_N.photonicVertIn;
        PSE2x2_NE.photonicHorizOut <--> PSE1x2_E.photonicVertIn;
        PSE2x2_SW.photonicVertOut <--> PSE1x2_S.photonicVertIn;
        PSE2x2_SW.photonicHorizOut <--> PSE1x2_W.photonicVertIn;


        PSE1x2_N.photonicVertOut <--> out[0];
        PSE1x2_S.photonicVertOut <--> out[2];
        PSE1x2_E.photonicVertOut <--> out[1];
        PSE1x2_W.photonicVertOut <--> out[3];

}

