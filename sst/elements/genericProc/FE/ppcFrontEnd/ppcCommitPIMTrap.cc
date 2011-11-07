// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "pimSysCallDefs.h"
#include "ppcFront.h"
#include "regs.h"
#include "fe_debug.h"
#include <sim_trace.h>
#include <sst_config.h>
#include <sst/core/simulation.h> // for current cycle count (i.e. Simulation)

#if HAVE_PHXSIM_H
#include <sst/elements/PHXSimC/PHXEvent.h>
#endif

ppcThread* ppcInstruction::createThreadWithStack(processor* proc,
						 simRegister *regs)
{
    // create new thread
    ppcThread *newT = new ppcThread(proc, pid());
    if (newT) {
	// copy all registers, so we get globals.
	for (int i = 0; i < 32+64; ++i) {
	    newT->packagedRegisters[i] = regs[i];
	}
	memcpy(newT->ppcRegisters, parent->ppcRegisters, sizeof(*parent->ppcRegisters));
	// set up argument
	newT->packagedRegisters[3] = regs[5];
	newT->packagedRegisters[4] = regs[6];
	newT->packagedRegisters[5] = regs[7];
	newT->packagedRegisters[6] = regs[8];
	newT->packagedRegisters[7] = regs[9];
	newT->ppcRegisters->regs_L = 0; // return address
	// set programcounter
	newT->ProgramCounter = regs[4];
	newT->setMigrate(false);

	int loc = regs[3];
	uint stack_size = proc->maxLocalChunk();
	simAddress stack = 0;
	if (loc == -1) {
	    static int locrr = 0;
	    loc = locrr++;
	    locrr *= (locrr < (int)proc->numLocales());
	    INFO("sending thread to loc %i\n", loc);
	    stack = proc->localAllocateAtID(stack_size, loc);
	} else if (loc < 0) {
	    ERROR("specified locale (%i) not valid (too small) (max %u, min 0, auto=-1)!\n", loc, (unsigned)memory::numLocales());
	} else if (loc > (int)proc->numLocales()) {
	    /* according to the documentation, this means we pick somewhere near r5
	     * (which became r3) Note that we aren't paying attention to virtual
	     * addresses here. If we need to, that should be moved into the memory
	     * controller. */
	    //printf("allocating thread stack near 0x%x\n", regs[3]);
	    stack = proc->localAllocateNearAddr(stack_size, regs[3]);
	} else {
	    /* loc is within the appropriate range! */
	    INFO("sending thread to loc %i (as requested)\n", loc);
	    stack = proc->localAllocateAtID(stack_size, loc);
	}
	if (stack == 0) {
	    stack = proc->globalAllocate(stack_size);
	    if (stack == 0) {
		ERROR("Ran out of global memory for thread stacks!\n");
	    }
	}
	stack += stack_size;
	newT->packagedRegisters[1] = stack - 256;
	//newT->setStackLimits(stack, stack - stack_size);
	newT->SequenceNumber = regs[3] = ppcThread::nextThreadID++;
	ppcThread::threadIDMap[regs[3]] = newT;
    } else {
	ERROR("couldn't allocate new thread\n");
    }
    return newT;
}

/* This creates a PIM thread with a stack, but does not schedule the thread to be run
 * r3 specifies which 'locale' to go to.
 * r4 is the PC to start running at (i.e. the address of the function to run)
 * r5-r9 become the arguments to the new thread (r3-r7)
 * Note: new thread is set to non-migrateable
 */
bool ppcInstruction::Perform_PIM_SPAWN_TO_LOCALE_STACK_STOPPED(processor* proc,
							       simRegister *regs)
{
  ERROR("Needs to be fixed");
  if (createThreadWithStack(proc, regs)) {
    return true;
  } else {
    return false;
  }
}

bool ppcInstruction::Perform_PIM_START_STOPPED_THREAD(processor* proc,
						       simRegister *regs)
{
  ERROR("Needs to be fixed");
    // fetch the thread pointer som
    ppcThread *stoppedT = ppcThread::threadIDMap[regs[3]];
    // set it in motion, we use its stack to hint where it should start
    if (proc->spawnToCoProc(PIM_ANY_PIM, stoppedT,
			    stoppedT->packagedRegisters[1])) {
      // return the status
      return true;
    } else {
	regs[3] = 0;
	return false;
    }
}

//: This creates a PIM thread with a stack
//
// r3 specifies which 'locale' to go to. 
// r4 is the PC to start running at (i.e. the address of the function to run)
// r5-9 become the arguments to the new thread (r3-r7)
// Note: new thread is set to non-migrateable
bool ppcInstruction::Perform_PIM_SPAWN_TO_LOCALE_STACK(processor* proc,
						       simRegister *regs) {
  // create new thread
  ppcThread *newT = createThreadWithStack(proc, regs);
  if (newT) {
    if (proc->spawnToCoProc(PIM_ANY_PIM, newT, newT->packagedRegisters[1])) {
      return true;
    } else {
      ppcThread::threadIDMap.erase(regs[3]);
      regs[3] = 0;
      delete newT;
      return false;
    }
  } else {
    ERROR("createThreadWithStack failed\n");
    return 0;
  }
}

/* This sets up a new thread.
 * r1 is the stack ptr
 * r3 specifies which processor to go to (2 means "any")
 * r4 is the PC to start running at (i.e. the address of the function to run)
 */

