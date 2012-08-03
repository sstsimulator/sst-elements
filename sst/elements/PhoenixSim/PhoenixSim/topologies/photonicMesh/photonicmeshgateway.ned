//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package topologies.photonicMesh;

import photonic.devices.**;
import photonic.switches.NonBlockingSwitch4x4Regular;
import topologies.ModulatorBank;
import topologies.DetectorBank;

import photonic.PhotonicSwitch;

//
// TODO auto-generated module
//
module PhotonicMeshGateway like PhotonicSwitch
{
    parameters:
        int numPorts = 5;


        double PathSeparation;
        double componentSizeX = 450;
        double componentSizeY = 450;
        double bankSizeX = default(100); //TODO: the gateway dimensions do not scale to bankSize values currently
        double bankSizeY = default(100);

        string switchVariant = default("NonBlockingSwitch4x4Regular");
        int variant = (switchVariant == "NonBlockingSwitch4x4Regular") ? 1 : 
        	((switchVariant == "NonBlockingSwitch4x4_P") ? 0 : 2);

        int numPSE = 4 + (variant == 2 ? 8 : 6);

    gates:
        inout in[numPorts];
        inout out[numPorts];
        input cntrl[numPSE];


    submodules:
        InjectSwA: PSE1x2NX {
            @display("p=65,222");
        }
        InjectSwB: PSE1x2NX {
            @display("p=231,222");
        }
        EjectSwA: PSE1x2NX {
            @display("p=390,28");
        }
        EjectSwB: PSE1x2NX {
            @display("p=401,165");
        }

        ModBank: ModulatorBank {
            @display("p=147,272");
        }

        DetBank: DetectorBank {
            @display("p=351,102;is=vl");
        }

        Switch: <switchVariant> like PhotonicSwitch {
            @display("p=147,111");
        }

        InjBend1: Bend {
            @display("p=143,179");
        }
        InjBend2: Bend {
            @display("p=65,179");
        }
        InjBend3: Bend {
            @display("p=65,319");
        }
        InjBend4: Bend {
            @display("p=132,319");
        }
        InjBend5: Bend {
            @display("p=164,179");
        }
        InjBend6: Bend {
            @display("p=231,179");
        }
        InjBend7: Bend {
            @display("p=231,319");
        }
        InjBend8: Bend {
            @display("p=164,319");
        }
        InjBend9: Bend {
            @display("p=98,219");
        }
        InjBend10: Bend {
            @display("p=196,219");
        }
        InjBend11: Bend {
            @display("p=196,277");
        }

        EjBend1: Bend {
            @display("p=238,111");
        }
        EjBend2: Bend {
            @display("p=238,28");
        }
        EjBend3: Bend {
            @display("p=472,28");
        }
        EjBend4: Bend {
            @display("p=472,85");
        }
        EjBend5: Bend {
            @display("p=238,131");
        }
        EjBend6: Bend {
            @display("p=238,165");
        }
        EjBend7: Bend {
            @display("p=472,165");
        }
        EjBend8: Bend {
            @display("p=472,131");
        }
        EjBend9: Bend {
            @display("p=437,69");
        }
        EjBend10: Bend {
            @display("p=437,131");
        }
        EjBend11: Bend {
            @display("p=358,145");
        }

        LineTop1: Line {
            parameters:
                Length_Line = 0;
                @display("p=163,59");
        }
        LineTop2: Line {
            parameters:
                Length_Line = 0;

                @display("p=142,59");
        }

        LineLeft1: Line {
            parameters:
                Length_Line = 0;
                @display("p=78,101;b=50,5");
        }
        LineLeft2: Line {
            parameters:
                Length_Line = 0;

                @display("p=78,130;b=50,5");
        }

        InjLine1: Line {
            parameters:
                Length_Line = 0;
                @display("p=142,153");
        }
        InjLine2: Line {
            parameters:
                Length_Line = 20;

                @display("p=103,178;b=50,5");
        }
        InjLine3: Line {
            parameters:
                Length_Line = 40;
                @display("p=64,280;b=5,50");
        }
        InjLine4: Line {
            parameters:
                Length_Line = 20;
                @display("p=103,318;b=50,5");
        }
        InjLine5: Line {
            parameters:
                Length_Line = 0;
                @display("p=163,153;b=5,25");
        }
        InjLine6: Line {
            parameters:
                Length_Line = 20;
                @display("p=196,178;b=50,5");
        }
        InjLine7: Line {
            parameters:
                Length_Line = 40;
                @display("p=230,277;b=5,50");
        }
        InjLine8: Line {
            parameters:
                Length_Line = 20;
                @display("p=196,318;b=50,5");
        }
        InjLine9: Line {
            parameters:
                Length_Line = 100;
                @display("p=143,204;b=100,5");
        }
        EjLine1: Line {
            parameters:
                Length_Line = 0;
                @display("p=204,110;b=50,5");
        }
        EjLine2: Line {
            parameters:
                Length_Line = 20;
                @display("p=237,68");
        }
        EjLine3: Line {
            parameters:
                Length_Line = 40;
                @display("p=288,27");
        }
        EjLine4: Line {
            parameters:
                Length_Line = 40;
                @display("p=471,59");
        }
        EjLine5: Line {
            parameters:
                Length_Line = 0;
                @display("p=204,130;b=50,5");
        }
        EjLine6: Line {
            parameters:
                Length_Line = 20;
                @display("p=237,144");
        }
        EjLine7: Line {
            parameters:
                Length_Line = 40;
                @display("p=318,171;b=50,5");
        }
        EjLine8: Line {
            parameters:
                Length_Line = 20;
                @display("p=471,144");
        }
        EjLine9: Line {
            parameters:
                Length_Line = 100;
                @display("p=436,101");
        }

