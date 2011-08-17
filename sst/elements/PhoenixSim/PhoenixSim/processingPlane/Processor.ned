package processingPlane;

//
// TODO auto-generated module
//
simple Processor
{
    parameters:
        double clockRate;
        string id;
        
        string addressTranslator;
        
       // double O_frequency_cntrl;
        double O_frequency_proc;
        
        string application;
        int electronicChannelWidth;
        
        int processorConcentration;
        //int numOfRoutingLevels;
        
        string networkProfile;
        
        //tells the processor how dram is organized.
        // 0 - periphery filled
        // 1 - periphery for concentrated network
       string ioPlaneConfig;
       bool useIOplane;

		double energyPerOP;
        
        int procMsgOverhead;
     
        
        double appParam1;
        double appParam2;
        double appParam3;
        double appParam4;
        string appParam5;
        
        string appSizeDist = default("constant");
        double appSizeParam1 = default(0);
        double appSizeParam2 = default(0);
        double appSizeParam3 = default(0);
        double appSizeParam4 = default(0);
        
        bool isNetworkSideConcentration = default(true);

        @display("i=device/cpu");
    gates:
        inout nicReq;
        output toNic;
        input fromNic;
       


}
