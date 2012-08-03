package topologies.blockingTorus;

import photonic.devices.PSE1x2;
import photonic.devices.Line;

module EjectionSwitch
{
    parameters:
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double LatencyRate_Line;

        @display("bgb=271,267");
    gates:
        inout photonicInThrough;
        inout photonicInN;
        inout photonicInS;
        input controlPSE1x2_A;
        input controlPSE1x2_B;
        inout photonicOutGateway;
        inout photonicOutN;
        inout photonicOutS;

    submodules:
        PSE1x2_A: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=100,141;i=components/pse1x2");
            //display: "o=,,1;p=100,141;b=42,42";
        }
        PSE1x2_B: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=200,141;i=components/pse1x2");
            //display: "o=,,1;p=200,141;b=42,42";
        }
        Line_SOutPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=200,211;b=4,40,,#408080,#000000,1");
        }
        Line_NOutPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=100,70;b=4,40,,#408080,#000000,1");
        }
    connections allowunconnected:
        photonicInThrough <--> { @display("m=e,,,0,0"); } <--> PSE1x2_B.photonicVertIn;
        photonicInS <--> { @display("m=s,,,52,0"); } <--> PSE1x2_A.photonicHorizIn;
        photonicInN <--> { @display("m=n"); } <--> PSE1x2_B.photonicHorizIn; //
                                                                //
        PSE1x2_B.photonicHorizOut <--> Line_SOutPort.photonicIn;
        Line_SOutPort.photonicOut <--> { @display("m=s"); } <--> photonicOutS;
        PSE1x2_B.photonicVertOut <--> PSE1x2_A.photonicVertIn; //
                                                              //
        PSE1x2_A.photonicHorizOut <--> Line_NOutPort.photonicIn;
        Line_NOutPort.photonicOut <--> { @display("m=n"); } <--> photonicOutN;
        PSE1x2_A.photonicVertOut <--> { @display("m=w"); } <--> photonicOutGateway; //
                                                                       //
        controlPSE1x2_A --> { @display("ls=,0"); } --> PSE1x2_A.fromRouter;
        controlPSE1x2_B --> { @display("ls=,0"); } --> PSE1x2_B.fromRouter;
}


module EjectionSwitchRight
{
    parameters:
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;
        double DropDelay_1x2;

        double LatencyRate_Line;

        @display("bgb=271,267");
    gates:
        inout photonicInThrough;
        inout photonicInN;
        inout photonicInS;
        input controlPSE1x2_A;
        input controlPSE1x2_B;
        inout photonicOutGateway;
        inout photonicOutN;
        inout photonicOutS;

    submodules:
        PSE1x2_A: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=100,141;b=42,42,,,,1");
        }
        PSE1x2_B: PSE1x2 {
            parameters:
                RingOn_ER_1x2 = RingOn_ER_1x2;
                RingOff_ER_1x2 = RingOff_ER_1x2;
                ThroughDelay_1x2 = ThroughDelay_1x2;
                DropDelay_1x2 = DropDelay_1x2;
                @display("p=200,141;b=42,42,,,,1");
        }
        Line_SOutPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=200,211;b=4,40,,#408080,#000000,1");
        }
        Line_NOutPort: Line {
            parameters:
                Length_Line = 30;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=100,70;b=4,40,,#408080,#000000,1");
        }
    connections allowunconnected:
        photonicInThrough <--> { @display("m=w,,,0,0"); } <--> PSE1x2_A.photonicVertIn;
        photonicInS <--> { @display("m=s,,,52,0"); } <--> PSE1x2_A.photonicHorizIn;
        photonicInN <--> { @display("m=n"); } <--> PSE1x2_B.photonicHorizIn; //
                                                                //
        PSE1x2_B.photonicHorizOut <--> Line_SOutPort.photonicIn;
        Line_SOutPort.photonicOut <--> { @display("m=s"); } <--> photonicOutS;
        PSE1x2_B.photonicVertOut <--> { @display("m=e"); } <--> photonicOutGateway; //
                                                                       //
        PSE1x2_A.photonicHorizOut <--> Line_NOutPort.photonicIn;
        Line_NOutPort.photonicOut <--> { @display("m=n"); } <--> photonicOutN;
        PSE1x2_A.photonicVertOut <--> PSE1x2_B.photonicVertIn; //
                                                              //
        controlPSE1x2_A --> { @display("ls=,0"); } --> PSE1x2_A.fromRouter;
        controlPSE1x2_B --> { @display("ls=,0"); } --> PSE1x2_B.fromRouter;
}


