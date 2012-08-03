package photonic.devices;

import photonic.devices.Bend;
import photonic.devices.Line;

module Shift
{
    parameters:
        double Shift;
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("bgb=195,404");
    gates:
        inout photonicInA;
        inout photonicInB;
        inout photonicOutA;
        inout photonicOutB;
    submodules:
        BendA: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=75,75;b=40,40,,,,1");
        }
        BendB: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=140,140;b=40,40,,,,1");
        }
        BendC: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=75,275;b=40,40,,,,1");
        }
        BendD: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=140,340;b=40,40,,,,1");
        }
        Line_NubA: Line {
            parameters:
                Length_Line = 5;
                @display("p=140,75;b=40,16,,,,1");
        }
        Line_NubB: Line {
            parameters:
                Length_Line = 5;
                @display("p=75,340;b=40,16,,,,1");
        }
        Line_A: Line {
            parameters:
                Length_Line = Shift-5;
                @display("p=75,175;b=16,100,,,,1");
        }
        Line_B: Line {
            parameters:
                Length_Line = Shift-5;
                @display("p=140,240;b=16,100,,,,1");
        }
    connections allowunconnected:
        photonicInA <--> Line_NubA.photonicIn;
        Line_NubA.photonicOut <--> BendA.photonicIn;
        BendA.photonicOut <--> Line_A.photonicIn;
        Line_A.photonicOut <--> BendC.photonicIn;
        BendC.photonicOut <--> photonicOutA;

        photonicInB <--> Line_NubB.photonicIn;
        Line_NubB.photonicOut <--> BendD.photonicIn;
        BendD.photonicOut <--> Line_B.photonicIn;
        Line_B.photonicOut <--> BendB.photonicIn;
        BendB.photonicOut <--> photonicOutB;
}

