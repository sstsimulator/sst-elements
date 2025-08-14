// Copyright 2025 EP Analytics
// SPDX-License-Identifier: BSD-3-Clause
//
// Authors: Allyson Cauble-Chantrenne

#ifndef _H_EPA_FRONTEND
#define _H_EPA_FRONTEND

#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/interprocess/shmparent.h>

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <map>

#include "arielfrontendcommon.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

#define STRINGIZE(input) #input

class EPAFrontend : public ArielFrontendCommon {
    public:

    /* SST ELI */
    //SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::ArielComponent::EPAFrontend, SST::ArielComponent::ArielFrontendCommon)
    SST_ELI_REGISTER_SUBCOMPONENT(EPAFrontend, "ariel", "frontend.epa", SST_ELI_ELEMENT_VERSION(1,0,0), "Ariel frontend for dynamic tracing using EPA tools", SST::ArielComponent::ArielFrontend)

        /* Ariel class */
        EPAFrontend(ComponentId_t id, Params& params, uint32_t cores,
            uint32_t qSize, uint32_t memPool);
        ~EPAFrontend();

        virtual void emergencyShutdown();

    private:

        SST::Core::Interprocess::SHMParent<ArielTunnel>* tunnelmgr;

        // Functions that need to be implemented for ArielFrontendCommon
        virtual int forkChildProcess(const char* app, char** args,
            std::map<std::string, std::string>& app_env,
            ariel_redirect_info_t redirect_info);
        virtual void setForkArguments();  // set execute_args

};

} // name ArielComponent
} // namespace SST

#endif // _H_EPA_FRONTEND