bool ppcInstruction::Perform_PIM_SPAWN_TO_COPROC(processor* proc,
						 simRegister *regs) {
  // create new thread
  ppcThread *newT = new ppcThread(proc, pid());
  if (newT) {
    // copy all registers, so we get globals.
    for (int i = 0; i < 32+64; ++i) {
      newT->packagedRegisters[i] = regs[i];
    }
    memcpy(newT->ppcRegisters, parent->ppcRegisters, 
	   sizeof(*parent->ppcRegisters));
    // set up argument
    newT->packagedRegisters[3] = regs[5];
    newT->packagedRegisters[4] = regs[6];
    newT->packagedRegisters[5] = regs[7];
    newT->packagedRegisters[6] = regs[8];
    newT->packagedRegisters[7] = regs[9];
    // set up stack (if needed)
    if (ppcInstruction::magicStack) 
      newT->packagedRegisters[1] = ppcInitStackBase; //stack pointer
    newT->ppcRegisters->regs_L = 0; // return address
    //set progamcounter
    newT->ProgramCounter = regs[4];
    //we don't know the min stack address
    //newT->setStackLimits(0, 0);
    PIM_coProc coProcTarg = (PIM_coProc)regs[3];
#if 0
    // assign the thread an ID
    newT->SequenceNumber = regs[3] = ppcThread::nextThreadID++;
    ppcThread::threadIDMap[regs[3]] = newT;
#endif
    // use stack as hint
    if (proc->spawnToCoProc(coProcTarg, newT, newT->packagedRegisters[1])) {
	return true;
    } else {
      delete newT;
      ppcThread::threadIDMap.erase(regs[3]);
      regs[3] = 0;
      return false;
    }
  } else {
    ERROR("couldn't allocate new thread\n");
    return 0;
  }
}

bool ppcInstruction::Perform_PIM_SWITCH_ADDR_MODE(processor* proc,
						  simRegister *regs) {
  bool ret = proc->switchAddrMode((PIM_addrMode)regs[3]);

  regs[3] = ret;

  return ret;
}

bool ppcInstruction::Perform_PIM_QUICK_PRINT(processor	*FromProcessor,
					     simRegister	*regs_R)
{
  /* cheap time counting mechanism. feel free to discard */
  static unsigned long long t = 0;
  unsigned long long elap = FromProcessor->getCurrentSimTimeNano() - t;
  t = FromProcessor->getCurrentSimTimeNano();
  
  printf("OUTPUT: %x(%d) %x(%d) %x(%d) @ %llu\n",
	 (unsigned int)ntohl(regs_R[3]), (unsigned int)ntohl(regs_R[3]),
	 (unsigned int)ntohl(regs_R[4]), (unsigned int)ntohl(regs_R[4]),
	 (unsigned int)ntohl(regs_R[5]), (unsigned int)ntohl(regs_R[5]),
	 elap);
  fflush(stdout);
  
  // return values
  regs_R[3] = 0;
  return true;
}

bool ppcInstruction::Perform_PIM_TRACE(processor	*FromProcessor,
					     simRegister	*regs_R) {
  
#if 0
  SIM_trace( simulation::cycle, 
	FromProcessor->getProcNum()/2, 
	(FromProcessor->getProcNum()%2) ? "PPC" : "Opteron",
	regs_R[3], regs_R[4], regs_R[5] );
#endif

  // return values
  regs_R[3] = 0;

  return true;
}

// This is the implementation of PIM_fastFileRead()
bool ppcInstruction::Perform_PIM_FFILE_RD(processor* proc,
					  simRegister *regs) {
  simAddress cstr = ntohl(regs[3]);
  unsigned int len = 0;
  char buf[1024];
  char c;

  while ((len < 1023) && ( (c = proc->ReadMemory8(cstr + len,0)) != 0)) {
    buf[len] = c;
    len++;
  }
  buf[len] = 0;

  FILE *fp = fopen (buf, "r");

  //printf ("Opening file %s fp %p first char %c\n", buf, fp, fgetc(fp));

  if (fp == NULL) {
    fprintf (stderr, "Error trying fast page in of file %s\n", buf);
    regs[3] = 0;
    return true;
  }

  simAddress oBuf = ntohl(regs[4]);
  unsigned int maxB = ntohl(regs[5]);
  unsigned int offS = ntohl(regs[6]);

  //printf ("Seeking forward %d bytes, max read %d bytes buffer %x\n",
  //  offS, maxB, oBuf);

  if (fseek (fp, (long)offS, SEEK_SET) != 0) {
    fprintf (stderr, "Error trying to seek to %d in file %s\n", offS, buf);
    regs[3] = 0;
    return true;
  }

  len = 0;
  for (uint i = 0; i < maxB; i++) {
    if ( (c = fgetc(fp)) == EOF ) {
      i = maxB;
    } else {
      proc->WriteMemory8(oBuf + len, c, 0);
      //printf ("Write to memory %x, c = %c\n", oBuf + len, c);
      len++;
    }
  }

  regs[3] = ntohl(len);

  fclose(fp);
  return true;
}

#include "rand.h"
bool ppcInstruction::Perform_PIM_RAND(processor *proc,
				      simRegister *regs) {
  unsigned int r = SimRand::rand();
  regs[3] = ntohl(r);
  return true;
}

