package ioComponents;

simple cDramModule
{
    parameters:
        double dramFrequency;
		double O_frequency_cntrl;
		
		string addressTranslator;     
        int processorConcentration;
        string networkProfile;
			
		string DRAM_config_file;
		
		int dramID;
		
        @display("i=device/memory");
    gates:
        inout nicReq;
        output toNic;
        input fromNic;
        
        inout toBuffer;
}


