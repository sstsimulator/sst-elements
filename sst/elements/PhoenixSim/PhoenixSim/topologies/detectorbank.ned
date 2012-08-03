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

import photonic.devices.Detector;
import electronicComponents.Serializer;
import chipComponents.InoutConverter;
//
// TODO auto-generated module
//
module DetectorBank
{
    parameters:
        int numOfWavelengthChannels;
        double firstWavelength;
        double Switch_FreeSpectralRange;
        double dispValue;
        bool useWDM;
        
        double O_frequency_data;
		double O_frequency_proc;

    gates:
        inout networkIn;
        inout dontcareOut;
        inout electronicOut;
        
    submodules:
        mDetector: Serializer {
            parameters:
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                
                rateIn = O_frequency_data;
                rateOut = O_frequency_proc;
                
                numWiresIn = numOfWavelengthChannels;
                numWiresOut = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                
                @display("p=$dispValueY,250;b=$dispValueX,10,,,,1");
            gates:
                in[(useWDM?numOfWavelengthChannels:1)];
        }
        Det[(useWDM?numOfWavelengthChannels:1)]: Detector {
            parameters:
                useWDM = useWDM;
                filterBaseWavelength = firstWavelength + index*Switch_FreeSpectralRange;
                Latency_Detector = 0;
                ringDiameter = (50e-6)*100000/(1600000 - 2273*index);
                numOfRings = useWDM?1:numOfWavelengthChannels;
                @display("p=200,200,r,80;b=24,24,,,,1");
        }
        
          
        conv : InoutConverter {
            
        }
        
        convWDM[useWDM?numOfWavelengthChannels:1] : InoutConverter {
            
        }
        
	connections allowunconnected:
        for i=0..(useWDM?numOfWavelengthChannels:1)-2 {
            Det[i].photonicOut <--> Det[i+1].photonicIn;

        }

        for i=0..(useWDM?numOfWavelengthChannels:1)-1 {
            Det[i].electronicOut <--> convWDM[i].bi;
            convWDM[i].uniOut --> mDetector.in[i];
        
        }

        mDetector.out --> conv.uniIn;
            conv.bi <--> electronicOut;
		Det[0].photonicIn <--> networkIn;
		Det[(useWDM?numOfWavelengthChannels:1)-1].photonicOut <--> dontcareOut;
		
}
