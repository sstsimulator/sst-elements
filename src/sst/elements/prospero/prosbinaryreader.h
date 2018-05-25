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


#ifndef _H_SST_PROSPERO_BINARY_READER
#define _H_SST_PROSPERO_BINARY_READER

#include "prosreader.h"

namespace SST {
namespace Prospero {

class ProsperoBinaryTraceReader : public ProsperoTraceReader {

public:
        ProsperoBinaryTraceReader( Component* owner, Params& params );
        ~ProsperoBinaryTraceReader();
        ProsperoTraceEntry* readNextEntry();

 	SST_ELI_REGISTER_SUBCOMPONENT(
        	ProsperoBinaryTraceReader,
        	"prospero",
        	"ProsperoBinaryTraceReader",
        	SST_ELI_ELEMENT_VERSION(1,0,0),
        	"Binary Trace Reader",
        	"SST::Prospero::ProsperoTraceReader"
    	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "file", "Sets the file for the trace reader to use", "" }
	)

private:
	void copy(char* target, const char* source,
		const size_t offset, const size_t len);
	FILE* traceInput;
	char* buffer;
	uint32_t recordLength;

};

}
}

#endif
