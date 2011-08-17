

#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "omnetEvent.h"
#include "common/fileutil.h"
#include "envir/inifilereader.h"
#include "omnetconfiguration.h"

#include "PhoenixSim.h"
#include "sst/core/element.h"
#include <sst/core/link.h>
#include <sst/core/stopAction.h>
#include <assert.h>
#include <string>

using namespace SST;
using namespace std;

Register_GlobalConfigOption(CFGID_LOAD_LIBS, "load-libs", CFG_FILENAMES, "", "A space-separated list of dynamic libraries to be loaded on startup. The libraries should be given without the `.dll' or `.so' suffix -- that will be automatically appended.");
Register_GlobalConfigOption(CFGID_CONFIGURATION_CLASS, "configuration-class", CFG_STRING, "", "Part of the Envir plugin mechanism: selects the class from which all configuration information will be obtained. This option lets you replace omnetpp.ini with some other implementation, e.g. database input. The simulation program still has to bootstrap from an omnetpp.ini (which contains the configuration-class setting). The class should implement the cConfigurationEx interface.");
Register_GlobalConfigOption(CFGID_USER_INTERFACE, "user-interface", CFG_STRING, "", "Selects the user interface to be started. Possible values are Cmdenv and Tkenv. This option is normally left empty, as it is more convenient to specify the user interface via a command-line option or the IDE's Run and Debug dialogs. New user interfaces can be defined by subclassing cRunnableEnvir.");

static void verifyIntTypes()
{
#define VERIFY(t,size) if (sizeof(t)!=size) {printf("INTERNAL ERROR: sizeof(%s)!=%d, please check typedefs in include/inttypes.h, and report this bug!\n\n", #t, size); abort();}
    VERIFY(int8,  1);
    VERIFY(int16, 2);
    VERIFY(int32, 4);
    VERIFY(int64, 8);

    VERIFY(uint8, 1);
    VERIFY(uint16,2);
    VERIFY(uint32,4);
    VERIFY(uint64,8);
#undef VERIFY

#define LL  INT64_PRINTF_FORMAT
    char buf[32];
    int64 a=1, b=2;
    sprintf(buf, "%"LL"d %"LL"d", a, b);
    if (strcmp(buf, "1 2")!=0) {printf("INTERNAL ERROR: INT64_PRINTF_FORMAT incorrectly defined in include/inttypes.h, please report this bug!\n\n"); abort();}
#undef LL
}

PhoenixSim::PhoenixSim(SST::ComponentId_t id, SST::Component::Params_t& params) : Component(id) {
  printf("constructing..\n");
  
  cStaticFlag::set(true);  
  
  //  cn = configureLink("cn",
  //		     new Event::Handler<PhoenixSim>(this, &PhoenixSim::handleEvent));  
  //  assert(cn);
  
  slink = configureSelfLink( "slink", 
		      	     new Event::Handler<PhoenixSim>(this, &PhoenixSim::handleSelfEvent) );
  assert(slink != NULL);
  
  // printf("sfsdf: %s", cn->params["module"]);
  //params.print_all_params(cout);
  
  linkModule = params["cn:module"];
  linkGate = params["cn:gate"]; 
  //  linkModuleMap["cn"] = linkModule;
  //  linkGateMap["cn"] = linkGate;
  
  //retrieve time base from config
  if( params.find("timebase") == params.end() ) {
    _abort(event_test, "no time base specified\n");
  }
  timeBase = params["timebase"];

  // register time base for this component
  TimeConverter* tc = registerTimeBase(timeBase);
  //--don't really want a clock...not sure about this -- setting for sim time resolution
  // registerClock( timeBase, new Clock::Handler<PhoenixSim>(this, &PhoenixSim::clockTic));

  timeExp = convertToExp(timeBase);
  if( timeExp < -18 || timeExp > 0 )
    _abort(event_test, "time scale exponent must be between (inclusive) -18 and 0.\n");


  setDefaultTimeBase(tc);
  slink->setDefaultTimeBase(tc); //set links time base to the same as this components' time base
  //cn->setDefaultTimeBase(tc);

  //retrieve params from config
  if( params.find("ini") == params.end() ) {
    _abort(event_test, "no .ini file specified\n"); 
  }
  ini = (params[ "ini" ]).c_str();

  if( params.find("nedsource") == params.end() ) {
    _abort(event_test, "no .ned file source folder specified\n"); 
  }
  nedSourceFolder = (params[ "nedsource" ]).c_str();

  if( params.find("network") == params.end() ) {
    _abort(event_test, "no top-level network to simulate specified\n"); 
  }
  topNetwork = (params[ "network" ]).c_str();  

  if( params.find("configuration") == params.end() ) {
    options["configuration"] = "General";
  } else {
    options["configuration"] = params["configuration"];
  }
  
  options["nedSourceFolder"] = nedSourceFolder;
  options["topNetwork"] = topNetwork;

  if( params.find("run") != params.end() ) {
    options["run"] = params["run"];
  } else {
    options["run"] = "0";
  }  

  if( params.find("mode") != params.end() ) {
    string temp = params["mode"];
    for( int i = 0; i < (int)(temp.length()); i++ ) {
      temp[i] = tolower(temp[i]);
    }
    if( temp.compare("express") == 0 ) {
      options["mode"] = "express";
    } else if( temp.compare("fast") == 0 ) {
      options["mode"] = "fast";
    } else if( temp.compare("normal") == 0 ) {
      options["mode"] = "normal";
    } else {
      printf("Unknown mode %s.  Setting to default mode \"express.\"\n", temp.c_str());
      options["mode"] = "express";
    }
	   
  } else {
    options["mode"] = "express";
  }


  //-----flags-----
  if( params.find("v") != params.end() )
    options["v"] = "true";
  if( params.find("h") != params.end())
    options["h"] = params["h"];
  if( params.find("a") != params.end())
    options["a"] = "true";
  if( params.find("x") != params.end())
    options["x"] = params["x"];
  if( params.find("g") != params.end())
    options["g"] = "true";
  if( params.find("G") != params.end())
    options["G"] = "true";

  registerExit();

  printf("Done constructing\n");
}

