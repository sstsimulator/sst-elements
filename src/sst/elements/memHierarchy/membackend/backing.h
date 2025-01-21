// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __SST_MEMH_BACKEND_BACKING
#define __SST_MEMH_BACKEND_BACKING

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "sst/elements/memHierarchy/util.h"

namespace SST {
namespace MemHierarchy {
namespace Backend {

class Backing {
public:
    // Constructor
    Backing( ) { }
    // Destructor
    virtual ~Backing() { }

    // Set the byte at 'addr' to 'value'
    virtual void set( Addr addr, uint8_t value ) = 0;

    // Set 'size' bytes starting at 'addr' to 'data'
    virtual void set( Addr addr, size_t size, std::vector<uint8_t>& data ) = 0;

    // Get the value of the byte at 'addr'
    virtual uint8_t get( Addr addr ) = 0;
    // Get the 'size' bytes ad 'addr' and put them in the vector 'data'
    virtual void get( Addr addr, size_t size, std::vector<uint8_t>& data ) = 0;
    // Dump contents of backing to the file named by 'outfile'
    virtual void dump( std::string outfile ) = 0;

    // Print contents of backing to stdout (testing purposes)
    virtual void test() = 0;
};

/*
 * Old - mmap a file in rdwr
 * New - mmap the output file, initialize from input file if present
 * Throws:
 * 1: Unable to open mmapfile
 * 2: Unable to mmap mmapfile
 * 3: Unable to open infile
 * 4: Unable to mmap infile
 */
class BackingMMAP : public Backing {
public:
    BackingMMAP( std::string mmapfile, std::string infile, size_t size, size_t offset = 0 ) : 
        Backing(), size_(size), offset_(offset) {
        int flags = MAP_SHARED;
        int fd = -1;
        if ( mmapfile != "" ) {
            int fd_flags = O_RDWR | O_CREAT;
            if (mmapfile != infile) fd_flags |= O_TRUNC; // Overwrite output file if it exists
            fd = open(mmapfile.c_str(), fd_flags, S_IRUSR | S_IWUSR);
            if (fd < 0) { printf("Error: fd=%d, %s\n", fd, strerror(errno)); throw 1; }
            ftruncate(fd, size); // Extend file to needed size
        } else {
            flags |= MAP_ANON;
        }

        buffer_ = (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, flags, fd, 0);
        
        if ( mmapfile != "" ) { 
            close(fd); 
        }

        if ( buffer_ == MAP_FAILED) {
            throw 2;
        }

        if ( infile != "" && infile != mmapfile ) {
            fd = open(infile.c_str(), O_RDONLY);
            if (fd < 0) { throw 3; } 

            uint8_t* tmp_buffer = (uint8_t*)mmap(NULL, size, PROT_READ, flags, fd, 0);
            close(fd);

            if ( tmp_buffer == MAP_FAILED ) { throw 4; }

            memcpy(buffer_, tmp_buffer, size);
            munmap(tmp_buffer, size);
        }
    }

    ~BackingMMAP() {
        munmap( buffer_, size_ );
    }

    void set( Addr addr, uint8_t value ) {
        buffer_[addr - offset_ ] = value;
    }

    void set ( Addr addr, size_t size, std::vector<uint8_t> &data ) {
        for (size_t i = 0; i < size; i++)
            buffer_[addr + i] = data[i];
    }

    uint8_t get( Addr addr ) {
        return buffer_[addr - offset_];
    }

    void get( Addr addr, size_t size, std::vector<uint8_t> &data ) {
        for (size_t i = 0; i < size; i++)
            data[i] = buffer_[addr + i];
    }
    
    void dump( std::string UNUSED(outfile) ) { }

    /* For testing only, print contents to stdout in plaintext */
    void test() {
        printf("Backing TEST...dumping memory contents to stdout\n");
        printf("buffer size: %zu\n", size_);
        printf("offset_: %zu\n",offset_);

        // Print in 64B words, regardless of line size
        for (size_t line = offset_; line < size_; line+= 64) {
            printf("%#-10llx | ", line);
            std::stringstream value;
            for (size_t byte = 0; byte < 64; byte++) {
                value << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer_[line+byte]);
            }
            printf("%s ", value.str().c_str());

            if (line % 8 == 0) printf("\n");
        }
    }

private:
    uint8_t* buffer_;
    size_t size_;
    size_t offset_;
};


class BackingMalloc : public Backing {
public:
    BackingMalloc( size_t size, bool init = false ) : init_(init) {
        alloc_unit_ = size;
        /* Alloc unit needs to be pwr-2 */
        if (!isPowerOfTwo(alloc_unit_)) {
            Output out("", 1, 0, Output::STDOUT);
            out.fatal(CALL_INFO, -1, "BackingMalloc, ERROR: Size must be a power of two. Got: %zu.\n", size);
        }
        shift_ = log2Of(alloc_unit_);
    }