#define ALLOC_GLOBAL 0
#define ALLOC_LOCAL_ADDR 1
#define ALLOC_LOCAL_ID 2

bool ppcInstruction::Perform_PIM_MALLOC(processor *proc,
					simRegister *regs) {
  unsigned int size = ntohl(regs[3]);
  unsigned int allocType = ntohl(regs[4]);
  unsigned int opt = ntohl(regs[5]);
  unsigned int addr;

  INFO ("PIM_MALLOC: size %d type %d PC %x)\n", size, allocType, PC());

  switch (allocType) {
  case ALLOC_GLOBAL:
    addr = proc->globalAllocate(size);
    break;
  default:
  case ALLOC_LOCAL_ADDR:
    addr = proc->localAllocateNearAddr(size, opt);
    break;
  case ALLOC_LOCAL_ID:
    addr = proc->localAllocateAtID(size, opt);
    break;
  }

  regs[3] = ntohl(addr);

  return true;
}

bool ppcInstruction::Perform_PIM_FREE(processor *proc,
				      simRegister *regs) {
  unsigned int addr = ntohl(regs[3]);
  unsigned int size = ntohl(regs[4]);

  //XXX: This probably does not interact well with mapped memory, but just
  //     using getPhysAddr won't fix it (KBW)
  unsigned int result = memory::memFree(addr, size);

  if (result == 0) {
    ERROR("Fast Free failed \n");
#if 0
    printf("%p@%d: Commit %6s %p %p(%lx) %8p %8p %llu\n", 
	   this->parent, proc->getProcNum(), MD_OP_NAME(simOp), 
	   (void*)ProgramCounter, (void*)_memEA, 
	   proc->ReadMemory32(_memEA,0), (void*)(parent->getRegisters())[0], 
	   (void*)(parent->getRegisters())[31], 
	   component::cycle()); 
    printf ("Memory at %x: %x\n", (uint)ProgramCounter, (uint)proc->ReadMemory32(ProgramCounter, 0));
    printf ("Op returned from %x: %x, simOp %x\n", (uint)ProgramCounter, (uint)getOp(ProgramCounter), simOp);

    this->parent->printCallStack();
    fflush(stdout);
#endif
  }

  regs[3] = ntohl(result);

  return true;
}

//: Write directly to memory
//
// bypass cache, FU, etc...
bool ppcInstruction::Perform_PIM_WRITE_MEM(processor* proc, 
					   simRegister *regs) {
  simAddress addr = ntohl(regs[3]);
  simRegister data = regs[4];

  proc->WriteMemory32(ntohl(addr), data, false); // false means
						 // "non-speculatively"

  return true;
}

bool ppcInstruction::Perform_PIM_WRITE_SPECIAL(processor* proc, 
					       simRegister *regs,
					       const int num) {
  exceptType ret = NO_EXCEPTION;
  switch ((PIM_cmd)ntohl(regs[3])) {
  case PIM_CMD_SET_EVICT:
    parent->setEvict(ntohl(regs[4]) > 0);
    break;
  case PIM_CMD_SET_MIGRATE:
    parent->setMigrate(ntohl(regs[4]) > 0);
    break;
  case PIM_CMD_SET_THREAD_ID:
    //printf("set TID: thread %p -> %x (seq %d)\n", parent, (uint)regs[4], myS);
    parent->ThreadID = ntohl(regs[4]);
    break;
  case PIM_CMD_SET_FUTURE:
    parent->isFuture = (ntohl(regs[4]) != 0);
    break;

  default:
    ret = proc->writeSpecial((PIM_cmd)ntohl(regs[3]), // pim_cmd
			     num, // number of arguments
			     (uint*)&regs[4]);
  }

  if (ret == NO_EXCEPTION) {
    return true;
  } else {
    return false;
  }
}

