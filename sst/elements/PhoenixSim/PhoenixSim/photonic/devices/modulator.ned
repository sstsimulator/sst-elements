package photonic.devices;

simple Modulator
{
    parameters:
        double filterBaseWavelength = default(0);
        double Latency_Modulator;
       	double LaserPower;
       	
        double Modulator_EperBit;
        double Modulator_Static;
        double O_frequency_data;
        double Modulator_Driver_EperBit;
        
  		double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        
        double ModulatorExtinctionRatio;
        double LaserRelativeIntensityNoise;
        //double NoiseBandwidth; // currently this is redundant with bitrate
       
        bool useWDM;
        int numOfWavelengthChannels;
        int numOfCircuits = default(1);
        
        double ringDiameter = default(3); // Assumes a 3 um circular ring
        bool useRingModel = default(false);
        int numOfRings = default(1);
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        string groupLabel = default("Modulator");
        int numOfPorts = 3;
        
        double SizeWidth = default(11);
        double SizeHeight = default(13);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
        int dispValueX;
        int dispValueY;
        
        @display("i=components/modulator");
    gates:
        inout driver;
        inout photonicIn;
        inout photonicOut;
        
      input cntrl;
}

import chipComponents.InoutConverter;

module Modulator_PhotoCAD {
    
	parameters:
	    
	    double ringDiameter;
	    
	    double xCoordinate;
	    double yCoordinate;
	    
	    double width;
	    double height;
	    
	gates:
	   inout photonicPort[2];   
	   input cntrl_modData[1];  

	submodules:
	    
	    L : Modulator {
	     	ringDiameter = ringDiameter;  
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate;
	    }
	    
	    conv : InoutConverter {
	        
	    }
	    
	connections allowunconnected:
	    L.photonicIn <--> photonicPort[1];
	    L.photonicOut <--> photonicPort[0];
	    
	    L.driver <--> conv.bi;
	    
	    conv.uniIn <-- cntrl_modData[0];
    
}
