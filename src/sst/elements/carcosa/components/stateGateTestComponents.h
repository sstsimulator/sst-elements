// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef SST_ELEMENTS_CARCOSA_STATE_GATE_TEST_COMPONENTS_H
#define SST_ELEMENTS_CARCOSA_STATE_GATE_TEST_COMPONENTS_H

#include <sst/core/component.h>

namespace SST { namespace Carcosa {

class StateGateRegressionTest : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(StateGateRegressionTest, "carcosa", "StateGateRegressionTest",
        SST_ELI_ELEMENT_VERSION(1, 0, 0), "Self-asserting state gate event regressions.",
        COMPONENT_CATEGORY_UNCATEGORIZED)
    SST_ELI_DOCUMENT_PARAMS(
        {"suite", "Regression suite: payload or regions.", "payload"})

    StateGateRegressionTest(ComponentId_t id, Params& params);
};

}} // namespace SST::Carcosa

#endif
