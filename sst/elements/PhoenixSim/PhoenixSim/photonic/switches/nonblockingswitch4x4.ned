
// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: nonblockingswitch4x4.ned                                        *
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

module NonBlockingSwitch4x4
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

        double LatencyRate_Line;
        double Latency_Bend;
        double Latency_Cross;
        
      
        
        @display("bgb=497,498");
    gates:
        inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;

        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;

        input controlPSE2x2NE;
        input controlPSE2x2SW;
        input controlPSE1x2N;
        input controlPSE1x2S;
        input controlPSE1x2E;
        input controlPSE1x2W;

    submodules:
        PSE2x2_NE: PSE2x2 {
            parameters:
                RingOn_ER_2x2 = RingOn_ER_2x2;
                RingOff_ER_2x2 = RingOff_ER_2x2;
                CrossDelay_2x2 = CrossDelay_2x2;

                BarDelay_2x2 = BarDelay_2x2;
                @display("p=309,206;i=components/pse2x2");
        }
        PSE2x2_SW: PSE2x2 {
            parameters:
                RingOn_ER_2x2 = RingOn_ER_2x2;
                RingOff_ER_2x2 = RingOff_ER_2x2;
                CrossDelay_2x2 = CrossDelay_2x2;

                BarDelay_2x2 = BarDelay_2x2;
                @display("p=209,306;i=components/pse2x2");
        }
        PSE1x2_N: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;

                DropDelay_1x2 = DropDelay_1x2;
                @display("p=309,106;i=components/pse1x2");
        }
        PSE1x2_S: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;

                DropDelay_1x2 = DropDelay_1x2;
                @display("p=209,406;i=components/pse1x2");
        }
        PSE1x2_E: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;

                DropDelay_1x2 = DropDelay_1x2;
                @display("p=409,206;i=components/pse1x2");
        }
        PSE1x2_W: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;

                DropDelay_1x2 = DropDelay_1x2;
                @display("p=109,306;i=components/pse1x2");
        }
        Cross_NEPort: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=409,106;b=24,24,,,,1");
        }
        Cross_SWPort: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=109,406;b=24,24,,,,1");
        }
        Cross_NWBend: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=209,206;b=24,24,,,,1");
        }
        Cross_SEBend: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=309,306;b=24,24,,,,1");
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
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=409,56;b=4,40,,#408080,#000000,1");
        }
        Line_SPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=109,456;b=4,40,,#408080,#000000,1");
        }
        Line_EPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=459,106;b=40,4,,#408080,#000000,1");
        }
        Line_WPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=59,406;b=40,4,,#408080,#000000,1");
        }
        Line_NBendA: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=251,105;b=40,4,,#408080,#000000,1");
        }
        Line_NBendB: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=208,151;b=4,40,,#408080,#000000,1");
        }
        Line_SBendA: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=265,406;b=40,4,,#408080,#000000,1");
        }
        Line_SBendB: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=309,356;b=4,40,,#408080,#000000,1");
        }
        Line_EBendA: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=409,264;b=4,40,,#408080,#000000,1");
        }
        Line_EBendB: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=364,306;b=40,4,,#408080,#000000,1");
        }
        Line_WBendA: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=109,247;b=4,40,,#408080,#000000,1");
        }
        Line_WBendB: Line {
            parameters:
                Length_Line = 75;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=156,206;b=40,4,,#408080,#000000,1");
        }
    connections allowunconnected:
        controlPSE2x2NE --> { @display("ls=,0"); } --> PSE2x2_NE.fromRouter;
        controlPSE2x2SW --> { @display("ls=,0"); } --> PSE2x2_SW.fromRouter;
        controlPSE1x2N --> { @display("ls=,0"); } --> PSE1x2_N.fromRouter;
        controlPSE1x2S --> { @display("ls=,0"); } --> PSE1x2_S.fromRouter;
        controlPSE1x2E --> { @display("ls=,0"); } --> PSE1x2_E.fromRouter;
        controlPSE1x2W --> { @display("ls=,0"); } --> PSE1x2_W.fromRouter;


        photonicInN <--> Line_NPort.photonicIn;
        photonicInS <--> Line_SPort.photonicIn;
        photonicInE <--> Line_EPort.photonicIn;
        photonicInW <--> Line_WPort.photonicIn;

        Line_NPort.photonicOut <--> Cross_NEPort.photonicVertIn;
        Line_SPort.photonicOut <--> Cross_SWPort.photonicVertIn;
        Line_EPort.photonicOut <--> Cross_NEPort.photonicHorizIn;
        Line_WPort.photonicOut <--> Cross_SWPort.photonicHorizIn;

        Cross_NEPort.photonicVertOut <--> PSE1x2_E.photonicHorizIn;
        Cross_SWPort.photonicVertOut <--> PSE1x2_W.photonicHorizIn;
        Cross_NEPort.photonicHorizOut <--> PSE1x2_N.photonicHorizIn;
        Cross_SWPort.photonicHorizOut <--> PSE1x2_S.photonicHorizIn;

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

        Line_NBendB.photonicOut <--> Cross_NWBend.photonicVertIn;
        Line_SBendB.photonicOut <--> Cross_SEBend.photonicVertIn;
        Line_EBendB.photonicOut <--> Cross_SEBend.photonicHorizIn;
        Line_WBendB.photonicOut <--> Cross_NWBend.photonicHorizIn;

        Cross_NWBend.photonicVertOut <--> PSE2x2_SW.photonicVertIn;
        Cross_SEBend.photonicVertOut <--> PSE2x2_NE.photonicVertIn;
        Cross_SEBend.photonicHorizOut <--> PSE2x2_SW.photonicHorizIn;
        Cross_NWBend.photonicHorizOut <--> PSE2x2_NE.photonicHorizIn;

        PSE2x2_NE.photonicVertOut <--> PSE1x2_N.photonicVertIn;
        PSE2x2_NE.photonicHorizOut <--> PSE1x2_E.photonicVertIn;
        PSE2x2_SW.photonicVertOut <--> PSE1x2_S.photonicVertIn;
        PSE2x2_SW.photonicHorizOut <--> PSE1x2_W.photonicVertIn;

        PSE1x2_N.photonicVertOut <--> photonicOutN;
        PSE1x2_S.photonicVertOut <--> photonicOutS;
        PSE1x2_E.photonicVertOut <--> photonicOutE;
        PSE1x2_W.photonicVertOut <--> photonicOutW;

}


