// PhoenixSim - Photonic and Electronic Network Integration and Execution Simulator
// (c) 2010 Gilbert Hendry and Johnnie Chan
//
// messages.msg
// Message definition file for OMNeT++


enum PSE_STATE
{

    PSE_OFF = 0;
    PSE_ON = 1;
}



enum ElectronicMessageTypes
{

    dataPacket = 0;

    pathSetup = 1;
    pathTeardown = 2;
    pathBlocked = 3;
    pathACK = 4;
    
    pathRespond = 5; //Request Respond initial packet


    pathUnknown = 8;
    
    pathBlockedTurnaround = 9;
    pathSetupCancelled = 10;
    
    router_bufferAck = 20;
    
    grantDataTx = 21;
    requestDataTx = 22;
    
    
    
    router_unblock = 101;
 	router_xbar = 102; 
 	router_arb_req = 103;
 	router_arb_grant = 104;
 	router_arb_deny = 105;  
 	router_arb_unblock = 106;
 	router_pse_setup = 107;
 	router_change_blocked = 108;
 	router_change_cancel = 109;
 	router_arb_escape = 110;
 	
 	router_power_arbiter = 111;
 	router_power_crossbar = 112;
 	router_power_inport = 113;
 	
 	router_grant_destroy = 114;
 	
    proc_req_send = 200;
 	proc_grant = 201;
 	proc_data = 202;
 	proc_msg_sent = 203;  
 	
 	 

 	router_partial_blocked = 301;
}

enum ProcessorCntrlTypes {
 
    
}

packet ElectronicMessage
{
   
    int Id;
    long SrcId;
  //  int SrcType enum(NodeType);
    long DstId;
   // int DstType enum(NodeType);
    
    int virtualChannel;
    
    int MsgType enum(ElectronicMessageTypes);
 
	long data;
	
	simtime_t time;
	
	bool bcast = false;


    // determines if the circuit is still valid or not (e.g. has it been blocked?)
    bool circuitAvailable[];
    
    // determines if the circuit should still be checked (e.g. down stream from a blocked router would be false)
    bool circuitCheck[];
}


packet PathSetupMsg extends ElectronicMessage {
    
 	int allowedTx;   //num of bits source is allowed to send
 	int dataWidth;  //num of wavelengths source should use.   
 	
 	long blockedAddr; 
}



enum PhotonicMessageTypes
{
    TXstart = 1;
    TXstop = 2;
    Llevel = 3;
    Nstart = 5;
    Nstop = 6;
    VTXstart = 7;
	VTXstop = 8;
	TXpulse = 9;
}

packet PhotonicMessage
{
    int MsgType enum(PhotonicMessageTypes);
    int Id;
  
  
   
    long PacketStat;
   
  
}


packet VWTcntrlMsg extends ElectronicMessage
{
 	int wavelengths;   
    
}



packet ApplicationData{
    int type enum (ProcMsgType);
    
    int id;
    int destId;
    int srcId;
    int payloadSize;
    long payload[];
    
    bool isComplete;
    
    simtime_t creationTime;
   
}


packet ProcessorData {
    
    int size;
    bool isComplete;
    long destAddr;
    long srcAddr;
    int type enum(ElectronicMessageTypes);
 
    int id;
    
    simtime_t creationTime = 0;
    simtime_t nifArriveTime = 0;
    simtime_t savedTime1 = 0;
    simtime_t savedTime2 = 0;
    
    int dataType = 0;
}




enum ProcMsgType {
    
    
    //shared memory type communications
    SM_read = 0;
    SM_write = 1;
    
    //distributed memory
    DM_readLocal = 10;
    DM_readRemote = 11;
    DM_writeLocal = 12;
    DM_writeRemote = 13;
    
    M_readResponse = 1000;
    
    //MPI
    MPI_send = 20;
    
    //CPU
    CPU_op = 50;
    
    //shared memory
    snoopReq = 60;
    snoopResp = 61;
    
    //local inside processor node
 	//procReadFromCache = 0;
 	//procWriteToCache = 1;
 	//procCacheReturnData = 2;
 	
 	//between processig nodes' cache and another cache (inside another proc node, or in distributed/shared in network)
 	//cacheWriteToRemoteCache = 4;
 	//cacheReadFromRemoteCache = 5;
 	//cacheRemoteCacheReturnData = 9;
 	
 	//proc node (rather, it's cache) to memory
 	//cacheWriteToMemory = 6;
 	//cacheReadFromMemory = 7;
 	//cacheMemoryReturnData = 8;
 	
 	//proc node to proc node, no cache involved
 	//procNotifyProc = 3;
 	//procDone = 10;
 	procSynch = 100;
 	
 	//cacheNotifyProc = 12;
 	//cacheLockProc = 13;
 	
 	//dramReadUnlock = 14;
 	//dramStop = 15;
 	//dramStart = 16;
 	
 	//nicNotifyNic = 17;
 	//nicMsgSent = 19;
 	//cacheMoreWriteData = 18;
}



packet RouterCntrlMsg {
 	int type enum(ElectronicMessageTypes);
 	long data;   //generic data
 	int vc;
 	int msgId;
 	int newVC;
 	simtime_t time;
 	
 	    // determines if the circuit is still valid or not (e.g. has it been blocked?)
    bool circuitAvailable[];
    
    // determines if the circuit should still be checked (e.g. down stream from a blocked router would be false)
    bool circuitCheck[];
}  

packet DeviceCntrlMsg {
 	bool on;
 	double data;
}

packet XbarMsg extends RouterCntrlMsg {
 	int fromPort;
 	int toPort;   
}

packet ArbiterRequestMsg extends RouterCntrlMsg {
 	long dest;  //a NetworkAddress*
 	long src;
 	
 	int reqType enum(ElectronicMessageTypes); 
 	int portIn;  
 	int size; 
 	
 	long data;
 	
 	bool bcast;
 	int stage = 0;  //0-N, 1-E, 2-S, 3-W, 4-local    
 
}

packet ElementControlMessage extends RouterCntrlMsg{
 	int state enum(PSE_STATE);
 	int PSEid;   
}


packet BufferAckMsg extends ElectronicMessage {
 	
}

enum DRAM_CntrlTypes {
 	Row_Access = 0;
 	Col_Access = 1;
 	Precharge = 2;   
 	Write_Data = 3;
    
}

packet DRAM_CntrlMsg extends ProcessorData {
 	int type enum(DRAM_CntrlTypes);
 	
 	int row;
 	int col;
 	
 	int bank;
 	int burst;
 	
 	bool writeEn;   
 	bool lastAccess;
 	
 	long coreAddr;
 	
 	simtime_t creationTime;
 	
 	long data;
}



enum MemoryAccessType {   //this matches what's in DRAMsim
 	MemoryIFetechCmd = 1;
 	MemoryWriteCmd = 2;
 	MemoryReadCmd = 3;
 	MemoryPrefetchCmd = 4;
}

packet MemoryAccess extends ApplicationData {
    
 	long addr;
 	int dimm;
 	int bank = 0;
 	
 	int accessType enum (MemoryAccessType);
 	int threadId;
 	int priority;
 	
 	int accessSize;  //size of the access in bits.  used when sending back read data
 	   
}