void PhoenixSim::sendMsg(cMessage* msg) {
  // printf("Sending message..\n");
  // - calculate delay from message arrival time
  simtime_t t = msg->getArrivalTime();
  uint64 delay = t.raw() - getCurrentSimTime();

  // - construct an OmnetEvent
  OmnetEvent* evt = new OmnetEvent();

  evt->payload = msg;
  slink->Send(delay, evt);
  
}

int PhoenixSim::Setup() {
  
  printf("Setting up...\n");

  printf("TimeScale: %d\n", timeExp);

  //Omnet Initializations
  ExecuteOnStartup::executeAll();
  SimTime::setScaleExp(timeExp); 
  verifyIntTypes();
 
  //load ini file
   InifileReader *inifile = new InifileReader();
   
  if( fileExists(ini) ) {
    inifile->readFile(ini);
  } else {
    printf("Attempted to open: %s\n", ini);
    _abort(event_test,"INI FILE DOES NOT EXIST!!\n");
  }
  
  //set up configuration
  OmnetConfiguration* config = new OmnetConfiguration();
  config->setConfigurationReader(inifile);
  //config->setCommandLineConfigOptions((new ArgList())->getLongOptions());
 
  // config->activateConfig("General", 0); //Activate in environmnet

  env = new SimEnv( 0, NULL, config, options);
  
  if( env->simulationRequired() ) {

    sim = new OmnetSimulation("simulation", env, this); 
    //    sim = new OmnetSimulation("simulation", env, this, linkModuleMap, linkGateMap );
    
    cSimulation::setActiveSimulation(sim);
    
    //prepare environment for run
    env->setup();  
  }
  printf("Done setup\n");
     
  return 0;
}

int PhoenixSim::Finish() {
  printf("Finishing simulation...\n");

  printf("Ending simulation time: %llu\n", (long long unsigned int)getCurrentSimTime("1GHz"));
  //exit OMNet simulation
  env->endRun();
  cSimulation::setActiveSimulation(NULL);
  componentTypes.clear();
  nedFunctions.clear();
  classes.clear();
  enums.clear();
  classDescriptors.clear();
  configOptions.clear();
  cSimulation::clearLoadedNedFiles();
  cStaticFlag::set(false);
  return 0;
}

PhoenixSim::~PhoenixSim() {
  
}

void PhoenixSim::handleEvent(Event *evt) {
  /*  OmnetEvent* omevent = dynamic_cast<OmnetEvent*>(evt);
  if( omevent == NULL ) 
    printf("Error in handle event!!\n");
  
  OmnetSimulation* osim = dynamic_cast<OmnetSimulation*>(&(simulation));

  osim->processMsg(omevent->payload);

  delete omevent;*/
}

void PhoenixSim::handleSelfEvent(Event *evt) {
  OmnetEvent* omevent = dynamic_cast<OmnetEvent*>(evt);

  if( ! omevent )
    printf("ERRROOORRR\n");

  // printf("sim time: %llu\n", (long long unsigned int)getCurrentSimTime("1GHz"));

  OmnetSimulation* osim = dynamic_cast<OmnetSimulation*>(&(simulation));

  osim->processSelfMsg(omevent->payload);

  delete omevent;
}

bool PhoenixSim::clockTic( Cycle_t ) { return false; }

