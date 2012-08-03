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

package topologies;

import photonic.devices.Modulator;
import electronicComponents.Parallelizer;
import chipComponents.InoutConverter;
//
// TODO auto-generated module
//
module ModulatorBank
{
    parameters:
        int numOfWavelengthChannels;
        double firstWavelength;
        double Switch_FreeSpectralRange;
        double dispValue;
        bool useWDM;
        double LaserPower;

		double O_frequency_data;
		double O_frequency_proc;
    gates:
        inout networkOut;
        inout sourceIn;
        inout electronicIn;
    submodules:
        mModulator: Parallelizer {
            parameters:
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                
                rateIn = O_frequency_proc;
                rateOut = O_frequency_data;
                
                numWiresOut = numOfWavelengthChannels;
                numWiresIn = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                @display("p=$dispValueY,50;b=$dispValueX,10,,,,1");
            gates:
                out[(useWDM?numOfWavelengthChannels:1)];
        }
        Mod[(useWDM?numOfWavelengthChannels:1)]: Modulator {
            parameters:
                useWDM = useWDM;
                filterBaseWavelength = firstWavelength + index*Switch_FreeSpectralRange;
                Latency_Modulator = 0;
                LaserPower = LaserPower;
                ringDiameter = (50e-6)*100000/(1600000 - 2273*index);
                numOfRings = useWDM?1:numOfWavelengthChannels;
                @display("p=200,100,r,80;b=24,24,,,,1");
        }
        
        conv : InoutConverter {
            
        }
        
        convWDM[useWDM?numOfWavelengthChannels:1] : InoutConverter {
            
        }
        
        
    connections allowunconnected:
        for i=0..(useWDM?numOfWavelengthChannels:1)-2 
        {
            Mod[i].photonicIn <--> Mod[i+1].photonicOut;
        }
        electronicIn <--> conv.bi;
        conv.uniOut --> mModulator.in;
        for i=0..(useWDM?numOfWavelengthChannels:1)-1 {
            Mod[i].driver <--> convWDM[i].bi;
            convWDM[i].uniIn <-- mModulator.out[i];
        }
        Mod[(useWDM?numOfWavelengthChannels:1)-1].photonicIn <--> {@display("m=e");} <--> sourceIn;
        
        Mod[0].photonicOut <--> networkOut;
}
