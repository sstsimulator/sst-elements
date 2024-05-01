// TODO  copyright infpo
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
    SST_ELI_REGISTER_SUBCOMPONENT(EPAFrontend, "ariel", "frontend.epa", SST_ELI_ELEMENT_VERSION(1,0,0), "Ariel frontend for dynamic tracing using EPA tools", SST::ArielComponent::ArielFrontend)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
        {"corecount", "Number of CPU cores to emulate", "1"},
        {"maxcorequeue", "Maximum queue depth per core", "64"},
        {"arielmode", "Tool interception mode, set to 1 to trace entire program (default), set to 0 to delay tracing until ariel_enable() call., set to 2 to attempt auto-detect", "2"},
        {"executable", "Executable to trace", ""},
        {"appstdin", "Specify a file to use for the program's stdin", ""},
        {"appstdout", "Specify a file to use for the program's stdout", ""},
        {"appstdoutappend", "If appstdout is set, set this to 1 to append the file intead of overwriting", "0"},
        {"appstderr", "Specify a file to use for the program's stderr", ""},
        {"appstderrappend", "If appstderr is set, set this to 1 to append the file intead of overwriting", "0"},
        {"mpimode", "Whether to use <mpilauncher> to to launch <launcher> in order to trace MPI-enabled applications.", "0"},
        {"mpilauncher", "Specify a launcher to be used for MPI executables in conjuction with <launcher>", STRINGIZE(MPILAUNCHER_EXECUTABLE)},
        {"mpiranks", "Number of ranks to be launched by <mpilauncher>. Only <mpitracerank> will be traced by <launcher>.", "1" },
        {"mpitracerank", "Rank to be traced by <launcher>.", "0" },
        {"appargcount", "Number of arguments to the traced executable", "0"},
        {"apparg%(appargcount)d", "Arguments for the traced executable", ""},
        {"envparamcount", "Number of environment parameters to supply to the Ariel executable, default=-1 (use SST environment)", "-1"},
        {"envparamname%(envparamcount)d", "Sets the environment parameter name", ""},
        {"envparamval%(envparamcount)d", "Sets the environment parameter value", ""})

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