int PhoenixSim::convertToExp(string tc) {
  double value;
  char si[2];
  char unit[3];
  char tmp[2];

  const char* tbs_char = tc.c_str();
  int ret = sscanf(tbs_char,"%lf %1s %2s %1s", &value, si, unit, tmp);
  if( ret <= 1 || ret > 3 ) {
    _abort(event_test, "Error parsing time scale: %s\n", tbs_char);
  }

  //Omnet can only handle integer scale exponents (i.e. can't have sim time with a 2.5ns resolution)
  int d = value;
  int factor = 0;
  while( d >= 10 ) {
    d = d / 10;
    factor++;
  }

  if( d != 1 )
    _abort(event_test, "Omnet requires an integer scale exponent; component clock value must be a factor of 10.\n");
  
  if( ret == 2 ) {
    if( si[0] == 's' ) 
      return 0;
    else
      _abort(event_test, "Error parsing time scale: %s\n", tbs_char);
  }
  int exp = -1;
 
  //clock period specified
  if( strcmp("s", unit) == 0 ) {
    switch(si[0]) {
    case 'f':
      // printf("femto seconds: ");
      exp = -18 + factor;
      break;
    case 'p':
      // printf("pico seconds: ");
      exp = -12 + factor;
      break;
    case 'n':
      // printf("nano seconds: ");
      exp = -9 + factor;
      break;
    case 'u':
      // printf("micro seconds: ");
      exp = -6 + factor;
      break;
    case 'm':
      // printf("milli seconds: ");
      exp = -3 + factor;
      break;
    default:
      _abort(event_test,"Base time format error (unregonized SI unit): %s\n",tbs_char);  
    }    
  }
  //clock frequency specified
  else {
    switch(si[0]) {
    case 'k':
      // printf("kilo hertz: ");
      exp = -3 - factor;
      break;
    case 'M':
      // printf("Mega hertz: ");
      exp = -6 - factor;
      break;
    case 'G':
      //  printf("Giga hertz: ");
      exp = -9 - factor;
      break;
    default:
      _abort(event_test,"Base time format error (unregonized SI unit): %s\n",tbs_char);  
    }

  }
  //printf("%d\n", exp); 

  return exp;
}


OmnetSimulation::OmnetSimulation(const char* name, cEnvir* env, PhoenixSim* ps_,
				 map<string, string> lmm, map<string, string> lgm) :
  cSimulation(name, env) {
  printf("constructing omnet simulation...\n");

  omnetFinished = false;
  
  ps = ps_;
  linkModuleMap = lmm;
  linkGateMap = lgm;
}


OmnetSimulation::OmnetSimulation(const char* name, cEnvir *env, PhoenixSim* ps_) :
  cSimulation(name, env) {
  printf("constructing omnet simulation...\n");

  omnetFinished = false;
  ps = ps_;

}

OmnetSimulation::OmnetSimulation(const char* name, cEnvir *env ) :
  cSimulation(name, env) {
  printf("constructing omnet simulation...\n");
  omnetFinished = false;
}

void OmnetSimulation::insertMsg(cMessage *msg) {
  if( omnetFinished )
    return;
  //printf("Inserting message...\n");		
  cSimulation::insertMsg(msg);
  ps->sendMsg(msg);
}

void OmnetSimulation::processMsg(cMessage *msg) {
  
  /*  SSTMessage* sstmsg = dynamic_cast<SSTMessage*>(msg);

  if( sstmsg == NULL ) {
    _abort(event_test, "NULL or non-SSTMessage msg arrived.\n");
  }
  
  cModule* mod = getModuleByPath((linkModuleMap[sstmsg->getLinkName()]).c_str());
  
  SSTIO* sstIO = dynamic_cast<SSTIO*>(mod);
  if( sstIO == NULL ) {
    _abort(event_test, "Attempted to inject message into module of type other than SSTIO."  
	   " You must connect SST links to SSTIO modules.\n");
  }
  
  sstIO->injectMessage(msg);    
  */
}

void OmnetSimulation::processSelfMsg(cMessage *msg) {
  if( msg == NULL )
    _abort(event_test,"NULL self message\n");

  //if( omnetFinished )
  // return;

  //printf("Processing self message...\n");
  
  
  // cSimulation::insertMsg(msg);
  //printf("message inserted\n");
  cSimpleModule* mod = selectNextModule();
  //  mod->snapshot();

  //TO DO!!
  SimEnv* envir = dynamic_cast<SimEnv*>(getEnvir());
  if( !envir )
    _abort(event_test, "SST environment not in use.  This is a problem.\n");
  envir->beforeEvent(mod);

  if( mod ) {
    doOneEvent(mod);
  } else {
    printf("ERROR PROCESSING MESSAGE!!\n");
  }
  
  if( envir->afterEvent(mod) ) { //****TO DO: CHECK IF THIS IS A VALID WAY TO STOP THE SIMULATION
    //time's up, simulation finished
    printf("OMNet Simulation Completed.  Waiting on other components.\n");
    ps->unregisterExit();
    omnetFinished = true;
  }
}

//KEEP FORMAT OF ALL THESE FUNCTIONS THE SAME

static Component*
create_PhoenixSim(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
  return new PhoenixSim( id, params );
}

static const ElementInfoComponent components[] = {
    { "PhoenixSim",
      "PhoenixSim Component",
      NULL,
      create_PhoenixSim
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo PhoenixSim_eli = {
        "PhoenixSim",
        "PhoenixSim",
        components,
    };
}
