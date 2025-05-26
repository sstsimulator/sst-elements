
#include <sst/core/component.h> // or
#include <sst/core/subcomponent.h>
#include <mercury/common/appHolderComponent.h>
#ifdef MERCURY_LIB
#define CREATE_STRING(x) #x
#define EXPAND_STRING(x) CREATE_STRING(x)
#define CLASS_NAME holderSubComponent
#define CONCAT(x, y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)

class EXPAND_CONCAT(CLASS_NAME, MERCURY_LIB): public holderSubComponentAPI
{
public:
EXPAND_CONCAT(CLASS_NAME, MERCURY_LIB)(SST::ComponentId_t id, SST::Params& params) : holderSubComponentAPI(id, params) {}

SST_ELI_REGISTER_SUBCOMPONENT(EXPAND_CONCAT(CLASS_NAME, MERCURY_LIB), EXPAND_STRING(MERCURY_LIB), "holder" , SST_ELI_ELEMENT_VERSION(1, 0, 0), "description", holderSubComponentAPI)


};
#endif //MERCURY_LIB

