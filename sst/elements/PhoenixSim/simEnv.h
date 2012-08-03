#ifndef _SIM_ENV_H
#define _SIM_ENV_H

#include <omnetpp.h>
#include "omnetconfiguration.h"
#include "common/enumstr.h"
#include "nullenvir.h"
#include "envir/eventlogfilemgr.h"

#include <map>
using namespace std;

class cXMLDocCache;
class Speedometer;

#define MAX_METHODCALL 1024
#define MAX_OBJECTFULLPATH 1024

//class cParsimPartition;

class SimEnv : public NullEnvir {

 public:
  SimEnv(int ac, char** av, OmnetConfiguration* c, 
	 std::map<std::string, std::string> opts);

  ~SimEnv();

  virtual void readParameter(cPar *par);

  virtual void sputn(const char*s, int n); 

  virtual void setup();
  virtual void endRun();
  virtual bool setupOneRun();
  virtual std::string gets(const char* prompt, const char* defaultReply);
  virtual void processFileName(opp_string& fname);
  virtual bool askyesno(const char *msg);


  //eventlog callback interface
  virtual void objectDeleted(cObject *object) { } //marked as "TODO?" in envirbase.cc
  virtual void simulationEvent(cMessage *msg);
  virtual void messageScheduled(cMessage *msg);
  virtual void messageCancelled(cMessage *msg);
  virtual void beginSend(cMessage *msg);
  virtual void messageSendDirect(cMessage *msg, cGate *toGate, simtime_t propagationDelay, simtime_t transmissionDelay);
  virtual void messageSendHop(cMessage *msg, cGate *srcGate);
  virtual void messageSendHop(cMessage *msg, cGate *srcGate, simtime_t propagationDelay, simtime_t transmissionDelay);
  virtual void endSend(cMessage *msg);
  virtual void messageDeleted(cMessage *msg);
  virtual void moduleReparented(cModule *module, cModule *oldparent);
  virtual void componentMethodBegin(cComponent *from, cComponent *to, const char *methodFmt, va_list va, bool silent);
  virtual void componentMethodEnd();
  virtual void moduleCreated(cModule *newmodule);
  virtual void moduleDeleted(cModule *module);
  virtual void gateCreated(cGate *newgate);
  virtual void gateDeleted(cGate *gate);
  virtual void connectionCreated(cGate *srcgate);
  virtual void connectionDeleted(cGate *srcgate);
  virtual void displayStringChanged(cComponent *component);
  virtual void undisposedObject(cObject *obj);

  //configuration, model parameters
  virtual void configure(cComponent *component);  
  virtual bool isModuleLocal(cModule *parentmod, const char *modname, int index);
  virtual cXMLElement *getXMLDocument(const char *filename, const char *path=NULL);
  virtual void forgetXMLDocument(const char *filename);
  virtual void flushXMLDocumentCache();
  // leave to subclasses: virtual unsigned getExtraStackForEnvir();
  virtual cConfiguration *getConfig();
  virtual cConfigurationEx *getConfigEx();

  // UI functions
  virtual void bubble(cComponent *component, const char *text);
  // leave to subclasses: virtual std::string gets(const char *prompt, const char *defaultreply=NULL);
  // leave to subclasses: virtual cEnvir& flush();
  
  // RNGs
  virtual int getNumRNGs() const;
  virtual cRNG *getRNG(int k);
  virtual void getRNGMappingFor(cComponent *component);
  
  //----------------------------------
  
  // output vectors
  //virtual void *registerOutputVector(const char *modulename, const char *vectorname);
  //virtual void deregisterOutputVector(void *vechandle);
  //virtual void setVectorAttribute(void *vechandle, const char *name, const char *value);
  //virtual bool recordInOutputVector(void *vechandle, simtime_t t, double value);
  
  // output scalars
  //virtual void recordScalar(cComponent *component, const char *name, double value, opp_string_map *attributes=NULL);
  //virtual void recordStatistic(cComponent *component, const char *name, cStatistic *statistic, opp_string_map *attributes=NULL);
  
  // snapshot file
  //virtual std::ostream *getStreamForSnapshot();
  //virtual void releaseStreamForSnapshot(std::ostream *os);
  
  // misc
  virtual int getArgCount() { printf("getArgCount: not using args...\n"); return 0; };
  virtual char **getArgVector() { printf("getArgVector: not using args...n"); return NULL; };
  virtual int getParsimProcId() const;
  virtual int getParsimNumPartitions() const;
  virtual unsigned long getUniqueNumber();
  virtual bool idle();
  //@}
  
  virtual bool simulationRequired();
 protected:
  // functions added locally
  virtual void shutdown();
  
  void printHelp();

  static bool sigint_received;
  
  bool simRequired;

  void dumpComponentList(const char *category);

  /**
   * Prints the contents of a registration list to the standard output.
   */
  //  void dumpComponentList(const char *category);
  
