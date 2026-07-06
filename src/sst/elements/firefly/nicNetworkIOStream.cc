// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

// ========================================================================
// Network Storage Stream Constructor
// ========================================================================
// Handles incoming network storage packets on storage nodes
// Two packet types:
//   1. Storage operations (READ/WRITE) - perform DMA and send ACK
//   2. ACK responses - match respKey and invoke stored callback
Nic::RecvMachine::NetworkIOStream::NetworkIOStream( Output& output, Ctx* ctx,
            int srcNode, int srcPid, int destPid, FireflyNetworkEvent* ev ) :
    StreamBase( output, ctx, srcNode, srcPid, destPid ),
    m_offset(0),
    m_length(0)
{
    // Pop the MsgHdr to get to packet payload
    ev->bufPop(sizeof(MsgHdr));
    
    unsigned char* bufPtr = (unsigned char*)ev->bufPtr();
    
    // Peek at first byte to determine packet type
    // ACK packets are small: only [isAck=true][respKey]
    bool isAck = false;
    if (ev->bufSize() == sizeof(bool) + sizeof(uint32_t)) {
        memcpy(&isAck, bufPtr, sizeof(isAck));
        
        if (isAck) {
            // Handle ACK packet (requester side)
            processAck(ev, bufPtr);
            delete ev;
            setDone();
            return;
        }
    }
    
    // Handle storage operation packet (storage node side)
    processStorageOp(ev, bufPtr);
}

// ========================================================================
// Process ACK Packet
// ========================================================================
// Matches respKey to invoke stored callback from m_respKeyMap
void Nic::RecvMachine::NetworkIOStream::processAck(FireflyNetworkEvent* ev, unsigned char* bufPtr)
{
    bufPtr += sizeof(bool);
    ev->bufPop(sizeof(bool));
    
    // Extract response key from ACK packet
    uint32_t respKey = 0;
    memcpy(&respKey, bufPtr, sizeof(respKey));
    ev->bufPop(sizeof(respKey));
    
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                "ACK received: respKey=%u\n", respKey);
    
    // Retrieve and invoke stored callback
    // getRespKeyValue() removes entry from m_respKeyMap
    auto* respCallback = static_cast<std::function<void()>*>(
        m_ctx->nic().getRespKeyValue(respKey));
    if (respCallback) {
        (*respCallback)();  // Invoke user callback
        delete respCallback;
    }
}

