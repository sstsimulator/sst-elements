package photonic.devices;

simple Detector
{
    parameters:
        double filterBaseWavelength = default(0);
        double Latency_Detector;
        double Detector_EperBit;
        double Detector_StaticPower;
      	double O_frequency_data;
      	double Detector_Receiver_EperBit;
       
        
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        
        double RingThrough_ER_DetectorFilter;
    	double RingDrop_ER_DetectorFilter;
        
        double ambientTemperature;
        
        bool useWDM;
        bool enablePhyLayerStatistics;
        bool enableDetailILStats;
        bool enableCommPairILStats;
        int numOfWavelengthChannels;
        
        double ringDiameter = default(3); // Assumes a 3 um circular ring
        bool useRingModel = default(false);
        int numOfRings = default(1);
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        double SizeWidth = default(20);
        double SizeHeight = default(11);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
        int dispValueX;
        int dispValueY;
        
        string groupLabel = default("Detector");
        int numOfPorts = 3;
        @display("i=components/detector");
    gates:
        inout photonicIn;
        inout electronicOut;
        inout photonicOut;
        input cntrl;
}

import chipComponents.InoutConverter;

module Detector_PhotoCAD {
    
	parameters:
	    
	    double ringDiameter;
	    
	    double xCoordinate;
	    double yCoordinate;
	    
	    double width;
	    double height;
	    
	gates:
	   inout photonicPort[2];   
	   output cntrl_detData[1];  

	submodules:
	    
	    L : Detector {
	     	ringDiameter = ringDiameter;  
	     	
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate; 
	    }
	    
	    conv : InoutConverter{
	        
	    }
	    
	connections allowunconnected:
	    L.photonicIn <--> photonicPort[1];
	    L.photonicOut <--> photonicPort[0];
	    
	    L.electronicOut <--> conv.bi;
	   
	    conv.uniOut -->  cntrl_detData[0];
    
}