bool ppcInstruction::Perform_PIM_READ_SPECIAL(processor *proc,
					      simRegister *regs, 
					      const int numIn,
					      const int numOut) {
  simRegister rets[numOut];
  exceptType ret = NO_EXCEPTION;

  static simAddress *local_ctrl;

  switch((PIM_cmd)ntohl(regs[3])) {
  case PIM_CMD_LOCAL_CTRL:
    if (local_ctrl == NULL) {
      local_ctrl = new simAddress[memory::numLocales()];
      for (unsigned int i = 0; i < memory::numLocales(); i++) 
	local_ctrl[i] = memory::localAllocateAtID(memory::maxLocalChunk(), i);
    }
    if ((unsigned)ntohl(regs[4]) > (unsigned)memory::numLocales())
      rets[0] = ntohl(local_ctrl[memory::getLocalID(proc)]);
    else
      rets[0] = ntohl(local_ctrl[regs[4]]);
    break;
  case PIM_CMD_LOC_COUNT:
    /* the number of different memory timing regions */
    rets[0] = ntohl(memory::numLocales());
    break;
  case PIM_CMD_MAX_LOCAL_ALLOC:
    /* the maximum size of contiguous memory that can be allocated that will
     * all be within the same timing region */
    rets[0] = ntohl(memory::maxLocalChunk());
    break;
  case PIM_CMD_CYCLE:
    /* the number of cycles executed thus far */
    //rets[0] = htonl((simRegister)parent->home->getCurrentSimTime());
    rets[0] = htonl((simRegister)Simulation::getSimulation()->getCurrentSimCycle());
    break;
  case PIM_CMD_ICOUNT:
    /* the number of committed instructions */
    rets[0] = ntohl((unsigned)ppcInstruction::totalCommitted);
    break;
  case PIM_CMD_THREAD_ID:
    /* the thread ID (this must be set by the thread, defaults to 0) */
    processor::checkNumArgs((PIM_cmd)ntohl(regs[3]), numIn, numOut, 0, 1); //XXX: What is this for?
    rets[0] = ntohl(parent->ThreadID);
    //printf("get TID: thread %p = %x\n", parent, rets[0]);
    break;
  case PIM_CMD_THREAD_SEQ:
    /* the thread sequence number (this can NOT be set by the thread) */
    processor::checkNumArgs((PIM_cmd)ntohl(regs[3]), numIn, numOut, 0, 1); //XXX: What is this for?
    rets[0] = ntohl(parent->SequenceNumber);
    //rets[0] = ppcThread::threadSeq[parent];
    break;
  case PIM_CMD_SET_FUTURE:
    /* a boolean mark on a thread; whether it is a future (i.e. subject to
     * futurelib bookkeeping) or not */
    rets[0] = ntohl(parent->isFuture);
    break;
  case PIM_CMD_GET_NUM_CORE:
    /* the number of cores per chip (why does anyone care?) */
    rets[0] = ntohl(proc->getNumCores());
    break;
  case PIM_CMD_GET_CORE_NUM:
    /* the number of the core we're currently executing on */
    rets[0] = ntohl(proc->getCoreNum());
    break;
  case PIM_CMD_GET_MHZ:
    //rets[0] = proc->ClockMhz();
    WARN("Get_Mhz not supported\n");
    break;
  case PIM_CMD_GET_CTOR:
    rets[0] = ntohl(parent->loadInfo.constrSize);
    rets[1] = ntohl(parent->loadInfo.constrLoc);
    break;
  default:
    ret = proc->readSpecial((PIM_cmd)ntohl(regs[3]), //pim_cmd
			    numIn, // number of Input arguments
			    numOut, // number of Output arguments
			    &regs[4], 
			    rets);
  }

  if (ret != NO_EXCEPTION) {
    _exception = ret;
    return false;
  } else {
    _exception = ret;
    for (int i = 0; i < numOut; ++i) 
      regs[3+i] = rets[i];
    return true;
  }
}

// NOTE: returns 0 for success, just like mutex_trylock
bool ppcInstruction::Perform_PIM_TRY_READFX(
    processor * proc,
    simRegister * regs)
{
    simAddress dest = ntohl(regs[3]);
    simAddress src  = ntohl(regs[4]);
    //printf ("try readfx %x\n", src); fflush(stdout);

    if (proc->getFE(src) == memory_interface::FULL) {
	// it is full, do the read, and set the bit as needed
	if (regs[0] == (int32_t) htonl(SS_PIM_TRY_READFE)) {
	    // if we need to, empty the FE bit
	    proc->setFE(src, memory_interface::EMPTY);
	}
	proc->WriteMemory32(dest, proc->ReadMemory32(src, false), false);	// false => "non-speculatively"
	regs[3] = 0;		       // return 0 to indicate success
    } else {
	// the FEB is empty, so return 1 to indicate failure
	regs[3] = htonl(1);
    }

    return true;
}

bool ppcInstruction::Perform_PIM_READFX(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  //printf ("readfx %x\n", addr); fflush(stdout);

  if (proc->getFE(addr) == memory_interface::FULL) {
    // it is full, do the read, and set the bit as needed
      if (regs[0] == (int32_t) ntohl(SS_PIM_READFE)) {
      // if we need to, empty the FE bit
      proc->setFE(addr, memory_interface::EMPTY);
    } 
    regs[3] = ntohl(proc->ReadMemory32(addr,0));

    return true;  
  } else {
    _exception = FEB_EXCEPTION;
    _febTarget  = addr;

    return false;  
  }
  
}

bool ppcInstruction::Perform_PIM_ATOMIC_INCREMENT(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister sum = ntohl(proc->ReadMemory32(addr,0));
  //return the original value
  regs[3] = ntohl(sum);

  sum += ntohl(regs[4]);
  //store the incremented value
  proc->WriteMemory32(addr, ntohl(sum), 0);

  //set feb to full
  //proc->setFE(addr,1);

  return true;
}

bool ppcInstruction::Perform_PIM_ATOMIC_DECREMENT(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister sum = ntohl(proc->ReadMemory32(addr,0));
  //return the original value
  regs[3] = ntohl(sum);

  sum -= ntohl(regs[4]);
  //store the incremented value
  proc->WriteMemory32(addr, ntohl(sum), 0);

  //set feb to full
  //proc->setFE(addr,1);

  return true;
}

bool ppcInstruction::Perform_PIM_BULK_set_FE(processor	*proc,
					     simRegister	*regs,
					     simRegister val) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister len = ntohl(regs[4]);

  if (val == 0) {
      for (int i = 0; i < len; ++i) {
	  proc->setFE(addr+i, memory_interface::EMPTY);
      }
  } else {
      for (int i = 0; i < len; ++i) {
	  proc->setFE(addr+i, memory_interface::FULL);
      }
  }

  regs[3] = 0;

  return true;
}

