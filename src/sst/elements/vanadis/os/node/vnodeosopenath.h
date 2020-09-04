
#ifndef _H_VANADIS_OS_WRITEAT_HANDLER
#define _H_VANADIS_OS_WRITEAT_HANDLER

#include <cstdint>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisOpenAtHandlerState : public VanadisHandlerState {
public:
        VanadisOpenAtHandlerState( int64_t dirfd,
                uint64_t path_ptr, int64_t flags ) :
                VanadisHandlerState(), openat_dirfd(dirfd),
                openat_path_ptr(path_ptr), openat_flags(flags) {

                completed_path_read = false;
        }

        int64_t getDirFD() const { return openat_dirfd; }
        uint64_t getPathPtr() const { return openat_path_ptr; }
        int64_t getFlags() const { return openat_flags; }

        void addToPath( std::vector<uint8_t> payload ) {
                if( ! completed_path_read ) {
                        for( int i = 0; i < payload.size(); ++i ) {
                                openat_path.push_back( payload[i] );

                                if( payload[i] == '\0' ) {
                                        completed_path_read = true;
                                }
                        }
                }
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
                addToPath( req->data );
        }

        bool completedPathRead() const { return completed_path_read; }
        uint64_t getOpenAtPathLength() const { return openat_path.size(); }

        const char* getOpenAtPath() {
                return (const char*) &openat_path[0];
        }

protected:
        const int64_t openat_dirfd;
        const uint64_t openat_path_ptr;
        const int64_t openat_flags;
        std::vector<uint8_t> openat_path;
        bool completed_path_read;
};

}
}

#endif
