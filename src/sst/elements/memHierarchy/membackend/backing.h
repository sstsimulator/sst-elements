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
    virtual void printToFile( std::string outfile ) = 0;

    // Print contents of backing to stdout (testing purposes)
    virtual void printToScreen(Addr addr_offset, Addr addr_start, Addr addr_interleave_size, Addr addr_interleave_step) = 0;

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
            if (mmapfile != infile) 
                fd_flags |= O_TRUNC; // Overwrite output file if it exists
            fd = open(mmapfile.c_str(), fd_flags, S_IRUSR | S_IWUSR);
            if (fd < 0) { 
                Output out("", 1, 0, Output::STDOUT);
                out.output("Error: fd=%d, %s\n", fd, strerror(errno)); 
                throw 1; 
            }
            (void) !ftruncate(fd, size); // Extend file to needed size
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
    
    void printToFile( std::string UNUSED(outfile) ) { }

    /* For testing only, print contents to stdout in plaintext */
    void printToScreen(Addr addr_offset, Addr addr_start, Addr addr_interleave_size, Addr addr_interleave_step) {
        Output out("", 1, 0, Output::STDOUT);
        out.output("==================================================================================================\n");
        out.output("Printing contents of mmap'd memory backing buffer\n");
        out.output("Buffer size: %zu\n", size_);
        out.output("Starting index: %zu\n",offset_);
        out.output("==================================================================================================\n");
        out.output("Address    | Value (hex)\n");
        out.output("--------------------------------------------------------------------------------------------------\n");
        
        // Print in 64B words, regardless of line size
        for (size_t line = offset_; line < size_; line+= 64) {

            // Convert local (contiguous) address to global so output is easier to read
            Addr global_addr = line - addr_offset;
            if (addr_interleave_size == 0) {
                global_addr += addr_start;
            } else {
                Addr tmp = global_addr % addr_interleave_size;
                global_addr -= tmp;
                global_addr = global_addr / addr_interleave_size;
                global_addr = global_addr * addr_interleave_step + tmp + addr_start;
            }
            out.output("%#-10" PRIx64 " | ", global_addr);

            std::stringstream value;
            for (size_t byte = 0; byte < 64; byte++) {
                if (byte % 8 == 0 && byte != 0) value << " ";
                value << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer_[line+byte]);
            }
            out.output("%s\n", value.str().c_str());
        }
        out.output("==================================================================================================\n");
    }

private:
    uint8_t* buffer_;
    size_t size_;
    size_t offset_;
};

/*
 * Throws:
 * 1: Unable to open infile
 */
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

    BackingMalloc( std::string infile ) {
        auto fp = fopen(infile.c_str(),"rb");
        if (!fp) throw 1;

        size_t buffer_size;
        (void) !fread(&buffer_size, sizeof(size_t), 1, fp);
        (void) !fread(&alloc_unit_, sizeof(unsigned int), 1, fp);
        (void) !fread(&shift_, sizeof(unsigned int), 1, fp);
        (void) !fread(&init_, sizeof(bool), 1, fp);
        Addr addr;
        for ( size_t i = 0; i < buffer_size; i++ ) {
            auto buf = (uint8_t*) malloc( alloc_unit_);
            (void) !fread(&addr, sizeof(addr), 1, fp);
            (void) !fread(buf, sizeof(uint8_t), alloc_unit_, fp);
            buffer_[addr] = buf;
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


    void printToFile( std::string outfile ) {
        auto fp = fopen(outfile.c_str(),"wb+");
        if (!fp) { throw 1; }
        size_t count = buffer_.size();
        fwrite(&count, sizeof(count), 1, fp);
        fwrite(&alloc_unit_, sizeof(alloc_unit_), 1, fp);
        fwrite(&shift_, sizeof(shift_), 1, fp);
        fwrite(&init_, sizeof(init_), 1, fp);

        for ( auto const& x : buffer_ ) {
            fwrite(&(x.first), sizeof(Addr), 1, fp);
            fwrite(x.second, sizeof(uint8_t), alloc_unit_, fp);
        }
    }

    void printToScreen(Addr addr_offset, Addr addr_start, Addr addr_interleave_size, Addr addr_interleave_step) {
        Output out("", 1, 0, Output::STDOUT);
        out.output("==================================================================================================\n");
        out.output("Printing contents of dynamically allocated memory backing buffer\n");
        out.output("Number of buffer chunks: %zu\n", buffer_.size());
        out.output("Chunk size: %d B\n", alloc_unit_);
        out.output("==================================================================================================\n");
        out.output("Address    | Value (hex)\n");
        out.output("--------------------------------------------------------------------------------------------------\n");
        Addr output_unit = (alloc_unit_ % 64 == 0) ? 64 : (alloc_unit_ % 32 == 0) ? 32 : alloc_unit_;
        Addr units_per_buffer = alloc_unit_ / output_unit;
        
        for ( auto const& x : buffer_ ) {
            Addr local_addr = x.first << shift_;
            uint8_t* value_ptr = x.second;
            for (Addr line = 0; line < units_per_buffer; line++) {
                Addr global_addr = local_addr - addr_offset;
                if (addr_interleave_size == 0) {
                    global_addr += addr_start;
                } else {
                    Addr tmp = global_addr % addr_interleave_size;
                    global_addr -= tmp;
                    global_addr = global_addr / addr_interleave_size;
                    global_addr = global_addr * addr_interleave_step + tmp + addr_start;
                }
                out.output("%#-10" PRIx64 " | ",global_addr);

                // Print output_unit # bytes, with a space between every 8 for readability
                std::stringstream value;
                for (size_t byte = 0; byte < output_unit; byte++) {
                    if (byte % 8 == 0 && byte != 0) value << " ";
                    value << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*(value_ptr));
                    value_ptr++;
                }
                out.output("%s\n", value.str().c_str());
                local_addr += output_unit;
            }
        }
        out.output("==================================================================================================\n");
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
