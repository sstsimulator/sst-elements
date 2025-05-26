
#include <sst/core/component.h> // or
#include <sst/core/subcomponent.h>

#ifndef LIB_MERCURY_HOLDER_API
#define LIB_MERCURY_HOLDER_API
class holderSubComponentAPI : public SST::SubComponent
{
public:
     
     
    SST_ELI_REGISTER_SUBCOMPONENT_API(holderSubComponentAPI)
 
    holderSubComponentAPI(SST::ComponentId_t id, SST::Params& params);
    virtual ~holderSubComponentAPI();

};
#endif //LIB_MERCURY_HOLDER_API
