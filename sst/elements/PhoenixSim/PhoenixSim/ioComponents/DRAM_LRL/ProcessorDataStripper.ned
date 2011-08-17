

package ioComponents.DRAM_LRL;




simple PhotonicToProcessorMsg 
{
    parameters:
        int numOfWavelengthChannels;
        double O_frequency_data;
        
    gates:
        inout photonicIn;
     	inout photonicOut;
        output procOut;
        input procIn;
    
    
}

simple ElectronicToProcessorMsg 
{
    
    gates:
       	input electronicIn;
     	output electronicOut;
        output procOut;
        input procIn;
    
    
}