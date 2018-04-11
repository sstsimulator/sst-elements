
#include "simpleModule.h"

using namespace SST::SimpleModule;

SimpleModuleExample::SimpleModuleExample(SST::Params& params) {
	modName = params.find<std::string>("modulename", "");
}

void SimpleModuleExample::printName() {
	printf("Name: %d\n", modName.c_str());
}
