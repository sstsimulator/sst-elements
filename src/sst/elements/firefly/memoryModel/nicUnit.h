// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

    class NicUnit : public Unit {
      public:
        NicUnit( SimpleMemoryModel& model, Output& dbg, int id ) : Unit( model, dbg ) {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::NicUnit::@p():@l ";
        }

        bool storeCB( UnitBase* src, MemReq* req, Callback* callback ) {
            m_dbg.verbosePrefix(prefix(), CALL_INFO,1,1,"\n");
            m_model.schedCallback( 0, callback );

			delete req;
			return false;
        }
        bool load( UnitBase* src, MemReq* req, Callback* callback ) {
            m_dbg.verbosePrefix(prefix(), CALL_INFO,1,1,"\n");

            m_model.schedCallback( 0, callback );
			delete req;
			return false;
        }
      private:
    };
