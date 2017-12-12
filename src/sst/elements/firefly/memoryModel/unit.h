 
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

		virtual void resume( UnitBase* src = NULL ) { assert(0); }
		virtual std::string& name() { assert(0); }
		~UnitBase() {}
	};

    class Unit : public UnitBase {
      public:
        Unit( SimpleMemoryModel& model, Output& dbg ) : m_model( model ), m_dbg(dbg) {}

        virtual bool load( UnitBase* src, MemReq*, Callback callback ) { assert(0); }
        virtual bool store( UnitBase* src, MemReq* ) { assert(0); }

      protected:
        const char* prefix() { return m_prefix.c_str(); }

        Output& m_dbg;
        SimpleMemoryModel& m_model;
        std::string m_prefix;
    };
