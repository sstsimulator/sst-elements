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

module PhotonicMeshGatewayCrossbar15Ring like PhotonicSwitch
{
    parameters:
        int numPorts = 5;
        int numPSE = 15;

        double PathSeparation;
        double componentSizeX = 530;
        double componentSizeY = 595;

        double bankSizeX = default(100); //TODO: the gateway dimensions do not scale to bankSize values currently
        double bankSizeY = default(100);

        int variant = 201;

    gates:
        inout in[numPorts];
        inout out[numPorts];
        input cntrl[numPSE];
    submodules:

        pse1x2_0: PSE1x2 {
            @display("p=125,145");
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
            @display("p=350,201");
        }
        pse1x2_5: PSE1x2 {
            @display("p=425,200");
        }
        pse1x2_6: PSE1x2 {
            @display("p=200,275");
        }
        pse1x2_7: PSE1x2 {
            @display("p=275,275");
        }
        pse1x2_8: PSE1x2 {
            @display("p=425,275");
        }
        pse1x2_9: PSE1x2 {
            @display("p=125,350");
        }
        pse1x2_10: PSE1x2 {
            @display("p=200,350");
        }
        pse1x2_11: PSE1x2 {
            @display("p=350,350");
        }
        pse1x2_12: PSE1x2 {
            @display("p=125,425");
        }
        pse1x2_13: PSE1x2 {
            @display("p=200,425");
        }
        pse1x2_14: PSE1x2 {
            @display("p=275,425");
        }


        cross_0: Cross {
            @display("p=125,200;b=25,25");
        }
        cross_1: Cross {
            @display("p=900,350;b=25,25");
        }
        cross_2: Cross {
            @display("p=125,275;b=25,25");
        }
        cross_3: Cross {
            @display("p=350,275;b=25,25");
        }
        cross_4: Cross {
            @display("p=275,350;b=25,25");
        }
        cross_5: Cross {
            @display("p=200,500;b=25,25");
        }

        line_9: Line {
            Length_Line = 130;
            @display("b=5,120;p=89,504");
        }
        line_10: Line {
            Length_Line = 145;
            @display("b=5,150;p=613,167");
        }
        line_11: Line {
            Length_Line = 5;
            @display("b=15,5;p=469,200");
        }
        line_12: Line {
            Length_Line = 397.5;
            @display("b=5,380;p=518,337");
        }
        line_13: Line {
            Length_Line = 312.5;
            @display("b=5,280;p=495,364");
        }
        line_20: Line {
            Length_Line = 52.5;
            @display("b=20,5;p=160,500");
        }
        line_21: Line {
            Length_Line = 5;
            @display("b=5,15;p=199,529");
        }
        line_22: Line {
            Length_Line = 10;
            @display("b=30,5;p=483,124");
        }
        line_23: Line {
            Length_Line = 30;
            @display("b=5,15;p=349,239");
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
            Length_Line = 30;
            @display("b=15,5;p=235,500");
        }
        line_27: Line {
            Length_Line = 22.5;
            @display("b=5,15;p=125,471");
        }
        line_28: Line {
            Length_Line = 30;
            @display("b=15,5;p=315,275");
        }
        line_29: Line {
            Length_Line = 160;
            @display("b=130,5;p=380,500");
        }

        bend_4: Bend {
            @display("b=5,5;p=89,424");
        }
        bend_5: Bend {
            @display("b=5,5;p=221,124");
        }
        bend_6: Bend {
            @display("b=5,5;p=89,593");
        }
        bend_7: Bend {
            @display("b=5,5;p=518,124");
        }
        bend_9: Bend {
            @display("b=5,5;p=518,552");
        }
        bend_16: Bend {
            @display("b=5,5;p=469,275");
        }
        bend_17: Bend {
            @display("b=5,5;p=495,200");
        }
        bend_18: Bend {
            @display("b=5,5;p=495,529");
        }
        bend_19: Bend {
            @display("b=5,5;p=124,500");
        }
        bend_20: Bend {
            @display("b=5,5;p=424,349");
        }

