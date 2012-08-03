package photonic.devices;

import photonic.devices.Line;
import photonic.devices.Bend;

module Twist
{
    parameters:
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
    gates:
        inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;
        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;
    submodules:
        LineShortNE: Line {
            parameters:
                Length_Line = 31.82;
        }
        BendInN: Bend {
            parameters:
                Angle_Bend = 45;
        }
        BendOutN: Bend {
            parameters:
                Angle_Bend = 45;
        }
        LineLongNE: Line {
            parameters:
                Length_Line = 144.96;
        }
        BendInS: Bend {
            parameters:
                Angle_Bend = 45;
        }
        BendOutS: Bend {
            parameters:
                Angle_Bend = 45;
        }
        LineShortSW: Line {
            parameters:
                Length_Line = 31.82;
        }
        BendInE: Bend {
            parameters:
                Angle_Bend = 45;
        }
        BendOutE: Bend {
            parameters:
                Angle_Bend = 45;
        }
        LineLongSW: Line {
            parameters:
                Length_Line = 144.96;
        }
        BendInW: Bend {
            parameters:
                Angle_Bend = 45;
        }
        BendOutW: Bend {
            parameters:
                Angle_Bend = 45;
        }
    connections allowunconnected:
        photonicInN <--> BendInN.photonicIn;
        photonicInS <--> BendInS.photonicIn;
        photonicInE <--> BendInE.photonicIn;
        photonicInW <--> BendInW.photonicIn;

        BendOutN.photonicOut <--> photonicOutN;
        BendOutS.photonicOut <--> photonicOutS;
        BendOutE.photonicOut <--> photonicOutE;
        BendOutW.photonicOut <--> photonicOutW;

        BendInN.photonicOut <--> LineLongNE.photonicIn;
        BendInS.photonicOut <--> LineLongSW.photonicIn;
        BendInE.photonicOut <--> LineShortNE.photonicIn;
        BendInW.photonicOut <--> LineShortSW.photonicIn;

        LineShortNE.photonicOut <--> BendOutN.photonicIn;
        LineShortSW.photonicOut <--> BendOutS.photonicIn;
        LineLongNE.photonicOut <--> BendOutE.photonicIn;
        LineLongSW.photonicOut <--> BendOutW.photonicIn;

}
