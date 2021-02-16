// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SERRANO_MESSAGE
#define _H_SERRANO_MESSAGE

#include <cstdint>
#include <cinttypes>

namespace SST {
namespace Serrano {

class SerranoMessage {

public:
	SerranoMessage( const size_t size ) : msg_size(size) {
		payload = new uint8_t[ msg_size ];
	}

	SerranoMessage( const size_t size, void* ptr ) : msg_size(size) {
		payload = new uint8_t[ msg_size ];

		uint8_t* ptr_u = (uint8_t*) ptr;

		for( size_t i = 0; i < msg_size; ++i ) {
			payload[i] = ptr_u[i];
		}
	}

	~SerranoMessage() {
		delete[] payload;
	}

	size_t getSize() const { return msg_size; }
	uint8_t* getPayload() { return payload; }

	void setPayload( const uint8_t* new_data ) {
		for( size_t i = 0; i < msg_size; ++i ) {
			payload[i] = new_data[i];
		}
	}

	void setPayload( const uint8_t* new_data, const size_t new_size ) {
		for( size_t i = 0; i < std::min( new_size, msg_size); ++i ) {
			payload[i] = new_data[i];
		}
	}

protected:
	const size_t msg_size;
	uint8_t* payload;

};

template<class T> SerranoMessage* constructMessage( T value ) {
	SerranoMessage* new_msg = new SerranoMessage( sizeof(T) );
	new_msg->setPayload( (uint8_t*) &value );

	return new_msg;
};

template<class T> T extractValue( SST::Output* output, SerranoMessage* msg ) {
	if( sizeof(T) == msg->getSize() ) {
		return *( (T*) msg->getPayload() );
	} else {
		output->fatal(CALL_INFO, -1, "Error: tried to construct a value needing %d bytes from a message with %d bytes in payload.\n",
			(int) sizeof(T), (int) msg->getSize());

		return T();
	}
};

}
}

#endif
