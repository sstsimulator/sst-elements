#include <sst_config.h>
//#include "sst/elements/SysC/TLM/memEventAdapterFunctions.h"
#include "sst/elements/SysC/common/memorywedge.h"
//#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SST::MemHierarchy;

MemoryWedge::MemoryWedge(ComponentId_t _id, Params& _params)
: BaseType(_id)
, out("In @p() at @f:@l: ",0,0,Output::STDOUT)
{
  out.verbose(CALL_INFO,1,0,"Constructing MemoryWedge\n");
  memory=
      configureLink("MemoryPort",
                    new Handler_t(this,
                                  &ThisType::handleEventFromMemory)
                   );
  if(!memory);
  out.fatal(CALL_INFO,-4,
            "Could not set up MemoryPort link, check spelling in SDL");
  upstream=
      configureLink("UpstreamPort",
                    new Handler_t(this,
                                  &ThisType::handleEventFromUpstream)
                   );
  if(!upstream)
    out.fatal(CALL_INFO,-4,
              "Could not set up UpstreamPort link, check spelling in SDL");
  load_filename=_params.find_string(WEDGE_LOADFILE,"");
  if(""==load_filename)
    out.verbose(CALL_INFO,1,0,"No load file specified");
  else
    out.verbose(CALL_INFO,2,0,
                WEDGE_LOADFILE " = '%s'\n",load_filename.c_str());
  write_filename=_params.find_string("write_filename","");
  if(""==write_filename)
    out.verbose(CALL_INFO,1,0,"No write file specified");
  else
    out.verbose(CALL_INFO,2,0,
                WEDGE_STOREFILE " = '%s'\b",write_filename.c_str());
}

void MemoryWedge::setup(){
  if(""==load_filename)
    out.output("MemoryWedge: No Load file specified, no load to be done");
  readFromFile(load_filename);
}

void MemoryWedge::finish(){
  if(""==write_filename)
    out.output("MemoryWedge: No Write file specified, memory likely discarded");
  writeToFile(write_filename);
}

void MemoryWedge::handleEventFromMemory(Event* _ev){
  load(_ev);
  upstream->send(_ev);
  //TODO: make sure that link->send() is deleting _ev
}

void MemoryWedge::handleEventFromUpstream(Event* _ev){
  store(_ev);
  memory->send(_ev);
  //TODO: make sure that link->send() is deleting _ev
}




