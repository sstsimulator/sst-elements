/* 
 * File:   replacementManager.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef REPLACEMENT_MGR_H
#define	REPLACEMENT_MGR_H

#include <assert.h>
#include "coherenceControllers.h"
#include "MESIBottomCoherenceController.h"
#include "MESITopCoherenceController.h"

using namespace std;
using namespace SST::Interfaces;
namespace SST {
namespace MemHierarchy {

    
class ReplacementMgr{
    public:
        typedef unsigned int uint;
        virtual void update(uint id) = 0;
        virtual void startReplacement()= 0; //many policies don't need it
        virtual void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) = 0;
        virtual uint getBestCandidate() = 0;
        virtual void replaced(uint id) = 0;
        void setTopCC(TopCacheController* cc) {topCC_ = cc;}
        void setBottomCC(MESIBottomCC* cc) {bottomCC_ = cc;}
        
    protected:
        TopCacheController* topCC_;
        MESIBottomCC* bottomCC_;
    };

    /* 
     * LRU
     */
    class LRUReplacementMgr : public ReplacementMgr {
    private:
        uint64_t timestamp; // incremented on each access
        int32_t bestCandidate; // id
        uint64_t* array;
        uint numLines_;
        Output* d_;
        bool sharersAware;

        struct Rank {
            uint64_t timestamp;
            uint sharers;
            BCC_MESIState state;

            void reset() {
                state = I;
                sharers = 0;
                timestamp = 0;
            }

            inline bool lessThan(const Rank& other) const {
                if(state == I && other.state != I) return true;
                //if(!CacheArray::CacheLine::inTransition(state) && CacheArray::CacheLine::inTransition(other.state)) return true;
                else{
                    if (sharers == 0 && other.sharers > 0) return true;
                    else if (sharers > 0 && other.sharers == 0) return false;
                    else return timestamp < other.timestamp;
                }
            }
        };

    Rank bestRank;

    public:
    LRUReplacementMgr(Output* _dbg, uint _numLines, bool _sharersAware) : timestamp(1), bestCandidate(-1), numLines_(_numLines), d_(_dbg), sharersAware(sharersAware) {
        array = (uint64_t*) calloc(numLines_, sizeof(uint64_t));
        bestRank.reset();
    }

    ~LRUReplacementMgr() {
        free(array);
    }

    void update(uint id) { array[id] = timestamp++; }

    void recordCandidate(uint id, bool sharersAware, BCC_MESIState state) {
        Rank candRank = {array[id], (sharersAware)? topCC_->numSharers(id) : 0, state};
        if (bestCandidate == -1 || candRank.lessThan(bestRank)) {
            bestRank = candRank;
            bestCandidate = id;
        }
        
    }

    uint getBestCandidate() {
        assert(bestCandidate != -1);
        return (uint)bestCandidate;
    }

    void replaced(uint id) {
        bestCandidate = -1;
        bestRank.reset();
        array[id] = 0;
    }
    
    void startReplacement() {
        bestCandidate = -1;
        bestRank.reset();
    }

};

/* 
 *  LFU
 */
class LFUReplacementMgr : public ReplacementMgr {
    private:
        uint64_t timestamp; // incremented on each access
        int32_t bestCandidate; // id
        struct LFUInfo {
            uint64_t ts;
            uint64_t acc;
        };
        LFUInfo* array;
        uint numLines;

        //NOTE: Rank code could be shared across Replacement policy implementations
        struct Rank {
            LFUInfo lfuInfo;
            uint sharers;
            bool valid;

            void reset() {
                valid = false;
                sharers = 0;
                lfuInfo = (LFUInfo){0, 0};
            }

            inline bool lessThan(const Rank& other, const uint64_t curTs) const {
                if (!valid && other.valid) {
                    return true;
                } else if (valid == other.valid) {
                    if (sharers == 0 && other.sharers > 0) {
                        return true;
                    } else if (sharers > 0 && other.sharers == 0) {
                        return false;
                    } else {
                        if (lfuInfo.acc == 0) return true;
                        if (other.lfuInfo.acc == 0) return false;
                        uint64_t ownInvFreq = (curTs - lfuInfo.ts)/lfuInfo.acc; //inverse frequency, lower is better
                        uint64_t otherInvFreq = (curTs - other.lfuInfo.ts)/other.lfuInfo.acc;
                        return ownInvFreq > otherInvFreq;
                    }
                }
                return false;
            }
           
        };

        Rank bestRank;

    public:
    
        LFUReplacementMgr(Output* _dbg, uint _numLines) : timestamp(1), bestCandidate(-1), numLines(_numLines) {
            array = (LFUInfo*)calloc(numLines, sizeof(LFUInfo));
            bestRank.reset();
        }

        ~LFUReplacementMgr() {
            free(array);
        }

        void update(uint id) {
            //ts is the "center of mass" of all the accesses, i.e. the average timestamp
            array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
            array[id].acc++;
            timestamp += 1000; //have larger steps to avoid losing too much resolution over successive divisions
        }

        void recordCandidate(uint id) {
            Rank candRank = {array[id], 0, NULL}; //tcc? tcc->numSharers(id) : 0, NULL};//bcc->isValid(id)};

            if (bestCandidate == -1 || candRank.lessThan(bestRank, timestamp)) {
                bestRank = candRank;
                bestCandidate = id;
            }
        }

        uint getBestCandidate() {
            assert(bestCandidate != -1);
            return (uint)bestCandidate;
        }

        void replaced(uint id) {
            bestCandidate = -1;
            bestRank.reset();
            array[id].acc = 0;
        }
     
};

}}
#endif	/* REPLACEMENT_PROTOCOL_H */

