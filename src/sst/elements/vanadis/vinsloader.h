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

#ifndef _H_VANADIS_INST_LOADER
#define _H_VANADIS_INST_LOADER

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/subcomponent.h>

#include <cinttypes>
#include <cstdint>
#include <vector>

#include "datastruct/vcache.h"
#include "vinsbundle.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionLoader {
public:
    VanadisInstructionLoader(const size_t uop_cache_size, const size_t predecode_cache_entries,
                             const uint64_t cachelinewidth) {

        cache_line_width = cachelinewidth;
        uop_cache = new VanadisCache<uint64_t, VanadisInstructionBundle*>(uop_cache_size);
        predecode_cache = new VanadisCache<uint64_t, std::vector<uint8_t>*>(predecode_cache_entries);

        mem_if = nullptr;
    }

    ~VanadisInstructionLoader() {
        delete uop_cache;
        delete predecode_cache;
    }

    uint64_t getCacheLineWidth() { return cache_line_width; }

    void setCacheLineWidth(const uint64_t new_line_width) {
        // if the line width is changed then we have to flush our cache
        if (cache_line_width != new_line_width) {
            predecode_cache->clear();
        }

        cache_line_width = new_line_width;
    }

    void setMemoryInterface(SST::Interfaces::StandardMem* new_if) { mem_if = new_if; }

    bool acceptResponse(SST::Output* output, SST::Interfaces::StandardMem::Request* req) {
        // Looks like we created this request, so we should accept and process it
        auto check_hit_local = pending_loads.find(req->getID());

        if (check_hit_local != pending_loads.end()) {
            SST::Interfaces::StandardMem::ReadResp* resp = static_cast<SST::Interfaces::StandardMem::ReadResp*>(req);
            if (resp->data.size() != cache_line_width) {
                output->fatal(CALL_INFO, -1,
                              "Error: request to the instruction cache was not for a "
                              "full line, req=%d, line-width=%d\n",
                              (int)resp->data.size(), (int)cache_line_width);
            }

            std::vector<uint8_t>* new_line = new std::vector<uint8_t>();
            new_line->reserve(cache_line_width);

            for (uint64_t i = 0; i < cache_line_width; ++i) {
                new_line->push_back(resp->data[i]);
            }

            output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> response has: %" PRIu64 " bytes in payload\n",
                            (uint64_t)resp->data.size());

            if (output->getVerboseLevel() >= 16) {
                char* print_line = new char[2048];
                char* temp_line = new char[2048];
                print_line[0] = '\0';

                for (uint64_t i = 0; i < cache_line_width; ++i) {
                    for (uint64_t j = 0; j < 2048; ++j) {
                        temp_line[j] = print_line[j];
                    }

                    snprintf(print_line, 2048, "%s %" PRIu8 "", temp_line, resp->data[i]);
                }

                output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> cache-line: %s\n", print_line);

                delete[] print_line;
                delete[] temp_line;
            }

            output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> hit (addr=0x%llx), caching line in predecoder.\n",
                            resp->pAddr);

            predecode_cache->store(resp->pAddr, new_line);

            // Remove from pending load stores.
            pending_loads.erase(check_hit_local);

            return true;
        } else {
            return false;
        }
    }

    

    bool getPredecodeBytes(SST::Output* output, const uint64_t addr, uint8_t* buffer, const size_t buffer_req) {

        if (buffer_req > cache_line_width) {
            output->fatal(CALL_INFO, -1,
                          "[inst-predecode-bytes-check]: Requested a decoded bytes "
                          "fill of longer than a cache line, req=%" PRIu64 ", line-width=%" PRIu64 "\n",
                          (uint64_t)buffer_req, cache_line_width);
        }

        // calculate offset within the cache line
        const uint64_t inst_line_offset = (addr % cache_line_width);

        // get the start of the cache line containing the request
        const uint64_t cache_line_start = addr - inst_line_offset;

        bool filled = true;

        output->verbose(CALL_INFO, 16, 0,
                        "[fill-decode]: ins-addr: 0x%llx line-offset: %" PRIu64 " line-start=%" PRIu64 " / 0x%llx\n",
                        addr, inst_line_offset, cache_line_start, cache_line_start);

        if (predecode_cache->contains(cache_line_start)) {
            std::vector<uint8_t>* cached_bytes = predecode_cache->find(cache_line_start);

				uint64_t bytes_from_this_line = std::min( static_cast<uint64_t>(buffer_req), cache_line_width - inst_line_offset );

				output->verbose(CALL_INFO, 16, 0, "[fill-decode]: load %" PRIu64 " bytes from this line.\n", bytes_from_this_line);

            for (uint64_t i = 0; i < bytes_from_this_line; ++i) {
                buffer[i] = cached_bytes->at(inst_line_offset + i);
            }

				if( bytes_from_this_line < buffer_req ) {
					output->verbose(CALL_INFO, 16, 0, "[fill-decode]: requires split cache line load, first-line: %" PRIu64 " bytes\n", bytes_from_this_line);

					if(predecode_cache->contains(cache_line_start + cache_line_width)) {
						cached_bytes = predecode_cache->find(cache_line_start + cache_line_width);

						output->verbose(CALL_INFO, 16, 0, "[fill-decode]: requires split cache line load, second-line: %" PRIu64 " bytes\n", (buffer_req - bytes_from_this_line));

						for( uint64_t i = 0; i < (buffer_req - bytes_from_this_line); ++i ) {
							buffer[bytes_from_this_line + i] = cached_bytes->at(i);
						}
					} else {
						output->verbose(CALL_INFO, 16, 0, "[fill-decode]: second line fill fails, line is not in predecode cache\n");
						filled = false;
					}
				}
        } else {
            filled = false;
        }

        output->verbose(CALL_INFO, 16, 0, "[fill-decode]: line-filled: %s\n", (filled) ? "yes" : "no");

        return filled;
    }

    void cacheDecodedBundle(VanadisInstructionBundle* bundle) {
        uop_cache->store(bundle->getInstructionAddress(), bundle);
    }

    void clearCache() {
        uop_cache->clear();
        predecode_cache->clear();
    }

    bool hasBundleAt(const uint64_t addr) const { return uop_cache->contains(addr); }

	bool hasPredecodeAt(const uint64_t addr, const uint64_t len) const {
		const uint64_t line_start    = addr - (addr % static_cast<uint64_t>(cache_line_width));
		const uint64_t len_line_left = cache_line_width - (addr % static_cast<uint64_t>(cache_line_width));

		if(len <= len_line_left) {
			return predecode_cache->contains(line_start);
		} else {
			const uint64_t line_start_right = line_start + cache_line_width;
			return predecode_cache->contains(line_start) && predecode_cache->contains(line_start_right);
		}
	}

    VanadisInstructionBundle* getBundleAt(const uint64_t addr) { return uop_cache->find(addr); }

    void requestLoadAt(SST::Output* output, const uint64_t addr, const uint64_t len) {
        if (len > cache_line_width) {
            output->fatal(CALL_INFO, -1,
                          "Error: requested an instruction load which is longer than "
                          "a cache line, req=%" PRIu64 ", line=%" PRIu64 "\n",
                          len, cache_line_width);
        }

        output->verbose(CALL_INFO, 8, 0,
                        "[ins-loader] ---> processing a requested load ip=%p, "
                        "icache-line: %" PRIu64 " bytes.\n",
                        (void*)addr, cache_line_width);

        const uint64_t line_start_offset = (addr % cache_line_width);
        uint64_t line_start = addr - line_start_offset;

        do {
            output->verbose(CALL_INFO, 8, 0,
                            "[ins-loader] ---> issue ins-load line-start: 0x%llx, line-len: %" PRIu64
                            " read-len=%" PRIu64 " \n",
                            line_start, line_start_offset, cache_line_width, cache_line_width);

				if(predecode_cache->contains(line_start)) {
					// line is already in the cache, touch to make sure it is kept in LRU
					predecode_cache->touch(line_start);

					output->verbose(CALL_INFO, 8, 0, "[ins-loader] ----> line (start-addr: 0x%llx) is already in pre-decode cache, updated LRU priority\n",
						line_start);
				} else {
	            bool found_pending_load = false;

	            for (auto pending_load_itr : pending_loads) {
	                if (pending_load_itr.second->pAddr == line_start) {
	                    found_pending_load = true;
	                    break;
	                }
	            }

	            if (!found_pending_load) {
						output->verbose(CALL_INFO, 8, 0, "[ins-loader] ----> creating a load for line at 0x%llx, len=%" PRIu64 "\n",
							line_start, cache_line_width);

	                SST::Interfaces::StandardMem::Read* req_line = new SST::Interfaces::StandardMem::Read(
                        line_start, cache_line_width);

	                pending_loads.insert(
	                    std::pair<SST::Interfaces::StandardMem::Request::id_t, SST::Interfaces::StandardMem::Read*>(
	                        req_line->getID(), req_line));

	                mem_if->send(req_line);

	            } else {
   	             output->verbose(CALL_INFO, 8, 0,
      	                          "[ins-loader] -----> load is already in progress, will "
         	                       "not issue another load.\n");
            	}
				}

            line_start += cache_line_width;
        } while (line_start < (addr + len));

			printPendingLoads(output);
    }

    void printStatus(SST::Output* output) {
        output->verbose(CALL_INFO, 8, 0, "Instruction Loader - Internal State Report:\n");
        output->verbose(CALL_INFO, 8, 0, "--> Cache Line Width:          %" PRIu64 "\n", cache_line_width);
        output->verbose(CALL_INFO, 8, 0, "--> Pending Loads:             %" PRIu32 "\n",
                        (uint32_t)pending_loads.size());

        for (auto pl_itr : pending_loads) {
            output->verbose(CALL_INFO, 16, 0, "-----> Address:       %p\n", (void*)pl_itr.second->pAddr);
        }

        output->verbose(CALL_INFO, 8, 0, "--> uop Cache Entries:         %" PRIu32 " / %" PRIu32 "\n",
                        (uint32_t)uop_cache->size(), (uint32_t)uop_cache->capacity());
        output->verbose(CALL_INFO, 8, 0, "--> Predecode Cache Entries:   %" PRIu32 " / %" PRIu32 "\n",
                        (uint32_t)predecode_cache->size(), (uint32_t)predecode_cache->capacity());
    }

private:

	void printPendingLoads(SST::Output* output) {
		output->verbose(CALL_INFO, 8, 0, "[ins-loader]: Pending loads table\n");
		for( auto next_load : pending_loads ) {
			output->verbose(CALL_INFO, 8, 0, "[ins-loader]:   load: 0x%llx / size %" PRIu64 "\n",
				next_load.second->pAddr, next_load.second->size);
		}
	}

    uint64_t cache_line_width;
    SST::Interfaces::StandardMem* mem_if;

    VanadisCache<uint64_t, VanadisInstructionBundle*>* uop_cache;
    VanadisCache<uint64_t, std::vector<uint8_t>*>* predecode_cache;

    std::unordered_map<SST::Interfaces::StandardMem::Request::id_t, SST::Interfaces::StandardMem::Read*> pending_loads;
};

} // namespace Vanadis
} // namespace SST

#endif
