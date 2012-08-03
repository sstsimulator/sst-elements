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

import photonic.PhotonicSwitch;
import photonic.devices.**;
import photonic.switches.NonBlockingSwitch4x4Regular;
import topologies.ModulatorBank;
import topologies.DetectorBank;

module PhotonicMeshGatewayCrossbar like PhotonicSwitch
{
    parameters:
        int numPorts = 5;
        int numPSE = 18;

        double PathSeparation;
        double componentSizeX = 575;
        double componentSizeY = 525;
        
        double bankSizeX = default(100); //TODO: the gateway dimensions do not scale to bankSize values currently
        double bankSizeY = default(100);

        int variant = 200;

    gates:
        inout in[numPorts];
        inout out[numPorts];
        input cntrl[numPSE];
    submodules:

        pse1x2nx_00: PSE1x2NX {
            @display("p=200,99");
        }
        pse1x2_1: PSE1x2 {
            @display("p=275,125");
        }
        pse1x2_2: PSE1x2 {
            @display("p=350,125");
        }
        pse1x2_3: PSE1x2 {
            @display("p=425,125");
        }
        pse1x2_4: PSE1x2 {
            @display("p=125,202");
        }
        pse1x2_5: PSE1x2 {
            @display("p=275,200");
        }
        pse1x2_6: PSE1x2 {
            @display("p=350,200");
        }
        pse1x2_7: PSE1x2 {
            @display("p=425,200");
        }
        pse1x2_8: PSE1x2 {
            @display("p=125,275");
        }
        pse1x2_9: PSE1x2 {
            @display("p=200,275");
        }
        pse1x2_10: PSE1x2 {
            @display("p=275,275");
        }
        pse1x2_11: PSE1x2 {
            @display("p=425,275");
        }
        pse1x2_12: PSE1x2 {
            @display("p=125,350");
        }
        pse1x2_13: PSE1x2 {
            @display("p=200,350");
        }
        pse1x2_14: PSE1x2 {
            @display("p=350,350");
        }
        pse1x2nx_0: PSE1x2NX {
            @display("p=165,425");
        }
        pse1x2nx_1: PSE1x2NX {
            @display("p=240,425");
        }
        pse1x2nx_2: PSE1x2NX {
            @display("p=315,425");
        }

        cross_0: Cross {
            @display("p=124,57;b=25,25");
        }
        cross_1: Cross {
            @display("p=199,199;b=25,25");
        }
        cross_2: Cross {
            @display("p=51,201;b=25,25");
        }
        cross_3: Cross {
            @display("p=349,274;b=25,25");
        }
        cross_4: Cross {
            @display("p=274,349;b=25,25");
        }
        cross_5: Cross {
            @display("p=547,236;b=25,25");
        }

