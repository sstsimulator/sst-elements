#include <signal.h>

#include "simEnv.h"
#include "common/stringutil.h"
#include "common/ver.h"
#include "common/commonutil.h"
#include "omnetconfiguration.h"
#include "simutil.h"
#include "envir/cxmldoccache.h"
#include "common/stringtokenizer.h"
#include "common/fileutil.h"
#include "common/opp_ctype.h"
#include "envir/speedometer.h"
#include "nedxml/nedparser.h"
#include "common/unitconversion.h"
#include "envir/resultfilters.h"
#include "envir/resultrecorders.h"

#ifdef WITH_PARSIM
#include "parsim/cparsimpartition.h"
#include "parsim/cparsimsynchr.h"
#include "parsim/creceivedexception.h"
#endif

#ifdef USE_PORTABLE_COROUTINES /* coroutine stacks reside in main stack area */

# define TOTAL_STACK_SIZE   (2*1024*1024)
# define MAIN_STACK_SIZE       (128*1024)  // Tkenv needs more than 64K

#else /* nonportable coroutines, stacks are allocated on heap */

# define TOTAL_STACK_SIZE        0  // dummy value
# define MAIN_STACK_SIZE         0  // dummy value

#endif
#define STRINGIZE0(x) #x
#define STRINGIZE(x) STRINGIZE0(x)


static const char *compilerInfo =
    #if defined __GNUC__
    "GCC " __VERSION__
    #elif defined _MSC_VER
    "Microsoft Visual C++ version " STRINGIZE(_MSC_VER)
    #elif defined __INTEL_COMPILER
    "Intel Compiler version " STRINGIZE(__INTEL_COMPILER)
    #else
    "unknown-compiler"
    #endif
    ;

static const char *buildInfoFormat =
    "%d-bit"

    #ifdef NDEBUG
    " RELEASE"
    #else
    " DEBUG"
    #endif

    " simtime_t=%s"
    " large-file-support=%s"
    ;

static const char *buildOptions = ""
    #ifdef USE_DOUBLE_SIMTIME
    " USE_DOUBLE_SIMTIME"
    #endif

    #ifdef WITHOUT_CPACKET
    " WITHOUT_CPACKET"
    #endif

    #ifdef WITH_PARSIM
    " WITH_PARSIM"
    #endif

    #ifdef WITH_MPI
    " WITH_MPI"
    #endif

    #ifdef WITH_NETBUILDER
    " WITH_NETBUILDER"
    #endif

    #ifdef WITH_AKAROA
    " WITH_AKAROA"
    #endif

    #ifdef XMLPARSER
    " XMLPARSER=" STRINGIZE(XMLPARSER)
    #endif
    ;



