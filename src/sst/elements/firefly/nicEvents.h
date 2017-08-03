// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class NicInitEvent : public Event {

  public:
    int node;
    int vNic;
    int num_vNics;

    NicInitEvent() :
        Event() {}

    NicInitEvent( int _node, int _vNic, int _num_vNics ) :
        Event(),
        node( _node ),
        vNic( _vNic ),
        num_vNics( _num_vNics )
    {
    }

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & node;
        ser & vNic;
        ser & num_vNics;
    }

    ImplementSerializable(SST::Firefly::NicInitEvent);
};

class NicCmdBaseEvent : public Event {

  public:
    enum Type { Shmem, Msg } base_type;

    NicCmdBaseEvent( Type type ) : Event(), base_type(type) {}

    NotSerializable(NicCmdBaseEvent)
};

class NicShmemCmdEvent : public NicCmdBaseEvent {
  public:
    typedef std::function<void( uint64_t )> Callback;

    enum Type { RegMem, Fence, PutP, PutV, GetP, GetV } type;
    NicShmemCmdEvent( Type type, Callback callback ) :
        NicCmdBaseEvent( Shmem ), type(type), callback(callback) {}

    NicShmemCmdEvent( Type type, Hermes::MemAddr& addr,
            size_t len, Callback callback ) :
        NicCmdBaseEvent( Shmem ), type(type), src(addr),
        len(len), callback(callback)
    {}

    NicShmemCmdEvent( Type type,
            int vnic, int node,
            Hermes::MemAddr& dest,
            Hermes::MemAddr& src,
            size_t len, Callback callback ) :
        NicCmdBaseEvent( Shmem ), type(type),
        vnic(vnic), node(node), src(src), dest(dest),
        len(len), callback(callback)
    {}

    NicShmemCmdEvent( Type type,
            int vnic, int node,
            Hermes::MemAddr& dest,
            uint64_t value,
            size_t len, Callback callback ) :
        NicCmdBaseEvent( Shmem ), type(type),
        vnic(vnic), node(node), value(value), dest(dest),
        len(len), callback(callback)
    {}

    NicShmemCmdEvent( Type type,
            int vnic, int node,
            Hermes::MemAddr& src,
            size_t len, Callback callback ) :
        NicCmdBaseEvent( Shmem ), type(type),
        vnic(vnic), node(node), src(src),
        len(len), callback(callback)
    { }

    Callback callback;
    Hermes::MemAddr src;
    Hermes::MemAddr dest;
    size_t len;
    int vnic;
    int node;
    uint64_t value;

    NotSerializable(NicShmemCmdEvent)
};

class NicCmdEvent : public NicCmdBaseEvent {
  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, RegMemRgn } type;
    int  node;
    int dst_vNic;
    int tag;
    std::vector<IoVec> iovec;
    void* key;

    NicCmdEvent( Type _type, int _vNic, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key ) :
        NicCmdBaseEvent(Msg),
        type( _type ),
        node( _node ),
        dst_vNic( _vNic ),
        tag( _tag ),
        iovec( _vec ),
        key( _key )
    {
    }

    NotSerializable(NicCmdEvent)
};

class NicRespBaseEvent : public Event {
  public:
    enum Type { Shmem, Msg } base_type;
    NicRespBaseEvent( Type type ) : Event(), base_type(type) {}

    NotSerializable(NicCmdEvent)
};

class NicShmemRespEvent : public NicCmdBaseEvent {
  public:
    typedef std::function<void( uint64_t )> Callback;

    enum Type { RegMem, Fence, PutP, PutV, GetP, GetV } type;

    uint64_t value;

    NicShmemRespEvent( Type type, Callback callback, uint64_t value = 0) : 
        NicCmdBaseEvent( Shmem ), type(type), value( value), callback(callback) {}

    Callback callback;

    NotSerializable(NicShmemRespEvent)
  
};

class NicRespEvent : public NicRespBaseEvent {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, NeedRecv } type;
    int src_vNic;
    int node;
    int tag;
    int len;
    void* key;

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len, void* _key ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len ),
        key( _key )
    {
    }

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len )
    {
    }

    NicRespEvent( Type _type, void* _key ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        key( _key )
    {
    }

    NotSerializable(NicRespEvent)
};