bool ppcInstruction::Perform_PIM_CHANGE_FE(processor	*proc,
					     simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);

  switch (ntohl(regs[0])) {
  case SS_PIM_FILL_FE:
    proc->setFE(addr, memory_interface::FULL);
    break;
  case SS_PIM_EMPTY_FE:
    proc->setFE(addr, memory_interface::EMPTY);
    break;
  default:
    WARN("unknown: %x %d\n", ntohl(regs[0]), ntohl(regs[3]));
    ERROR("Unrecognized PIM_CHANGE_FE type.\n");
    break;
  }

  regs[3] = 0;

  return true;
}

bool ppcInstruction::Perform_PIM_UNLOCK(processor *proc, simRegister *regs)
{
  int current_state = proc->getFE(ntohl(regs[3])) == memory_interface::FULL;
  if (base_memory::getDefaultFEB() == memory_interface::FULL) {
    regs[0] = ntohl(SS_PIM_FILL_FE);
    if (ppcThread::verbose > 5 && current_state != 0) {
      INFO("unlocking a currently unlocked address: %p\n", 
	   (void*)(size_t)ntohl(regs[3]));
    }
  } else {
    if (ppcThread::verbose > 5 && current_state == 0) {
      INFO("unlocking a currently unlocked address: %p\n", 
	   (void*)(size_t)ntohl(regs[3]));
    }
    regs[0] = ntohl(SS_PIM_EMPTY_FE);
  }
  return Perform_PIM_CHANGE_FE(proc, regs);
}

bool ppcInstruction::Perform_PIM_LOCK(processor *proc, simRegister *regs)
{
    if (base_memory::getDefaultFEB() == memory_interface::FULL) {
      regs[0] = ntohl(SS_PIM_READFE);
      return Perform_PIM_READFX(proc, regs);
    } else {
      // regs[4] = 1;
      return Perform_PIM_WRITEEF(proc, regs);
    }
}

bool ppcInstruction::Perform_PIM_TRY_WRITEEF(
    processor * proc,
    simRegister * regs)
{
    simAddress dest = ntohl(regs[3]);
    simAddress src  = ntohl(regs[4]);

    //printf("try writeef 0x%x 0x%x\n", dest, src);
    if (proc->getFE(dest) == memory_interface::EMPTY) {
	// it is empty, so do the write, and set the bit
	proc->WriteMemory32(dest, proc->ReadMemory32(src, false), false);
	proc->setFE(dest, memory_interface::FULL);
	regs[3] = 0;
    } else {
	regs[3] = htonl(1);
    }
    return true;
}

bool ppcInstruction::Perform_PIM_WRITEEF(
    processor * proc,
    simRegister * regs)
{
    simAddress addr = ntohl(regs[3]);

    //printf("writeef 0x%x\n", addr);
    if (proc->getFE(addr) == memory_interface::EMPTY) {
	// its empty to write and fill
	proc->WriteMemory32(addr, htonl(regs[4]), 0);
	proc->setFE(addr, memory_interface::FULL);

	regs[3] = 0;
	return true;
    } else {
	// fail, raise exception
	_exception = FEB_EXCEPTION;
	_febTarget = addr;

	return false;
    }
}

bool ppcInstruction::Perform_PIM_IS_FE_FULL(processor	*proc,
					    simRegister	*regs) {
  simAddress addr = ntohl(regs[3]);
  //printf ("isfull %x\n", addr); fflush(stdout);

  regs[3] = ntohl(proc->getFE(addr)==memory_interface::FULL);

  return true;
}

// NOTE: returns 0 for success, just like mutex_trylock
bool ppcInstruction::Perform_PIM_TRYEF(processor	*proc,
				       simRegister	*regs) {
  simAddress addr = ntohl(regs[3]);
  //printf ("tryef %x\n", addr); fflush(stdout);

  if (proc->getFE(addr) == memory_interface::EMPTY) {
    // its empty, so we can fill it
    proc->setFE(addr, memory_interface::FULL);
    regs[3] = 0;
  } else {
    // the FEB is already full, so return 1
    regs[3] = ntohl(1);
  }

  return true;
}

bool ppcInstruction::Perform_PIM_FORK(processor	*proc,
				      simRegister	*regs) {
  ERROR("Needs to be fixed");
  ppcThread *newT = new ppcThread(proc, pid());
  bool ret = proc->insertThread(newT);
  if (ret) {
    // init new thread
#if 0
    simRegister *newR = proc->getFrame(newT->registers);
#else
    simRegister *newR = newT->getRegisters();
#endif
    // copy all registers, so we get globals.
    for (int i = 0; i < 32; ++i) {
      newR[i] = regs[i];
    }
    // set up argument
    newR[3] = regs[4];

    // set up stack
    //newR[1] = 0x7fff0000; 
    if (magicStack) newR[1] = ppcInitStackBase; //stack pointer 

    newT->ppcRegisters->regs_L = 0; // return address
    newT->ProgramCounter = regs[3]; // set PC

    //we don't know the min stack address
    //newT->setStackLimits (0,0);

    //return values
    regs[3] = 0;
  } else {
    regs[3] = 0;
    delete newT;
  }

  return ret;
}

bool ppcInstruction::Perform_PIM_RESET(processor	*proc,
				      simRegister	*regs) {
  proc->resetCounters();
  return true;
}


bool ppcInstruction::Perform_PIM_EXIT(processor	*proc,
				      simRegister *regs) {
  //printf("%p: PIM_EXIT\n", parent);
  parent->_isDead = 1;
  return true;
}

