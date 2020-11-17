
#ifndef _H_VANADIS_MEM_FLAG_TYPE
#define _H_VANADIS_MEM_FLAG_TYPE


namespace SST {
namespace Vanadis {

enum VanadisMemoryTransaction {
	MEM_TRANSACTION_NONE,
	MEM_TRANSACTION_LLSC_LOAD,
	MEM_TRANSACTION_LLSC_STORE,
	MEM_TRANSACTION_LOCK
};

const char* getTransactionTypeString( VanadisMemoryTransaction transT ) {
	switch( transT ) {
	case MEM_TRANSACTION_NONE:
		return "STD";
	case MEM_TRANSACTION_LLSC_LOAD:
		return "LLSC_LOAD";
	case MEM_TRANSACTION_LLSC_STORE:
		return "LLSC_STORE";
	case MEM_TRANSACTION_LOCK:
		return "LOCK";
	default:
		return "UNKNOWN";
	}
};

}
}

#endif
