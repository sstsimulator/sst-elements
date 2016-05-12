// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _dummyThreadContext_h
#define _dummyThreadContext_h

#include <cpu/thread_context.hh>

#include <debug.h>

using namespace std;

namespace SST {
namespace M5 {

class DummyThreadContext : public ThreadContext {
    virtual BaseCPU *getCpuPtr() 
    { DBGX(2,"\n");  return NULL; }

    virtual TheISA::PCState pcState()
    { DBGX(2,"\n"); return 0;}

    virtual void pcState(const TheISA::PCState &val)
    { DBGX(2,"\n"); }

    virtual Addr instAddr()
    { DBGX(2,"\n"); return 0;}

    virtual Addr nextInstAddr()
    { DBGX(2,"\n"); return 0;}

    virtual MicroPC microPC()
    { DBGX(2,"\n"); return 0;}

    virtual int flattenIntIndex(int reg)
    { DBGX(2,"\n"); return 0;}

    virtual int flattenFloatIndex(int reg)
    { DBGX(2,"\n"); return 0;}

    virtual int cpuId()
    { DBGX(2,"\n"); return 0;}

    virtual int threadId()
    { DBGX(2,"\n"); return 0;}

    virtual void setThreadId(int id)
    { DBGX(2,"id=%d\n",id); }

    virtual int contextId()
    { DBGX(2,"\n"); return 0;}

    virtual void setContextId(int id)
    { DBGX(2,"\n"); }

    virtual TheISA::TLB *getITBPtr()
    { DBGX(2,"\n"); return NULL;}

    virtual TheISA::TLB *getDTBPtr()
    { DBGX(2,"\n"); return NULL;}

    virtual Decoder *getDecoderPtr()
    { DBGX(2,"\n"); return NULL;}

    virtual System *getSystemPtr()
    { DBGX(2,"\n"); return NULL;}
#if FULL_SYSTEM
    virtual TheISA::Kernel::Statistics *getKernelStats()
    { DBGX(2,"\n"); return NULL;}

    virtual FunctionalPort *getPhysPort()
    { DBGX(2,"\n"); return NULL;}

    virtual VirtualPort *getVirtPort()
    { DBGX(2,"\n"); return NULL;}

    virtual void connectMemPorts(ThreadContext *tc)
    { DBGX(2,"\n"); return NULL;}
#else
    virtual TranslatingPort *getMemPort()
    { DBGX(2,"\n"); return NULL; }

    virtual Process *getProcessPtr()
    { DBGX(2,"\n"); return NULL; }
#endif

    virtual Status status() const
    { DBGX(2,"\n"); return Status();}

    virtual void setStatus(Status new_status)
    { DBGX(2,"new_status=%d\n",new_status); }

    /// Set the status to Active.  Optional delay indicates number of
    /// cycles to wait before beginning execution.
    virtual void activate(int delay = 1)
    { DBGX(2,"\n"); }

    /// Set the status to Suspended.
    virtual void suspend(int delay = 0)
    { DBGX(2,"\n"); }

    /// Set the status to Halted.
    virtual void halt(int delay = 0)
    { DBGX(2,"\n"); }


#if FULL_SYSTEM
    virtual void dumpFuncProfile()
    { DBGX(2,"\n"); }
#endif

    virtual void takeOverFrom(ThreadContext *old_context)
    { DBGX(2,"\n"); }

    virtual void regStats(const string &name)
    { DBGX(2,"\n"); }

    virtual void serialize(ostream &os)
    { DBGX(2,"\n"); }
    virtual void unserialize(Checkpoint *cp, const string &section)
    { DBGX(2,"\n"); }

#if FULL_SYSTEM
    virtual EndQuiesceEvent *getQuiesceEvent()
    { DBGX(2,"\n"); }

    // Not necessarily the best location for these...
    // Having an extra function just to read these is obnoxious
    virtual Tick readLastActivate()
    { DBGX(2,"\n"); return 0;}
    virtual Tick readLastSuspend()
    { DBGX(2,"\n"); return 0;}

    virtual void profileClear()
    { DBGX(2,"\n"); }
    virtual void profileSample()
    { DBGX(2,"\n"); }
#endif

    // Also somewhat obnoxious.  Really only used for the TLB fault.
    // However, may be quite useful in SPARC.
    virtual TheISA::MachInst getInst()
    { DBGX(2,"\n"); return 0;}

