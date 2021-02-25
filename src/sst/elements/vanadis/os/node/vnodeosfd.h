// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
			file_handle = fopen( file_path, "w+" );
		} else {
	                file_handle = nullptr;
		}

                path = file_path ;
        }

        VanadisOSFileDescriptor( uint32_t desc_id, const char* file_path, FILE* f_handle ) :
                file_id( desc_id ), file_handle(f_handle) {

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
