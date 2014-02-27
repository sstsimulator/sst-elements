/*
 * File:   coherenceControllers.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <vector>
#include "coherenceControllers.h"
using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


/*---------------------------------------------------------------------------------------------------
 * Coherence Controller - Helper Functions
 *--------------------------------------------------------------------------------------------------*/

inline bool CoherencyController::sendResponse(MemEvent *_event, BCC_MESIState _newState, std::vector<uint8_t>* _data, int _childId){
    if(_event->isPrefetch() || _childId == -1){
         d_->debug(_WARNING_,"Warning: No Response sent! Thi event is a prefetch or childId in -1");
        return true;
    }
    MemEvent *responseEvent; Command cmd = _event->getCmd();
    Addr offset, base;
    switch(cmd){
        case GetS: case GetSEx:
            assert(_data);
            if(L1_){
                base = (_event->getAddr()) & ~(lineSize_ - 1);
                offset = _event->getAddr() - base;
                responseEvent = _event->makeResponse((SST::Component*)owner_);
                responseEvent->setPayload(_event->getSize(), &_data->at(offset));
            }
            else responseEvent = _event->makeResponse((SST::Component*)owner_, *_data, _newState);
        break;
        case GetX:
            if(L1_){
                base = (_event->getAddr()) & ~(lineSize_ - 1);
                offset = _event->getAddr() - base;
                responseEvent = _event->makeResponse((SST::Component*)owner_);
                responseEvent->setPayload(_event->getSize(), &_data->at(offset));
            }
            else    responseEvent = _event->makeResponse((SST::Component*)owner_, *_data, _newState); //TODO: make sure to look at "Given State" before setting new state at receivingn end
            break;
        default:
            _abort(CoherencyController, "Command not valid as a response. \n");
    }
    if(L1_ && (cmd == GetS || cmd == GetSEx)) printData(d_, "Response Data", _data, offset, (int)_event->getSize());
    else printData(d_, "Response Data", _data);
    d_->debug(_L1_,"Sending Response:  Addr = %#016llx,  Dst = %s, Size = %i, Granted State = %s\n", (uint64_t)_event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
    Link* deliveryLink = _event->getDeliveryLink();
    uint64_t deliveryTime = _event->queryFlag(MemEvent::F_UNCACHED) ? timestamp_ : timestamp_ + accessLatency_;
    response resp = {deliveryLink, responseEvent, deliveryTime, true};
    outgoingEventQueue_.push(resp);
    return true;
}


inline bool CoherencyController::sendAckResponse(MemEvent *_event){
    Link* deliveryLink = _event->getDeliveryLink();
    MemEvent *responseEvent;
    Command cmd = _event->getCmd();
    if (cmd == Inv || cmd == InvX || cmd == PutM ||  cmd == PutS){
        responseEvent = _event->makeResponse((SST::Component*)owner_);
    }
    d_->debug(_L1_,"Sending Ack Response:  Addr = %#016llx, Cmd = %s \n", (uint64_t)responseEvent->getAddr(), CommandString[responseEvent->getCmd()]);
    response resp = {deliveryLink, responseEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
    return true;
}

inline void CoherencyController::sendCommand(Command cmd, CacheLine* cacheLine, Link* deliveryLink){
    d_->debug(_L1_,"Sending Command:  Cmd = %s\n", CommandString[cmd]);
    vector<uint8_t>* data = cacheLine->getData();
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(),cacheLine->getBaseAddr(), cacheLine->getLineSize(), cmd, *data);
    response resp = {deliveryLink, newCommandEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
}