    virtual void copyArchRegs(ThreadContext *tc)
    { DBGX(2,"\n"); }

    virtual void clearArchRegs()
    { DBGX(2,"\n"); }

    //
    // New accessors for new decoder.
    //
    virtual uint64_t readIntReg(int reg_idx)
    { DBGX(2,"\n"); return 0;}

    virtual FloatReg readFloatReg(int reg_idx, int width)
    { DBGX(2,"\n"); return 0;}

    virtual FloatReg readFloatReg(int reg_idx)
    { DBGX(2,"\n"); return 0;}

    virtual FloatRegBits readFloatRegBits(int reg_idx, int width)
    { DBGX(2,"\n"); return 0;}

    virtual FloatRegBits readFloatRegBits(int reg_idx)
    { DBGX(2,"\n"); return 0;}

    virtual void setIntReg(int reg_idx, uint64_t val)
    { DBGX(2,"reg=%d val=%#lx\n",reg_idx, val); }

    virtual void setFloatReg(int reg_idx, FloatReg val, int width)
    { DBGX(2,"\n"); }

    virtual void setFloatReg(int reg_idx, FloatReg val)
    { DBGX(2,"\n"); }

    virtual void setFloatRegBits(int reg_idx, FloatRegBits val)
    { DBGX(2,"\n"); }

    virtual void setFloatRegBits(int reg_idx, FloatRegBits val, int width)
    { DBGX(2,"\n"); }

    virtual uint64_t readPC()
    { DBGX(2,"\n"); return 0;}

    virtual void setPC(uint64_t val)
    { DBGX(2,"\n"); }

    virtual uint64_t readNextPC()
    { DBGX(2,"\n"); return 0;}

    virtual void setNextPC(uint64_t val)
    { DBGX(2,"\n"); }

    virtual uint64_t readNextNPC()
    { DBGX(2,"\n"); return 0;}

    virtual void setNextNPC(uint64_t val)
    { DBGX(2,"\n"); }

    virtual uint64_t readMicroPC()
    { DBGX(2,"\n"); return 0;}

    virtual void setMicroPC(uint64_t val)
    { DBGX(2,"\n"); }

    virtual uint64_t readNextMicroPC()
    { DBGX(2,"\n"); return 0;}

    virtual void setNextMicroPC(uint64_t val)
    { DBGX(2,"\n"); }

    virtual MiscReg readMiscRegNoEffect(int misc_reg)
    { DBGX(2,"\n"); return 0;}

    virtual MiscReg readMiscReg(int misc_reg)
    { DBGX(2,"\n"); return 0;}

    virtual void setMiscRegNoEffect(int misc_reg, const MiscReg &val)
    { DBGX(2,"reg=%d val=%#lx\n",misc_reg, val); }

    virtual void setMiscReg(int misc_reg, const MiscReg &val)
    { DBGX(2,"reg=%d val=%#lx\n",misc_reg, val); }

    virtual uint64_t
    readRegOtherThread(int misc_reg, ThreadID tid)
    { DBGX(2,"\n"); return 0;}

    virtual void
    setRegOtherThread(int misc_reg, const MiscReg &val, ThreadID tid)
    { DBGX(2,"\n"); }

    // Also not necessarily the best location for these two.  Hopefully will go
    // away once we decide upon where st cond failures goes.
    virtual unsigned readStCondFailures()
    { DBGX(2,"\n"); return 0;}

    virtual void setStCondFailures(unsigned sc_failures)
    { DBGX(2,"\n"); }

    // Only really makes sense for old CPU model.  Still could be useful though.
    virtual bool misspeculating()
    { DBGX(2,"\n"); return false;}

#if !FULL_SYSTEM
    // Same with st cond failures.
    virtual Counter readFuncExeInst()
    { DBGX(2,"\n"); return 0;}

    virtual bool syscall(int64_t callnum)
    { DBGX(2,"\n"); return false;}

    // This function exits the thread context in the CPU and returns
    // 1 if the CPU has no more active threads (meaning it's OK to exit);
    // Used in syscall-emulation mode when a  thread calls the exit syscall.
    virtual int exit()
    { DBGX(2,"\n"); return 0;}
#endif

};

}
}
#endif
