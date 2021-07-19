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


#ifndef _H_SST_PROSPERO_TEXT_READER
#define _H_SST_PROSPERO_TEXT_READER

#include "prosreader.h"

using namespace SST::Prospero;

namespace SST {
namespace Prospero {

class ProsperoTextTraceReader : public ProsperoTraceReader {

public:
        ProsperoTextTraceReader( ComponentId_t id, Params& params, Output* out );
        ~ProsperoTextTraceReader();
        ProsperoTraceEntry* readNextEntry();

	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
               	ProsperoTextTraceReader,
               	"prospero",
               	"ProsperoTextTraceReader",
               	SST_ELI_ELEMENT_VERSION(1,0,0),
               	"Text Trace Reader",
	       	SST::Prospero::ProsperoTraceReader
	)

       	SST_ELI_DOCUMENT_PARAMS(
               	{ "file", "Sets the file for the trace reader to use", "" }
       	)

private:
	FILE* traceInput;

};

}
}

#endif