        bend_23: Bend {
            @display("b=5,5;p=469,500");
        }
        bend_24: Bend {
            @display("b=5,5;p=275,529");
        }
        line_31: Line {
            Length_Line = 205;
            @display("b=5,170;p=469,390");
        }
        ModBank: ModulatorBank {
            @display("p=147,594");
        }

        DetBank: DetectorBank {
            @display("p=571,140;is=vl");
        }

        bend_26: Bend {
            @display("p=199,200;b=5,5");
        }
        bend_27: Bend {
            @display("p=235,200;b=5,5");
        }
        bend_28: Bend {
            @display("p=200,552;b=5,5");
        }
        line_34: Line {
            Length_Line = 45;
            @display("b=5,25;p=235,171");
        }
        bend_1: Bend {
            @display("b=5,5;p=349,422");
        }

        line_1: Line {
            Length_Line = 52.5;
            @display("b=15,5;p=323,422");
        }
        line_2: Line {
            Length_Line = 52.5;
            @display("b=5,15;p=349,396");
        }
        cross_1: Cross {
            @display("p=275,199;b=25,25");
        }
        cross_6: Cross {
            @display("p=275,500;b=25,25");
        }

        cross_7: Cross {
            @display("p=613,275;b=25,25");
        }

        line_3: Line {
            Length_Line = 187.5;
            @display("b=170,5;p=387,529");
        }

        line_4: Line {
            Length_Line = 272.5;
            @display("b=280,5;p=351,552");
        }

        line_5: Line {
            Length_Line = 30;
            @display("b=15,5;p=254,199");
        }
        line_6: Line {
            Length_Line = 30;
            @display("b=5,15;p=275,166");
        }
        line_7: Line {
            Length_Line = 75;
            @display("b=50,5;p=187,145");
        }
        line_8: Line {
            Length_Line = 30;
            @display("b=5,15;p=125,239");
        }
        line_14: Line {
            Length_Line = 22.5;
            @display("b=5,25;p=199,224");
        }
        line_15: Line {
            Length_Line = 52.5;
            @display("b=30,5;p=168,200");
        }
        line_16: Line {
            Length_Line = 5;
            @display("b=15,5;p=84,350");
        }
        bend_2: Bend {
            @display("b=5,5;p=63,350");
        }
        line_17: Line {
            Length_Line = 30;
            @display("b=5,15;p=275,318");
        }
        line_18: Line {
            Length_Line = 30;
            @display("b=15,5;p=240,350");
        }
        line_19: Line {
            Length_Line = 45;
            @display("b=5,25;p=125,93");
        }
        line_30: Line {
            Length_Line = 22.5;
            @display("b=15,5;p=240,124");
        }
        line_32: Line {
            Length_Line = 67.5;
            @display("b=5,15;p=221,88");
        }
        line_33: Line {
            Length_Line = 262.5;
            @display("b=5,250;p=63,501");
        }
        bend_3: Bend {
            @display("b=5,5;p=63,646");
        }
        line_36: Line {
            Length_Line = 490;
            @display("b=470,5;p=324,646");
        }

        bend_8: Bend {
            @display("b=5,5;p=574,646");
        }
        line_37: Line {
            Length_Line = 392.5;
            @display("b=5,320;p=574,472");
        }
        bend_10: Bend {
            @display("b=5,5;p=574,275");
        }
        line_38: Line {
            Length_Line = 87.5;
            @display("b=100,5;p=502,93");
        }

        bend_11: Bend {
            @display("b=5,5;p=570,93");
        }
        bend_12: Bend {
            @display("b=5,5;p=424,93");
        }
        bend_13: Bend {
            @display("b=5,5;p=613,73");
        }
        line_39: Line {
            Length_Line = 207.5;
            @display("b=230,5;p=483,73");
        }

