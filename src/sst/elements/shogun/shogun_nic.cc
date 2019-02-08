
#include <sst/core/interfaces/simpleNetwork.h>

#include "shogun_event.h"
#include "shogun_credit_event.h"
#include "shogun_init_event.h"
#include "shogun_nic.h"

using namespace SST::Shogun;

ShogunNIC::ShogunNIC( SST::Component* component ) :
		SimpleNetwork( component ), netID(-1) {

		//TODO: output = new ...
		reqQ = nullptr;
		remote_input_slots = -1;
}

bool ShogunNIC::initialize(const std::string &portName, const UnitAlgebra& link_bw,
                            int vns, const UnitAlgebra& in_buf_size,
                            const UnitAlgebra& out_buf_size) {

		link = configureLink( portName, new Event::Handler<ShogunNIC>(this, &ShogunNIC::recvLinkEvent) );
//		link->sendUntimedData( new ShogunInitEvent() );

		return ( nullptr != link );
}

void ShogunNIC::sendInitData(SimpleNetwork::Request *req) {

}

SimpleNetwork::Request* ShogunNIC::recvInitData() {

}

bool ShogunNIC::send(SimpleNetwork::Request *req, int vn) {
	if( netID > -1 ) {
		if( remote_input_slots > 0 ) {
			ShogunEvent* newEv = new ShogunEvent( req->dest, netID );
			newEv->setPayload( req->takePayload() );

			link->send( newEv );

			(*onSendFunctor)( 0 );
			remote_input_slots--;
		} else {
			fprintf(stderr, "called send but no free slots on remote send side.\n");
		}
	} else {
		fprintf(stderr, "network not init yet. STOP.\n");
		exit(-1);
	}
}

SimpleNetwork::Request* ShogunNIC::recv(int vn) {
	if( netID == -1 ) {
		fprintf(stderr, "ERROR - STOP. in recv, network not init yet.\n");
		exit(-1);
	}

	if( reqQ->empty() ) {
		return nullptr;
	} else {
		SimpleNetwork::Request* req = reqQ->pop();
		link->send( new ShogunCreditEvent() );
		return req;
	}
    }

void ShogunNIC::setup() {}
void ShogunNIC::init(unsigned int phase) {
	if( 0 == phase ) {
		// Let the crossbar know how many slots we have in our incoming queue
		link->send( new ShogunInitEvent( -1, -1, reqQ->capacity() ) );
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
	onRecvFunctor = functor;
}

void ShogunNIC::setNotifyOnSend(SimpleNetwork::HandlerBase* functor) {
	onSendFunctor = functor;
}

bool ShogunNIC::isNetworkInitialized() const {
	return ( netID > -1 );
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
			SimpleNetwork::Request* req = new SimpleNetwork::Request();
			req->dest = inEv->getDestination();
			req->src  = inEv->getSource();
			req->givePayload( inEv->getPayload() );

			if( reqQ->full() ) {
				fprintf(stderr, "ERROR: QUEUE IS FULL\n");
				exit(-1);
			}

			reqQ->push(req);
		} else {
			ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>( ev );

			if( nullptr != creditEv ) {
				remote_input_slots++;
			} else {

				ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>( ev );

				if( nullptr != initEv ) {
					remote_input_slots = initEv->getQueueSlotCount();
					netID = initEv->getNetworkID();
					port_count = initEv->getPortCount();

					printf("Shogun: network-id: %5d configured with %5d slots total-ports: %5d\n", netID, remote_input_slots, port_count );
				} else {
					fprintf(stderr, "UNKNOWN EVENT TYPE RECV\n");
					exit(-1);
				}
			}
		}
}