bool ppcInstruction::Perform_PIM_EXIT_FREE(processor	*proc,
					   simRegister *regs) {
  //printf("%p: PIM_EXIT_FREE\n", parent);
  
  //parent->freeStack();
  parent->_isDead = 1;
  return true;
}

bool ppcInstruction::Perform_PIM_MEM_REGION_CREATE(processor    *proc,
                                             simRegister        *regs )
{
  int region         = ntohl(regs[3]);
  simAddress vaddr   = ntohl(regs[4]);
  unsigned long size = ntohl(regs[5]);
  simAddress kaddr   = ntohl(regs[6]);

  //DPRINT(0,"region=%d vaddr=%#lx size=%d kaddr=%#lx\n",
  //region,(long unsigned int)vaddr,(int)size,(long unsigned int)kaddr,regs[7]);

  memType_t type;
  switch( ntohl(regs[7]) ) {
    case PIM_REGION_CACHED:
	type = CACHED;
	break;
    default:
    case PIM_REGION_UNCACHED:
	type = UNCACHED;
	break;
    case PIM_REGION_WC:
	type = WC;
	break;
  }
  regs[3] = ntohl(proc->CreateMemRegion(region, vaddr, size, kaddr, type ));

  return true;
}

bool ppcInstruction::Perform_PIM_MEM_REGION_GET(processor       *proc,
                                             simRegister        *regs )
{
  int region           = ntohl(regs[3]);
  unsigned long addr  = (unsigned long)ntohl(regs[4]);
  unsigned long len   = (unsigned long)ntohl(regs[5]);

  DPRINT(0,"region=%d addr=%#lx len=%#lx\n", region, addr, len);

  switch( region ) {
     case PIM_REGION_DATA:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.data_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.data_len), 0);
       break;
     case PIM_REGION_STACK:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.stack_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.stack_len), 0);
       break;
     case PIM_REGION_TEXT:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.text_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.text_len), 0);
       break;
     case PIM_REGION_HEAP:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.heap_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.heap_len), 0);
       break;
  }

  return true;
}

//: Send an Atomic Memory Operations to memory
// 
// Note: we actually do the operaiton here, not in the memory
// component, due to the execute-on-fetch problem.
bool ppcInstruction::Perform_PIM_AMO(processor *proc, simRegister *regs) {
  simAddress addr    = ntohl(regs[3]);
  PIM_amo_types op   = (PIM_amo_types)ntohl(regs[4]);
  uint32 immediate = ntohl(regs[5]);

  // perform the operation
  // get the info from memory
  simRegister data = ntohl(proc->ReadMemory32(addr,0));
  simRegister modified;

  switch(op) {
  case PIM_AMO_XOR:
    modified = data ^ immediate;
    proc->WriteMemory32(addr, ntohl(modified), 0); 
    break;
  case PIM_AMO_ADD16: // a 16-bit add
    {
      //printf("AMO ADD16 to addr %x\n", addr);
      uint16 data16 = ntohl(proc->ReadMemory16(addr,0));
      data16 += (uint16)immediate;
      proc->WriteMemory16(addr, ntohl(data16), 0); 
    }
    break;
  case PIM_AMO_XOR64:
    {
      //printf("AMO XOR64 to addr %x\n", addr);
      qword_t data64 = proc->ReadMemory64(addr,0);
      data64 = endian_swap(data64);
      data64 ^= (qword_t)immediate;
      data64 = endian_swap(data64);
      //proc->WriteMemory64(addr,data64,0);
    }
    break;
  default:
    ERROR("Unrecognized PIM_AMO_type %d.\n", op);
  }

  // send the parcel to memory
#if HAVE_PHXSIM_H
  bool ret = proc->sendMemoryParcel(addr, this, 0, 
				    proc->getCurrentRunningCore());
#else
  bool ret = false;
#endif
  // set result. A failure of sendMemoryParcel() probably indicates
  // the need to retry
  regs[3] = ntohl(ret);

  return true;
}