    connections allowunconnected:

        cntrl[(variant != 2) ? 6 : 8] --> InjectSwA.fromRouter;
        cntrl[(variant != 2) ? 7 : 9] --> InjectSwB.fromRouter;
        cntrl[(variant != 2) ? 8 : 10] --> EjectSwA.fromRouter;
        cntrl[(variant != 2) ? 9 : 11] --> EjectSwB.fromRouter;
        cntrl[0] --> Switch.cntrl[0];
        cntrl[1] --> Switch.cntrl[1];
        cntrl[2] --> Switch.cntrl[2];
        cntrl[3] --> Switch.cntrl[3];
        cntrl[4] --> Switch.cntrl[4];
        cntrl[5] --> Switch.cntrl[5];
        cntrl[6] --> Switch.cntrl[6] if (variant == 2);
        cntrl[7] --> Switch.cntrl[7] if (variant == 2);



        in[0] <--> LineTop1.photonicIn;
        in[1] <--> EjBend4.photonicIn;
        in[2] <--> InjBend4.photonicIn;
        in[3] <--> LineLeft2.photonicIn;
        in[4] <--> { @display("ls=,0"); } <--> ModBank.electronicIn;

        out[0] <--> LineTop2.photonicOut;
        out[1] <--> EjBend8.photonicOut;
        out[2] <--> InjBend8.photonicOut;
        out[3] <--> LineLeft1.photonicOut;
        out[4] <--> { @display("ls=,0"); } <--> DetBank.electronicOut;

        Switch.in[0] <--> LineTop1.photonicOut;
        Switch.out[0] <--> LineTop2.photonicIn;
        Switch.in[2] <--> InjLine1.photonicOut;
        Switch.out[2] <--> InjLine5.photonicIn;
        Switch.in[1] <--> EjLine1.photonicOut;
        Switch.out[1] <--> EjLine5.photonicIn;
        Switch.in[3] <--> LineLeft2.photonicOut;
        Switch.out[3] <--> LineLeft1.photonicIn;

        // Modulator Area
        InjLine1.photonicIn <--> InjBend1.photonicOut;
        InjBend1.photonicIn <--> InjLine2.photonicOut;
        InjLine2.photonicIn <--> InjBend2.photonicOut;
        InjBend2.photonicIn <--> InjectSwA.photonicOutA;
        InjectSwA.photonicInA <--> InjLine3.photonicOut;
        InjLine3.photonicIn <--> InjBend3.photonicOut;
        InjBend3.photonicIn <--> InjLine4.photonicOut;
        InjLine4.photonicIn <--> InjBend4.photonicOut;

        InjLine5.photonicOut <--> InjBend5.photonicIn;
        InjBend5.photonicOut <--> InjLine6.photonicIn;
        InjLine6.photonicOut <--> InjBend6.photonicIn;
        InjBend6.photonicOut <--> InjectSwB.photonicInA;
        InjectSwB.photonicOutA <--> InjLine7.photonicIn;
        InjLine7.photonicOut <--> InjBend7.photonicIn;
        InjBend7.photonicOut <--> InjLine8.photonicIn;
        InjLine8.photonicOut <--> InjBend8.photonicIn;

        InjectSwA.photonicInB <--> InjBend9.photonicOut;
        InjBend9.photonicIn <--> InjLine9.photonicOut;
        InjLine9.photonicIn <--> InjBend10.photonicOut;
        InjBend10.photonicIn <--> InjectSwB.photonicOutB;
        InjectSwB.photonicInB <--> InjBend11.photonicOut;
        InjBend11.photonicIn <--> ModBank.networkOut;

        // Detector Area
        EjLine1.photonicIn <--> EjBend1.photonicOut;
        EjBend1.photonicIn <--> EjLine2.photonicOut;
        EjLine2.photonicIn <--> EjBend2.photonicOut;
        EjBend2.photonicIn <--> EjLine3.photonicOut;
        EjLine3.photonicIn <--> EjectSwA.photonicOutA;
        EjectSwA.photonicInA <--> EjBend3.photonicOut;
        EjBend3.photonicIn <--> EjLine4.photonicOut;
        EjLine4.photonicIn <--> EjBend4.photonicOut;

        EjLine5.photonicOut <--> EjBend5.photonicIn;
        EjBend5.photonicOut <--> EjLine6.photonicIn;
        EjLine6.photonicOut <--> EjBend6.photonicIn;
        EjBend6.photonicOut <--> EjLine7.photonicIn;
        EjLine7.photonicOut <--> EjectSwB.photonicInA;
        EjectSwB.photonicOutA <--> EjBend7.photonicIn;
        EjBend7.photonicOut <--> EjLine8.photonicIn;
        EjLine8.photonicOut <--> EjBend8.photonicIn;

        EjectSwA.photonicOutB <--> EjBend9.photonicIn;
        EjBend9.photonicOut <--> EjLine9.photonicIn;
        EjLine9.photonicOut <--> EjBend10.photonicIn;
        EjBend10.photonicOut <--> EjectSwB.photonicInB;
        EjectSwB.photonicOutB <--> EjBend11.photonicIn;
        EjBend11.photonicOut <--> DetBank.networkIn;



}
