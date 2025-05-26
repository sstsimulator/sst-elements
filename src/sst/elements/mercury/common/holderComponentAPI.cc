
#include <sst/core/component.h> // or
#include <sst/core/subcomponent.h>
#include <mercury/common/holderComponentAPI.h>
     
     
 
   holderSubComponentAPI::holderSubComponentAPI(SST::ComponentId_t id, SST::Params& params) : SST::SubComponent(id) { }
   holderSubComponentAPI::~holderSubComponentAPI() { }

