#include "sst/elements/SysC/common/TLMgenerator.h"
#include "sst/elements/SysC/common/TLManouncer.h"

struct Top: public sc_module{
  TLMGenerator* generator;
  TLMAnouncer*  anouncer;

  SC_CTOR(Top){
      generator=new TLMGenerator("generator");
      anouncer=new TLMAnouncer("anouncer");
      generator->socket.bind(anouncer->socket);
  }
  
  void printData(){
  generator->printData();
  anouncer->printData();
  }

};

int sc_main(int argc, char* argv[])
{
  Top top("top");
  sc_start();
  top.printData();
  return 0;
}