//: Send a matrix-vector multiply Operation to memory
//
// PIM_MatVec(int start, int end, const double *cur_vals, 
//            const double *x, const int *cur_inds, double *sum)
//
// becomes
//
// for (int j=0; j< end; j++)
//    sum += cur_vals[j]*x[cur_inds[j]];
// 
// Note: we actually do the operaiton here, not in the memory
// component, due to the execute-on-fetch problem.
bool ppcInstruction::Perform_PIM_MATVEC(processor *proc, simRegister *regs) {
#if HAVE_PHXSIM_H
  // collect arguments
  int start = ntohl(regs[3]);
  int end = ntohl(regs[4]);
  simAddress cur_vals = ntohl(regs[5]);
  simAddress x = ntohl(regs[6]);
  simAddress cur_inds = ntohl(regs[7]);
  simAddress sumAddr = ntohl(regs[8]);

  /*printf("SST MatVec: %d,%d,%p,%p,%p,%p\n", start,end,(void*)cur_vals,
    (void*)x,(void*)cur_inds,(void*)sumAddr);*/

  PHXEvent *pe = new PHXEvent();
  pe->eventType = MATVEC;

  // perform the operation, recording the addresses we needed
  double sum = 0.0;
  for (int j = start; j < end; ++j) {
    // get cur_vals[j]
    simAddress addr = cur_vals + sizeof(double) * j;
    qword_t c_v_j = proc->ReadMemory64(addr,0);
    c_v_j = endian_swap(c_v_j);
    double cur_vals_j = *((double*)&c_v_j);
    pe->mvAddrs.CV.push_back(addr);

    // get cur_inds[j]
    addr = cur_inds + sizeof(int) * j;
    int cur_inds_j = ntohl(proc->ReadMemory32(addr,0));
    pe->mvAddrs.I.push_back(addr);
    //printf(" j=%d  cur_inds[j] = %d\n", j, cur_inds_j);

    // get x[cur_inds[j]]
    addr = x + sizeof(double) * cur_inds_j;
    qword_t x_c_i_j = proc->ReadMemory64(addr,0);
    x_c_i_j = endian_swap(x_c_i_j);
    double x_cur_inds_j = *((double*)&x_c_i_j);
    pe->mvAddrs.X.push_back(addr);

    //printf(" %f * %f\n", cur_vals_j, x_cur_inds_j);

    sum += cur_vals_j * x_cur_inds_j;
  }

  // send the parcel to memory
  // set result. A failure of sendMemoryParcel() probably indicates
  bool ret = proc->sendMemoryParcel(0, this, pe, 
				    proc->getCurrentRunningCore());

  if (ret) {
    //printf(" SST Sum is %f.\n", sum);
    // save result
    qword_t sumWord = *((qword_t*)&sum);
    proc->WriteMemory64(sumAddr, endian_swap(sumWord), 0);
    pe->mvAddrs.sum = sumAddr;
  } else {
    printf("redo\n");
  }

  // the need to retry
  regs[3] = ntohl(ret);
  
  //exit(0);
  return true;
#else
  return false;
#endif
}

bool ppcInstruction::Perform_PIM_ADV_OUT(processor *proc, simRegister *regs) {
  regs[3] = ntohl(proc->getOutstandingAdvancedMemReqs(proc->getCurrentRunningCore(), 0));
  return true;
}

bool ppcInstruction::Perform_PIM_FORCE_CALC(processor *proc, simRegister *regs) {
#if HAVE_PHXSIM_H
  // collect arguments
  const int j = ntohl(regs[3]);
  const int i = ntohl(regs[4]);
  const simAddress af = ntohl(regs[5]);
  const simAddress ax = ntohl(regs[6]);
  const simAddress cutforceAddr = ntohl(regs[7]);
  //const int numneigh = ntohl(regs[8]);

  // check if we can send to memory
  bool ret;
  int max = 0;
  int haveOut = 
    proc->getOutstandingAdvancedMemReqs(proc->getCurrentRunningCore(), &max);
  if (haveOut < max) {
    ret = true;
  } else {
    ret = false;
    printf("redo-no room");
  }

  // create new event
  PHXEvent *pe = new PHXEvent();
  pe->eventType = FORCE_CALC;

  // perform the operation and record addresses
  if (ret) {
    double cutforcesq = ReadMemoryDouble(cutforceAddr,proc);
    //pe->fAddrs.inits.push_back(cutforceAddr);

    //const double xtmp = a.x[i][0];
    const double xtmp = ReadMemoryDouble(ax+((i*3+0)*8),proc);
    //pe->fAddrs.inits.push_back(ax+((i*3+0)*8));
    //const double ytmp = a.x[i][1];
    const double ytmp = ReadMemoryDouble(ax+((i*3+1)*8),proc);
    //pe->fAddrs.inits.push_back(ax+((i*3+1)*8));
    //const double ztmp = a.x[i][2];
    const double ztmp = ReadMemoryDouble(ax+((i*3+2)*8),proc);
    //pe->fAddrs.inits.push_back(ax+((i*3+2)*8));    
    /* assume these were already loaded*/
    
    //const double delx = xtmp - a.x[j][0];
    const double delx = xtmp - ReadMemoryDouble(ax+((j*3+0)*8),proc);
    pe->fAddrs.inits.push_back(ax+((j*3+0)*8));    
    //const double dely = ytmp - a.x[j][1];
    const double dely = ytmp - ReadMemoryDouble(ax+((j*3+1)*8),proc);
    pe->fAddrs.inits.push_back(ax+((j*3+1)*8));    
    //const double delz = ztmp - a.x[j][2];
    const double delz = ztmp - ReadMemoryDouble(ax+((j*3+2)*8),proc);
    pe->fAddrs.inits.push_back(ax+((j*3+2)*8));    
    //const double rsq = delx*delx + dely*dely + delz*delz;
    const double rsq = delx*delx + dely*dely + delz*delz;
    if (rsq < cutforcesq) {
      pe->tookBranch = true;

      const double sr2 = 1.0/rsq;
      const double sr6 = sr2*sr2*sr2;
      const double force = sr6*(sr6-0.5)*sr2;
      //a.f[i][0] += delx*force;
      simAddress addr = af+((i*3+0)*8);
      double answer = ReadMemoryDouble(addr,proc) + delx*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    
      //a.f[i][1] += dely*force;
      addr = af+((i*3+1)*8);
      answer = ReadMemoryDouble(addr,proc) + dely*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    
      //a.f[i][2] += delz*force;
      addr = af+((i*3+2)*8);
      answer = ReadMemoryDouble(addr,proc) + delz*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    

      //a.f[j][0] -= delx*force;
      addr = af+((j*3+0)*8);
      answer = ReadMemoryDouble(addr,proc) - delx*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    
      //a.f[j][1] -= dely*force;
      addr = af+((j*3+1)*8);
      answer = ReadMemoryDouble(addr,proc) - dely*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    
      //a.f[j][2] -= delz*force;
      addr = af+((j*3+2)*8);
      answer = ReadMemoryDouble(addr,proc) - delz*force;
      WriteMemoryDouble(addr, proc, answer);
      pe->fAddrs.a_fs.push_back(addr);    
      //printf("sst af_j_2 %d,%d,%d %f\n", i,j,k,answer);
    } else {
      pe->tookBranch = false;
    }
    
    //send parcel to memory
    bool ret = proc->sendMemoryParcel(0, this, pe, 
				      proc->getCurrentRunningCore());
    
    // save results
    if (ret) {
      // we should save results here, but we do up above instead.
    } else {
      // this should really never happen if we pass the check up tom
      printf("redo - odd\n");
    }
  } else {
    delete pe;
  }
  
  // return the need to retry
  regs[3] = ntohl(ret);
  
  return true;
#else
  printf("no HAVE_PHXSIM_H\n");
  return false;
#endif
}