// ========================================================================
// Process Storage Operation (Storage Node Side)
// ========================================================================
// Handles READ or WRITE operations sent to storage node
// 
// Packet format:
//   [NetworkMsgHdr][offset][src][length][respKey]
//
// Flow:
//   1. Extract operation parameters
//   2. Submit DMA: READ=BusDmaFromHost, WRITE=BusDmaToHost
//   3. Send ACK with respKey when DMA completes
void Nic::RecvMachine::NetworkIOStream::processStorageOp(FireflyNetworkEvent* ev, unsigned char* bufPtr)
{
    // Extract NetworkIOMsgHdr to determine operation type
    Nic::NetworkIOMsgHdr netHdr;
    memcpy(&netHdr, bufPtr, sizeof(netHdr));
    bufPtr += sizeof(netHdr);
    ev->bufPop(sizeof(netHdr));
    
    // Extract operation parameters from packet
    memcpy(&m_offset, bufPtr, sizeof(m_offset));
    bufPtr += sizeof(m_offset);
    memcpy(&m_src, bufPtr, sizeof(m_src));
    bufPtr += sizeof(m_src);
    memcpy(&m_length, bufPtr, sizeof(m_length));
    bufPtr += sizeof(m_length);
    ev->bufPop(sizeof(m_offset) + sizeof(m_src) + sizeof(m_length));
    
    // Extract respKey (always present - ACKs always sent)
    uint32_t respKey;
    memcpy(&respKey, bufPtr, sizeof(respKey));
    ev->bufPop(sizeof(respKey));
    
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                "op=%u offset=%lu length=%lu respKey=%u\n",
                netHdr.op, m_offset, m_length, respKey);
    
    // Submit DMA operation based on operation type
    std::vector<MemOp>* vec = new std::vector<MemOp>;
    
    if (netHdr.op == Nic::NetworkIOMsgHdr::Read) {
        // READ: BusDmaFromHost (load data FROM storage to send to requester)
        vec->push_back(MemOp(m_offset, m_length, MemOp::Op::BusDmaFromHost));
    } else {
        // WRITE: BusDmaToHost (store data TO storage)
        vec->push_back(MemOp(m_offset, m_length, MemOp::Op::BusDmaToHost));
    }
    
    // Use dmaRead() for READ, dmaWrite() for WRITE to select correct DMA engine
    int unit = m_ctx->nic().allocNicRecvUnit(m_myPid);
    
    if (netHdr.op == Nic::NetworkIOMsgHdr::Read) {
        // READ uses dmaRead() -> DetailedCompute[0]
        auto callback = [=]() {
         m_dbg.debug(CALL_INFO_LAMBDA,"NetworkIOStream",1,NIC_DBG_RECV_STREAM,
                            "DMA READ complete, sending ACK respKey=%u to node=%d\n", 
                            respKey, m_srcNode);
                
                // Create ACK packet with matching respKey
                NetworkIOAckSendEntry* ackEntry = new NetworkIOAckSendEntry(
                    m_myPid, m_srcNode, respKey
                );
                m_ctx->nic().qSendEntry(ackEntry);
                
                delete ev;
                setDone();
    };
        if(m_ctx->nic().getSimpleSSDPtr()!=NULL){
            std::cout<<"Current node id : "<<m_ctx->nic().getNodeId()<<std::endl;
            SimpleSSD* m_ssd = m_ctx->nic().getSimpleSSDPtr();
            assert(m_ssd);

            std::cout<<"SSD Read for node Id : "<<m_ctx->nic().getNodeId()<<std::endl;
            // Start SSD read operation with callback
            m_ssd->read(0, m_length, callback);
        }
        else{
            std::cout<<"DMA Read for node Id : "<<m_ctx->nic().getNodeId()<<std::endl;
            // Submit DMA operation with completion callback
            m_ctx->nic().dmaRead(unit, m_myPid, vec,callback);
        }
    }
    else {
        // WRITE uses dmaWrite() -> DetailedCompute[1]
        auto writeCallback = [=]() {
            m_dbg.debug(CALL_INFO_LAMBDA,"NetworkIOStream",1,NIC_DBG_RECV_STREAM,
                        "DMA WRITE complete, sending ACK respKey=%u to node=%d\n", 
                        respKey, m_srcNode);
            
            // Create ACK packet with matching respKey
            NetworkIOAckSendEntry* ackEntry = new NetworkIOAckSendEntry(
                m_myPid, m_srcNode, respKey
            );
            m_ctx->nic().qSendEntry(ackEntry);
            
            delete ev;
            setDone();
        };
        
        if(m_ctx->nic().getSimpleSSDPtr()!=NULL){
            std::cout<<"Current node id : "<<m_ctx->nic().getNodeId()<<std::endl;
            SimpleSSD* m_ssd = m_ctx->nic().getSimpleSSDPtr();
            assert(m_ssd);

            std::cout<<"SSD Write for node Id : "<<m_ctx->nic().getNodeId()<<std::endl;
            // Start SSD write operation with callback
            m_ssd->write(0, m_length, writeCallback);
        }
        else{
            std::cout<<"DMA Write for node Id : "<<m_ctx->nic().getNodeId()<<std::endl;
            // Submit DMA operation with completion callback
            m_ctx->nic().dmaWrite(unit, m_myPid, vec, writeCallback);
        }
    }
}
