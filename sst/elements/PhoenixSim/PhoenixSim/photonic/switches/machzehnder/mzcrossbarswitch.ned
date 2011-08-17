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

package photonic.switches.machzehnder;

import photonic.devices.machzehnder.MZSwitch2x2;
import photonic.devices.Line;
import photonic.devices.Cross;
import photonic.devices.Bend;
import photonic.PhotonicSwitch;
//
// TODO auto-generated module
//

module MZCrossbarSwitch
{
    parameters:
        int numInputs;
        int numOutputs;
        double waveguideSpacing; //spacing above the minimum required for the pses (50um)
    gates:
        inout in[numInputs];
        inout out[numOutputs];

        input control[numInputs*numOutputs*2];

    submodules:
        switch[numInputs*numOutputs-1]: MZCrossbarSwitchUnit {
            @display("p=80,80,m,$numOutputs,80,80;b=20,20,,,#000000,1");
        }
        line_horiz[numInputs*(numOutputs-1)]: Line {
            parameters:
                Length_Line = waveguideSpacing + (index==(numInputs*(numOutputs-1)-1)?500:0);
                dispValueX = numOutputs - 1;
                @display("p=120,80,m,$dispValueX,80,80;b=20,5,,#408080,#000000,1");
        }
        line_vert[(numInputs-1)*numOutputs]: Line {
            parameters:
                Length_Line = waveguideSpacing + (index==((numInputs-1)*numOutputs-1)?500:0);
                @display("p=80,120,m,$numOutputs,80,80;b=5,20,,#408080,#000000,1");
        }
        bend: Bend {
            parameters:
                Angle_Bend = 90;
                dispValueX = (numOutputs-1)*80;
                dispValueY = (numInputs-1)*80;
                @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
        }


    connections allowunconnected:

        // Connect switches together
        for i=0..numInputs -1, for j=0..numOutputs - 1 {
            switch[j + i * numOutputs].photonicHorizIn <--> line_horiz[j - 1 + i * (numOutputs-1)].photonicOut if (j!= 0) && !(j == numOutputs - 1 && i == numInputs - 1);
            switch[j + i * numOutputs].photonicHorizOut <--> line_horiz[j + i * (numOutputs-1)].photonicIn if j != numOutputs - 1;

            switch[j + i * numOutputs].photonicVertIn <--> line_vert[j + i * numOutputs].photonicOut if i != numInputs - 1;
            switch[j + i * numOutputs].photonicVertOut <--> line_vert[j + (i - 1) * numOutputs].photonicIn if i != 0 && !(j==numOutputs - 1 && i == numInputs - 1);
        }

        // Connect final bend optimization
        bend.photonicIn <--> line_horiz[numInputs * (numOutputs-1) - 1].photonicOut if numOutputs > 1;
        bend.photonicOut <--> line_vert[(numInputs-1) * numOutputs - 1].photonicIn if numInputs > 1;

        // Connect inputs and outputs to module interface
        for i=0..numInputs - 1 {
            switch[i * numOutputs].photonicHorizIn <--> { @display("m=w"); } <--> in[i] if !(numOutputs==1 && i == numInputs -1);
        }
        for i=0..numOutputs - 1 {
            switch[i].photonicVertOut <--> { @display("m=n"); } <--> out[i] if !(numInputs==1 && i == numOutputs -1);
        }

        bend.photonicIn <--> { @display("m=w"); } <--> in[numInputs - 1] if numOutputs == 1;
        bend.photonicOut <--> { @display("m=n"); } <--> out[numOutputs - 1] if numInputs == 1;

        // Connect PSEs to module interface
        for i=0..numInputs - 1, for j=0..numOutputs - 1 {
            switch[j + i * numOutputs].fromRouter1 <-- { @display("ls=red,0"); } <-- control[j + i * numOutputs] if j + i * numOutputs != numInputs * numOutputs -1;
            switch[j + i * numOutputs].fromRouter2 <-- { @display("ls=red,0"); } <-- control[j + i * numOutputs + numInputs*numOutputs] if j + i * numOutputs != numInputs * numOutputs -1;
        }

}

module MZCrossbarSwitchUnit
{
    gates:
        inout photonicHorizIn;
        inout photonicHorizOut;
        inout photonicVertIn;
        inout photonicVertOut;

        input fromRouter1;
        input fromRouter2;
    submodules:
        u1: MZSwitch2x2 {
            @display("p=114,176");
        }
        u2: MZSwitch2x2 {
            @display("p=254,70");
        }
        b1: Bend {
            @display("p=179,134");
        }
        c1: Cross {
            @display("p=259,181");
        }
    connections allowunconnected:
        fromRouter1 --> u1.fromRouter;
        fromRouter2 --> u2.fromRouter;
        photonicHorizIn <--> u1.photonicInA;
        photonicHorizOut <--> c1.photonicHorizOut;
        u1.photonicOutB <--> c1.photonicHorizIn;
        u1.photonicOutA <--> b1.photonicIn;
        b1.photonicOut <--> u2.photonicInA;
        photonicVertIn <--> c1.photonicVertIn;
        c1.photonicVertOut <--> u2.photonicInB;
        photonicVertOut <--> u2.photonicOutA;
}
