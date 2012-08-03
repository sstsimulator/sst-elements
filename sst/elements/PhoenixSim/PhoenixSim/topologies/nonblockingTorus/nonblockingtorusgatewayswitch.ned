package topologies.nonblockingTorus;

import photonic.devices.PSE1x2;
import photonic.devices.Line;
import photonic.devices.Bend;
import photonic.devices.End;

module NonblockingTorusGatewaySwitch
{
    parameters:
        @display("bgb=330,340");
    gates:
        inout photonicInEjection;
        inout photonicInGateway;
        input controlPSE1x2;
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
    submodules:
        PSEOut: PSE1x2 {
            parameters:
                @display("p=164,200;i=components/pse1x2");
        }
        Bend_Injection: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=68,200;b=24,24,,,,1");
        }
        Bend_Ejection: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=164,144;b=24,24,,,,1");
        }
        Line_ToInjectionTurn: Line {
            parameters:
                Length_Line = 30;
                @display("p=113,200;b=40,4,,#408080,#000000,1");
        }
        Line_InjectionDeflected: Line {
            parameters:
                Length_Line = 30;
                @display("p=160,261;b=10,42,,#408080,#000000,1");
        }
        Line_Ejection: Line {
            parameters:
                Length_Line = 30;
                @display("p=212,145;b=44,10,,#408080,#000000,1");
        }
        Line_AfterInjectionTurn: Line {
            parameters:
                Length_Line = 30;
                @display("p=68,144;b=4,72,,#408080,#000000,1");
        }
    connections allowunconnected:

        Line_Ejection.photonicOut <--> photonicOutGateway;
        Bend_Ejection.photonicOut <--> Line_Ejection.photonicIn;
        photonicInEjection <--> Bend_Ejection.photonicIn;

        photonicInGateway <--> { @display("m=e,100,56,79,50"); } <--> PSEOut.photonicHorizIn;
        PSEOut.photonicVertOut <--> Line_InjectionDeflected.photonicIn;
        Line_InjectionDeflected.photonicOut <--> photonicOutInjectionS;
        PSEOut.photonicHorizOut <--> Line_ToInjectionTurn.photonicIn;
        Line_ToInjectionTurn.photonicOut <--> Bend_Injection.photonicIn;
        Bend_Injection.photonicOut <--> Line_AfterInjectionTurn.photonicIn;
        Line_AfterInjectionTurn.photonicOut <--> { @display("m=n,150,6,0,0"); } <--> photonicOutInjectionN;

        controlPSE1x2 --> { @display("ls=,0"); } --> PSEOut.fromRouter;
}