        line_0: Line {
            Length_Line = 27.5;
            @display("p=164,57;b=15,5");
        }
        line_1: Line {
            Length_Line = 92.5;
            @display("p=51,125;b=5,90");
        }
        line_2: Line {
            Length_Line = 52.5;
            @display("b=25,5;p=79,57");
        }
        line_3: Line {
            Length_Line = 65;
            @display("b=5,75;p=124,124");
        }
        line_4: Line {
            Length_Line = 30;
            @display("b=15,5;p=164,201");
        }
        line_5: Line {
            Length_Line = 27.5;
            @display("b=5,25;p=208,57");
        }
        line_6: Line {
            Length_Line = 10;
            @display("b=5,30;p=273,76");
        }
        line_7: Line {
            Length_Line = 285;
            @display("b=255,5;p=420,47");
        }
        line_8: Line {
            Length_Line = 205;
            @display("b=165,5;p=446,67");
        }
        line_9: Line {
            Length_Line = 100;
            @display("b=5,120;p=564,125");
        }
        line_10: Line {
            Length_Line = 155;
            @display("b=5,120;p=547,148");
        }
        line_11: Line {
            Length_Line = 20;
            @display("b=15,5;p=590,213");
        }
        line_12: Line {
            Length_Line = 275;
            @display("b=5,260;p=547,395");
        }
        line_13: Line {
            Length_Line = 297.5;
            @display("b=5,250;p=518,383");
        }
        line_14: Line {
            Length_Line = 470;
            @display("b=385,5;p=305,524");
        }
        line_15: Line {
            Length_Line = 332.5;
            @display("b=235,5;p=412,538");
        }
        line_16: Line {
            Length_Line = 57.5;
            @display("b=125,5;p=147,538");
        }
        line_17: Line {
            Length_Line = 265;
            @display("b=5,230;p=65,409");
        }
        line_18: Line {
            Length_Line = 180;
            @display("b=5,145;p=86,439");
        }
        line_19: Line {
            Length_Line = 5;
            @display("b=15,5;p=86,274");
        }
        line_20: Line {
            Length_Line = 30;
            @display("b=15,5;p=239,349");
        }
        line_21: Line {
            Length_Line = 30;
            @display("b=5,15;p=274,317");
        }
        line_22: Line {
            Length_Line = 30;
            @display("b=15,5;p=314,274");
        }
        line_23: Line {
            Length_Line = 30;
            @display("b=5,15;p=349,241");
        }
        line_24: Line {
            Length_Line = 52.5;
            @display("b=15,5;p=398,349");
        }
        line_25: Line {
            Length_Line = 52.5;
            @display("b=5,15;p=424,323");
        }
        line_26: Line {
            Length_Line = 15;
            @display("b=15,5;p=349,435");
        }
        line_27: Line {
            Length_Line = 52.5;
            @display("b=5,35;p=362,403");
        }
        line_28: Line {
            Length_Line = 20;
            @display("b=15,5;p=199,435");
        }
        line_29: Line {
            Length_Line = 20;
            @display("b=15,5;p=274,435");
        }

        bend_0: Bend {
            @display("p=190,57;b=5,5");
        }
        bend_1: Bend {
            @display("p=51,302;b=5,5");
        }
        bend_2: Bend {
            @display("b=5,5;p=273,47");
        }
        bend_3: Bend {
            @display("b=5,5;p=348,67");
        }
        bend_4: Bend {
            @display("b=5,5;p=563,47");
        }
        bend_5: Bend {
            @display("b=5,5;p=546,67");
        }
        bend_6: Bend {
            @display("b=5,5;p=564,213");
        }
        bend_7: Bend {
            @display("b=5,5;p=518,236");
        }
        bend_8: Bend {
            @display("b=5,5;p=547,538");
        }
        bend_9: Bend {
            @display("b=5,5;p=518,524");
        }
        bend_10: Bend {
            @display("b=5,5;p=65,538");
        }
        bend_11: Bend {
            @display("b=5,5;p=86,524");
        }
        bend_12: Bend {
            @display("b=5,5;p=65,274");
        }
        bend_13: Bend {
            @display("b=5,5;p=86,349");
        }
        bend_14: Bend {
            @display("b=5,5;p=222,538");
        }
        bend_15: Bend {
            @display("b=5,5;p=274,538");
        }
        bend_16: Bend {
            @display("b=5,5;p=124,418");
        }
        bend_17: Bend {
            @display("b=5,5;p=199,418");
        }
        bend_18: Bend {
            @display("b=5,5;p=274,418");
        }
        bend_19: Bend {
            @display("b=5,5;p=362,435");
        }
        bend_20: Bend {
            @display("b=5,5;p=424,349");
        }