        bend_14: Bend {
            @display("b=5,5;p=349,73");
        }
        line_40: Line {
            Length_Line = 5;
            @display("b=5,15;p=350,88");
        }
        line_41: Line {
            Length_Line = 10;
            @display("b=5,25;p=275,83");
        }
        bend_15: Bend {
            @display("b=5,5;p=275,58");
        }
        line_42: Line {
            Length_Line = 302.5;
            @display("b=310,5;p=462,58");
        }

        bend_21: Bend {
            @display("b=5,5;p=634,58");
        }
        line_43: Line {
            Length_Line = 87.5;
            @display("b=5,60;p=634,105");
        }
        bend_22: Bend {
            @display("b=5,5;p=634,149");
        }
        line_44: Line {
            Length_Line = 375;
            @display("b=5,330;p=613,477");
        }
        bend_25: Bend {
            @display("b=5,5;p=613,664");
        }
        bend_29: Bend {
            @display("b=5,5;p=213,664");
        }
        bend_30: Bend {
            @display("b=5,5;p=125,664");
        }
        line_45: Line {
            Length_Line = 355;
            @display("b=330,5;p=420,664");
        }

        line_46: Line {
            Length_Line = 40;
            @display("b=30,5;p=85,275");
        }
        line_47: Line {
            Length_Line = 40;
            @display("b=30,5;p=64,257");
        }
        bend_31: Bend {
            @display("p=93,200;b=5,5");
        }
        bend_32: Bend {
            @display("p=93,257;b=5,5");
        }
        line_48: Line {
            Length_Line = 27.5;
            @display("b=5,25;p=93,231");
        }
        bend_33: Bend {
            @display("b=5,5;p=51,275");
        }
        bend_34: Bend {
            @display("b=5,5;p=51,664");
        }
        line_49: Line {
            Length_Line = 30;
            @display("b=30,5;p=90,664");
        }
        line_50: Line {
            Length_Line = 350;
            @display("b=5,350;p=51,459");
        }
        line_51: Line {
            Length_Line = 15;
            @display("b=50,5;p=64,145");
        }
        bend_35: Bend {
            @display("p=235,145;b=5,5");
        }
    connections allowunconnected:

        cntrl[0] --> { @display("ls=,0"); } --> pse1x2_0.fromRouter;
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

//	N-0, E-1, S-2, W-3, GW-4

        in[0] <--> line_32.photonicIn;
        in[1] <--> cross_7.photonicHorizIn;
        in[2] <--> bend_30.photonicIn;
        in[3] <--> line_51.photonicIn;
        in[4] <--> { @display("ls=,0"); } <--> ModBank.electronicIn;

        out[0] <--> line_19.photonicOut;
        out[1] <--> bend_22.photonicOut;
        out[2] <--> bend_29.photonicOut;
        out[3] <--> line_47.photonicOut;
        out[4] <--> { @display("ls=,0"); } <--> DetBank.electronicOut;

        pse1x2_0.photonicHorizIn <--> line_51.photonicOut;
        pse1x2_0.photonicHorizOut <--> line_7.photonicIn;
        pse1x2_0.photonicVertIn <--> cross_0.photonicVertOut;
        pse1x2_0.photonicVertOut <--> line_19.photonicIn;

        pse1x2_1.photonicHorizIn <--> line_30.photonicOut;
        pse1x2_1.photonicHorizOut <--> pse1x2_2.photonicHorizIn;
        pse1x2_1.photonicVertIn <--> line_6.photonicOut;
        pse1x2_1.photonicVertOut <--> line_41.photonicIn;

        pse1x2_2.photonicHorizOut <--> pse1x2_3.photonicHorizIn;
        pse1x2_2.photonicVertIn <--> pse1x2_4.photonicVertOut;
        pse1x2_2.photonicVertOut <--> line_40.photonicIn;

        pse1x2_3.photonicHorizOut <--> line_22.photonicIn;
        pse1x2_3.photonicVertIn <--> pse1x2_5.photonicVertOut;
        pse1x2_3.photonicVertOut <--> bend_12.photonicIn;

