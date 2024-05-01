// TODO copyright/license/other info?
#ifndef _H_ARIEL_FRONTEND_COMMON
#define _H_ARIEL_FRONTEND_COMMON

#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <map>

#include "arielfrontend.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

// Common struct for I/O redirection that can be used by all frontends
typedef struct ariel_redirect_info_t {
    std::string stdin_file;
    std::string stdout_file;
    std::string stderr_file;
    int stdoutappend = 0;
    int stderrappend = 0;
} ariel_redirect_info;

// Common base implementation for Ariel frontends
class ArielFrontendCommon : public ArielFrontend {
    public:
        ArielFrontendCommon(ComponentId_t id, Params& params, uint32_t cores,
            uint32_t qSize, uint32_t memPool);
        ~ArielFrontendCommon() {}

        // Common Functions
        virtual void finish();
        virtual ArielTunnel* getTunnel();
        virtual void init(unsigned int phase);

        // Functions that should be implemented by derived classes
        virtual void emergencyShutdown();

    protected:
        // Common data members
        pid_t child_pid;
        uint32_t core_count;
        uint32_t max_core_queue_len;
        uint32_t def_mem_pool;
        SST::Output* output;
        ArielTunnel* tunnel;

        // SubComponenent parameters
        int verbosemode; // "verbosity"
        uint32_t startup_mode;  // "arielmode"
        std::string executable;
        ariel_redirect_info_t redirect_info;  // "appstd*"
        int mpimode;
        std::string mpilauncher;
        int mpiranks;
        int mpitracerank;
        uint32_t appargcount;
        std::vector<std::string> app_arguments; // "apparg*"

        // arguments and environment for fork
        std::string app_name; // application name (e.g. mpilauncher, pin)
        char **execute_args;
        std::map<std::string, std::string> execute_env;

        // Common Functions
        virtual void parseCommonSubComponentParams(Params& params);

        // Functions that should be impemented by derived classes
        virtual int forkChildProcess(const char* app, char** args,
            std::map<std::string, std::string>& app_env,
            ariel_redirect_info_t redirect_info) = 0;
        virtual void setForkArguments() = 0; // set execute_args

};

} // namespace ArielComponent
} // namespace SST

#endif // _H_ARIEL_FRONTEND_COMMON