    BackingMalloc( FILE* infile ) {
        int num; 
        fscanf(infile,"Number-of-pages: %d\n", &num );
        fscanf(infile,"alloc_unit_: %d\n", &alloc_unit_ );
        int tmpInit;
        fscanf(infile,"init_: %d\n",  &tmpInit );
        init_ = tmpInit;
        fscanf(infile,"shift_: %d\n",  &shift_ );
        Addr addr;
        while ( 1 == fscanf(infile,"addr: %" PRIx64 "\n",&addr) ) {
            Addr bAddr = addr >> shift_;

            assert( buffer_.find( bAddr )  == buffer_.end() );

            auto buf = (uint8_t*) malloc( alloc_unit_ );
            buffer_[ bAddr ] = buf;
            auto ptr = (uint64_t*) buffer_[bAddr];
            auto length = ( sizeof(uint8_t) * alloc_unit_ ) / sizeof(uint64_t);

            for ( auto i = 0; i < length ; i++ ) {
                uint64_t data;
                assert( 1 == fscanf(infile,"%" PRIx64 " ",&data) ); 
                ptr[i] = data;
            }
        }
    }

    void set( Addr addr, uint8_t value ) {
        Addr bAddr = addr >> shift_;
        Addr offset = addr - (bAddr << shift_);
        allocIfNeeded(bAddr);
        buffer_[bAddr][offset] = value;
    }

    void set( Addr addr, size_t size, std::vector<uint8_t> &data ) {
        /* Account for size exceeding alloc unit size */
        Addr bAddr = addr >> shift_;
        Addr offset = addr - (bAddr << shift_);
        size_t dataOffset = 0;

        allocIfNeeded(bAddr);

        while (dataOffset != size) {
            buffer_[bAddr][offset] = data[dataOffset];
            offset++;
            dataOffset++;

            if (offset == alloc_unit_) {
                offset = 0;
                bAddr++;
                allocIfNeeded(bAddr);
            }
        }
    }

    void get( Addr addr, size_t size, std::vector<uint8_t> &data ) {
        Addr bAddr = addr >> shift_;
        Addr offset = addr - (bAddr << shift_);
        size_t dataOffset = 0;

        allocIfNeeded(bAddr);
        assert( data.size() == size );

        assert( buffer_.find(bAddr) != buffer_.end() ); 
        auto buf = buffer_[bAddr];
        while (dataOffset != size) {
            
            data[dataOffset] = buf[offset];
            offset++;
            dataOffset++;
            if (offset == alloc_unit_) {
                bAddr++;
                offset = 0;
                allocIfNeeded(bAddr);
            }
        }
    }

    uint8_t get( Addr addr ) {
        Addr bAddr = addr >> shift_;
        Addr offset = addr - (bAddr << shift_);
        allocIfNeeded(bAddr);
        return buffer_[bAddr][offset];
    }


    void dump( std::string outfile ) {
        auto fp = fopen(outfile.c_str(),"w+");
        if (!fp) { throw 1; }
        
        fprintf(fp,"Number-of-pages: %zu\n",buffer_.size());
        fprintf(fp,"alloc_unit_: %d\n",alloc_unit_);
        fprintf(fp,"init_: %d\n",init_);
        fprintf(fp,"shift_: %d\n",shift_);

        for ( auto const& x : buffer_ ) {
            fprintf(fp,"addr: %#llx\n",x.first << shift_);
            auto length = sizeof(uint8_t)*alloc_unit_;
            length /= sizeof(uint64_t);
            auto ptr = (uint64_t*) x.second;
            for ( auto i = 0; i < length ; i++ ) {
                fprintf(fp,"%#" PRIx64 "",ptr[i]); 
                if ( i + 1 < length ) {
                    fprintf(fp," ");
                }
            }
            fprintf(fp,"\n");
        }
    }

    void test() {
        printf("Number-of-pages: %zu\n",buffer_.size());
        printf("alloc_unit_: %d\n",alloc_unit_);
        printf("init_: %d\n",init_);
        printf("shift_: %d\n",shift_);

        for ( auto const& x : buffer_ ) {
            printf("addr: %#llx\n",x.first << shift_);
            auto length = sizeof(uint8_t)*alloc_unit_;
            length /= sizeof(uint64_t);
            auto ptr = (uint64_t*) x.second;
            for ( auto i = 0; i < length ; i++ ) {
                printf("%#" PRIx64 "",ptr[i]); 
                if ( i + 1 < length ) {
                    printf(" ");
                }
            }
            printf("\n");
        }
    }

private:
    void allocIfNeeded( Addr bAddr ) {
        if (buffer_.find(bAddr) == buffer_.end()) {
            uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t)*alloc_unit_);
            if ( init_ ) {
                bzero( data, alloc_unit_ );
            }
            if (!data) {
                Output out("", 1, 0, Output::STDOUT);
                out.fatal(CALL_INFO, -1, "BackingMalloc: Error - malloc failed.\n");
            }
            buffer_[bAddr] = data;
        }
    }

    std::map<Addr,uint8_t*> buffer_;
    unsigned int alloc_unit_;
    unsigned int shift_;
    bool init_;
};

}
}
}

#endif
