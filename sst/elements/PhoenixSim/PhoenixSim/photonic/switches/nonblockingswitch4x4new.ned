// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: nonblockingswitch4x4new.ned                                        *
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

module NonBlockingSwitch4x4New like PhotonicSwitch
{
    parameters:
      
        
        int numPorts = 4;
        int numPSE = 8;
        
        int variant = 2;
        
      	
        
    gates:
        //0-N
        //1-E
        //2-S
        //3-W
        inout in[numPorts];
        inout out[numPorts];

	
        input cntrl[numPSE];
    submodules:
        PSE1x2_N: PSE1x2 {
            parameters:
               
                @display("p=309,106;b=42,42,,,,1");
        }
        PSE1x2_S: PSE1x2 {
            parameters:
                
                @display("p=209,406;b=42,42,,,,1");
        }
        PSE1x2_E: PSE1x2 {
            parameters:
              
                @display("p=409,206;b=42,42,,,,1");
        }
        PSE1x2_W: PSE1x2 {
            parameters:
               
                @display("p=109,306;b=42,42,,,,1");
        }
        PSE1x2_NW: PSE1x2 {
            parameters:
               
                @display("p=309,106;b=42,42,,,,1");
        }
        PSE1x2_NE: PSE1x2 {
            parameters:
               
                @display("p=209,406;b=42,42,,,,1");
        }
        PSE1x2_SW: PSE1x2 {
            parameters:
               
                @display("p=409,206;b=42,42,,,,1");
        }
        PSE1x2_SE: PSE1x2 {
            parameters:
               
                @display("p=109,306;b=42,42,,,,1");
        }
        Bend_N: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=209,106;b=10,10,,,,1");
        }
        Bend_S: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=309,406;b=10,10,,,,1");
        }
        Bend_E: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=409,306;b=10,10,,,,1");
        }
        Bend_W: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=109,206;b=10,10,,,,1");
        }
        Line_NPort: Line {
            parameters:
                Length_Line = 80;
                @display("p=409,56;b=4,40,,#408080,#000000,1");
        }
        Line_SPort: Line {
            parameters:
                Length_Line = 80;
                @display("p=109,456;b=4,40,,#408080,#000000,1");
        }
        Line_EPort: Line {
            parameters:
                Length_Line = 80;
                @display("p=459,106;b=40,4,,#408080,#000000,1");
        }
        Line_WPort: Line {
            parameters:
                Length_Line = 80;
                @display("p=59,406;b=40,4,,#408080,#000000,1");
        }
        Line_NBendA: Line {
            parameters:
                Length_Line = 20;
                @display("p=251,105;b=40,4,,#408080,#000000,1");
        }
        Line_NBendB: Line {
            parameters:
                Length_Line = 20;
                @display("p=208,151;b=4,40,,#408080,#000000,1");
        }
        Line_SBendA: Line {
            parameters:
                Length_Line = 20;
                @display("p=265,406;b=40,4,,#408080,#000000,1");
        }
        Line_SBendB: Line {
            parameters:
                Length_Line = 20;
                @display("p=309,356;b=4,40,,#408080,#000000,1");
        }
        Line_EBendA: Line {
            parameters:
                Length_Line = 20;
                @display("p=409,264;b=4,40,,#408080,#000000,1");
        }
        Line_EBendB: Line {
            parameters:
                Length_Line = 20;
                @display("p=364,306;b=40,4,,#408080,#000000,1");
        }
        Line_WBendA: Line {
            parameters:
                Length_Line = 20;
                @display("p=109,247;b=4,40,,#408080,#000000,1");
        }
        Line_WBendB: Line {
            parameters:
                Length_Line = 20;
                @display("p=156,206;b=40,4,,#408080,#000000,1");
        }
    connections allowunconnected:
        cntrl[0] --> { @display("ls=,0"); } --> PSE1x2_NW.fromRouter;
        cntrl[1] --> { @display("ls=,0"); } --> PSE1x2_NE.fromRouter;
        cntrl[2] --> { @display("ls=,0"); } --> PSE1x2_SW.fromRouter;
        cntrl[3] --> { @display("ls=,0"); } --> PSE1x2_SE.fromRouter;
        cntrl[4] --> { @display("ls=,0"); } --> PSE1x2_N.fromRouter;
        cntrl[5] --> { @display("ls=,0"); } --> PSE1x2_S.fromRouter;
        cntrl[6] --> { @display("ls=,0"); } --> PSE1x2_E.fromRouter;
        cntrl[7] --> { @display("ls=,0"); } --> PSE1x2_W.fromRouter;

        in[0] <--> Line_NPort.photonicIn;
        in[2] <--> Line_SPort.photonicIn;
        in[1] <--> Line_EPort.photonicIn;
        in[3] <--> Line_WPort.photonicIn;

        Line_NPort.photonicOut <--> PSE1x2_W.photonicHorizIn;
        Line_SPort.photonicOut <--> PSE1x2_E.photonicHorizIn;
        Line_EPort.photonicOut <--> PSE1x2_N.photonicHorizIn;
        Line_WPort.photonicOut <--> PSE1x2_S.photonicHorizIn;

        PSE1x2_N.photonicHorizOut <--> Line_NBendA.photonicIn;
        PSE1x2_S.photonicHorizOut <--> Line_SBendA.photonicIn;
        PSE1x2_E.photonicHorizOut <--> Line_EBendA.photonicIn;
        PSE1x2_W.photonicHorizOut <--> Line_WBendA.photonicIn;

        Line_NBendA.photonicOut <--> Bend_N.photonicIn;
        Line_SBendA.photonicOut <--> Bend_S.photonicIn;
        Line_EBendA.photonicOut <--> Bend_E.photonicIn;
        Line_WBendA.photonicOut <--> Bend_W.photonicIn;

        Bend_N.photonicOut <--> Line_NBendB.photonicIn;
        Bend_S.photonicOut <--> Line_SBendB.photonicIn;
        Bend_E.photonicOut <--> Line_EBendB.photonicIn;
        Bend_W.photonicOut <--> Line_WBendB.photonicIn;

        Line_NBendB.photonicOut <--> PSE1x2_NW.photonicHorizIn;
        Line_SBendB.photonicOut <--> PSE1x2_SE.photonicHorizIn;
        Line_EBendB.photonicOut <--> PSE1x2_NE.photonicHorizIn;
        Line_WBendB.photonicOut <--> PSE1x2_SW.photonicHorizIn;

        PSE1x2_NW.photonicHorizOut <--> PSE1x2_SW.photonicVertIn;
        PSE1x2_NE.photonicHorizOut <--> PSE1x2_NW.photonicVertIn;
        PSE1x2_SW.photonicHorizOut <--> PSE1x2_SE.photonicVertIn;
        PSE1x2_SE.photonicHorizOut <--> PSE1x2_NE.photonicVertIn;

        PSE1x2_NW.photonicVertOut <--> PSE1x2_W.photonicVertIn;
        PSE1x2_NE.photonicVertOut <--> PSE1x2_N.photonicVertIn;
        PSE1x2_SW.photonicVertOut <--> PSE1x2_S.photonicVertIn;
        PSE1x2_SE.photonicVertOut <--> PSE1x2_E.photonicVertIn;

        PSE1x2_N.photonicVertOut <--> out[0];
        PSE1x2_S.photonicVertOut <--> out[2];
        PSE1x2_E.photonicVertOut <--> out[1];
        PSE1x2_W.photonicVertOut <--> out[3];

}