        line_30: Line {
            Length_Line = 5;
            @display("b=5,15;p=349,86");
        }
        bend_21: Bend {
            @display("b=5,5;p=424,86");
        }
        bend_22: Bend {
            @display("b=5,5;p=495,86");
        }
        bend_23: Bend {
            @display("b=5,5;p=124,439");
        }
        bend_24: Bend {
            @display("b=5,5;p=124,492");
        }
        line_31: Line {
            Length_Line = 50;
            @display("b=5,30;p=124,466");
        }
        line_32: Line {
            Length_Line = 75;
            @display("b=40,5;p=460,86");
        }

        ModBank: ModulatorBank {
            @display("p=191,493");
        }

        DetBank: DetectorBank {
            @display("p=496,156;is=vl");
        }

        bend_25: Bend {
            @display("p=51,57;b=5,5");
        }
        line_33: Line {
            Length_Line = 52.5;
            @display("p=51,258;b=5,60");
        }
        bend_26: Bend {
            @display("p=213,138;b=5,5");
        }
        bend_27: Bend {
            @display("p=208,171;b=5,5");
        }
        bend_28: Bend {
            @display("p=190,171;b=5,5");
        }
        line_34: Line {
            Length_Line = 52.5;
            @display("b=5,25;p=190,147");
        }
        line_35: Line {
            Length_Line = 25;
            @display("b=5,5;p=199,171");
        }
        line_36: Line {
            Length_Line = 22.5;
            @display("b=15,5;p=32,302");
        }
    connections allowunconnected:

        cntrl[0] --> { @display("ls=,0"); } --> pse1x2nx_00.fromRouter;
        cntrl[1] --> { @display("ls=,0"); } --> pse1x2_1.fromRouter;
        cntrl[2] --> { @display("ls=,0"); } --> pse1x2_2.fromRouter;
        cntrl[3] --> { @display("ls=,0"); } --> pse1x2_3.fromRouter;
        cntrl[4] --> { @display("ls=,0"); } --> pse1x2_4.fromRouter;
        cntrl[5] --> { @display("ls=,0"); } --> pse1x2_5.fromRouter;
        cntrl[6] --> { @display("ls=,0"); } --> pse1x2_6.fromRouter;
        cntrl[7] --> { @display("ls=,0"); } --> pse1x2_7.fromRouter;
        cntrl[8] --> { @display("ls=,0"); } --> pse1x2_8.fromRouter;
        cntrl[9] --> { @display("ls=,0"); } --> pse1x2_9.fromRouter;
        cntrl[10] --> { @display("ls=,0"); } --> pse1x2_10.fromRouter;
        cntrl[11] --> { @display("ls=,0"); } --> pse1x2_11.fromRouter;
        cntrl[12] --> { @display("ls=,0"); } --> pse1x2_12.fromRouter;
        cntrl[13] --> { @display("ls=,0"); } --> pse1x2_13.fromRouter;
        cntrl[14] --> { @display("ls=,0"); } --> pse1x2_14.fromRouter;
        cntrl[15] --> { @display("ls=,0"); } --> pse1x2nx_0.fromRouter;
        cntrl[16] --> { @display("ls=,0"); } --> pse1x2nx_1.fromRouter;
        cntrl[17] --> { @display("ls=,0"); } --> pse1x2nx_2.fromRouter;

//	N-0, E-1, S-2, W-3, GW-4

        in[0] <--> line_5.photonicIn;
        in[1] <--> cross_5.photonicHorizIn;
        in[2] <--> bend_14.photonicIn;
        in[3] <--> cross_2.photonicHorizIn;
        in[4] <--> { @display("ls=,0"); } <--> ModBank.electronicIn;

        out[0] <--> cross_0.photonicVertOut;
        out[1] <--> line_11.photonicOut;
        out[2] <--> bend_15.photonicOut;
        out[3] <--> line_36.photonicOut;
        out[4] <--> { @display("ls=,0"); } <--> DetBank.electronicOut;

        pse1x2nx_00.photonicInA <--> line_5.photonicOut;
        pse1x2nx_00.photonicOutA <--> bend_26.photonicIn;
        pse1x2nx_00.photonicInB <--> line_34.photonicOut;
        pse1x2nx_00.photonicOutB <--> bend_0.photonicIn;

