
package processingPlane;

simple GatewaySignalControl {
  	
  	parameters:
  		int numOfWavelengthChannels;
  		bool doVWT;
  		
  	gates:
  	    input elInProc;
  	    output elOutProc;
  	    
  	    input elInNet;
  	    output elOutNet;
  	    
  	    output toLBC;
  	    output toSelectors[];
  	    output toAttenuator;
    
};
