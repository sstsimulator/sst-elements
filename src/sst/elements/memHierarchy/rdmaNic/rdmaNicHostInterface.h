
typedef uint32_t Addr_t;

typedef Addr_t Context;
typedef int QueueIndex;
typedef enum { RdmaDone=0, RdmaSend=1, RdmaRecv, RdmaFini, RdmaCreateCQ, RdmaCreateRQ, RdmaMemRgnReg, RdmaMemRgnUnreg, RdmaMemWrite, RdmaMemRead, RdmaBarrier } RdmaCmd;

typedef int MemRgnKey;
typedef int RecvQueueKey;
typedef int RecvQueueId;
typedef int CompQueueId;


typedef struct {
	RdmaCmd type;
	Addr_t  respAddr;
    union Data {
        struct {
			MemRgnKey key;
            Addr_t addr;
            uint32_t len;
		} memRgnReg;
        struct {
			MemRgnKey key;
		} memRgnUnreg;
        struct {
			MemRgnKey key;
			uint32_t offset;
			int node;
            int pe;
            Addr_t srcAddr;
            uint32_t len;
			CompQueueId cqId;
			Context context;
		} write;
        struct {
			MemRgnKey key;
			uint32_t offset;
			int node;
            int pe;
            Addr_t destAddr;
            uint32_t len;
			CompQueueId cqId;
			Context context;
		} read;
        struct {
            Addr_t addr;
            uint32_t len;
			int node;
            int pe;
			RecvQueueKey rqKey;
			CompQueueId cqId;
			Context context;
        } send;
        struct {
            Addr_t addr;
            uint32_t len;
			RecvQueueId rqId;
			Context context;
        } recv;
        struct {
			RecvQueueKey rqKey;
			CompQueueId cqId;
		} createRQ;
        struct {
			Addr_t headPtr;
			Addr_t dataPtr;
			int	   num;
		} createCQ;
    } data;
} NicCmd;

#define NUM_COMP_Q 16
#define RDMA_COMP_Q_SIZE 32
typedef struct {
	Context context;
	uint32_t pad[15];
} RdmaCompletion;

typedef struct {
	uint32_t numThreads;
	uint32_t reqQueueAddress;
	uint32_t reqQueueHeadIndexAddress;
	uint32_t reqQueueSize;

	uint32_t nodeId;
	uint32_t numNodes;
	uint32_t myThread;
} NicQueueInfo;

typedef struct {
	uint32_t respAddress;
	uint32_t reqQueueTailIndexAddress;

} HostQueueInfo;

typedef struct {
	union {
		struct {
			Addr_t tailIndexAddr;
		} createCQ;
	} data __attribute__((aligned(64)));
	// this has to be last as it is the flag to indicate the 
	// NIC has completed the write, the NIC calculates  the offset
	// of this member and write it to the host after it write data
    volatile uint32_t retval __attribute__((aligned(64)));
} NicResp;