        pse1x2_1.photonicHorizIn <--> bend_26.photonicOut;
        pse1x2_1.photonicHorizOut <--> pse1x2_2.photonicHorizIn;
        pse1x2_1.photonicVertIn <--> pse1x2_5.photonicVertOut;
        pse1x2_1.photonicVertOut <--> line_6.photonicIn;

        pse1x2_2.photonicHorizOut <--> pse1x2_3.photonicHorizIn;
        pse1x2_2.photonicVertIn <--> pse1x2_6.photonicVertOut;
        pse1x2_2.photonicVertOut <--> line_30.photonicIn;

        pse1x2_3.photonicVertIn <--> pse1x2_7.photonicVertOut;
        pse1x2_3.photonicVertOut <--> bend_21.photonicIn;

        pse1x2_4.photonicHorizIn <--> cross_2.photonicHorizOut;
        pse1x2_4.photonicHorizOut <--> line_4.photonicIn;
        pse1x2_4.photonicVertIn <--> pse1x2_8.photonicVertOut;
        pse1x2_4.photonicVertOut <--> line_3.photonicIn;

        pse1x2_5.photonicHorizIn <--> cross_1.photonicHorizOut;
        pse1x2_5.photonicHorizOut <--> pse1x2_6.photonicHorizIn;
        pse1x2_5.photonicVertIn <--> pse1x2_10.photonicVertOut;

        pse1x2_6.photonicHorizOut <--> pse1x2_7.photonicHorizIn;
        pse1x2_6.photonicVertIn <--> line_23.photonicOut;

        pse1x2_7.photonicVertIn <--> pse1x2_11.photonicVertOut;

        pse1x2_8.photonicHorizIn <--> line_19.photonicOut;
        pse1x2_8.photonicHorizOut <--> pse1x2_9.photonicHorizIn;
        pse1x2_8.photonicVertIn <--> pse1x2_12.photonicVertOut;

        pse1x2_9.photonicHorizOut <--> pse1x2_10.photonicHorizIn;
        pse1x2_9.photonicVertIn <--> pse1x2_13.photonicVertOut;
        pse1x2_9.photonicVertOut <--> cross_1.photonicVertIn;

        pse1x2_10.photonicHorizOut <--> line_22.photonicIn;
        pse1x2_10.photonicVertIn <--> line_21.photonicOut;

        pse1x2_11.photonicHorizIn <--> cross_3.photonicHorizOut;
        pse1x2_11.photonicVertIn <--> line_25.photonicOut;

        pse1x2_12.photonicHorizIn <--> bend_13.photonicOut;
        pse1x2_12.photonicHorizOut <--> pse1x2_13.photonicHorizIn;
        pse1x2_12.photonicVertIn <--> bend_16.photonicOut;

        pse1x2_13.photonicHorizOut <--> line_20.photonicIn;
        pse1x2_13.photonicVertIn <--> bend_17.photonicOut;

        pse1x2_14.photonicHorizIn <--> cross_4.photonicHorizOut;
        pse1x2_14.photonicHorizOut <--> line_24.photonicIn;
        pse1x2_14.photonicVertIn <--> line_27.photonicOut;
        pse1x2_14.photonicVertOut <--> cross_3.photonicVertIn;

        pse1x2nx_0.photonicInA <--> bend_23.photonicOut;
        pse1x2nx_0.photonicOutA <--> line_28.photonicIn;
        pse1x2nx_0.photonicOutB <--> bend_16.photonicIn;

        pse1x2nx_1.photonicInA <--> line_28.photonicOut;
        pse1x2nx_1.photonicOutA <--> line_29.photonicIn;
        pse1x2nx_1.photonicOutB <--> bend_17.photonicIn;

