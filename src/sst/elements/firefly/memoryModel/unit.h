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

#if 0

     struct Entry {
        enum Op { Load, Store };
        Entry( Op op, Callback callback, Hermes::Vaddr addr, size_t length) : op(op), callback(callback), addr(addr), length(length) {}
        Op  op;
        Callback callback;
        Hermes::Vaddr addr;
        size_t  length;
    };
#endif

	class UnitBase {
	  public:

		UnitBase() : m_pendingWrite(0) {}
		virtual void resume( UnitBase* src = NULL ) { assert(0); }
		virtual std::string& name() { assert(0); }
		virtual ~UnitBase() {}
		void incPendingWrites() {
			++m_pendingWrite;
		}
		void decPendingWrites() {
			assert( m_pendingWrite>0);
			--m_pendingWrite;
		}
		int numPendingWrites() {
			return m_pendingWrite;
		}
        virtual void printStatus( Output&, int ) {assert(0);}
		virtual void init(unsigned int phase) { assert(0);}

	  private:
		int m_pendingWrite;
	};

    class Unit : public UnitBase {
      public:
        Unit( SimpleMemoryModel& model, Output& dbg ) : m_model( model ), m_dbg(dbg) {}

        virtual bool load( UnitBase* src, MemReq*, Callback* callback ) { assert(0); }
        virtual bool store( UnitBase* src, MemReq* ) { assert(0); }
        virtual bool storeCB( UnitBase* src, MemReq*, Callback* callback = NULL ) { assert(0); }

      protected:
        const char* prefix() { return m_prefix.c_str(); }

        Output& m_dbg;
        SimpleMemoryModel& m_model;
        std::string m_prefix;
    };
