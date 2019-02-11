
#include <sst_config.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "shogun_event.h"
#include "shogun_credit_event.h"
#include "shogun_init_event.h"
#include "shogun_nic.h"

using namespace SST::Shogun;

ShogunNIC::ShogunNIC( SST::Component* component, Params &params ) :
		SimpleNetwork( component ), netID(-1) {

		//TODO: output = new ...
		reqQ = nullptr;
		remote_input_slots = -1;
}

bool ShogunNIC::initialize(const std::string &portName, const UnitAlgebra& link_bw,
                            int vns, const UnitAlgebra& in_buf_size,
                            const UnitAlgebra& out_buf_size) {

		link = configureLink( portName, "1ps", 
			new Event::Handler<ShogunNIC>(this, &ShogunNIC::recvLinkEvent) );
//		link->sendUntimedData( new ShogunInitEvent() );

		return ( nullptr != link );
}

void ShogunNIC::sendInitData(SimpleNetwork::Request *req) {
	ShogunEvent* ev = new ShogunEvent();
	ev->setSource( netID );
	ev->setPayload( req );

	link->sendInitData( ev );
}

SimpleNetwork::Request* ShogunNIC::recvInitData() {
	printf("recvInitData on netid %5d initEvents has %d events\n", netID, initReqs.size());

	if( ! initReqs.empty() ) {
		SimpleNetwork::Request* req = initReqs.front();
		initReqs.erase( initReqs.begin() );
		return req;
	} else {
		return nullptr;
	}
}

bool ShogunNIC::send(SimpleNetwork::Request *req, int vn) {
	if( netID > -1 ) {
		printf("send, netID=%d remote slot count: %d\n", netID, remote_input_slots);

		if( remote_input_slots > 0 ) {
			ShogunEvent* newEv = new ShogunEvent( req->dest, netID );
			req->src = netID;
			newEv->setPayload( req );

			printf("sending now...\n");
			//TimeConverter* tc = new TimeConverter(0);
			//link->send( 0, tc, newEv );
			link->send(newEv);

			if( nullptr != onSendFunctor ) {
				(*onSendFunctor)( 0 );
			}

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
		printf("net-id: %d recv event from the network\n", netID);
		SimpleNetwork::Request* req = reqQ->pop();
		link->send( new ShogunCreditEvent() );
		return req;
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
	onRecvFunctor = functor;
}

void ShogunNIC::setNotifyOnSend(SimpleNetwork::HandlerBase* functor) {
	onSendFunctor = functor;
}

bool ShogunNIC::isNetworkInitialized() const {
	printf("Is network ready? I am id %5d\n", netID );
	const bool netReady = (netID > -1);

	if( netReady ) {
		printf("network is ready!\n");
	} else {
		printf("network is NOT ready\n");
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
			printf("recvLinkEvent, netID=%d, q_count=%d\n", netID, reqQ->count());

			if( reqQ->full() ) {
				fprintf(stderr, "ERROR: QUEUE IS FULL\n");
				exit(-1);
			}

			printf("pushing payload...\n");
			reqQ->push( inEv->getPayload() );
		} else {
			ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>( ev );

			if( nullptr != creditEv ) {
				remote_input_slots++;
			} else {

				ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>( ev );

				if( nullptr != initEv ) {
					reconfigureNIC( initEv );
				} else {
					fprintf(stderr, "UNKNOWN EVENT TYPE RECV\n");
					exit(-1);
				}
			}
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
	printf("Shogun: network-id: %5d configured with %5d slots total-ports: %5d\n", netID, remote_input_slots, port_count );

}
