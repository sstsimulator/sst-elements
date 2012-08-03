
// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *	(c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: gatewayswitch.ned				                               *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package topologies.blockingTorus;
import photonic.devices.PSE1x2;
import photonic.devices.Line;
import photonic.devices.Cross;
import photonic.devices.End;

module GatewaySwitchRight
{
    parameters:
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double LatencyRate_Line;
        double Latency_Cross;
        @display("bgb=266,276");
    gates:
        inout photonicInInjectionN;
        inout photonicInInjectionS;
        inout photonicInEjection;
        inout photonicInGateway;
        
        input controlPSE1x2_A;
        input controlPSE1x2_B;
        
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
    submodules:
        PSE1x2_A: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=116,168;i=components/pse1x2");
        }
        PSE1x2_B: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=216,168;i=components/pse1x2");
        }
        Cross_A: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=116,68;b=24,24,,,,1");
        }
        Cross_B: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=216,68;b=24,24,,,,1");
        }
        Line_ToGatewayOut: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=65,72;b=40,4,,#408080,#000000,1");
        }
        Line_EjectionLane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=161,72;b=40,4,,#408080,#000000,1");
        }
        Line_ELane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=217,112;b=4,40,,#408080,#000000,1");
        }
        Line_WLane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=113,232;b=4,40,,#408080,#000000,1");
        }
        Terminate: End {

            parameters:
                @display("p=261,168;b=10,8");
        }
    connections allowunconnected:
        controlPSE1x2_A --> { @display("ls=,0"); } --> PSE1x2_A.fromRouter;
        controlPSE1x2_B --> PSE1x2_B.fromRouter; //
                                                 //
        photonicInInjectionN <--> { @display("m=n"); } <--> Cross_B.photonicVertIn;
        photonicInInjectionS <--> { @display("m=s"); } <--> Line_WLane.photonicIn;
        photonicInEjection <--> { @display("m=e"); } <--> Cross_B.photonicHorizIn;
        photonicInGateway <--> { @display("m=w"); } <--> PSE1x2_A.photonicHorizIn; //
                                                                      //
        Line_WLane.photonicOut <--> PSE1x2_A.photonicVertIn;
        PSE1x2_A.photonicHorizOut <--> PSE1x2_B.photonicHorizIn;
        PSE1x2_A.photonicVertOut <--> Cross_A.photonicVertIn;

        Cross_B.photonicVertOut <--> Line_ELane.photonicIn;
        Cross_B.photonicHorizOut <--> Line_EjectionLane.photonicIn;

        Line_ELane.photonicOut <--> PSE1x2_B.photonicVertIn;
        Line_EjectionLane.photonicOut <--> Cross_A.photonicHorizIn;
        Cross_A.photonicHorizOut <--> Line_ToGatewayOut.photonicIn;

        Line_ToGatewayOut.photonicOut <--> { @display("m=w"); } <--> photonicOutGateway;
        Cross_A.photonicVertOut <--> { @display("m=n"); } <--> photonicOutInjectionN;
        PSE1x2_B.photonicVertOut <--> { @display("m=s"); } <--> photonicOutInjectionS;
        PSE1x2_B.photonicHorizOut <--> Terminate.photonicIn;
}

module GatewaySwitch
{
    parameters:
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double LatencyRate_Line;

        double Latency_Cross;
        @display("bgb=330,340");
    gates:
        inout photonicInInjectionN;
        inout photonicInInjectionS;
        inout photonicInEjection;
        inout photonicInGateway;
        
        input controlPSE1x2_A;
        input controlPSE1x2_B;
        
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
    submodules:
        PSE1x2_A: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=108,136;i=components/pse1x2");
        }
        PSE1x2_B: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=216,136;i=components/pse1x2");
        }
        Cross_A: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=108,248;b=24,24,,,,1");
        }
        Cross_B: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=216,248;b=24,24,,,,1");
        }
        Line_ToGatewayOut: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=265,248;b=40,4,,#408080,#000000,1");
        }
        Line_EjectionLane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=161,248;b=56,4,,#408080,#000000,1");
        }
        Line_ELane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=216,80;b=4,40,,#408080,#000000,1");
        }
        Line_WLane: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=108,192;b=4,40,,#408080,#000000,1");
        }
        Terminate: End {
            parameters:
                @display("p=66,136;b=8,8,,,,1");
        }
    connections allowunconnected:
        controlPSE1x2_A --> { @display("ls=,0"); } --> PSE1x2_A.fromRouter;
        controlPSE1x2_B --> PSE1x2_B.fromRouter; //
                                                 //
        photonicInInjectionN <--> { @display("m=n"); } <--> Line_ELane.photonicIn;
        photonicInInjectionS <--> { @display("m=s"); } <--> Cross_A.photonicVertIn;
        photonicInEjection <--> { @display("m=w"); } <--> Cross_A.photonicHorizIn;
        photonicInGateway <--> { @display("m=e"); } <--> PSE1x2_B.photonicHorizIn; //
                                                                      //
        Line_WLane.photonicOut <--> PSE1x2_A.photonicVertIn;
        Cross_A.photonicVertOut <--> Line_WLane.photonicIn;

        PSE1x2_B.photonicHorizOut <--> PSE1x2_A.photonicHorizIn;
        PSE1x2_B.photonicVertOut <--> Cross_B.photonicVertIn;

        Line_ELane.photonicOut <--> PSE1x2_B.photonicVertIn;

        Cross_A.photonicHorizOut <--> Line_EjectionLane.photonicIn;
        Line_EjectionLane.photonicOut <--> Cross_B.photonicHorizIn;

        Cross_B.photonicHorizOut <--> Line_ToGatewayOut.photonicIn;

        Line_ToGatewayOut.photonicOut <--> { @display("m=e,0,0"); } <--> photonicOutGateway;
        PSE1x2_A.photonicVertOut <--> { @display("m=n"); } <--> photonicOutInjectionN;
        PSE1x2_A.photonicHorizOut <--> Terminate.photonicIn;
        Cross_B.photonicVertOut <--> { @display("m=s"); } <--> photonicOutInjectionS;
}
