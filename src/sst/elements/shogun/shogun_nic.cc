
#include <sst_config.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "shogun_event.h"
#include "shogun_credit_event.h"
#include "shogun_init_event.h"
#include "shogun_nic.h"

using namespace SST::Shogun;

ShogunNIC::ShogunNIC( SST::Component* component, Params &params ) :
		SimpleNetwork( component ), netID(-1) {

		//TODO: output = new ...
		output = new SST::Output("ShogunNIC-Startup ", 16, 0, Output::STDOUT);
		reqQ = nullptr;
		remote_input_slots = -1;
}

ShogunNIC::~ShogunNIC() {
	delete output;
}

bool ShogunNIC::initialize(const std::string &portName, const UnitAlgebra& link_bw,
                            int vns, const UnitAlgebra& in_buf_size,
                            const UnitAlgebra& out_buf_size) {

	output->verbose(CALL_INFO, 4, 0, "Configuring port %s...\n", portName.c_str());

	link = configureLink( portName, "1ps",
		new Event::Handler<ShogunNIC>(this, &ShogunNIC::recvLinkEvent) );

	output->verbose(CALL_INFO, 4, 0, "-> result: %s\n", (nullptr == link) ? "null, not-configured" : "configure successful");
	return ( nullptr != link );
}

void ShogunNIC::sendInitData(SimpleNetwork::Request *req) {
	output->verbose(CALL_INFO, 8, 0, "Send init-data called.\n");

	ShogunEvent* ev = new ShogunEvent();
	ev->setSource( netID );
	ev->setPayload( req );

	link->sendInitData( ev );

	output->verbose(CALL_INFO, 8, 0, "Send init-data completed.\n");
}

SimpleNetwork::Request* ShogunNIC::recvInitData() {
	output->verbose(CALL_INFO, 8, 0, "Recv init-data on net: %5d init-events have %5d events.\n", netID, initReqs.size());

	if( ! initReqs.empty() ) {
		SimpleNetwork::Request* req = initReqs.front();
		initReqs.erase( initReqs.begin() );

		output->verbose(CALL_INFO, 8, 0, "-> returning event to caller.\n");
		return req;
	} else {
		return nullptr;
	}
}

bool ShogunNIC::send(SimpleNetwork::Request *req, int vn) {
	if( netID > -1 ) {
		output->verbose(CALL_INFO, 8, 0, "Send: remote-slots: %5d\n", remote_input_slots);

		if( remote_input_slots > 0 ) {
			ShogunEvent* newEv = new ShogunEvent( req->dest, netID );
			newEv->setPayload( req );

			link->send(newEv);

			if( nullptr != onSendFunctor ) {
				if ( (*onSendFunctor)( vn ) ) {
					onSendFunctor = nullptr;
				}
			}

			remote_input_slots--;
			output->verbose(CALL_INFO, 8, 0, "-> sent, remote slots now %5d\n", remote_input_slots);

			return true;
		} else {
			output->verbose(CALL_INFO, 8, 0, "-> called send but no free remote slots, so cannot send request\n");
			return false;
		}
	} else {
		output->fatal(CALL_INFO, -1, "Error: send operation was called but the network is not configured yet (netID still equals -1)\n");
	}
}

SimpleNetwork::Request* ShogunNIC::recv(int vn) {
	if( netID > -1 ) {
		output->verbose(CALL_INFO, 8, 0, "Recv called, pending local entries: %d\n", reqQ->count());

		if( ! reqQ->empty() ) {
			output->verbose(CALL_INFO, 8, 0, "-> Popping request from local entries queue.\n");
			SimpleNetwork::Request* req = reqQ->pop();

			if( nullptr != req ) {
				output->verbose(CALL_INFO, 8, 0, "-> request src: %d\n", req->src);
			}

			link->send( new ShogunCreditEvent() );
			return req;
		} else {
			output->verbose(CALL_INFO, 8, 0, "-> request-q empty, nothing to receive\n");
			return nullptr;
		}
	} else {
		output->fatal(CALL_INFO, -1, "Error: recv was called but the network is not configured yet, netID is still -1\n");
		return nullptr;
	}
}

