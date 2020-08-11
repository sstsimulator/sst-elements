
#ifndef _H_SERRANO_PRINT_UNIT
#define _H_SERRANO_PRINT_UNIT

#include <sst/core/output.h>
#include "sercgunit.h"

namespace SST {
namespace Serrano {

class SerranoPrinterUnit : public SerranoCoarseUnit {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		SST::Serrano::SerranoPrinterUnit,
		"serrano",
		"SerranoPrinterUnit",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Performs printing of a value",
		SST::Serrano::SerranoCoarseUnit )

	SST_ELI_DOCUMENT_PARAMS(

		)
	SST_ELI_DOCUMENT_STATISTICS()

	SerranoPrinterUnit( SST::ComponentId_t id, SST::Params& params ) :
		SerranoCoarseUnit(id, params) {

		const int param_d_type = params.find<int>("data_type", 0);

		switch( param_d_type ) {
		case 1: d_type = TYPE_INT32; break;
		case 2: d_type = TYPE_INT64; break;
		case 4: d_type = TYPE_FP32; break;
		case 8: d_type = TYPE_FP64; break;
		}

	}

	virtual bool stillProcessing() {
		return false;
	}

	virtual void execute( const uint64_t current_cycle ) {
		print();
	}

	virtual void checkRequiredQueues( SST::Output* output ) {
		if( 0 == input_qs.size() ) {
			output->fatal(CALL_INFO, -1, "Error - not enough input queues for a printer unit.\n");
		}
	}

	virtual const char* getUnitTypeString() {
		return "PRINTER";
	}

protected:
	SerranoStandardType d_type;

	void print() {
		if(! input_qs[0]->empty() ) {
			SerranoMessage* msg = input_qs[0]->pop();

			switch(d_type) {
			case TYPE_INT32:
				output->verbose(CALL_INFO, 0, 0, "%" PRId32 "\n", extractValue<int32_t>(output, msg) ); break;
			case TYPE_INT64:
				output->verbose(CALL_INFO, 0, 0, "%" PRId64 "\n", extractValue<int64_t>(output, msg) ); break;
			case TYPE_FP32:
				output->verbose(CALL_INFO, 0, 0, "%f\n", extractValue<float>(output, msg) ); break;
			case TYPE_FP64:
				output->verbose(CALL_INFO, 0, 0, "%f\n", extractValue<double>(output, msg) ); break;
			default:
				output->fatal(CALL_INFO, -1, "Unknown data type.\n");
				break;
			}

			delete msg;
		}
	}

};

}
}

#endif
