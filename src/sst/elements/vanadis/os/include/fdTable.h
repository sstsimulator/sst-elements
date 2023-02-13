// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_FD_TABLE
#define _H_VANADIS_NODE_OS_INCLUDE_FD_TABLE

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <unordered_map>

namespace SST {
namespace Vanadis {

namespace OS {

class FileDescriptor {
public:
    FileDescriptor( const FileDescriptor& obj ) {
        path=obj.path;

        int flags = fcntl(obj.fd, F_GETFL); 

        // the mode could be 0 which will prevent us from opening it, so temporarily change the mode to RWX 
        // but first we need to get the actual mode so we can set it after the open

        off_t pos = lseek( obj.fd, 0, SEEK_CUR ); 
        assert( -1 != pos );

        struct stat buf;
        assert( 0 == fstat(obj.fd,&buf) );

        assert( 0 == fchmod( obj.fd, S_IRWXU ) );

        fd = open( path.c_str(), flags );
        assert( -1 != fd );

        assert( 0 == fchmod( obj.fd, buf.st_mode ) );

        pos = lseek( obj.fd, pos, SEEK_SET ); 
        assert( -1 != pos );

        //printf("%s() %s oldfd=%d flags=%#x mode=%#x newfd=%d\n",__func__,path.c_str(), obj.fd,flags,buf.st_mode,fd);
    }

    FileDescriptor(std::string& file_path, int flags, mode_t mode) : path(file_path)
    {
        fd = open(file_path.c_str(), flags, mode );
        if ( -1 == fd ) {
            printf("%s() FAILED: %s flags=%#x mode=%#x\n",__func__,path.c_str(),flags,mode);
            throw errno;
        }
        //printf("%s() %s flags=%#x mode=%#x fd=%d\n",__func__,path.c_str(),flags,mode,fd);
    }

    FileDescriptor(std::string& file_path, int dirfd, int flags, mode_t mode) : path(file_path)
    {
        fd = openat(dirfd, file_path.c_str(), flags, mode );
        if ( -1 == fd ) {
            throw errno;
        }
    }

    ~FileDescriptor() {
        close(fd);
    }

    std::string getPath() const {
        return path;
    }

    int getFileDescriptor() {
        return fd;
    }

protected:
    std::string path;
    int fd;
};

class FileDescriptorTable {

public:
    FileDescriptorTable( int maxFd ) : m_maxFD(maxFd), m_refCnt(1) {} 

    void update( FileDescriptorTable* old ) {
        for ( const auto& kv: old->m_fileDescriptors ) {
            if ( m_fileDescriptors.find(kv.first) == m_fileDescriptors.end()) {
                m_fileDescriptors[kv.first] = new FileDescriptor( *kv.second );
            }             
        }
    }

    ~FileDescriptorTable() {
        for (auto next_file = m_fileDescriptors.begin(); next_file != m_fileDescriptors.end(); next_file++) {
            delete next_file->second;
        }
    }

    void decRefCnt() { 
        assert( m_refCnt > 0 );
        --m_refCnt; 
    };
    void incRefCnt() { ++m_refCnt; };

    int refCnt() { return m_refCnt; }

    int close( int fd ) {
        if (m_fileDescriptors.find(fd) == m_fileDescriptors.end()) {
            return -EBADF; 
        } 
        delete m_fileDescriptors[fd];
        m_fileDescriptors.erase(fd);
        
        return 0;
    }

    int open(std::string file_path, int flags, mode_t mode, int fd = -1 ){
        if ( -1 == fd ) { 
            fd = findFreeFd();
        }
        if ( fd >= 0 ) {
            try {
                m_fileDescriptors[fd] = new FileDescriptor(file_path,flags,mode); 
            } catch ( int err ) {
                fd = -err;
            }
        }
        return fd;
    }

    int openat(std::string file_path, int dirfd, int flags, mode_t mode) {
        int fd = findFreeFd();
        if ( fd >= 0 ) {
            try {
                m_fileDescriptors[fd] = new FileDescriptor(file_path,dirfd,flags,mode); 
            } catch ( int err ) {
                fd = -err;
            }
        }
        return fd;
    } 

    int findFreeFd() {
        int fd = 3;
        while (m_fileDescriptors.find(fd) != m_fileDescriptors.end() && fd < m_maxFD) {
            fd++;
        } 
        if ( m_maxFD == fd ) {
            fd = -EMFILE;
        }
        return fd;
    }

    int getDescriptor( uint32_t handle ) {
        auto iter= m_fileDescriptors.find(handle);
        if (iter == m_fileDescriptors.end()) {
            return -1;
        }
        return iter->second->getFileDescriptor();
    }

    std::string getPath( uint32_t handle ) {
        auto iter= m_fileDescriptors.find(handle);
        if (iter == m_fileDescriptors.end()) {
            return "";
        }
        return iter->second->getPath();
    }
private:

    std::unordered_map< uint32_t, FileDescriptor* > m_fileDescriptors;

    int m_refCnt;
    int m_maxFD;
};

}
}
}

#endif