void ShogunNIC::setup() {}
void ShogunNIC::init(unsigned int phase) {
//	if( 0 == phase ) {
//		// Let the crossbar know how many slots we have in our incoming queue
//		link->sendInitData( new ShogunInitEvent( -1, -1, -1 ) );
//	}

	SST::Event* ev = link->recvInitData();

	printf("init phase %u on net-id: %5d\n", phase, netID);

	while( nullptr != ev ) {
		ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>(ev);

		if( nullptr != initEv ) {
			reconfigureNIC( initEv );
			delete ev;
		} else {
			ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>(ev);

			if( nullptr == creditEv ) {
				ShogunEvent* shogunEvent = dynamic_cast<ShogunEvent*>(ev);

				if( nullptr == shogunEvent) {
					fprintf(stderr, "ERROR: UNKNOWN EVENT TYPE AT INIT IN NIC\n");
				} else {
					initReqs.push_back( shogunEvent->getPayload() );
				}
			} else {
				delete ev;
			}
		}

		ev = link->recvInitData();
	}
}

void ShogunNIC::complete(unsigned int UNUSED(phase)) {}
void ShogunNIC::finish() {}

bool ShogunNIC::spaceToSend(int vn, int num_bits) {
	return (remote_input_slots > 0);
}

bool ShogunNIC::requestToReceive( int vn ) {
	return ! ( reqQ->empty() );
}

void ShogunNIC::setNotifyOnReceive(SimpleNetwork::HandlerBase* functor) {
	output->verbose(CALL_INFO, 4, 0, "Set recv-notify functor.\n");
	onRecvFunctor = functor;
}

void ShogunNIC::setNotifyOnSend(SimpleNetwork::HandlerBase* functor) {
	output->verbose(CALL_INFO, 4, 0, "Set send-notify functor\n");
	onSendFunctor = functor;
}

bool ShogunNIC::isNetworkInitialized() const {
	const bool netReady = (netID > -1);

	if( netReady ) {
		output->verbose(CALL_INFO, 16, 0, "network-config: ready\n");
	} else {
		output->verbose(CALL_INFO, 16, 0, "network-config: not-ready\n");
	}

	return netReady;
}

SimpleNetwork::nid_t ShogunNIC::getEndpointID() const {
	return netID;
}

const UnitAlgebra& ShogunNIC::getLinkBW() const {
	UnitAlgebra ag("1GB/s");
	return ag;
}

void ShogunNIC::recvLinkEvent( SST::Event* ev ) {
		ShogunEvent* inEv = dynamic_cast<ShogunEvent*>( ev );

		if( nullptr != inEv ) {
			output->verbose(CALL_INFO, 8, 0, "Recv link in handler: current pending request count: %5d\n", reqQ->count());

			if( reqQ->full() ) {
				output->fatal(CALL_INFO, -1, "Error - received a message but the NIC queues are full.\n");
				exit(-1);
			}

			reqQ->push( inEv->getPayload() );

			if( nullptr != onRecvFunctor ) {
				if( ! (*onRecvFunctor)(0) ) {
 					onRecvFunctor = nullptr;
				}
			}
		} else {
			ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>( ev );

			if( nullptr != creditEv ) {
				remote_input_slots++;
				output->verbose(CALL_INFO, 8, 0, "Recv link credit event, remote_input_slots now set to: %5d\n", remote_input_slots);
			} else {
				ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>( ev );

				if( nullptr != initEv ) {
					reconfigureNIC( initEv );
				} else {
					output->fatal(CALL_INFO, -1, "Unknown event type received. Not sure how to process.\n");
				}
			}

			delete ev;
		}
}

void ShogunNIC::reconfigureNIC( ShogunInitEvent* initEv ) {
	remote_input_slots = initEv->getQueueSlotCount();

        if( nullptr != reqQ ) {
	        delete reqQ;
        }

        reqQ = new ShogunQueue< SimpleNetwork::Request* >( initEv->getQueueSlotCount() );

        netID = initEv->getNetworkID();
        port_count = initEv->getPortCount();

	output->verbose(CALL_INFO, 4, 0, "Shogun NIC reconfig (%s) with %5d slots, xbar-total-ports: %5d\n",
		getName().c_str(), remote_input_slots, port_count);

	if( nullptr != output && netID > -1 ) {
		const uint32_t currentVerbosity = output->getVerboseLevel();
		delete output;
		char outPrefix[256];

		sprintf(outPrefix, "[t=@t][NIC%5d][%25s][%5d]: ", netID, getName().c_str(), netID);
		output = new SST::Output(outPrefix, currentVerbosity, 0, SST::Output::STDOUT);
	}

}