#define CREATE_BY_CLASSNAME(var,classname,baseclass,description) \
     baseclass *var ## _tmp = (baseclass *) createOne(classname); \
     var = dynamic_cast<baseclass *>(var ## _tmp); \
     if (!var) \
         throw cRuntimeError("Class \"%s\" is not subclassed from " #baseclass, (const char *)classname);

Register_GlobalConfigOption(CFGID_OUTPUT_FILE, "cmdenv-output-file", CFG_FILENAME, NULL, "When a filename is specified, Cmdenv redirects standard output into the given file. This is especially useful with parallel simulation. See the `fname-append-host' option as well.");
Register_GlobalConfigOption(CFGID_CONFIG_NAME, "cmdenv-config-name", CFG_STRING, NULL, "Specifies the name of the configuration to be run (for a value `Foo', section [Config Foo] will be used from the ini file). See also cmdenv-runs-to-execute=. The -c command line option overrides this setting.")
Register_GlobalConfigOption(CFGID_RUNS_TO_EXECUTE, "cmdenv-runs-to-execute", CFG_STRING, NULL, "Specifies which runs to execute from the selected configuration (see cmdenv-config-name=). It accepts a comma-separated list of run numbers or run number ranges, e.g. 1,3..4,7..9. If the value is missing, Cmdenv executes all runs in the selected configuration. The -r command line option overrides this setting.")
Register_GlobalConfigOptionU(CFGID_CMDENV_EXTRA_STACK, "cmdenv-extra-stack", "B",  "8KB", "Specifies the extra amount of stack that is reserved for each activity() simple module when the simulation is run under Cmdenv.")
Register_PerRunConfigOption(CFGID_EXPRESS_MODE, "cmdenv-express-mode", CFG_BOOL, "true", "Selects ``normal'' (debug/trace) or ``express'' mode.")
Register_GlobalConfigOption(CFGID_CMDENV_INTERACTIVE, "cmdenv-interactive", CFG_BOOL,  "false", "Defines what Cmdenv should do when the model contains unassigned parameters. In interactive mode, it asks the user. In non-interactive mode (which is more suitable for batch execution), Cmdenv stops with an error.")
Register_PerRunConfigOption(CFGID_AUTOFLUSH, "cmdenv-autoflush", CFG_BOOL, "false", "Call fflush(stdout) after each event banner or status update; affects both express and normal mode. Turning on autoflush may have a performance penalty, but it can be useful with printf-style debugging for tracking down program crashes.")
Register_PerRunConfigOption(CFGID_MODULE_MESSAGES, "cmdenv-module-messages", CFG_BOOL, "true", "When cmdenv-express-mode=false: turns printing module ev<< output on/off.")
Register_PerRunConfigOption(CFGID_EVENT_BANNERS, "cmdenv-event-banners", CFG_BOOL, "true", "When cmdenv-express-mode=false: turns printing event banners on/off.")
Register_PerRunConfigOption(CFGID_EVENT_BANNER_DETAILS, "cmdenv-event-banner-details", CFG_BOOL, "false", "When cmdenv-express-mode=false: print extra information after event banners.")
Register_PerRunConfigOption(CFGID_MESSAGE_TRACE, "cmdenv-message-trace", CFG_BOOL, "false", "When cmdenv-express-mode=false: print a line per message sending (by send(),scheduleAt(), etc) and delivery on the standard output.")
Register_PerRunConfigOptionU(CFGID_STATUS_FREQUENCY, "cmdenv-status-frequency", "s", "2s", "When cmdenv-express-mode=true: print status update every n seconds.")
Register_PerRunConfigOption(CFGID_PERFORMANCE_DISPLAY, "cmdenv-performance-display", CFG_BOOL, "true", "When cmdenv-express-mode=true: print detailed performance information. Turning it on results in a 3-line entry printed on each update, containing ev/sec, simsec/sec, ev/simsec, number of messages created/still present/currently scheduled in FES.")
Register_PerObjectConfigOption(CFGID_CMDENV_EV_OUTPUT, "cmdenv-ev-output", CFG_BOOL, "true", "When cmdenv-express-mode=false: whether Cmdenv should print debug messages (ev<<) from the selected modules.")

// the following options are declared in other files
extern cConfigOption *CFGID_WARNINGS;
extern cConfigOption *CFGID_SCALAR_RECORDING;
extern cConfigOption *CFGID_VECTOR_RECORDING;
extern cConfigOption *CFGID_NETWORK;
extern cConfigOption *CFGID_TOTAL_STACK;
extern cConfigOption *CFGID_PARALLEL_SIMULATION;
extern cConfigOption *CFGID_SCHEDULER_CLASS;
extern cConfigOption *CFGID_PARSIM_COMMUNICATIONS_CLASS;
extern cConfigOption *CFGID_PARSIM_SYNCHRONIZATION_CLASS;
extern cConfigOption *CFGID_OUTPUTVECTORMANAGER_CLASS;
extern cConfigOption *CFGID_OUTPUTSCALARMANAGER_CLASS;
extern cConfigOption *CFGID_SNAPSHOTMANAGER_CLASS;
extern cConfigOption *CFGID_FNAME_APPEND_HOST;
extern cConfigOption *CFGID_DEBUG_ON_ERRORS;
extern cConfigOption *CFGID_PRINT_UNDISPOSED;
extern cConfigOption *CFGID_SIMTIME_SCALE;
extern cConfigOption *CFGID_SIM_TIME_LIMIT;
extern cConfigOption *CFGID_CPU_TIME_LIMIT;
extern cConfigOption *CFGID_WARMUP_PERIOD;
extern cConfigOption *CFGID_FINGERPRINT;
extern cConfigOption *CFGID_NUM_RNGS;
extern cConfigOption *CFGID_RNG_CLASS;
extern cConfigOption *CFGID_SEED_SET;
extern cConfigOption *CFGID_DEBUG_STATISTICS_RECORDING;
extern cConfigOption *CFGID_RECORD_EVENTLOG;
extern cConfigOption *CFGID_NED_PATH;
extern cConfigOption *CFGID_PARTITION_ID;


bool SimEnv::sigint_received;

// note: also updates "since" (sets it to the current time) if answer is "true"
inline bool elapsed(long millis, struct timeval& since)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    bool ret = timeval_diff_usec(now, since) > 1000*millis;
    if (ret)
        since = now;
    return ret;
}

// utility function for printing elapsed time
char *timeToStr(timeval t, char *buf=NULL)
{
   static char buf2[64];
   char *b = buf ? buf : buf2;

   if (t.tv_sec<3600)
       sprintf(b,"%ld.%.3ds (%dm %02ds)", (long)t.tv_sec, (int)(t.tv_usec/1000), int(t.tv_sec/60L), int(t.tv_sec%60L));
   else if (t.tv_sec<86400)
       sprintf(b,"%ld.%.3ds (%dh %02dm)", (long)t.tv_sec, (int)(t.tv_usec/1000), int(t.tv_sec/3600L), int((t.tv_sec%3600L)/60L));
   else
       sprintf(b,"%ld.%.3ds (%dd %02dh)", (long)t.tv_sec, (int)(t.tv_usec/1000), int(t.tv_sec/86400L), int((t.tv_sec%86400L)/3600L));

   return b;
}


//extern cConfigOption *CFGID_CMDENV_EXTRA_STACK;
//extern cConfigOption *CFGID_CONFIG_NAME;
//extern cConfigOption *CFGID_RUNS_TO_EXECUTE;

static char buffer[1024];

SimEnv::SimEnv(int ac, char** av, OmnetConfiguration* c, std::map<std::string, std::string> opts) :
  NullEnvir(ac, av, c)
{
  // initialize fout to stdout, then we'll replace it if redirection is
  // requested in the ini file
  fout = stdout;
  
  // init config variables that are used even before readOptions()
  opt_autoflush = true;
  
  cfg = c;
  options = opts;

  xmlcache = NULL;
  
  record_eventlog = false;
  eventlogmgr = NULL;
  outvectormgr = NULL;
  outscalarmgr = NULL;
  snapshotmgr = NULL;

  simRequired = false;
 

  num_rngs = 0;
  rngs = NULL;
  
#ifdef WITH_PARSIM
  parsimcomm = NULL;
  parsimpartition = NULL;
  #endif

  speedometer = new Speedometer();
  
  exitcode = 0;
  
}

//TO DO: finish implementing destructor
SimEnv::~SimEnv() {
  delete cfg;
  delete xmlcache;
  
  delete outvectormgr;
  delete outscalarmgr;
  delete snapshotmgr;
  
  delete speedometer;

  for (int i = 0; i < num_rngs; i++)
    delete rngs[i];
  delete [] rngs;
  
    #ifdef WITH_PARSIM
    delete parsimcomm;
    delete parsimpartition;
    #endif
}

void SimEnv::readOptions() {
  //TO DO: Complete this body
  //  printf("Reading options...\n");
 
  //cConfiguration *cfg = getConfig();
  
  opt_total_stack = (size_t) cfg->getAsDouble(CFGID_TOTAL_STACK, TOTAL_STACK_SIZE);
  opt_parsim = cfg->getAsBool(CFGID_PARALLEL_SIMULATION);
  
  if (!opt_parsim)
    {
      opt_scheduler_class = cfg->getAsString(CFGID_SCHEDULER_CLASS);
    }
  else
    {
#ifdef WITH_PARSIM
      opt_parsimcomm_class = cfg->getAsString(CFGID_PARSIM_COMMUNICATIONS_CLASS);
      opt_parsimsynch_class = cfg->getAsString(CFGID_PARSIM_SYNCHRONIZATION_CLASS);
#else
      throw cRuntimeError("Parallel simulation is turned on in the ini file, but OMNeT++ was compiled without parallel simulation support (WITH_PARSIM=no)");
#endif
    }
  
  /*opt_outputvectormanager_class = cfg->getAsString(CFGID_OUTPUTVECTORMANAGER_CLASS);
    opt_outputscalarmanager_class = cfg->getAsString(CFGID_OUTPUTSCALARMANAGER_CLASS);
    opt_snapshotmanager_class = cfg->getAsString(CFGID_SNAPSHOTMANAGER_CLASS);*/
  
  opt_fname_append_host = cfg->getAsBool(CFGID_FNAME_APPEND_HOST, opt_parsim);
  
  ev.debug_on_errors = cfg->getAsBool(CFGID_DEBUG_ON_ERRORS);
  opt_print_undisposed = cfg->getAsBool(CFGID_PRINT_UNDISPOSED);
  
  //TO DO: Make sure this should be commented out--scale setting happens in PhoenixSim.cc:setup
  // int scaleexp = (int) cfg->getAsInt(CFGID_SIMTIME_SCALE); 
  // SimTime::setScaleExp(scaleexp);
  
  // note: this is read per run as well, but Tkenv needs its value on startup too
  //opt_inifile_network_dir = cfg->getConfigEntry(CFGID_NETWORK->getName()).getBaseDirectory();

  
  // other options are read on per-run basis
  
  // note: configname and runstoexec will possibly be overwritten
  // with the -c, -r command-line options in our setup() method
  opt_configname = cfg->getAsString(CFGID_CONFIG_NAME);
  opt_runstoexec = cfg->getAsString(CFGID_RUNS_TO_EXECUTE);
  
  opt_extrastack = (size_t) cfg->getAsDouble(CFGID_CMDENV_EXTRA_STACK);
  opt_outputfile = cfg->getAsFilename(CFGID_OUTPUT_FILE).c_str();
  
  if (!opt_outputfile.empty()) {
    processFileName(opt_outputfile);
    ::printf("SimEnv: redirecting output to file `%s'...\n",opt_outputfile.c_str());
    FILE *out = fopen(opt_outputfile.c_str(), "w");
    if (!out)
      throw cRuntimeError("Cannot open output redirection file `%s'",opt_outputfile.c_str());
    fout = out;
  }
  //  printf("done reading options\n");
}

bool SimEnv::askyesno(const char *question)
{
    if (!opt_interactive)
    {
        throw cRuntimeError("Simulation needs user input in non-interactive mode (prompt text: \"%s (y/n)\")", question);
    }
    else
    {
        // should also return -1 (==CANCEL)
        for(;;)
        {
            ::fprintf(fout, "%s (y/n) ", question);
            ::fflush(fout);
            ::fgets(buffer, 512, stdin);
            buffer[strlen(buffer)-1] = '\0'; // chop LF
            if (opp_toupper(buffer[0])=='Y' && !buffer[1])
                return true;
            else if (opp_toupper(buffer[0])=='N' && !buffer[1])
                return false;
            else
                putsmsg("Please type 'y' or 'n'!\n");
        }
    }
}


void SimEnv::printEventBanner(cSimpleModule *mod)
{
    ::fprintf(fout, "** Event #%d  T=%s%s   %s (%s, id=%d)\n",
	      (int)simulation.getEventNumber(),
            SIMTIME_STR(simulation.getSimTime()),
	    progressPercentage(), // note: IDE launcher uses this to track progress
            mod->getFullPath().c_str(),
            mod->getComponentType()->getName(),
            mod->getId()
          );
    if (opt_eventbanner_details)
    {
        ::fprintf(fout, "   Elapsed: %s   Messages: created: %ld  present: %ld  in FES: %d\n",
                timeToStr(totalElapsed()),
                cMessage::getTotalMessageCount(),
                cMessage::getLiveMessageCount(),
                simulation.msgQueue.getLength());
    }
}


void SimEnv::doStatusUpdate(Speedometer& smeter)
{
    smeter.beginNewInterval();

    if (opt_perfdisplay)
    {
        ::fprintf(fout, "** Event #%d   T=%s   Elapsed: %s%s\n",
		  (int)simulation.getEventNumber(),
                SIMTIME_STR(simulation.getSimTime()),
                timeToStr(totalElapsed()),
                progressPercentage()); // note: IDE launcher uses this to track progress
        ::fprintf(fout, "     Speed:     ev/sec=%g   simsec/sec=%g   ev/simsec=%g\n",
                smeter.getEventsPerSec(),
                smeter.getSimSecPerSec(),
                smeter.getEventsPerSimSec());

        ::fprintf(fout, "     Messages:  created: %ld   present: %ld   in FES: %d\n",
                cMessage::getTotalMessageCount(),
                cMessage::getLiveMessageCount(),
                simulation.msgQueue.getLength());
    }
    else
    {
        ::fprintf(fout, "** Event #%d   T=%s   Elapsed: %s%s   ev/sec=%g\n",
		  (int)simulation.getEventNumber(),
                SIMTIME_STR(simulation.getSimTime()),
                timeToStr(totalElapsed()),
                progressPercentage(), // note: IDE launcher uses this to track progress
                smeter.getEventsPerSec());
    }

    // status update is always autoflushed (not only if opt_autoflush is on)
    ::fflush(fout);
}





void SimEnv::putsmsg(const char *s)
{
    ::fprintf(fout, "\n<!> %s\n\n", s);
    ::fflush(fout);
}


void SimEnv::readPerRunOptions() {
  //  printf("Reading per run options..\n");
  //cConfiguration *cfg = getConfig();
  
  // get options from ini file
  opt_network_name = cfg->getAsString(CFGID_NETWORK);
  opt_inifile_network_dir = cfg->getConfigEntry(CFGID_NETWORK->getName()).getBaseDirectory();
  opt_warnings = cfg->getAsBool(CFGID_WARNINGS);
  opt_simtimelimit = cfg->getAsDouble(CFGID_SIM_TIME_LIMIT);
  opt_cputimelimit = (long) cfg->getAsDouble(CFGID_CPU_TIME_LIMIT);
  opt_warmupperiod = cfg->getAsDouble(CFGID_WARMUP_PERIOD);
  opt_fingerprint = cfg->getAsString(CFGID_FINGERPRINT);
  opt_num_rngs = cfg->getAsInt(CFGID_NUM_RNGS);
  opt_rng_class = cfg->getAsString(CFGID_RNG_CLASS);
  opt_seedset = cfg->getAsInt(CFGID_SEED_SET);
  opt_debug_statistics_recording = cfg->getAsBool(CFGID_DEBUG_STATISTICS_RECORDING);
  
  simulation.setWarmupPeriod(opt_warmupperiod);
  
  // install hasher object
  if (!opt_fingerprint.empty())
    simulation.setHasher(new cHasher());
  else
    simulation.setHasher(NULL);
  
  // run RNG self-test on RNG class selected for this run
  cRNG *testrng;
  CREATE_BY_CLASSNAME(testrng, opt_rng_class.c_str(), cRNG, "random number generator");
  testrng->selfTest();
  delete testrng;
  
  // set up RNGs
  int i;
  for (i=0; i<num_rngs; i++)
    delete rngs[i];
  delete [] rngs;
  
  num_rngs = opt_num_rngs;
  rngs = new cRNG *[num_rngs];
  for (i=0; i<num_rngs; i++)
    {
      cRNG *rng;
      //      printf("rng class: %s\tnum: %d\n", opt_rng_class.c_str(), num_rngs);
      CREATE_BY_CLASSNAME(rng, opt_rng_class.c_str(), cRNG, "random number generator");
      rngs[i] = rng;
      //      printf("random number %f\n", rngs[i]->doubleRand());
      //      printf("here\n");
      rngs[i]->initialize(opt_seedset, i, num_rngs, getParsimProcId(), getParsimNumPartitions(), getConfig());
    }
  
  // init nextuniquenumber -- startRun() is too late because simple module ctors have run by then
  nextuniquenumber = 0;
  
#ifdef WITH_PARSIM
  if (opt_parsim)
    nextuniquenumber = (unsigned)parsimcomm->getProcId() * ((~0UL) / (unsigned)parsimcomm->getNumPartitions());
#endif
  
  // open message log file. Note: in startRun() it would be too late,
  // because modules have already been created by then
  record_eventlog = cfg->getAsBool(CFGID_RECORD_EVENTLOG);
  if (record_eventlog) {
    eventlogmgr = new EventlogFileManager();
    eventlogmgr->configure();
    eventlogmgr->open();
  }  
  
  //opt_expressmode = cfg->getAsBool(CFGID_EXPRESS_MODE);
  opt_expressmode = ((options["mode"]).compare("express") == 0);
  opt_interactive = cfg->getAsBool(CFGID_CMDENV_INTERACTIVE);
  opt_autoflush = cfg->getAsBool(CFGID_AUTOFLUSH);
  opt_modulemsgs = cfg->getAsBool(CFGID_MODULE_MESSAGES);
  opt_eventbanners = cfg->getAsBool(CFGID_EVENT_BANNERS);
  opt_eventbanner_details = cfg->getAsBool(CFGID_EVENT_BANNER_DETAILS);
  opt_messagetrace = cfg->getAsBool(CFGID_MESSAGE_TRACE);
  opt_status_frequency_ms = 1000*cfg->getAsDouble(CFGID_STATUS_FREQUENCY);
  opt_perfdisplay = cfg->getAsBool(CFGID_PERFORMANCE_DISPLAY);
  //  printf("Done reading per run options\n");
}

void SimEnv::setup() {
  //printf("SimEnv::setup()\n");
  //  cout << "SimEnv::setup()" << endl;

  simRequired = true;

  try {
    setPosixLocale();

    readOptions();

    // initialize coroutine library
    if (TOTAL_STACK_SIZE!=0 && opt_total_stack<=MAIN_STACK_SIZE+4096)  {
	ev.printf("Total stack size %d increased to %d\n", opt_total_stack, MAIN_STACK_SIZE);
	opt_total_stack = MAIN_STACK_SIZE+4096;
    }
    cCoroutine::init(opt_total_stack, MAIN_STACK_SIZE);
    
    // install XML document cache
    xmlcache = new cXMLDocCache();
    

    // TO DO: WHAT DOES THESE THREE CLASSES DO??
    // install output vector manager
/*
    CREATE_BY_CLASSNAME(outvectormgr, opt_outputvectormanager_class.c_str(), cOutputVectorManager, "output vector manager");
    
    // install output scalar manager
    CREATE_BY_CLASSNAME(outscalarmgr, opt_outputscalarmanager_class.c_str(), cOutputScalarManager, "output scalar manager");
 
    // install snapshot manager
    CREATE_BY_CLASSNAME(snapshotmgr, opt_snapshotmanager_class.c_str(), cSnapshotManager, "snapshot manager");
*/
   
    // set up for sequential or distributed execution
    if (!opt_parsim)
      {
	// sequential
	cScheduler *scheduler;
	CREATE_BY_CLASSNAME(scheduler, opt_scheduler_class.c_str(), cScheduler, "event scheduler");
	simulation.setScheduler(scheduler);
      }
    else
      { 
#ifdef WITH_PARSIM
	// parsim: create components
	CREATE_BY_CLASSNAME(parsimcomm, opt_parsimcomm_class.c_str(), cParsimCommunications, "parallel simulation communications layer");
	parsimpartition = new cParsimPartition();
	cParsimSynchronizer *parsimsynchronizer;
	CREATE_BY_CLASSNAME(parsimsynchronizer, opt_parsimsynch_class.c_str(), cParsimSynchronizer, "parallel simulation synchronization layer");
	
	// wire them together (note: 'parsimsynchronizer' is also the scheduler for 'simulation')
	parsimpartition->setContext(&simulation, parsimcomm, parsimsynchronizer);
	parsimsynchronizer->setContext(&simulation, parsimpartition, parsimcomm);
	simulation.setScheduler(parsimsynchronizer);
	
	// initialize them
	parsimcomm->init();
#else
	throw cRuntimeError("Parallel simulation is turned on in the ini file, but OMNeT++ was compiled without parallel simulation support (WITH_PARSIM=no)");
	#endif
      }
    
    //    const char *nedpath1 = args->optionValue('n',0);
    const char *nedpath1 = (options["nedSourceFolder"]).c_str();
    if (!nedpath1)
      nedpath1 = getenv("NEDPATH");
    std::string nedpath2 = getConfig()->getAsPath(CFGID_NED_PATH);
    std::string nedpath = opp_join(";", nedpath1, nedpath2.c_str());
    if (nedpath.empty())
      nedpath = ".";
    
    //    printf("nedpath1: %s \t nedpath2: %s\n", nedpath.c_str(), nedpath2.c_str());
    
    StringTokenizer tokenizer(nedpath.c_str(), PATH_SEPARATOR);
    std::set<std::string> foldersloaded;
    while (tokenizer.hasMoreTokens())
      {
	const char *folder = tokenizer.nextToken();
	if (foldersloaded.find(folder)==foldersloaded.end())
	  {
	    //ev.printf("Loading NED files from %s:", folder); ev.flush();
	    simulation.loadNedSourceFolder(folder);
	    //ev.printf(" %d\n", count);
	    foldersloaded.insert(folder);
	  }
      }
    simulation.doneLoadingNedFiles();
  } catch (std::exception& e) {
    displayException(e);
    return;
  }

  // '-c' and '-r' option: configuration to activate, and run numbers to run.
  // Both command-line options take precedence over inifile settings.
  // (NOTE: inifile settings *already* got read at this point! as EnvirBase::setup()
  // invokes readOptions()).
  /*
  const char *configname = (options["configuration"]).c_str();
  if (configname)
    opt_configname = configname;
  if (opt_configname.empty())
    opt_configname = "General";
  */

// opt_configname = "PhotonicMesh";
  //TO DO:  Allow for multiple run (multiple configuration) simulations
  //  const char *runstoexec = "1";
  
  /*const char *runstoexec = args->optionValue('r');
  if (runstoexec)
    opt_runstoexec = runstoexec;
  */

  //set up one run
  setupOneRun();
  
  ::fflush(fout);

  //  printf("done setup\n");

}

bool SimEnv::setupOneRun() {
  //  printf("SimEnv::setupOneRun()\n");
  //TO DO: this must eventually be declared outside this method so multiple configurations can be run
  // EnumStringIterator runiterator(opt_runstoexec.c_str());
  /*
  if( runiterator.hasError()) {
    ev.printfmsg("Error parsing list of runs to execute: `%s'", opt_runstoexec.c_str());
    exitcode = 1;
    return true;
  }

  if( runiterator() == -1 ) 
    return true;
  */
  setupnetwork_done = false;
  startrun_done = false;
  //runnumber = runiterator();
  //  printf("options[run] = %s\n", (options["run"]).c_str());
  runnumber = atoi((options["run"]).c_str());

  try {
    ::fprintf(fout, "\nPreparing for running configuration %s, run #%d...\n", opt_configname.c_str(), runnumber);
    ::fflush(fout);

    opt_configname = options["configuration"];
    cfg->activateConfig(opt_configname.c_str(), runnumber);
    ::fprintf(fout, "activated configuration: %s\n", opt_configname.c_str());
    ::fflush(fout);

    //TODO: print scenario and runid info to file (see cmdenv.cc:run())
    
    readPerRunOptions();

    //find network
    cModuleType *network = resolveNetwork((options["topNetwork"]).c_str());
    ASSERT(network);

    ::fprintf(fout, "Setting up network `%s'...\n", (options["topNetwork"]).c_str());
    ::fflush(fout);

    simulation.setupNetwork(network);
    setupnetwork_done = true;

    startRun();
    startrun_done = true;
    
    //from Cmdenv::simulate()
    installSignalHandler();
    startClock();
    sigint_received = false;

    if( opt_expressmode ) {
      speedometer->start(simulation.getSimTime());
      
      gettimeofday(&last_update, NULL);
      
      doStatusUpdate(*speedometer);
    }    

    try {
      if( !opt_expressmode) {
	disable_tracing = false;
      } else {
	disable_tracing = true;
	speedometer->start(simulation.getSimTime());
	struct timeval last_update;
	gettimeofday(&last_update, NULL);
	doStatusUpdate(*speedometer);

      }


    } catch (cTerminationException& e) {
      //      printf("Termination.\n");
        if (opt_expressmode)
            doStatusUpdate(*speedometer);
        disable_tracing = false;
        stopClock();
        deinstallSignalHandler();

        stoppedWithTerminationException(e);
        displayException(e);
        return true;
    }
    catch (std::exception& e) {
        if (opt_expressmode)
            doStatusUpdate(*speedometer);
        disable_tracing = false;
        stopClock();
        deinstallSignalHandler();
        throw;
    }


  } catch(std::exception& e) {
    exitcode = 1;
    stoppedWithException(e);
    displayException(e);
  }
  
  //  if( sigint_received )
  //    return true;

  // runiterator++;
  
  //  printf("done setting up one run\n");

  return false;
}

bool SimEnv::beforeEvent(cModule* cmod) {

  cSimpleModule* mod = dynamic_cast<cSimpleModule*>(cmod);

  try {

    if( !mod )
      throw cTerminationException("scheduler interrupted while waiting");

    //normal or fast mode
    if( !opt_expressmode) {

      if( opt_eventbanners && mod->isEvEnabled())
	printEventBanner(mod);
      
      if(opt_autoflush)
	::fflush(fout);

    }
    //express mode
    else {

      speedometer->addEvent(simulation.getSimTime());
      
      if((simulation.getEventNumber()&0xff)==0 && elapsed(opt_status_frequency_ms, last_update))
	doStatusUpdate(*speedometer);  
      

    }
  } catch (cTerminationException& e) {
    //    printf("Termination.\n");
    if (opt_expressmode)
      doStatusUpdate(*speedometer);
    disable_tracing = false;
    stopClock();
    deinstallSignalHandler();
    
    stoppedWithTerminationException(e);
    displayException(e);
    return true;
  }
  catch (std::exception& e) {
    if (opt_expressmode)
      doStatusUpdate(*speedometer);
    disable_tracing = false;
    stopClock();
    deinstallSignalHandler();
    throw;
  }
  return false;
}

bool SimEnv::afterEvent(cModule* mod) {
 try {
    
    if( !mod )
      throw cTerminationException("scheduler interrupted while waiting");

    //normal or fast mode
    if( !opt_expressmode) {

      flushLastLine();
      
      checkTimeLimits();
      if(sigint_received)
	throw cTerminationException("SIGINT or SIGTERM received, exiting");

    }
    //express mode
    else {
      
      checkTimeLimits();
      if(sigint_received)
	throw cTerminationException("SIGINT or SIGTERM received, exiting");
    }
  } catch (cTerminationException& e) {
   //   printf("Termination.\n");
    if (opt_expressmode)
      doStatusUpdate(*speedometer);
    disable_tracing = false;
    stopClock();
    deinstallSignalHandler();
    
    stoppedWithTerminationException(e);
    displayException(e);
    return true;
  }
  catch (std::exception& e) {
    if (opt_expressmode)
      doStatusUpdate(*speedometer);
    disable_tracing = false;
    stopClock();
    deinstallSignalHandler();
    throw;
  }

 

 return false;
}


void SimEnv::endRun()
{ 
  if( simRequired ) {
  fprintf(fout, "\nCalling finish() at end of Run #%d...\n", runnumber);
  simulation.callFinish();
  flushLastLine();
  checkFingerprint();

  if( opt_expressmode)
    doStatusUpdate(*speedometer);
  disable_tracing = false;
  stopClock();
  deinstallSignalHandler();

  
  //delete network
  if( setupnetwork_done ) {
    try {
      simulation.deleteNetwork();
    } catch (std::exception& e) {
      exitcode = 1;
      displayException(e);
    }
  }


    simulation.endRun();
    
    if (opt_parsim)
    {
#ifdef WITH_PARSIM
        parsimpartition->endRun();
#endif
}
    if (record_eventlog) {
        eventlogmgr->endRun();
        delete eventlogmgr;
        eventlogmgr = NULL;
        record_eventlog = false;
    }
  }
    /*
    snapshotmgr->endRun();
    outscalarmgr->endRun();
    outvectormgr->endRun();
    */
}

cModuleType *SimEnv::resolveNetwork(const char *networkname)
{ 
  cModuleType *network = NULL;
  std::string inifilePackage = simulation.getNedPackageForFolder(opt_inifile_network_dir.c_str());
  
  bool hasInifilePackage = !inifilePackage.empty() && strcmp(inifilePackage.c_str(),"-")!=0;
  if (hasInifilePackage)
    network = cModuleType::find((inifilePackage+"."+networkname).c_str());
  if (!network)
    network = cModuleType::find(networkname);
  if (!network) {
    if (hasInifilePackage)
            throw cRuntimeError("Network `%s' or `%s' not found, check .ini and .ned files",
                                networkname, (inifilePackage+"."+networkname).c_str());
    else
      throw cRuntimeError("Network `%s' not found, check .ini and .ned files", networkname);
  }
  if (!network->isNetwork())
    throw cRuntimeError("Module type `%s' is not a network", network->getFullName());
  return network;
}



void SimEnv::checkFingerprint()
{

    if (opt_fingerprint.empty() || !simulation.getHasher())
        return;

    int k = 0;
    StringTokenizer tokenizer(opt_fingerprint.c_str());
    while (tokenizer.hasMoreTokens())
    {
        const char *fingerprint = tokenizer.nextToken();
        if (simulation.getHasher()->equals(fingerprint))
        {
            printfmsg("Fingerprint successfully verified: %s", fingerprint);
            return;
        }
        k++;
    }

    printfmsg("Fingerprint mismatch! calculated: %s, expected: %s%s",
              simulation.getHasher()->str().c_str(),
              (k>=2 ? "one of: " : ""), opt_fingerprint.c_str());
}


void SimEnv::startRun()
{ 
  resetClock();
  /*  outvectormgr->startRun();
      outscalarmgr->startRun();
      snapshotmgr->startRun();*/
  if (record_eventlog)
    eventlogmgr->startRun();
  if (opt_parsim)
    {
#ifdef WITH_PARSIM
      parsimpartition->startRun();
      #endif
    }
  simulation.startRun();
  flushLastLine();
}

void SimEnv::resetClock()
{
    timeval now;
    gettimeofday(&now, NULL);
    laststarted = simendtime = simbegtime = now;
    elapsedtime.tv_sec = elapsedtime.tv_usec = 0;
}

void SimEnv::readParameter(cPar* par) {
    ASSERT(!par->isSet());  // must be unset at this point

    // get it from the ini file
    std::string moduleFullPath = par->getOwner()->getFullPath();
    //printf("Full path: %s\n", moduleFullPath.c_str());
    //printf("parname: %s\n", par->getName());

    const char *str = getConfigEx()->getParameterValue(moduleFullPath.c_str(), par->getName(), par->containsValue());

    //printf("STR: %s\n", str);

    if (opp_sst_strcmp(str, "default")==0)
    {
        ASSERT(par->containsValue());  // cConfiguration should not return "=default" lines for params that have no default value
        par->acceptDefault();
    }
    else if (opp_sst_strcmp(str, "ask")==0)
    {
      askParameter(par, false);
    }
    else if (!opp_isempty(str))
    {
      // printf("Parsing parameter value: %s\n", str);
        par->parse(str);
    }
    else
    {
      //printf("str empty\n");
        // str empty: no value in the ini file
        if (par->containsValue())
            par->acceptDefault();
        else {
	  askParameter(par, true);
	}
    }
}

void SimEnv::askParameter(cPar *par, bool unassigned)
{
  bool success = false;
  while (!success)
    {
      cProperties *props = par->getProperties();
      cProperty *prop = props->get("prompt");
      std::string prompt = prop ? prop->getValue(cProperty::DEFAULTKEY) : "";
      std::string reply;
      
      // ask the user. note: gets() will signal "cancel" by throwing an exception
      if (!prompt.empty())
	reply = this->gets(prompt.c_str(), par->str().c_str());
      else
	// DO NOT change the "Enter parameter" string. The IDE launcher plugin matches
	// against this string for detecting user input
	reply = this->gets((std::string("Enter parameter `")+par->getFullPath()+"' ("+(unassigned?"unassigned":"ask")+"):").c_str(), par->str().c_str());
      
      try
        {
	  par->parse(reply.c_str());
	  success = true;
        }
      catch (std::exception& e)
        {
	  ev.printfmsg("%s -- please try again.", e.what());
        }
    }
}

void SimEnv::sputn(const char *s, int n) {
  (void)::fwrite(s,1,n,stdout);

    if (disable_tracing)
        return;

    cComponent *ctx = simulation.getContext();
    if (!ctx || (opt_modulemsgs && ctx->isEvEnabled()) || simulation.getContextType()==CTX_FINISH)
    {
        ::fwrite(s,1,n,fout);
        if (opt_autoflush)
            ::fflush(fout);
    }
}

void SimEnv::processFileName(opp_string& fname)
{ //TO DO: Uncomment this--need to initialize opt_fname_append_host
  std::string text = fname.c_str();
  
  // insert ".<hostname>.<pid>" if requested before file extension
  // (note: parsimProcId cannot be appended because of initialization order)
  if (opt_fname_append_host)
    {
      std::string extension = "";
      std::string::size_type index = text.rfind('.');
      if (index != std::string::npos) {
	extension = std::string(text, index);
	text.erase(index);
      }
      
      const char *hostname=getenv("HOST");
      if (!hostname)
	hostname=getenv("HOSTNAME");
      if (!hostname)
	hostname=getenv("COMPUTERNAME");
      if (!hostname)
	throw cRuntimeError("Cannot append hostname to file name `%s': no HOST, HOSTNAME "
			    "or COMPUTERNAME (Windows) environment variable",
			    fname.c_str());
      int pid = getpid();
      
      // append
      text += opp_stringf(".%s.%d%s", hostname, pid, extension.c_str());
    }
  fname = text.c_str();
}

void SimEnv::displayException(std::exception& ex)
{ 
    cException *e = dynamic_cast<cException *>(&ex);
    if (!e)
        ev.printfmsg("Error: %s.", ex.what());
    else
        ev.printfmsg("%s", e->getFormattedMessage().c_str());
}

std::string SimEnv::gets(const char *prompt, const char *defaultReply)
{
  if(!opt_interactive)
    {
      throw cRuntimeError("The simulation wanted to ask a question, set cmdenv-interactive=true to allow it: \"%s\"", prompt);
    }
  else
    {
      ::fprintf(fout, "%s", prompt);
      if (!opp_isempty(defaultReply))
	::fprintf(fout, "(default: %s) ", defaultReply);
	::fflush(fout);
      
      ::fgets(buffer, 512, stdin);
      buffer[strlen(buffer)-1] = '\0'; // chop LF

      if (buffer[0]=='\x1b') // ESC?
	throw cRuntimeError(eCANCEL);
      
      return std::string(buffer);
    }
}


void SimEnv::stoppedWithException(std::exception& e)
{
    // if we're running in parallel and this exception is NOT one we received
    // from other partitions, then notify other partitions
#ifdef WITH_PARSIM
  if (opt_parsim && !dynamic_cast<cReceivedException *>(&e))
       parsimpartition->broadcastException(e);
#endif
}


// --------------eventlog callback interface implementation---------------------

void SimEnv::simulationEvent(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->simulationEvent(msg);

    if (!opt_expressmode && opt_messagetrace)
    {
        ::fprintf(fout, "DELIVD: %s\n", msg->info().c_str());  //TODO can go out!
        if (opt_autoflush)
            ::fflush(fout);
    }
}

cEnvir& SimEnv::flush()
{
    ::fflush(fout);
    return *this;
}

void SimEnv::beginSend(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->beginSend(msg);
}

void SimEnv::messageScheduled(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->messageScheduled(msg);
}

void SimEnv::messageCancelled(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->messageCancelled(msg);
}

void SimEnv::messageSendDirect(cMessage *msg, cGate *toGate, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    if (record_eventlog)
        eventlogmgr->messageSendDirect(msg, toGate, propagationDelay, transmissionDelay);
}

void SimEnv::messageSendHop(cMessage *msg, cGate *srcGate)
{
    if (record_eventlog)
        eventlogmgr->messageSendHop(msg, srcGate);
}

void SimEnv::messageSendHop(cMessage *msg, cGate *srcGate, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    if (record_eventlog)
        eventlogmgr->messageSendHop(msg, srcGate, propagationDelay, transmissionDelay);
}

void SimEnv::endSend(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->endSend(msg);
}

void SimEnv::messageDeleted(cMessage *msg)
{
    if (record_eventlog)
        eventlogmgr->messageDeleted(msg);
}

void SimEnv::componentMethodBegin(cComponent *from, cComponent *to, const char *methodFmt, va_list va, bool silent)
{
    if (record_eventlog)
        eventlogmgr->componentMethodBegin(from, to, methodFmt, va);
}

void SimEnv::componentMethodEnd()
{
    if (record_eventlog)
        eventlogmgr->componentMethodEnd();
}

void SimEnv::moduleCreated(cModule *newmodule)
{
    if (record_eventlog)
        eventlogmgr->moduleCreated(newmodule);

    bool ev_enabled = getConfig()->getAsBool(newmodule->getFullPath().c_str(), CFGID_CMDENV_EV_OUTPUT);
    newmodule->setEvEnabled(ev_enabled);
}

void SimEnv::messageSent_OBSOLETE(cMessage *msg, cGate *)
{
    if (!opt_expressmode && opt_messagetrace)
    {
        ::fprintf(fout, "SENT:   %s\n", msg->info().c_str());
        if (opt_autoflush)
            ::fflush(fout);
    }
}

void SimEnv::moduleDeleted(cModule *module)
{
    if (record_eventlog)
        eventlogmgr->moduleDeleted(module);
}

void SimEnv::moduleReparented(cModule *module, cModule *oldparent)
{
    if (record_eventlog)
        eventlogmgr->moduleReparented(module, oldparent);
}

void SimEnv::gateCreated(cGate *newgate)
{
    if (record_eventlog)
        eventlogmgr->gateCreated(newgate);
}

void SimEnv::gateDeleted(cGate *gate)
{
    if (record_eventlog)
        eventlogmgr->gateDeleted(gate);
}

void SimEnv::connectionCreated(cGate *srcgate)
{
    if (record_eventlog)
        eventlogmgr->connectionCreated(srcgate);
}

void SimEnv::connectionDeleted(cGate *srcgate)
{
    if (record_eventlog)
        eventlogmgr->connectionDeleted(srcgate);
}

void SimEnv::displayStringChanged(cComponent *component)
{
    if (record_eventlog)
        eventlogmgr->displayStringChanged(component);
}

void SimEnv::undisposedObject(cObject *obj)
{
    if (opt_print_undisposed)
        ::printf("undisposed object: (%s) %s -- check module destructor\n", obj->getClassName(), obj->getFullPath().c_str());
}

//------------------------configuration, model parameters
void SimEnv::configure(cComponent *component)
{
  //    addResultRecorders(component);
}

bool SimEnv::isModuleLocal(cModule *parentmod, const char *modname, int index)
{
#ifdef WITH_PARSIM
    if (!opt_parsim)
       return true;

    // toplevel module is local everywhere
    if (!parentmod)
       return true;

    // find out if this module is (or has any submodules that are) on this partition
    char parname[MAX_OBJECTFULLPATH];
    if (index<0)
        sprintf(parname,"%s.%s", parentmod->getFullPath().c_str(), modname);
    else
        sprintf(parname,"%s.%s[%d]", parentmod->getFullPath().c_str(), modname, index); //FIXME this is incorrectly chosen for non-vector modules too!
    std::string procIds = getConfig()->getAsString(parname, CFGID_PARTITION_ID, "");
    if (procIds.empty())
    {
        // modules inherit the setting from their parents, except when the parent is the system module (the network) itself
        if (!parentmod->getParentModule())
            throw cRuntimeError("incomplete partitioning: missing value for '%s'",parname);
        // "true" means "inherit", because an ancestor which answered "false" doesn't get recursed into
        return true;
    }
    else if (strcmp(procIds.c_str(), "*") == 0)
    {
        // present on all partitions (provided that ancestors have "*" set as well)
        return true;
    }
    else
    {
        // we expect a partition Id (or partition Ids, separated by commas) where this
        // module needs to be instantiated. So we return true if any of the numbers
        // is the Id of the local partition, otherwise false.
        EnumStringIterator procIdIter(procIds.c_str());
        if (procIdIter.hasError())
            throw cRuntimeError("wrong partitioning: syntax error in value '%s' for '%s' "
                                "(allowed syntax: '', '*', '1', '0,3,5-7')",
                                procIds.c_str(), parname);
        int numPartitions = parsimcomm->getNumPartitions();
        int myProcId = parsimcomm->getProcId();
        for (; procIdIter()!=-1; procIdIter++)
        {
            if (procIdIter() >= numPartitions)
                throw cRuntimeError("wrong partitioning: value %d too large for '%s' (total partitions=%d)",
                                    procIdIter(), parname, numPartitions);
            if (procIdIter() == myProcId)
                return true;
        }
        return false;
    }
#else
    return true;
#endif
}

cXMLElement *SimEnv::getXMLDocument(const char *filename, const char *path)
{
    cXMLElement *documentnode = xmlcache->getDocument(filename);
    assert(documentnode);
    if (path)
    {
        ModNameParamResolver resolver(simulation.getContextModule()); // resolves $MODULE_NAME etc in XPath expr.
        return cXMLElement::getDocumentElementByPath(documentnode, path, &resolver);
    }
    else
    {
        // returns the root element (child of the document node)
        return documentnode->getFirstChild();
    }
}


void SimEnv::forgetXMLDocument(const char *filename)
{
    xmlcache->forgetDocument(filename);
}

void SimEnv::flushXMLDocumentCache()
{
    xmlcache->flushCache();
}

cConfiguration *SimEnv::getConfig()
{
    return cfg;
}

cConfigurationEx *SimEnv::getConfigEx()
{
    return cfg;
}

void SimEnv::bubble(cComponent *component, const char *text)
{
    if (record_eventlog)
        eventlogmgr->bubble(component, text);
}

int SimEnv::getNumRNGs() const
{
    return num_rngs;
}


cRNG *SimEnv::getRNG(int k)
{
    if (k<0 || k>=num_rngs)
        throw cRuntimeError("RNG index %d is out of range (num-rngs=%d, check the configuration)", k, num_rngs);
    return rngs[k];
}

void SimEnv::getRNGMappingFor(cComponent *component)
{
    cConfigurationEx *cfg = getConfigEx();
    std::string componentFullPath = component->getFullPath();
    std::vector<const char *> suffixes = cfg->getMatchingPerObjectConfigKeySuffixes(componentFullPath.c_str(), "rng-*"); // CFGID_RNG_K
    if (suffixes.size()==0)
        return;

    // extract into tmpmap[]
    int mapsize=0;
    int tmpmap[100];
    for (int i=0; i<(int)suffixes.size(); i++)
    {
        const char *suffix = suffixes[i];  // contains "rng-1", "rng-4" or whichever has been found in the config for this module/channel
        const char *value = cfg->getPerObjectConfigValue(componentFullPath.c_str(), suffix);
        ASSERT(value!=NULL);
        char *s1, *s2;
        int modRng = strtol(suffix+strlen("rng-"), &s1, 10);
        int physRng = strtol(value, &s2, 10);
        if (*s1!='\0' || *s2!='\0')
            throw cRuntimeError("Configuration error: %s=%s for module/channel %s: "
                                "numeric RNG indices expected",
                                suffix, value, component->getFullPath().c_str());

        if (physRng>getNumRNGs())
            throw cRuntimeError("Configuration error: rng-%d=%d of module/channel %s: "
                                "RNG index out of range (num-rngs=%d)",
                                modRng, physRng, component->getFullPath().c_str(), getNumRNGs());
        if (modRng>=mapsize)
        {
            if (modRng>=100)
                throw cRuntimeError("Configuration error: rng-%d=... of module/channel %s: "
                                    "local RNG index out of supported range 0..99",
                                    modRng, component->getFullPath().c_str());
            while (mapsize<=modRng)
            {
                tmpmap[mapsize] = mapsize;
                mapsize++;
            }
        }
        tmpmap[modRng] = physRng;
    }

    // install map into the module
    if (mapsize>0)
    {
        int *map = new int[mapsize];
        memcpy(map, tmpmap, mapsize*sizeof(int));
        component->setRNGMap(mapsize, map);
    }
}

//-------------------------------------------------------------

/*
std::ostream *SimEnv::getStreamForSnapshot()
{
    return snapshotmgr->getStreamForSnapshot();
}

void SimEnv::releaseStreamForSnapshot(std::ostream *os)
{
    snapshotmgr->releaseStreamForSnapshot(os);
}
*/

int SimEnv::getParsimProcId() const
{
#ifdef WITH_PARSIM
    return parsimcomm ? parsimcomm->getProcId() : 0;
#else
    return 0;
#endif
}

int SimEnv::getParsimNumPartitions() const
{
#ifdef WITH_PARSIM
    return parsimcomm ? parsimcomm->getNumPartitions() : 0;
#else
    return 0;
#endif
}


unsigned long SimEnv::getUniqueNumber()
{
    // TBD check for overflow
    return nextuniquenumber++;
}


bool SimEnv::idle()
{
  return sigint_received;
}

unsigned SimEnv::getExtraStackForEnvir() const
{
    return opt_extrastack;
}


bool SimEnv::simulationRequired()
{ //TO DO:  uncomment!!
    // handle -h and -v command-line options
  const char *category = options["h"].c_str();
  if ( category != NULL && (strcmp(category, "") != 0) )
    {
        if (!category)
            printHelp();
        else
            dumpComponentList(category);
        return false;
    }

  if (strcmp(options["v"].c_str(), "true") == 0)
    {
        struct opp_stat_t statbuf;
        ev << "\n";
        ev << "Build: " OMNETPP_RELEASE " " OMNETPP_BUILDID << "\n";
        ev << "Compiler: " << compilerInfo << "\n";
        ev << "Options: " << opp_stringf(buildInfoFormat,
                                         8*sizeof(void*),
                                         opp_typename(typeid(simtime_t)),
                                         sizeof(statbuf.st_size)>=8 ? "yes" : "no");
        ev << buildOptions << "\n";
        return false;
    }

    cConfigurationEx *cfg = getConfigEx();

    // -a option: print all config names, and number of runs in them
    if (strcmp(options["a"].c_str(), "true") == 0)
    {
      ev.printf("\n");
      std::vector<std::string> configNames = cfg->getConfigNames();
      for (int i=0; i<(int)configNames.size(); i++)
	ev.printf("Config %s: %d\n", configNames[i].c_str(), cfg->getNumRunsInConfig(configNames[i].c_str()));
      return false;
    }

    // '-x' option: print number of runs in the given config, and exit (overrides configname)
    const char *configToPrint = options["x"].c_str();
    if (configToPrint != NULL && (strcmp(configToPrint, "") != 0))
    {
        //
        // IMPORTANT: the simulation launcher will parse the output of this
        // option, so it should be modified with care and the two kept in sync
        // (see OmnetppLaunchUtils.getSimulationRunInfo()).
        //
        // Rules:
        // (1) the number of runs should appear on the rest of the line
        //     after the "Number of runs:" text
        // (2) per-run information lines should span from the "Number of runs:"
        //     line until the next blank line ("\n\n").
        //

        // '-g'/'-G' options: modifies -x: print unrolled config, iteration variables, etc as well
      bool unrollBrief = (strcmp(options["g"].c_str(), "true") == 0 );
      bool unrollDetailed = (strcmp(options["G"].c_str(), "true") == 0 );

        ev.printf("\n");
        ev.printf("Config: %s\n", configToPrint);
        ev.printf("Number of runs: %d\n", cfg->getNumRunsInConfig(configToPrint));

        if (unrollBrief || unrollDetailed)
        {
            std::vector<std::string> runs = cfg->unrollConfig(configToPrint, unrollDetailed);
            const char *fmt = unrollDetailed ? "Run %d:\n%s" : "Run %d: %s\n";
            for (int i=0; i<(int)runs.size(); i++)
                ev.printf(fmt, i, runs[i].c_str());
        }
        return false;
    }
  
    return true;
}


void SimEnv::shutdown()
{
    try
    {
        simulation.deleteNetwork();
	
#ifdef WITH_PARSIM
        if (opt_parsim && parsimpartition)
            parsimpartition->shutdown();
	    #endif
    }
    catch (std::exception& e)
    {
        displayException(e);
    }
}


void SimEnv::printHelp() {
    ev << "\n";
    ev << "Help not available (yet) for SST.\n";
    ev << "\n";
}

void SimEnv::startClock()
{
    gettimeofday(&laststarted, NULL);
}

void SimEnv::stopClock()
{
    gettimeofday(&simendtime, NULL);
    elapsedtime = elapsedtime + simendtime - laststarted;
    simulatedtime = simulation.getSimTime();
}

timeval SimEnv::totalElapsed()
{
    timeval now;
    gettimeofday(&now, NULL);
    return now - laststarted + elapsedtime;
}

void SimEnv::checkTimeLimits()
{
  if (opt_simtimelimit!=0 && simulation.getSimTime()>=opt_simtimelimit) {
         throw cTerminationException(eSIMTIME);
  }
    if (opt_cputimelimit==0) // no limit
         return;
    if (disable_tracing && (simulation.getEventNumber()&0xFF)!=0) // optimize: in Express mode, don't call gettimeofday() on every event
         return;
    timeval now;
    gettimeofday(&now, NULL);
    long elapsedsecs = now.tv_sec - laststarted.tv_sec + elapsedtime.tv_sec;
    if (elapsedsecs>=opt_cputimelimit)
         throw cTerminationException(eREALTIME);
}

void SimEnv::stoppedWithTerminationException(cTerminationException& e)
{
    // if we're running in parallel and this exception is NOT one we received
    // from other partitions, then notify other partitions
#ifdef WITH_PARSIM
    if (opt_parsim && !dynamic_cast<cReceivedTerminationException *>(&e))
        parsimpartition->broadcastTerminationException(e);
#endif
}


void SimEnv::componentInitBegin(cComponent *component, int stage)
{
    if (!opt_expressmode)
        ::fprintf(fout, "Initializing %s %s, stage %d\n",
                component->isModule() ? "module" : "channel", component->getFullPath().c_str(), stage);
}


const char *SimEnv::progressPercentage()
{
    double simtimeRatio = -1;
    if (opt_simtimelimit!=0)
         simtimeRatio = simulation.getSimTime() / opt_simtimelimit;

    double cputimeRatio = -1;
    if (opt_cputimelimit!=0) {
        timeval now;
        gettimeofday(&now, NULL);
        long elapsedsecs = now.tv_sec - laststarted.tv_sec + elapsedtime.tv_sec;
        cputimeRatio = elapsedsecs / (double)opt_cputimelimit;
    }

    double ratio = std::max(simtimeRatio, cputimeRatio);
    if (ratio == -1)
        return "";
    else {
        static char buf[32];
        // DO NOT change the "% completed" string. The IDE launcher plugin matches
        // against this string for detecting user input
        sprintf(buf, "  %d%% completed", (int)(100*ratio));
        return buf;
    }
}

void SimEnv::installSignalHandler()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
}

