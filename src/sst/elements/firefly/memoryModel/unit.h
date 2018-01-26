 
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
		~UnitBase() {}
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

	  private:
		int m_pendingWrite;
	};

    class Unit : public UnitBase {
      public:
        Unit( SimpleMemoryModel& model, Output& dbg ) : m_model( model ), m_dbg(dbg) {}

        virtual bool load( UnitBase* src, MemReq*, Callback callback ) { assert(0); }
        virtual bool store( UnitBase* src, MemReq* ) { assert(0); }
        virtual bool write( UnitBase* src, MemReq* ) { assert(0); }

      protected:
        const char* prefix() { return m_prefix.c_str(); }

        Output& m_dbg;
        SimpleMemoryModel& m_model;
        std::string m_prefix;
    };