        pse1x2_4.photonicHorizIn <--> cross_1.photonicHorizOut;
        pse1x2_4.photonicHorizOut <--> pse1x2_5.photonicHorizIn;
        pse1x2_4.photonicVertIn <--> line_23.photonicOut;

        pse1x2_5.photonicHorizOut <--> line_11.photonicIn;
        pse1x2_5.photonicVertIn <--> pse1x2_8.photonicVertOut;

        pse1x2_6.photonicHorizIn <--> cross_2.photonicHorizOut;
        pse1x2_6.photonicHorizOut <--> pse1x2_7.photonicHorizIn;
        pse1x2_6.photonicVertIn <--> pse1x2_10.photonicVertOut;
        pse1x2_6.photonicVertOut <--> line_14.photonicIn;

        pse1x2_7.photonicHorizOut <--> line_28.photonicIn;
        pse1x2_7.photonicVertIn <--> line_17.photonicOut;
        pse1x2_7.photonicVertOut <--> cross_1.photonicVertIn;

        pse1x2_8.photonicHorizIn <--> cross_3.photonicHorizOut;
        pse1x2_8.photonicHorizOut <--> bend_16.photonicIn;
        pse1x2_8.photonicVertIn <--> line_25.photonicOut;

        pse1x2_9.photonicHorizIn <--> line_16.photonicOut;
        pse1x2_9.photonicHorizOut <--> pse1x2_10.photonicHorizIn;
        pse1x2_9.photonicVertIn <--> pse1x2_12.photonicVertOut;
        pse1x2_9.photonicVertOut <--> cross_2.photonicVertIn;

        pse1x2_10.photonicHorizOut <--> line_18.photonicIn;
        pse1x2_10.photonicVertIn <--> pse1x2_13.photonicVertOut;

        pse1x2_11.photonicHorizIn <--> cross_4.photonicHorizOut;
        pse1x2_11.photonicHorizOut <--> line_24.photonicIn;
        pse1x2_11.photonicVertIn <--> line_2.photonicOut;
        pse1x2_11.photonicVertOut <--> cross_3.photonicVertIn;

        pse1x2_12.photonicHorizIn <--> bend_4.photonicOut;
        pse1x2_12.photonicHorizOut <--> pse1x2_13.photonicHorizIn;
        pse1x2_12.photonicVertIn <--> line_27.photonicOut;

        pse1x2_13.photonicHorizOut <--> pse1x2_14.photonicHorizIn;
        pse1x2_13.photonicVertIn <--> cross_5.photonicVertOut;

        pse1x2_14.photonicHorizOut <--> line_1.photonicIn;
        pse1x2_14.photonicVertIn <--> cross_6.photonicVertOut;
        pse1x2_14.photonicVertOut <--> cross_4.photonicVertIn;

        ModBank.networkOut <--> bend_6.photonicIn;
        bend_6.photonicOut <--> line_9.photonicIn;
        line_9.photonicOut <--> bend_4.photonicIn;

        bend_12.photonicOut <--> line_38.photonicIn;
        line_38.photonicOut <--> bend_11.photonicIn;
        bend_11.photonicOut <--> DetBank.networkIn;

        line_22.photonicOut <--> bend_7.photonicIn;
        bend_7.photonicOut <--> line_12.photonicIn;
        line_12.photonicOut <--> bend_9.photonicIn;
        bend_9.photonicOut <--> line_4.photonicIn;
        line_4.photonicOut <--> bend_28.photonicIn;
        bend_28.photonicOut <--> line_21.photonicIn;
        line_21.photonicOut <--> cross_5.photonicVertIn;

        line_11.photonicOut <--> bend_17.photonicIn;
        bend_17.photonicOut <--> line_13.photonicIn;
        line_13.photonicOut <--> bend_18.photonicIn;
        bend_18.photonicOut <--> line_3.photonicIn;
        line_3.photonicOut <--> bend_24.photonicIn;
        bend_24.photonicOut <--> cross_6.photonicVertIn;