  /**
   * To be redefined to print Cmdenv or Tkenv-specific help on available
   * command-line options. Invoked from printHelp().
   */
  //  virtual void printUISpecificHelp();
  
  /**
   * Called from configure(component); adds result recording listeners
   * for each declared signal (@statistic property) in the component.
   */
  //  virtual void addResultRecorders(cComponent *component);
  
  /*
   * Original command-line args.
   
   ArgList *argList()  {return args;}
   
   ArgList* args;
  */
  
 protected:

  virtual void putsmsg(const char *s);
  virtual void printEventBanner(cSimpleModule *mod);
  virtual void doStatusUpdate(Speedometer& speedometer);
  
  cXMLDocCache *xmlcache;

  OmnetConfiguration* cfg;

  cParsimPartition* parsimpartition;
  cParsimCommunications *parsimcomm;

  opp_string opt_configname;
  opp_string opt_runstoexec;
  opp_string opt_outputfile;
  opp_string opt_network_name;
  opp_string opt_inifile_network_dir;
  size_t opt_total_stack;
  bool opt_parsim;
  bool opt_warnings;
  opp_string opt_scheduler_class;
  opp_string opt_parsimcomm_class; // if opt_parsim: cParsimCommunications class to use
  opp_string opt_parsimsynch_class; // if opt_parsim: cParsimSynchronizer class to use

  simtime_t opt_simtimelimit;
  long opt_cputimelimit;
  simtime_t opt_warmupperiod;

  int num_rngs;
  cRNG **rngs;
  
  opp_string opt_fingerprint;

  // Output file managers
  EventlogFileManager *eventlogmgr;  // NULL if no event log is being written, must be non NULL if record_eventlog is true
  cOutputVectorManager *outvectormgr;
  cOutputScalarManager *outscalarmgr;
  cSnapshotManager *snapshotmgr;
  
  opp_string opt_outputvectormanager_class;
  opp_string opt_outputscalarmanager_class;
  opp_string opt_snapshotmanager_class;
  bool opt_debug_statistics_recording;
  bool opt_fname_append_host;

  size_t opt_extrastack;
  
  bool opt_print_undisposed;

  int opt_num_rngs;
  opp_string opt_rng_class;
  int opt_seedset; // which set of seeds to use

  bool opt_expressmode;
  bool opt_interactive;
  bool opt_autoflush; // all modes
  bool opt_modulemsgs;  // if normal mode
  bool opt_eventbanners; // if normal mode
  bool opt_eventbanner_details; // if normal mode
  bool opt_messagetrace; // if normal mode
  long opt_status_frequency_ms; // if express mode
  bool opt_perfdisplay; // if express mode
  
  unsigned long nextuniquenumber;

  FILE* fout;

  void readOptions();
  void displayException(std::exception& ex);
  void readPerRunOptions();
  void stoppedWithException(std::exception& e);
  void startRun();
  void resetClock();
  void checkFingerprint();
  void askParameter(cPar* par, bool unassigned);
  cModuleType* resolveNetwork(const char*networkname);


  int exitcode;
  int runnumber;

  timeval simbegtime;  // real time when sim. started
  timeval simendtime;  // real time when sim. ended
  timeval laststarted; // real time from where sim. was last cont'd
  timeval elapsedtime; // time spent simulating
  simtime_t simulatedtime;  // sim. time after finishing simulation
 

  std::map<std::string, std::string> options;

 public:
  //per event execution environment stuff -- extra work taken from
  //  cmdenv::simulate()
  bool beforeEvent(cModule* mod);
  bool afterEvent(cModule* mod);


  //from cmdenv
  virtual void componentInitBegin(cComponent *component, int stage);
  virtual void messageSent_OBSOLETE(cMessage *msg, cGate *directToGate);
  virtual bool isGUI() const {return false;}
  virtual cEnvir& flush();
  virtual unsigned getExtraStackForEnvir() const;

  // new functions: from cmdenv
  //  void simulate();
  const char *progressPercentage();
  
  void installSignalHandler();
  void deinstallSignalHandler();
  static void signalHandler(int signum);

  //---clock/time stuff from envirbase

   /**
     * Checks if the current simulation has reached the simulation
     * or real time limits, and if so, throws an appropriate exception.
     */
    void checkTimeLimits();


    /**
     * Start measuring elapsed (real) time spent in this simulation run.
     */
    void startClock();

    /**
     * Stop measuring elapsed (real) time spent in this simulation run.
     */
    void stopClock();

    /**
     * Elapsed time
     */
    timeval totalElapsed();
    //@}

    //@{
    /**
     * Hook called when the simulation terminates normally.
     * Its current use is to notify parallel simulation part.
     */
    void stoppedWithTerminationException(cTerminationException& e);

 private:

    Speedometer* speedometer;
    struct timeval last_update;
    
    bool setupnetwork_done;
    bool startrun_done;
};

#endif