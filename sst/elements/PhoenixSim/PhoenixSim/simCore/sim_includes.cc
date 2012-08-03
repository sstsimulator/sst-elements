#include "sim_includes.h"
#include "debug.h"

string dbg_filter = "*";

void debug(string path, string s, int unit) {



#ifdef EN_DEBUG

	if(dbg_filter.compare("*") != 0 && path.find(dbg_filter) == string::npos){
		return;
	}

	switch (unit) {
	case UNIT_NIC:
#ifdef DEBUG_NIC
		std::cout << simTime() << "..." << "__nic__" << path << "--" << s
				<< endl;
#endif
		break;

	case UNIT_ROUTER:
#ifdef DEBUG_ROUTER
		std::cout << simTime() << "..." << "__rtr__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_APP:
#ifdef DEBUG_APP
		std::cout << simTime() << "..." << "__app__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_PROCESSOR:
#ifdef DEBUG_PROCESSOR
		std::cout << simTime() << "..." << "__prc__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_CACHE:
#ifdef DEBUG_CACHE
		std::cout << simTime() << "..." << "__csh__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_BUFFER:
#ifdef DEBUG_BUFFER
		std::cout << simTime() << "..." << "__buf__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_OPTICS:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__opt__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_WIRES:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__wir__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_INIT:
#ifdef DEBUG_INIT
		std::cout << simTime() << "..." << "__ini__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_DRAM:
#ifdef DEBUG_DRAM
		std::cout << simTime() << "..." << "__mem__" << path << "--" << s
				<< endl;
#endif
		break;

	case UNIT_FINISH:
#ifdef DEBUG_FINISH
		std::cout << simTime() << "..." << "__fin__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_DESTRUCT:
#ifdef DEBUG_DESTRUCT
		std::cout << simTime() << "..." << "__des__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_PATH_SETUP:
#ifdef DEBUG_PATH_SETUP
		std::cout << simTime() << "..." << "__pthset__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_POWER:
#ifdef DEBUG_POWER
		std::cout << simTime() << "..." << "__pwr__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_PATH:
#ifdef DEBUG_PATH
		std::cout << simTime() << "..." << "__pth__" << path << "--" << s << endl;
#endif
		break;

	case UNIT_XBAR:
#ifdef DEBUG_XBAR
		std::cout << simTime() << "..." << "__xb__" << path << "--" << s
				<< endl;
#endif
		break;
	}
#endif
}

void debug(string path, string s, double data, int unit) {
#ifdef EN_DEBUG
	if(dbg_filter.compare("*") != 0 && path.find(dbg_filter) == string::npos){
		return;
	}

	switch (unit) {
	case UNIT_NIC:
#ifdef DEBUG_NIC
		std::cout << simTime() << "..." << "__nic__" << path << "--" << s
				<< data << endl;
#endif
		break;

	case UNIT_ROUTER:
#ifdef DEBUG_ROUTER
		std::cout << simTime() << "..." << "__rtr__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_APP:
#ifdef DEBUG_APP
		std::cout << simTime() << "..." << "__app__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PROCESSOR:
#ifdef DEBUG_PROCESSOR
		std::cout << simTime() << "..." << "__prc__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_CACHE:
#ifdef DEBUG_CACHE
		std::cout << simTime() << "..." << "__csh__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_BUFFER:
#ifdef DEBUG_BUFFER
		std::cout << simTime() << "..." << "__buf__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_OPTICS:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__opt__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_WIRES:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__wir__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_INIT:
#ifdef DEBUG_INIT
		std::cout << simTime() << "..." << "__ini__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_DRAM:
#ifdef DEBUG_DRAM
		std::cout << simTime() << "..." << "__mem__" << path << "--" << s
				<< data << endl;
#endif
		break;

	case UNIT_FINISH:
#ifdef DEBUG_FINISH
		std::cout << simTime() << "..." << "__fin__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_DESTRUCT:
#ifdef DEBUG_DESTRUCT
		std::cout << simTime() << "..." << "__des__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PATH_SETUP:
#ifdef DEBUG_PATH_SETUP
		std::cout << simTime() << "..." << "__pthset__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_POWER:
#ifdef DEBUG_POWER
		std::cout << simTime() << "..." << "__pwr__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PATH:
#ifdef DEBUG_PATH
		std::cout << simTime() << "..." << "__pth__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_XBAR:
#ifdef DEBUG_XBAR
		std::cout << simTime() << "..." << "__xb__" << path << "--" << s << data << endl;
#endif
		break;

	}

#endif
}

void debug(string path, string s, string data, int unit) {
#ifdef EN_DEBUG

	if(dbg_filter.compare("*") != 0 && path.find(dbg_filter) == string::npos){
		return;
	}


	switch (unit) {
	case UNIT_NIC:
#ifdef DEBUG_NIC
		std::cout << simTime() << "..." << "__nic__" << path << "--" << s
				<< data << endl;
#endif
		break;

	case UNIT_ROUTER:
#ifdef DEBUG_ROUTER
		std::cout << simTime() << "..." << "__rtr__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_APP:
#ifdef DEBUG_APP
		std::cout << simTime() << "..." << "__app__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PROCESSOR:
#ifdef DEBUG_PROCESSOR
		std::cout << simTime() << "..." << "__prc__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_CACHE:
#ifdef DEBUG_CACHE
		std::cout << simTime() << "..." << "__csh__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_BUFFER:
#ifdef DEBUG_BUFFER
		std::cout << simTime() << "..." << "__buf__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_OPTICS:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__opt__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_WIRES:
#ifdef DEBUG_OPTICS
		std::cout << simTime() << "..." << "__wir__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_INIT:
#ifdef DEBUG_INIT
		std::cout << simTime() << "..." << "__ini__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_DRAM:
#ifdef DEBUG_DRAM
		std::cout << simTime() << "..." << "__mem__" << path << "--" << s
				<< data << endl;
#endif
		break;

	case UNIT_FINISH:
#ifdef DEBUG_FINISH
		std::cout << simTime() << "..." << "__fin__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_DESTRUCT:
#ifdef DEBUG_DESTRUCT
		std::cout << simTime() << "..." << "__des__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PATH_SETUP:
#ifdef DEBUG_PATH_SETUP
		std::cout << simTime() << "..." << "__pthset__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_POWER:
#ifdef DEBUG_POWER
		std::cout << simTime() << "..." << "__pwr__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_PATH:
#ifdef DEBUG_PATH
		std::cout << simTime() << "..." << "__pth__" << path << "--" << s << data << endl;
#endif
		break;

	case UNIT_XBAR:
	#ifdef DEBUG_XBAR
			std::cout << simTime() << "..." << "__xb__" << path << "--" << s << data << endl;
	#endif
			break;
	}

#endif
}
