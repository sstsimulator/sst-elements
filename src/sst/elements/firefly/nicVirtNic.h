// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class VirtNic {
        Nic& m_nic;
      public:
        VirtNic(Nic& nic, int _id, std::string portName) : m_nic(nic), id(_id)
        {
            std::ostringstream tmp;
            tmp <<  id;

            m_toCoreLink = nic.configureLink( portName + tmp.str(), "1 ns",
                new Event::Handler<Nic::VirtNic>(
                    this, &Nic::VirtNic::handleCoreEvent ) );
            assert( m_toCoreLink );
        }
        ~VirtNic() {}

        void handleCoreEvent( Event* ev ) {
            m_nic.handleVnicEvent( ev, id );
        }

        void init( unsigned int phase ) {
            if ( 0 == phase ) {
                m_toCoreLink->sendInitData( new NicInitEvent(
                        m_nic.getNodeId(), id, m_nic.getNum_vNics() ) );
            }
        }

        Link* m_toCoreLink;
        int id;
        void notifyRecvDmaDone( int src_vNic, int src, int tag, size_t len,
                                                            void* key ) {
            m_toCoreLink->send(0,
                new NicRespEvent( NicRespEvent::DmaRecv, src_vNic,
                        src, tag, len, key ) );
        }
        void notifyNeedRecv( int src_vNic, int src, int tag, size_t len ) {
            m_toCoreLink->send(0,
                new NicRespEvent( NicRespEvent::NeedRecv, src_vNic,
                        src, tag, len ) );
        }
        void notifySendDmaDone( void* key ) {
            m_toCoreLink->send(0,new NicRespEvent( NicRespEvent::DmaSend, key));
        }
        void notifySendPioDone( void* key ) {
            m_toCoreLink->send(0,new NicRespEvent( NicRespEvent::PioSend, key));
        }
        void notifyPutDone( void* key ) {
            m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::Put, key ));
        }
        void notifyGetDone( void* key ) {
            m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::Get, key ));
        }
    };
