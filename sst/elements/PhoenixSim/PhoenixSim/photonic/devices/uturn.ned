// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Johnnie Chan 2008                           *
// *                                                                       *
// * file: uturn.ned                                                       *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

import photonic.devices.Bend;
import photonic.devices.Line;

module UTurn
{
    parameters:
        double TurnDistance;
        double LatencyRate_Line;
        double Latency_Bend;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
    gates:
        inout photonicIn;
        inout photonicOut;
    submodules:
        Bend_In: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=100,100;b=26,26");
        }
        Line_Straight: Line {
            parameters:
                Length_Line = TurnDistance;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=150,100;b=48,8");
        }
        Bend_Out: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=200,100;b=26,26");

        }
    connections allowunconnected:

        photonicIn <--> { @display("m=s"); } <--> Bend_In.photonicIn;
        Bend_In.photonicOut <--> Line_Straight.photonicIn;
        Line_Straight.photonicOut <--> Bend_Out.photonicIn;
        Bend_Out.photonicOut <--> { @display("m=s"); } <--> photonicOut;
}

