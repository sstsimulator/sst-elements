#ifndef _PHOENIXSIM_H
#define _PHOENIXSIM_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <string.h>
#include <map>

#include "simEnv.h"

#include <omnetpp.h>

using namespace std;

class OmnetSimulation;

class PhoenixSim : public SST::Component {
 public:

  PhoenixSim(SST::ComponentId_t id, SST::Component::Params_t& params);
  ~PhoenixSim();

  void handleSelfEvent(SST::Event *evt);
  void handleEvent(SST::Event *evt);
  
  virtual bool clockTic( SST::Cycle_t );

  int Finish();
  int Setup();

  /**
   * translate Omnet messages into SST events and schedule them
   * in SST event queue.
   */
  void sendMsg(cMessage* msg);
  
  /**
   * convert SST component clock frequency to Omnet resolution exponent.
   * 
   */
  int convertToExp(string tc);

 private:

  SST::Link* slink;
  SST::Link* cn;
  
  //SST::TimeConverter *tc;

  string nedSourceFolder;
  string topNetwork;

  map<string, string> linkModuleMap;
  map<string, string> linkGateMap;

  SimEnv* env;
  OmnetSimulation* sim;

  std::map<std::string, std::string> options;
  
  std::string timeBase;
  int timeExp;
  const char* ini;

  string linkModule;
  string linkGate;

  string iniFileName;

};


class OmnetSimulation : public cSimulation {
 public:

  OmnetSimulation(const char* name, cEnvir *env, PhoenixSim* ps, 
		  map<string, string> linkModuleMap, map<string, string> linkGateMap );
  OmnetSimulation(const char* name, cEnvir *env, PhoenixSim* ps);
  OmnetSimulation(const char* name, cEnvir *env);

  virtual void insertMsg(cMessage *msg);
  /**
   * Inserts messages from SST system into network.
   *
   **/
  void processMsg(cMessage *msg);

  /**
   * Handles self-link messages.
   *
   **/
  void processSelfMsg(cMessage *msg);

 private:

  map<string, string> linkModuleMap;
  map<string, string> linkGateMap;

  PhoenixSim* ps;
  bool omnetFinished;

};



#endif 