        bend_16.photonicOut <--> line_31.photonicIn;
        line_31.photonicOut <--> bend_23.photonicIn;
        bend_23.photonicOut <--> line_29.photonicIn;
        line_29.photonicOut <--> cross_6.photonicHorizIn;
        cross_6.photonicHorizOut <--> line_26.photonicIn;
        line_26.photonicOut <--> cross_5.photonicHorizIn;
        cross_5.photonicHorizOut <--> line_20.photonicIn;
        line_20.photonicOut <--> bend_19.photonicIn;
        bend_19.photonicOut <--> line_27.photonicIn;

        cross_7.photonicHorizOut <--> bend_10.photonicIn;
        bend_10.photonicOut <--> line_37.photonicIn;
        line_37.photonicOut <--> bend_8.photonicIn;
        bend_8.photonicOut <--> line_36.photonicIn;
        line_36.photonicOut <--> bend_3.photonicIn;
        bend_3.photonicOut <--> line_33.photonicIn;
        line_33.photonicOut <--> bend_2.photonicIn;
        bend_2.photonicOut <--> line_16.photonicIn;

        line_40.photonicOut <--> bend_14.photonicIn;
        bend_14.photonicOut <--> line_39.photonicIn;
        line_39.photonicOut <--> bend_13.photonicIn;
        bend_13.photonicOut <--> line_10.photonicIn;
        line_10.photonicOut <--> cross_7.photonicVertIn;
        cross_7.photonicVertOut <--> line_44.photonicIn;
        line_44.photonicOut <--> bend_25.photonicIn;
        bend_25.photonicOut <--> line_45.photonicIn;
        line_45.photonicOut <--> bend_29.photonicIn;

        bend_30.photonicOut <--> line_49.photonicIn;
        line_49.photonicOut <--> bend_34.photonicIn;
        bend_34.photonicOut <--> line_50.photonicIn;
        line_50.photonicOut <--> bend_33.photonicIn;
        bend_33.photonicOut <--> line_46.photonicIn;
        line_46.photonicOut <--> cross_2.photonicHorizIn;

        line_41.photonicOut <--> bend_15.photonicIn;
        bend_15.photonicOut <--> line_42.photonicIn;
        line_42.photonicOut <--> bend_21.photonicIn;
        bend_21.photonicOut <--> line_43.photonicIn;
        line_43.photonicOut <--> bend_22.photonicIn;

        line_1.photonicOut <--> bend_1.photonicIn;
        bend_1.photonicOut <--> line_2.photonicIn;

        line_24.photonicOut <--> bend_20.photonicIn;
        bend_20.photonicOut <--> line_25.photonicIn;

        line_14.photonicOut <--> bend_26.photonicIn;
        bend_26.photonicOut <--> line_15.photonicIn;
        line_15.photonicOut <--> cross_0.photonicHorizIn;
        cross_0.photonicHorizOut <--> bend_31.photonicIn;
        bend_31.photonicOut <--> line_48.photonicIn;
        line_48.photonicOut <--> bend_32.photonicIn;
        bend_32.photonicOut <--> line_47.photonicIn;

        line_32.photonicOut <--> bend_5.photonicIn;
        bend_5.photonicOut <--> line_30.photonicIn;

        line_7.photonicOut <--> bend_35.photonicIn;
        bend_35.photonicOut <--> line_34.photonicIn;
        line_34.photonicOut <--> bend_27.photonicIn;

        cross_2.photonicVertOut <--> line_8.photonicIn;
        line_8.photonicOut <--> cross_0.photonicVertIn;

        cross_3.photonicVertOut <--> line_23.photonicIn;

        line_5.photonicOut <--> cross_1.photonicHorizIn;

        line_28.photonicOut <--> cross_3.photonicHorizIn;

        line_18.photonicOut <--> cross_4.photonicHorizIn;

        line_17.photonicIn <--> cross_4.photonicVertOut;
        
        cross_1.photonicVertOut <--> line_6.photonicIn;


        bend_27.photonicOut <--> line_5.photonicIn;
}
