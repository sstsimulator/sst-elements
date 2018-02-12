    class NicUnit : public Unit {
      public:
        NicUnit( SimpleMemoryModel& model, Output& dbg, int id ) : Unit( model, dbg ) {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::NicUnit::@p():@l ";
        }

        bool storeCB( UnitBase* src, MemReq* req, Callback callback ) {
            m_dbg.verbosePrefix(prefix(), CALL_INFO,1,1,"\n");
            m_model.schedCallback( 1, callback );

			delete req;
			return false;
        }
        bool load( UnitBase* src, MemReq* req, Callback callback ) {
            m_dbg.verbosePrefix(prefix(), CALL_INFO,1,1,"\n");

            m_model.schedCallback( 1, callback );
			delete req;
			return false;
        }
      private:
    };
