
package topologies;

import photonic.devices.Modulator;
import photonic.devices.Detector;
import photonic.devices.PSE2x2;
import photonic.switches.NonBlockingSwitch4x4_P;

import photonic.devices.Line;
import photonic.PhotonicSwitch;


import electronicComponents.ElectronicRouter;
import electronicComponents.Serializer;
import electronicComponents.Parallelizer;
import electronicComponents.mux;
import electronicComponents.demux;

import chipComponents.InoutConverter;


module Gateway like PhotonicSwitch
{
    parameters:
        int numPorts = 3;

        int numPSE = 6;
        
        int variant = 0;
        
        bool useWDM;
        
        int numOfWavelengthChannels;
        double firstWavelength;
        double Switch_FreeSpectralRange;
                double O_frequency_data;
		double O_frequency_proc;
          double LaserPower;
        int dispValue;
    gates:
        inout in[numPorts];
        inout out[numPorts];
        input cntrl[numPSE];
    submodules:
        
        mModulator: Parallelizer {
            parameters:               
                
                rateIn = O_frequency_proc;
                rateOut = O_frequency_data;
                
                numWiresOut = numOfWavelengthChannels;
                numWiresIn = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                @display("p=$dispValueY,50;b=$dispValueX,10,,,,1");
            gates:
                out[(useWDM?numOfWavelengthChannels:1)];
        }
        mDetector: Serializer {
            parameters:
                
                rateIn = O_frequency_data;
                rateOut = O_frequency_proc;
                
                numWiresIn = numOfWavelengthChannels;
                numWiresOut = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                @display("p=$dispValueY,250;b=$dispValueX,10,,,,1");
            gates:
                in[(useWDM?numOfWavelengthChannels:1)];
        }


        Mod[(useWDM?numOfWavelengthChannels:1)]: Modulator {
            parameters:
                useWDM = useWDM;
                filterBaseWavelength = firstWavelength + index*Switch_FreeSpectralRange;
                Latency_Modulator = 0;
                LaserPower = LaserPower / numOfWavelengthChannels;
                ringDiameter = (50e-6)*100000/(1600000 - 2273*index);
                numOfRings = useWDM?1:numOfWavelengthChannels;
                @display("p=200,100,r,80;b=24,24,,,,1");
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
       // OutputControl: PSE2x2 {
        //    parameters:
        //       @display("p=93,88;b=40,24,,,,1");
        //}
        //InputControl: PSE2x2 {
         //   parameters:
          //      @display("p=93,191;b=40,24,,,,1");
       // }

        switch: NonBlockingSwitch4x4_P {
            parameters:
                numPorts = 3;
                @display("p=132,151");
            gates:
                in[4];
                out[4];
                cntrl[6];
        }
        
       // conv[2] : InoutConverter {
        //}
        
        conv : InoutConverter{
            
        }
         conv2 : InoutConverter{
            
        }
        
        conv3 : InoutConverter{
            
        }
        
        conv4 : InoutConverter{
            
        }
        
      
    connections allowunconnected:

        for i=0..(useWDM?numOfWavelengthChannels:1)-2 {
            Mod[i].photonicIn <--> Mod[i+1].photonicOut;
            Det[i].photonicOut <--> Det[i+1].photonicIn;

        }
        in[0]  <--> {@display("m=n");} <--> conv3.bi;
        conv3.uniOut --> mModulator.in;

        for i=0..(useWDM?numOfWavelengthChannels:1)-1 {
            Mod[i].driver <--> conv.bi;
            conv.uniIn <-- mModulator.out[i];
            Det[i].electronicOut <--> conv2.bi; 
            conv2.uniOut --> mDetector.in[i];
        }

        mDetector.out --> conv4.uniIn;
        
        conv4.bi <--> out[0];

       // for i=0..numIF-1 {
       //     mIn.in[i] <--> electronicIn[i];
       //     mOut.out[i] <--> electronicOut[i];
       // }


        for i=0..5 {
            switch.cntrl[i] <-- {@display("m=w");} <-- cntrl[i];
        }

       // Mod[(useWDM?numOfWavelengthChannels:1)-1].photonicIn <--> {@display("m=e");} <--> sourceIn;


        switch.in[0] <--> Mod[0].photonicOut;
        switch.out[0] <--> Det[0].photonicIn;

        switch.in[1] <--> {@display("m=w");} <--> in[1];
        switch.out[1] <--> {@display("m=w");} <--> out[1];

        switch.in[2] <--> in[2];
        switch.out[2] <--> out[2];
        

        //gw.fromRouterOutput <-- cntrl[0];
        //gw.fromRouterInput <-- cntrl[1];
}

module Gateway_NoMem like PhotonicSwitch
{
    parameters:
        int numPorts = 2;