        pse1x2nx_2.photonicInA <--> line_29.photonicOut;
        pse1x2nx_2.photonicOutA <--> line_26.photonicIn;
        pse1x2nx_2.photonicOutB <--> bend_18.photonicIn;

        ModBank.networkOut <--> bend_24.photonicIn;
        DetBank.networkIn <--> bend_22.photonicOut;

        cross_0.photonicHorizIn <--> line_0.photonicOut;
        cross_0.photonicHorizOut <--> line_2.photonicIn;
        cross_0.photonicVertIn <--> line_3.photonicOut;

        cross_1.photonicHorizIn <--> line_4.photonicOut;
        cross_1.photonicVertOut <--> bend_27.photonicIn;

        cross_2.photonicVertIn <--> line_1.photonicOut;
        cross_2.photonicVertOut <--> line_33.photonicIn;

        cross_3.photonicHorizIn <--> line_22.photonicOut;
        cross_3.photonicVertOut <--> line_23.photonicIn;

        cross_4.photonicHorizIn <--> line_20.photonicOut;
        cross_4.photonicVertIn <--> bend_18.photonicOut;
        cross_4.photonicVertOut <--> line_21.photonicIn;

        cross_5.photonicHorizOut <--> bend_7.photonicIn;
        cross_5.photonicVertIn <--> line_10.photonicOut;
        cross_5.photonicVertOut <--> line_12.photonicIn;

        line_2.photonicOut <--> bend_25.photonicIn;
        bend_25.photonicOut <--> line_1.photonicIn;

        line_33.photonicOut <--> bend_1.photonicIn;
        bend_1.photonicOut <--> line_36.photonicIn;

        bend_0.photonicOut <--> line_0.photonicIn;

        bend_27.photonicOut <--> line_35.photonicIn;
        line_35.photonicOut <--> bend_28.photonicIn;
        bend_28.photonicOut <--> line_34.photonicIn;

        line_6.photonicOut <--> bend_2.photonicIn;
        bend_2.photonicOut <--> line_7.photonicIn;
        line_7.photonicOut <--> bend_4.photonicIn;
        bend_4.photonicOut <--> line_9.photonicIn;
        line_9.photonicOut <--> bend_6.photonicIn;
        bend_6.photonicOut <--> line_11.photonicIn;

        line_30.photonicOut <--> bend_3.photonicIn;
        bend_3.photonicOut <--> line_8.photonicIn;
        line_8.photonicOut <--> bend_5.photonicIn;
        bend_5.photonicOut <--> line_10.photonicIn;

        bend_21.photonicOut <--> line_32.photonicIn;
        line_32.photonicOut <--> bend_22.photonicIn;

        line_24.photonicOut <--> bend_20.photonicIn;
        bend_20.photonicOut <--> line_25.photonicIn;

        line_26.photonicOut <--> bend_19.photonicIn;
        bend_19.photonicOut <--> line_27.photonicIn;

        bend_24.photonicOut <--> line_31.photonicIn;
        line_31.photonicOut <--> bend_23.photonicIn;

        line_12.photonicOut <--> bend_8.photonicIn;
        bend_8.photonicOut <--> line_15.photonicIn;
        line_15.photonicOut <--> bend_15.photonicIn;

        bend_14.photonicOut <--> line_16.photonicIn;
        line_16.photonicOut <--> bend_10.photonicIn;
        bend_10.photonicOut <--> line_17.photonicIn;
        line_17.photonicOut <--> bend_12.photonicIn;
        bend_12.photonicOut <--> line_19.photonicIn;

        bend_7.photonicOut <--> line_13.photonicIn;
        line_13.photonicOut <--> bend_9.photonicIn;
        bend_9.photonicOut <--> line_14.photonicIn;
        line_14.photonicOut <--> bend_11.photonicIn;
        bend_11.photonicOut <--> line_18.photonicIn;
        line_18.photonicOut <--> bend_13.photonicIn;



}
