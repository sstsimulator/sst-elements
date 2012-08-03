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

package topologies.switches.machzehndercrossbar;
import electronicComponents.ElectronicChannel;
import chipComponents.HybridRouter;

//
// TODO auto-generated module
//
module MZCrossbarSwitchNetwork
{
    parameters:
        int numOfNodesX;

    gates:
        inout photonicGatewayIn[numOfNodesX];
        inout photonicGatewayOut[numOfNodesX];

        input electronicGatewayIn[numOfNodesX];
        output electronicGatewayOut[numOfNodesX];

    submodules:

        gw[numOfNodesX*1]: HybridRouter {
            parameters:
                id = string(index) + ".";

                optSwitch = "Gateway_NoMem";
                elRouter = 1;

                level = "NET";
                numX = numOfNodesX;
                numY = 1;
                @display("p=134,191,m,$numOfNodesX,80,80;b=50,50;o=,,1");
        }
        sw: MZCrossbarSwitchRouter {
            @display("p=229,86,m,$numOfNodesX,80,80;b=50,50;o=,,1");

        }
    connections allowunconnected:
        for i=0..numOfNodesX-1 {
            gw[i].phOut[1] <--> sw.photonicIn[i];
            gw[i].phIn[1] <--> sw.photonicOut[i];

            gw[i].elOut[1] --> ElectronicChannel --> sw.electronicIn[i];
            gw[i].elIn[1] <-- ElectronicChannel <-- sw.electronicOut[i];


            photonicGatewayIn[i] <--> gw[i].phIn[0];
            photonicGatewayOut[i] <--> gw[i].phOut[0];


            electronicGatewayIn[i] --> ElectronicChannel --> gw[i].elIn[0];
            electronicGatewayOut[i] <-- ElectronicChannel <-- gw[i].elOut[0];
        }
}