module NonBlockingSwitch4x4_P like PhotonicSwitch
{
    parameters:
        int numPorts = default(4);
        int numPSE = 6;
        
        int variant = 0;
        double pse1x2Width;
        double pse2x2Width;

    gates:
        //0-N
        //1-E
        //2-S
        //3-W
        inout in[numPorts];
        inout out[numPorts];

		
         //0-controlPSE2x2NE 
        // 1-controlPSE2x2SW
        // 2-controlPSE1x2N 
        // 3-controlPSE1x2S
        //4-controlPSE1x2E
        //5-controlPSE1x2W 
        input cntrl[numPSE];



    submodules:
        sw: NonBlockingSwitch4x4;
    connections allowunconnected:
        cntrl[0] --> { @display("ls=,0"); } --> sw.controlPSE2x2NE;
        cntrl[1] --> { @display("ls=,0"); } --> sw.controlPSE2x2SW;
        cntrl[2] --> { @display("ls=,0"); } --> sw.controlPSE1x2N;
        cntrl[3] --> { @display("ls=,0"); } --> sw.controlPSE1x2S;
        cntrl[4] --> { @display("ls=,0"); } --> sw.controlPSE1x2E;
        cntrl[5] --> { @display("ls=,0"); } --> sw.controlPSE1x2W;

        in[0] <--> sw.photonicInN;
        in[2] <--> sw.photonicInS;
        in[1] <--> sw.photonicInE;
        in[3] <--> sw.photonicInW;

        sw.photonicOutN <--> out[0];
        sw.photonicOutS <--> out[2];
        sw.photonicOutE <--> out[1];
        sw.photonicOutW <--> out[3];

}