bool ppcInstruction::Perform_PIM_PAGERANK(processor *proc, simRegister *regs)
{
#if HAVE_PHXSIM_H
  // collect arguments
  const int begin = ntohl(regs[3]);
  const int end = ntohl(regs[4]);
  const int i = ntohl(regs[5]);
  const simAddress types = ntohl(regs[6]);
  const simAddress fh = ntohl(regs[7]);
  const simAddress rev_end_points = ntohl(regs[8]);
  const simAddress rinfo = ntohl(regs[9]);

#if 0
  static int c = 0;
  c++;
  if (c & 0x3f) {
    printf("pagerank: %d %d %d %p %p %p %p\n", begin, end, i, (void*)types,
	   (void*)fh, (void*)rev_end_points, (void*)rinfo);
  }
#endif

  // check if we can send to memory
  bool ret;
  int max = 0;
  int haveOut = 
    proc->getOutstandingAdvancedMemReqs(proc->getCurrentRunningCore(), &max);
  if (haveOut < max) {
    ret = true;
  } else {
    ret = false;
    printf("redo-no room");
  }

  // create new event
  PHXEvent *pe = new PHXEvent();
  pe->eventType = PAGERANK;

  static int care = 0;
  static int dcare = 0;

  // perform the operation and record addresses
  if (ret) {
    simAddress addr;
    double total = 0.0;
    //if (types[i] != 1 || fh[i] < 10) { continue; }
    addr = types + i * 4;
    unsigned int types_i = ntohl(proc->ReadMemory32(addr,0));
    pe->pAddrs.inits.push_back(addr);
    addr = fh + i * 4;
    unsigned int fh_i = ntohl(proc->ReadMemory32(addr,0));
    pe->pAddrs.inits.push_back(addr);
    bool dontCareAboutNode = (types_i == 0 || fh_i < 10);
    if (dontCareAboutNode) dcare++;
    else care++;   
    if (!dontCareAboutNode) {
      pe->tookBranch = 1;
      // do care about node
      //for (int j  = begin ; j < end ; ++j) {
      for (int j = begin; j < end; ++j) {
	//  vertex_descriptor_t src = rev_end_points[j];
	addr = rev_end_points + j * 4;
	unsigned int src = ntohl(proc->ReadMemory32(addr,0));
	pe->pAddrs.neigh.push_back(addr);
	//  if (types[src] != 1 || fh[src] < 10) {continue;}
	addr = types + src * 4;
	unsigned int types_src = ntohl(proc->ReadMemory32(addr,0));
	pe->pAddrs.neigh.push_back(addr);
	addr = fh + src * 4;
	unsigned int fh_src = ntohl(proc->ReadMemory32(addr,0));
	pe->pAddrs.neigh.push_back(addr);
	bool dontCareAboutNeighbor = (types_src == 0 || fh_src < 10);
	if (!dontCareAboutNeighbor) {
	  //  double r = qt_rinfo->rinfo[src].rank;
	  addr = rinfo + (src * 20) + 12;
	  double r = ReadMemoryDouble(addr,proc);
	  pe->pAddrs.care_neigh.push_back(addr);
	  //  double incr = (r / qt_rinfo->rinfo[src].degree);
	  addr = rinfo + (src * 20);	  
	  int degree = ntohl(proc->ReadMemory32(addr,0));
	  pe->pAddrs.care_neigh.push_back(addr);
	  double incr = r / double(degree);
	  //  total += incr;
	  total += incr;
	}
	//}   
      }
      //qt_rinfo->rinfo[i].acc = total;
      addr = rinfo + (i * 20) + 4;
      WriteMemoryDouble(addr, proc, total);
      pe->pAddrs.total = addr;
    } else {
      pe->tookBranch = 0;
    }

    // send parcel to memory
    bool ret = proc->sendMemoryParcel(0, this, pe, 
				      proc->getCurrentRunningCore());
    
    // save results
    if (ret) {
    } else {
      // this should really never happen if we pass the check up tom
      printf("redo - odd\n");
    }
  } else {
    delete pe;
  }

  // return the need to retry
  regs[3] = ntohl(ret);

  return true;
#else
  return false;
#endif
}
