
#ifndef _H_VANADIS_NODE_OS_FD
#define _H_VANADIS_NODE_OS_FD

#include <cstdio>
#include <string>
#include <cstdint>

namespace SST {
namespace Vanadis {

class VanadisOSFileDescriptor {
public:
        VanadisOSFileDescriptor( uint32_t desc_id, const char* file_path ) :
                file_id( desc_id ) {

		if( nullptr != file_path ) {
			printf("OPENING FILE: %s, result: ", file_path);
			file_handle = fopen( file_path, "w+" );

			if( nullptr == file_handle ) {
				printf(" failed\n");
			} else {
				printf(" success\n");
			}
		} else {
	                file_handle = nullptr;
		}

                path = file_path ;
        }

        ~VanadisOSFileDescriptor() {
                closeFile();
        }

        uint32_t getHandle() const { return file_id; }
        const char* getPath() const {
                if( path.size() == 0 ) {
                        return "";
                } else {
                        return &path[0];
                }
        }

        FILE* getFileHandle() {
                if( nullptr == file_handle ) {
                        file_handle = fopen( path.c_str(), "rw" );
                }

                return file_handle;
        }

        void closeFile() {
                if( nullptr != file_handle ) {
                        fclose( file_handle );
                }
        }

protected:
        const uint32_t file_id;
        std::string path;
        FILE* file_handle;

};

}
}

#endif