void SimEnv::deinstallSignalHandler()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
}

void SimEnv::signalHandler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
        sigint_received = true;
}

void SimEnv::dumpComponentList(const char *category)
{
    bool wantAll = !strcmp(category, "all");
    bool processed = false;
    if (wantAll || !strcmp(category, "config") || !strcmp(category, "configdetails"))
    {
        processed = true;
        ev << "Supported configuration options:\n";
        bool printDescriptions = strcmp(category, "configdetails")==0;

        cRegistrationList *table = configOptions.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cConfigOption *obj = dynamic_cast<cConfigOption *>(table->get(i));
            ASSERT(obj);
            if (!printDescriptions) ev << "  ";
            if (obj->isPerObject()) ev << "<object-full-path>.";
            ev << obj->getName() << "=";
            ev << "<" << cConfigOption::getTypeName(obj->getType()) << ">";
            if (obj->getUnit())
                ev << ", unit=\"" << obj->getUnit() << "\"";
            if (obj->getDefaultValue())
                ev << ", default:" << obj->getDefaultValue() << "";
            ev << "; " << (obj->isGlobal() ? "global" : obj->isPerObject() ? "per-object" : "per-run") << " setting";
            ev << "\n";
            if (printDescriptions && !opp_isempty(obj->getDescription()))
                ev << opp_indentlines(opp_breaklines(obj->getDescription(),75).c_str(), "    ") << "\n";
            if (printDescriptions) ev << "\n";
        }
        ev << "\n";

        ev << "Predefined variables that can be used in config values:\n";
        std::vector<const char *> v = getConfigEx()->getPredefinedVariableNames();
        for (int i=0; i<(int)v.size(); i++)
        {
            if (!printDescriptions) ev << "  ";
            ev << "${" << v[i] << "}\n";
            const char *desc = getConfigEx()->getVariableDescription(v[i]);
            if (printDescriptions && !opp_isempty(desc))
                ev << opp_indentlines(opp_breaklines(desc,75).c_str(), "    ") << "\n";
        }
        ev << "\n";
    }
    if (!strcmp(category, "jconfig")) // internal undocumented option, for maintenance purposes
    {
        // generate Java code for ConfigurationRegistry.java in the IDE
        processed = true;
        ev << "Supported configuration options (as Java code):\n";
        cRegistrationList *table = configOptions.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cConfigOption *key = dynamic_cast<cConfigOption *>(table->get(i));
            ASSERT(key);

            std::string id = "CFGID_";
            for (const char *s = key->getName(); *s; s++)
                id.append(1, opp_isalpha(*s) ? opp_toupper(*s) : *s=='-' ? '_' : *s=='%' ? 'n' : *s);
            const char *method = key->isGlobal() ? "addGlobalOption" :
                                 !key->isPerObject() ? "addPerRunOption" :
                                 "addPerObjectOption";
            #define CASE(X)  case cConfigOption::X: typestring = #X; break;
            const char *typestring = NULL;
            switch (key->getType()) {
                CASE(CFG_BOOL)
                CASE(CFG_INT)
                CASE(CFG_DOUBLE)
                CASE(CFG_STRING)
                CASE(CFG_FILENAME)
                CASE(CFG_FILENAMES)
                CASE(CFG_PATH)
                CASE(CFG_CUSTOM)
            }
            #undef CASE

            ev << "    public static final ConfigOption " << id << " = ";
            ev << method << (key->getUnit() ? "U" : "") << "(\n";
            ev << "        \"" << key->getName() << "\", ";
            if (!key->getUnit())
                ev << typestring << ", ";
            else
                ev << "\"" << key->getUnit() << "\", ";
            if (!key->getDefaultValue())
                ev << "null";
            else
                ev << "\"" << opp_replacesubstring(key->getDefaultValue(), "\"", "\\\"", true) << "\"";
            ev << ",\n";

            std::string desc = key->getDescription();
            desc = opp_replacesubstring(desc.c_str(), "\n", "\\n\n", true); // keep explicit line breaks
            desc = opp_breaklines(desc.c_str(), 75);  // break long lines
            desc = opp_replacesubstring(desc.c_str(), "\"", "\\\"", true);
            desc = opp_replacesubstring(desc.c_str(), "\n", " \" +\n\"", true);
            desc = opp_replacesubstring(desc.c_str(), "\\n \"", "\\n\"", true); // remove bogus space after explicit line breaks
            desc = "\"" + desc + "\"";

            ev << opp_indentlines(desc.c_str(), "        ") << ");\n";
        }
        ev << "\n";

        std::vector<const char *> vars = getConfigEx()->getPredefinedVariableNames();
        for (int i=0; i<(int)vars.size(); i++)
        {
            opp_string id = vars[i];
            opp_strupr(id.buffer());
            const char *desc = getConfigEx()->getVariableDescription(vars[i]);
            ev << "    public static final String CFGVAR_" << id << " = addConfigVariable(";
            ev << "\"" << vars[i] << "\", \"" << opp_replacesubstring(desc, "\"", "\\\"", true) << "\");\n";
        }
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "classes"))
    {
        processed = true;
        ev << "Registered C++ classes, including modules, channels and messages:\n";
        cRegistrationList *table = classes.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cObject *obj = table->get(i);
            ev << "  class " << obj->getFullName() << "\n";
        }
        ev << "Note: if your class is not listed, it needs to be registered in the\n";
        ev << "C++ code using Define_Module(), Define_Channel() or Register_Class().\n";
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "classdesc"))
    {
        processed = true;
        ev << "Classes that have associated reflection information (needed for Tkenv inspectors):\n";
        cRegistrationList *table = classDescriptors.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cObject *obj = table->get(i);
            ev << "  class " << obj->getFullName() << "\n";
        }
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "nedfunctions"))
    {
        processed = true;
        ev << "Functions that can be used in NED expressions and in omnetpp.ini:\n";
        cRegistrationList *table = nedFunctions.getInstance();
        table->sort();
        std::set<std::string> categories;
        for (int i=0; i<table->size(); i++)
        {
            cNEDFunction *nf = dynamic_cast<cNEDFunction *>(table->get(i));
            cMathFunction *mf = dynamic_cast<cMathFunction *>(table->get(i));
            categories.insert(nf ? nf->getCategory() : mf ? mf->getCategory() : "???");
        }
        for (std::set<std::string>::iterator ci=categories.begin(); ci!=categories.end(); ++ci)
        {
            std::string category = (*ci);
            ev << "\n Category \"" << category << "\":\n";
            for (int i=0; i<table->size(); i++)
            {
                cObject *obj = table->get(i);
                cNEDFunction *nf = dynamic_cast<cNEDFunction *>(table->get(i));
                cMathFunction *mf = dynamic_cast<cMathFunction *>(table->get(i));
                const char *fcat = nf ? nf->getCategory() : mf ? mf->getCategory() : "???";
                const char *desc = nf ? nf->getDescription() : mf ? mf->getDescription() : "???";
                if (fcat==category)
                {
                    ev << "  " << obj->getFullName() << " : " << obj->info() << "\n";
                    if (desc)
                        ev << "    " << desc << "\n";
                }
            }
        }
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "neddecls"))
    {
        processed = true;
        ev << "Built-in NED declarations:\n\n";
        ev << "---START---\n";
        ev << NEDParser::getBuiltInDeclarations();
        ev << "---END---\n";
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "units"))
    {
        processed = true;
        ev << "Recognized physical units (note: other units can be used as well, only\n";
        ev << "no automatic conversion will be available for them):\n";
        std::vector<const char *> units = UnitConversion::getAllUnits();
        for (int i=0; i<(int)units.size(); i++)
        {
            const char *u = units[i];
            const char *bu = UnitConversion::getBaseUnit(u);
            ev << "  " << u << "\t" << UnitConversion::getLongName(u);
            if (opp_strcmp(u,bu)!=0)
                ev << "\t" << UnitConversion::convertUnit(1,u,bu) << bu;
            ev << "\n";
        }
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "enums"))
    {
        processed = true;
        ev << "Enums defined in .msg files\n";
        cRegistrationList *table = enums.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cObject *obj = table->get(i);
            ev << "  " << obj->getFullName() << " : " << obj->info() << "\n";
        }
        ev << "\n";
    }
    if (wantAll || !strcmp(category, "userinterfaces"))
    {
        processed = true;
        ev << "User interfaces loaded: SST\n";
        
    }

    if (wantAll || !strcmp(category, "resultfilters"))
    {
        processed = true;
        ev << "Result filters that can be used in @statistic properties:\n";
        cRegistrationList *table = resultFilters.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cObject *obj = table->get(i);
            ev << "  " << obj->getFullName() << " : " << obj->info() << "\n";
        }
    }

    if (wantAll || !strcmp(category, "resultrecorders"))
    {
        processed = true;
        ev << "Result recorders that can be used in @statistic properties:\n";
        cRegistrationList *table = resultRecorders.getInstance();
        table->sort();
        for (int i=0; i<table->size(); i++)
        {
            cObject *obj = table->get(i);
            ev << "  " << obj->getFullName() << " : " << obj->info() << "\n";
        }
    }

    if (!processed)
        throw cRuntimeError("Unrecognized category for '-h' option: %s", category);
}
