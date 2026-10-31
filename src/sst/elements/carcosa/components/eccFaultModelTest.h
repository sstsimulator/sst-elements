#ifndef SST_ELEMENTS_CARCOSA_ECC_FAULT_MODEL_TEST_H
#define SST_ELEMENTS_CARCOSA_ECC_FAULT_MODEL_TEST_H

#include "sst/elements/carcosa/components/eccGuard.h"

namespace SST { namespace Carcosa {

class EccFaultModelTest : public EccGuard {
public:
    SST_ELI_REGISTER_COMPONENT(EccFaultModelTest, "carcosa", "EccFaultModelTest",
        SST_ELI_ELEMENT_VERSION(1,0,0), "Self-asserting ECC resident fault tests.",
        COMPONENT_CATEGORY_UNCATEGORIZED)
    SST_ELI_DOCUMENT_PORTS(
        {"highlink", "Loopback to lowlink; no traffic is generated.", {"memHierarchy.MemEventBase"}},
        {"lowlink", "Loopback to highlink; no traffic is generated.", {"memHierarchy.MemEventBase"}})

    EccFaultModelTest(ComponentId_t id, Params& params) : EccGuard(id, params) {}
    void setup() override;
    void finish() override {}

private:
    void require(bool condition, const char* message);
    void testResidentFootprints();
    void testResidentDecode();
};

}} // namespace SST::Carcosa

#endif
