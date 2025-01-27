// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

    FileDescriptor(std::string& file_path, int flags, mode_t mode, bool lazy) : path(file_path), flags(flags), mode(mode)
    {
        if ( ! lazy ) {
            fd = open(file_path.c_str(), flags, mode );
            if ( -1 == fd ) {
                printf("%s() FAILED: %s flags=%#x mode=%#x\n",__func__,path.c_str(),flags,mode);
                throw errno;
            }
        } else {
            fd = -1;
        }
        //printf("%s() %s flags=%#x mode=%#x fd=%d\n",__func__,path.c_str(),flags,mode,fd);
    }

    FileDescriptor(std::string& file_path, int dirfd, int flags, mode_t mode) : path(file_path), fd( -1 )
    {
        fd = openat(dirfd, file_path.c_str(), flags, mode );
        if ( -1 == fd ) {
            throw errno;
        }
    }


    ~FileDescriptor() {
        if ( -1 != fd ) {
            close(fd);
        }
    }

    int openLate() {
        fd = open(path.c_str(), flags, mode );
        if ( -1 == fd ) {
            printf("%s() FAILED: %s flags=%#x mode=%#x\n",__func__,path.c_str(),flags,mode);
            throw errno;
        } else {
            printf("%s() %s flags=%#x mode=%#x\n",__func__,path.c_str(),flags,mode);
        }
        return fd;
    }

    std::string getPath() const {
        return path;
    }

    int getFileDescriptor() {
        return fd;
    }

    FileDescriptor( SST::Output* output, FILE* fp ) {
        char str[80];
        assert( 1 == fscanf(fp,"path: %s\n",str) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"path: %s\n",str );
        path = str;

        assert( 1 == fscanf(fp,"fd: %d\n", &fd) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"fd: %d\n", fd);

        assert( 1 == fscanf(fp,"flags: %d\n", &flags ) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"flags: %d\n", flags );

        assert( 1 == fscanf(fp,"mode: %hd\n", &mode) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"mode: %d\n", mode);
    }

    void checkpoint( FILE* fp) {
        fprintf(fp, "path: %s\n", path.c_str() );
        fprintf(fp, "fd: %d\n", fd );
        fprintf(fp, "flags: %d\n", flags );
        fprintf(fp, "mode: %d\n", mode );
    }

protected:
    std::string path;
    int fd;
    int flags;
    mode_t mode;
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
        //        m_fileDescriptors[fd] = new FileDescriptor(file_path,flags,mode, fd <= 2 );
                m_fileDescriptors[fd] = new FileDescriptor(file_path,flags,mode, false );
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
        auto fd = iter->second->getFileDescriptor();
        if ( -1 == fd ) {
            fd = iter->second->openLate();
        }
        return fd;
    }

    std::string getPath( uint32_t handle ) {
        auto iter= m_fileDescriptors.find(handle);
        if (iter == m_fileDescriptors.end()) {
            return "";
        }
        return iter->second->getPath();
    }

    FileDescriptorTable( SST::Output* output, FILE* fp ) {
        char* tmp = nullptr;
        size_t num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#FileDescriptorTable start\n") );
        free(tmp);

        assert( 1 == fscanf(fp,"m_refCnt: %d\n",&m_refCnt) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_refCnt: %d\n",m_refCnt);

        assert( 1 ==fscanf(fp,"m_maxFD: %d\n",&m_maxFD) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_maxFD: %d\n",m_maxFD);

        size_t size;
        assert( 1 == fscanf(fp,"m_fileDescriptors.size(): %zu\n",&size) );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"m_fileDescriptors.size(): %zu\n",size);

        for ( auto i = 0; i < size; i++ ) {
            int fd;
            assert( 1 == fscanf(fp,"fd: %d\n", &fd ) );
            output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"fd: %d\n", fd );
            m_fileDescriptors[fd] = new FileDescriptor(output, fp); 
        }

        tmp = nullptr;
        num = 0;
        getline( &tmp, &num, fp );
        output->verbose(CALL_INFO, 0, VANADIS_DBG_CHECKPOINT,"%s",tmp);
        assert( 0 == strcmp(tmp,"#FileDescriptorTable end\n") );
        free(tmp);
    }

    void checkpoint( FILE* fp ) {
        fprintf(fp,"#FileDescriptorTable start\n");
        fprintf(fp,"m_refCnt: %d\n",m_refCnt);
        fprintf(fp,"m_maxFD: %d\n",m_maxFD);

        fprintf(fp,"m_fileDescriptors.size(): %zu\n",m_fileDescriptors.size());

        for ( auto & x : m_fileDescriptors ) {
            fprintf(fp,"fd: %d\n",x.first);
            x.second->checkpoint(fp);
        }
        fprintf(fp,"#FileDescriptorTable end\n");
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
