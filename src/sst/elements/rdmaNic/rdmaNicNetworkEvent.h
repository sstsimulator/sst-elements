// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class RdmaNicNetworkEvent : public Event {

    size_t calcOverHead() {
        return 4 + // streamId
                4 + // sequence number
                4 + // dest pid
                4 + // src pid
                4; // src node
    }

  public:

    typedef int StreamId;

	enum PktType { Stream, Barrier };
    RdmaNicNetworkEvent( PktType type = Stream ) : Event(), pktType(type), pktOverhead(calcOverHead()) {}
    RdmaNicNetworkEvent(const RdmaNicNetworkEvent* me) : Event() { copy( *this, *me ); }
    RdmaNicNetworkEvent(const RdmaNicNetworkEvent& me) : Event() { copy( *this, me ); }

    int getPayloadSizeBytes() { return pktOverhead + bufSize(); }

    virtual Event* clone(void) override {
        return new RdmaNicNetworkEvent(*this);
    }

    void setStreamId( StreamId val ) { streamId = val; }
    void setStreamSeqNum( int val ) { streamSeq = val; }
    void setDestPid( int val ) { destPid = val; }
    void setSrcPid( int val ) { srcPid = val; }
    void setSrcNode( int val ) { srcNode = val; }

    StreamId getStreamId() { return streamId; }
    int getStreamSeqNum() { return streamSeq; }
    int getSrcNode( ) { return srcNode; }
    int getDestPid() { return destPid; }
    int getSrcPid() { return srcPid; }
	PktType getPktType() { return pktType; }
    std::vector<uint8_t>& getData() { return buf; }

  private:

    void copy( RdmaNicNetworkEvent& to, const RdmaNicNetworkEvent& from ) {
        to.pktOverhead = from.pktOverhead;
        to.streamId = from.streamId;
        to.streamSeq = from.streamSeq;
        to.destPid = from.destPid;
        to.srcPid = from.srcPid;
        to.srcNode = from.srcNode;
        to.buf = from.buf;
        to.pktType = from.pktType;
    }

    int bufSize() { return buf.size(); }

    int pktOverhead;

    StreamId streamId;
    int streamSeq;
    int destPid;
    int srcPid;
    int srcNode;
 	PktType pktType;

    std::vector<uint8_t> buf;

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & pktOverhead;
        ser & streamId;
        ser & streamSeq;
        ser & destPid;
        ser & srcPid;
        ser & srcNode;
        ser & buf;
		ser & pktType;
    }
    ImplementSerializable(SST::MemHierarchy::RdmaNicNetworkEvent);
};