        int numPSE = 0;
        
        int variant = 0;
        
        
        bool useWDM;
        int numOfWavelengthChannels;
          
        double firstWavelength;
        double Switch_FreeSpectralRange;
		double O_frequency_data;
		double O_frequency_proc;
        double LaserPower;
    gates:
        inout in[numPorts];
        // in[0] = Photonic waveguide into detector bank
        // in[1] = Electrical signal that is used to modulate modulators
        inout out[numPorts]; 
        // out[0] = Photonic waveguide going out of modulator bank
        // out[1] = Electrical Signal that is detected by detector bank
        input cntrl[numPSE];
    submodules:
        
          mModulator: Parallelizer {
            parameters:
                
                rateIn = O_frequency_proc;
                rateOut = O_frequency_data;
                
                numWiresOut = numOfWavelengthChannels;
                numWiresIn = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                
                
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                @display("p=$dispValueY,50;b=$dispValueX,10,,,,1");
            gates:
                out[(useWDM?numOfWavelengthChannels:1)];
        }
        mDetector: Serializer {
            parameters:
                
                rateIn = O_frequency_data;
                rateOut = O_frequency_proc;
                
                numWiresIn = numOfWavelengthChannels;
                numWiresOut = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                @display("p=$dispValueY,250;b=$dispValueX,10,,,,1");
            gates:
                in[(useWDM?numOfWavelengthChannels:1)];
        }


        Mod[(useWDM?numOfWavelengthChannels:1)]: Modulator {
            parameters:
                useWDM = useWDM;
                filterBaseWavelength = firstWavelength + index*Switch_FreeSpectralRange;
                Latency_Modulator = 0;
                LaserPower = LaserPower / numOfWavelengthChannels;
                ringDiameter = (50e-6)*100000/(1600000 - 2273*index);
                numOfRings = useWDM?1:numOfWavelengthChannels;
                @display("p=200,100,r,80;b=24,24,,,,1");
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
       
        
       conv : InoutConverter{
            
        }
        conv2: InoutConverter{
            
        }
       
     
        convWDM[useWDM?numOfWavelengthChannels:1] : InoutConverter{
            
        }
        convWDM2[useWDM?numOfWavelengthChannels:1] : InoutConverter{
            
        }
      
    connections allowunconnected:

        for i=0..(useWDM?numOfWavelengthChannels:1)-2 {
            Mod[i].photonicIn <--> Mod[i+1].photonicOut;
            Det[i].photonicOut <--> Det[i+1].photonicIn;

        }
        in[0]  <--> conv.bi;
        conv.uniOut --> mModulator.in;

        for i=0..(useWDM?numOfWavelengthChannels:1)-1 {
            Mod[i].driver <--> convWDM[i].bi;
            convWDM[i].uniIn <-- mModulator.out[i];
            Det[i].electronicOut <--> convWDM2[i].bi;
            convWDM2[i].uniOut --> mDetector.in[i];
        }

        mDetector.out --> conv2.uniIn;

		conv2.bi <--> out[0];


       // Mod[(useWDM?numOfWavelengthChannels:1)-1].photonicIn <--> {@display("m=e");} <--> sourceIn;


        out[1] <--> Mod[0].photonicOut;
        in[1] <--> Det[0].photonicIn;

       
}

