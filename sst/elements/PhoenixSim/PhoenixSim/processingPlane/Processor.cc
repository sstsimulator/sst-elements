/*
 * Processor.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "Processor.h"

#include "statistics.h"

#include "Application.h"

#include "synthetics/AppRandom.h"
#include "synthetics/AppNeighbor.h"
#include "synthetics/AppTornado.h"
#include "synthetics/AppBitreverse.h"
#include "synthetics/AppHotspot.h"

#include "DRAM/AppDRAMRandom.h"
#include "DRAM/AppDRAMAll2One.h"
#include "DRAM/AppDRAMOne2One.h"
#include "DRAM/AppDRAMOne2All.h"

#include "synthetics/AppRandom_FiniteQueue.h"

#include "test/AppAll2All.h"
#include "test/AppOne2One.h"
#include "test/AppBcastTest.h"

#include "trace/AppTrace_LBL.h"
#include "trace/App_LL_DepGraph.h"

#include "model/AppFFT.h"
#include "model/AppFFTstream.h"
#include "model/AppDataFlow.h"

#include "DRAM_Cfg_Periphery.h"
#include "DRAM_Cfg_AllNodes.h"
#include "DRAM_Cfg_Periphery_SR16x16.h"
#include "DRAM_Cfg_Periphery_SR8x8.h"
#include "DRAM_Cfg_Periphery_SR4x4.h"

#include "SizeDistribution_Constant.h"
#include "SizeDistribution_Normal.h"
#include "SizeDistribution_Control.h"
#include "SizeDistribution_Gamma.h"
#include "SizeDistribution_InvGamma.h"

#include "AddressTranslator_Standard.h"
#include "AddressTranslator_SquareRoot.h"
#include "AddressTranslator_FatTree.h"

#include "MITUCB/App_MITUCB_Random.h"
#include "MITUCB/App_MITUCB_All2All.h"

Define_Module(Processor)
;

DRAM_Cfg* Processor::dramcfg;
bool Processor::dram_init = false;

int Processor::msgsarrived = 0;

int Processor::numDone = 0;

int Processor::numOfProcs = 0;

AddressTranslator* Processor::translator = NULL;

Processor::Processor() {
	// TODO Auto-generated constructor stub
	myNum = 0;

}

Processor::~Processor() {
	if (dramcfg != NULL) {
		delete dramcfg;
		dramcfg = NULL;
		dram_init = false;
	}

}

int Processor::numInitStages() const {
	return 2;
}

void Processor::initialize(int stage) {

  	if (stage == 0) {
		myNum = numOfProcs;
		numOfProcs++;
		return;
	}
	if (stage == 1) {
#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY < 4
		if(true)
		{
			int side = floor(sqrt(numOfProcs) + 0.5);
			double sideLength = 20000;
			ThermalModel::registerThermalObject("Processor", getId(), sideLength/side, sideLength/side, int(myNum%side)*double(sideLength/side), int(myNum/side)*double(sideLength/side));
		}
		else
		{
			throw cRuntimeError("HotSpot Simulator requires definition of SizeWidth, SizeHeight, PositionLeftX, and PositionBottomY");
		}
#endif
#endif
	}
	if (stage == 2) {
		std::cout << numOfProcs << endl;
		return;
	}
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	if (translator == NULL) {
		string trans_str = par("addressTranslator");

		if (trans_str.compare("standard") == 0) {
			translator = new AddressTranslator_Standard();
		} else if (trans_str.compare("square_root") == 0) {
			translator = new AddressTranslator_SquareRoot();
		} else if (trans_str.compare("fat_tree") == 0) {
			translator = new AddressTranslator_FatTree();
		} else {
			opp_error("Invalid address translator in Processor.");
		}

		translator->concentration = par("processorConcentration");
		translator->numProcs = numOfProcs;

		string str_pro = par("networkProfile");
		AddressTranslator::makeProfile(str_pro.substr(str_pro.find(";") + 1)
				+ "1." + par("processorConcentration").str() + ".",
				str_pro.substr(0, str_pro.find(";")) + "DRAM.PROC.");

	}

	string appStr = par("application");

	reqOut = gate("nicReq$o");
	fromNic = gate("fromNic");
	toNic = gate("toNic");

	isNetworkSideConcentration = par("isNetworkSideConcentration");

	string str_id = par("id");

	clockPeriod_cntrl = 1.0 / (double) par("O_frequency_proc");
	flit_width = par("electronicChannelWidth");

	double ePerOp = par("energyPerOP");
	Application::ePerOp = ePerOp;
	Application::clockPeriod = SIMTIME_DBL(clockPeriod_cntrl);

	bool useioplane = par("useIOplane");

	myAddr = new NetworkAddress(str_id, translator->numLevels);

	int id = translator->untranslate_pid(myAddr);

	debug(getFullPath(), "id_str: ", translator->untranslate_str(myAddr),
			UNIT_PROCESSOR);
	debug(getFullPath(), "id_pid: ", translator->untranslate_pid(myAddr),
			UNIT_PROCESSOR);

	Application::param1 = par("appParam1");
	Application::param2 = par("appParam2");
	Application::param3 = par("appParam3");
	Application::param4 = par("appParam4");

	Application::param5 = par("appParam5").str();


	//size dist
	SizeDistribution *sizedist = NULL;

	string sd = par("appSizeDist");

	if(sd.compare("constant") == 0){
		sizedist = new SizeDistribution_Constant(par("appSizeParam1"));
	}else if(sd.compare("normal") == 0){
		sizedist = new SizeDistribution_Normal(par("appSizeParam1"), par("appSizeParam2"));
	}else if(sd.compare("gamma") == 0){
		sizedist = new SizeDistribution_Gamma(par("appSizeParam1"), par("appSizeParam2"));
	}else if(sd.compare("invgamma") == 0){
		sizedist = new SizeDistribution_InvGamma(par("appSizeParam1"), par("appSizeParam2"));
	}else if(sd.compare("control") == 0){
		sizedist = new SizeDistribution_Control(par("appSizeParam1"), par("appSizeParam2"), par("appSizeParam3"),
				par("appSizeParam4"));
	}

	Application::sizeDist = sizedist;

	procOverhead = (int) par("procMsgOverhead") / (double) par("clockRate");

	numDone = 0;

	SO_latency = Statistics::registerStat("Latency (us)", StatObject::MMA,
			"application");
	SO_msg = Statistics::registerStat("Msg Size (B)", StatObject::MMA,
			"application");
	SO_bw = Statistics::registerStat("Bandwidth App In (Gb/s)",
			StatObject::TIME_AVG, "application");

	if (useioplane) {
		if (!dram_init) {
			string dc = par("ioPlaneConfig");

			if (dc.compare("Periphery") == 0) {

				dramcfg = new DRAM_Cfg_Periphery(numOfProcs, par(
						"processorConcentration"));
				dram_init = true;
			} else if (dc.compare("SR4x4_Periphery") == 0) {

				dramcfg = new DRAM_Cfg_Periphery_SR4x4(numOfProcs, par(
						"processorConcentration"));
				dram_init = true;
			} else if (dc.compare("SR8x8_Periphery") == 0) {

				dramcfg = new DRAM_Cfg_Periphery_SR8x8(numOfProcs, par(
						"processorConcentration"));
				dram_init = true;
			} else if (dc.compare("SR16x16_Periphery") == 0) {

				dramcfg = new DRAM_Cfg_Periphery_SR16x16(numOfProcs, par(
						"processorConcentration"));
				dram_init = true;
			} else if (dc.compare("AllNodes") == 0) {

				dramcfg = new DRAM_Cfg_AllNodes(numOfProcs, par(
						"processorConcentration"));
				dram_init = true;
			} else {
				opp_error("unknown dramCfg specified.");
			}

			dramcfg->init();
		}
	} else {
		dramcfg = NULL;
	}

	//std::cout << "application: " << appStr << endl;
	if (appStr.find("fft_stream") != string::npos) {

		app = new AppFFTstream(id, numOfProcs, dramcfg);

	} else if (appStr.find("fft") != string::npos) {

		app = new AppFFT(id, numOfProcs, dramcfg);

	} else if (appStr.find("randomfq") != string::npos) {

		app = new AppRandom_FiniteQueue(id, numOfProcs, 0, 0);

	} else if (appStr.find("randomDRAM") != string::npos) { //format is random_[arrival]_[size]

		app = new AppRandomDRAM(id, numOfProcs, dramcfg);

	} else if (appStr.find("random") != string::npos) { //format is random_[arrival]_[size]

		app = new AppRandom(id, numOfProcs, 0, 0);

	} else if (appStr.find("neighbor") != string::npos) { //format is random_[arrival]_[size]

		app = new AppNeighbor(id, numOfProcs, 0, 0);

	}else if (appStr.find("tornado") != string::npos) { //format is random_[arrival]_[size]

		app = new AppTornado(id, numOfProcs, 0, 0);

	}else if (appStr.find("bitreverse") != string::npos) { //format is random_[arrival]_[size]

		app = new AppBitreverse(id, numOfProcs, 0, 0);

	}else if (appStr.find("hotspot") != string::npos) { //format is random_[arrival]_[size]

		app = new AppHotspot(id, numOfProcs, 0, 0);

	}else if (appStr.find("trace_lbl") != string::npos) {

		app = new AppTrace_LBL(id, numOfProcs, dramcfg);
	} else if (appStr.find("trace_ll") != string::npos) {

		app = new App_LL_DepGraph(id, numOfProcs, dramcfg);
	} else if (appStr.find("all2oneDRAM") != string::npos) {

		app = new AppAll2OneDRAM(id, numOfProcs, dramcfg);

	} else if (appStr.find("one2oneDRAM") != string::npos) {

		app = new AppOne2OneDRAM(id, numOfProcs, dramcfg);

	} else if (appStr.find("one2allDRAM") != string::npos) {

		app = new AppOne2AllDRAM(id, numOfProcs, dramcfg);

	} else if (appStr.find("all2all") != string::npos) {

		app = new AppAll2All(id, numOfProcs, dramcfg);

	}else if (appStr.find("BcastTest") != string::npos) {

		app = new AppBcastTest(id, numOfProcs, dramcfg);

	} else if (appStr.find("dataFlow") != string::npos) {

		app = new AppDataFlow(id, numOfProcs, dramcfg);

	} else if (appStr.find("one2one") != string::npos) {

		app = new AppOne2One(id, numOfProcs, dramcfg);

	} else if (appStr.find("MITUCBrand") != string::npos) {
		app = new App_MITUCB_Random(id, numOfProcs, 0, 0);
	} else if (appStr.find("MITUCBa2a") != string::npos) {

		app = new App_MITUCB_All2All(id, numOfProcs, dramcfg);
	} else {
		opp_error(
				"no application specified.  how am i supposed to know what kind of traffic to run?");
	}

	app->useioplane = useioplane;

	ApplicationData* pdata = app->getFirstMsg();
	if (pdata != NULL) {
	  //printf("Scheduling first message\n");
	  scheduleAt(simTime() + app->process(pdata), pdata);
	}

	outstandingRequest = false;

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

void Processor::finish() {

	debug(getFullPath(), "FINISH: start", UNIT_FINISH);

	if (app != NULL) {
		app->currentTime = simTime();
		app->finish();
		delete app;
	}

	delete myAddr;

	if (translator != NULL) {
		delete translator;
		translator = NULL;
	}
/*
	 while(theQ.size() > 0){
	 ProcessorData* pdata = theQ.front();
	 theQ.pop();
	 ApplicationData* adata = (ApplicationData*)pdata->decapsulate();
	 SO_latency->track(SIMTIME_DBL(simTime() - adata->getCreationTime()) / 1e-6); //put into us
	 delete adata;
	 delete pdata;
	 }
*/
	dram_init = false;
	numDone = 0;
	numOfProcs = 0;

	debug(getFullPath(), "FINISH: done", UNIT_FINISH);
}

void Processor::handleMessage(cMessage* msg) {

	app->currentTime = simTime();

	if (msg->isSelfMessage()) { //self messages are to be sent somewhere
		ApplicationData* adata = check_and_cast<ApplicationData*> (msg);

		if (adata->getType() == CPU_op) {
			ApplicationData* next = app->dataArrive(adata);

			if (next != NULL) {
				scheduleAt(simTime() + app->process(next), next);
			}

			delete adata;

			return;
		}

		adata->setIsComplete(true);
		adata->setCreationTime(simTime());

		ProcessorData* pdata = new ProcessorData();

		pdata->setDestAddr((long) translator->translateAddr(adata));
		pdata->setSrcAddr((long) myAddr->dup());
		pdata->setIsComplete(true);
		pdata->setId(globalMsgId++);
		pdata->setCreationTime(simTime());

		debug(getFullPath(), " from ", translator->untranslate_str(myAddr),
				UNIT_PROCESSOR);
		debug(getFullPath(), "message to (id) ", adata->getDestId(),
				UNIT_PROCESSOR);
		debug(getFullPath(), "message to (addr) ", translator->untranslate_str(
				(NetworkAddress*) pdata->getDestAddr()), UNIT_PROCESSOR);

		pdata->setSize(adata->getPayloadSize());
		pdata->encapsulate(adata);
		pdata->setType(adata->getType());
		theQ.push(pdata);

		if (!outstandingRequest) {
			outstandingRequest = true;

			sendRequest(pdata);
		}

		ApplicationData* next = app->msgCreated(adata);
		if (next != NULL) {
			scheduleAt(simTime() + app->process(next), next);
		}

	} else if (msg->getArrivalGate() == fromNic) {
		ProcessorData* pdata = check_and_cast<ProcessorData*> (msg);

		debug(getFullPath(), "message arrived at: ",
				translator->untranslate_str(
						(NetworkAddress*) pdata->getDestAddr()), UNIT_PROCESSOR);
		debug(getFullPath(), "from: ", translator->untranslate_str(
				(NetworkAddress*) pdata->getSrcAddr()), UNIT_PROCESSOR);

		//std::cout << "messages arrived: " << ++msgsarrived << endl;


		if (pdata->getIsComplete()) {
			debug(getFullPath(), "message is complete", UNIT_PROCESSOR);

			if (!((NetworkAddress*) pdata->getDestAddr())->equals(myAddr) && pdata->getType() != snoopReq) {
				std::cout << "id: " << translator->untranslate_str(myAddr)
						<< endl;
				std::cout << "dest: " << translator->untranslate_str(
						((NetworkAddress*) pdata->getDestAddr())) << endl;

				std::cout << pdata->getId() << endl;
				opp_error("ERROR: message delivered to the wrong processor");
			}

			//msgRx.erase(pdata->getId());

			ApplicationData* adata = (ApplicationData*) pdata->decapsulate();

			//if (adata->getIsComplete()) {


			ApplicationData* next = app->dataArrive(adata);

			SO_latency->track(SIMTIME_DBL(simTime()
					- adata->getCreationTime()) / 1e-6); //put into us
			SO_bw->track(adata->getPayloadSize() / 1e9);

			if (next != NULL) {
				scheduleAt(simTime() + app->process(next), next);
			}
			//}
			SO_msg->track(adata->getPayloadSize() / 8);

			delete adata;

			delete (NetworkAddress*) pdata->getDestAddr();
			delete (NetworkAddress*) pdata->getSrcAddr();

#ifdef ENABLE_HOTSPOT
	ThermalModel::addThermalObjectEnergy("Processor", getId(), 10000*1e-9);
#endif

			//}
		}

		//}
		//if (pdata->getIsComplete()) {
		int p = myAddr->id[AddressTranslator::convertLevel("PROC")];
		//send unblock
		XbarMsg *unblock = new XbarMsg();
		unblock->setType(router_arb_unblock);
		unblock->setToPort(p);

		sendDelayed(unblock, clockPeriod_cntrl, reqOut);

		delete pdata;

	} else { //the request port
		RouterCntrlMsg* rmsg = check_and_cast<RouterCntrlMsg*> (msg);

		if (rmsg->getType() == proc_grant) {
			ProcessorData* pdata = theQ.front();
			send(pdata, toNic);

			ApplicationData* next = app->sending(
					(ApplicationData*) pdata->getEncapsulatedPacket());

			if (next != NULL) {
				scheduleAt(simTime() + app->process(next), next);
			}

			theQ.pop();

			//if we have core-side concentration and the destination is a neighbor processor,
			// let the application know that the message was completely sent
			NetworkAddress* src = (NetworkAddress*) pdata->getSrcAddr();
			NetworkAddress* dest = (NetworkAddress*) pdata->getDestAddr();
			if (translator->concentration > 1 && !isNetworkSideConcentration
					&& src->id[AddressTranslator::convertLevel("PROC")]
							!= dest->id[AddressTranslator::convertLevel("PROC")]
					&& src->id[AddressTranslator::convertLevel("DRAM")]
							== dest->id[AddressTranslator::convertLevel("DRAM")]
					&& src->id[AddressTranslator::convertLevel("NET")]
							== dest->id[AddressTranslator::convertLevel("NET")]) {
				ApplicationData* next = app->msgSent(
						(ApplicationData*) pdata->getEncapsulatedPacket());

				if (next != NULL) {
					scheduleAt(simTime() + app->process(next), next);
				}
			}

			outstandingRequest = false;

			if (theQ.size() > 0) {
				sendRequest(theQ.front());

				outstandingRequest = true;
			}
		} else if (rmsg->getType() == proc_req_send) { //from the nic, immediately grant
			RouterCntrlMsg* gr = new RouterCntrlMsg();
			gr->setType(proc_grant);
			send(gr, reqOut);
		} else if (rmsg->getType() == proc_msg_sent) {
			ApplicationData* adata = (ApplicationData*) rmsg->getData();

			ApplicationData* next = app->msgSent(adata);

			if (next != NULL) {
				scheduleAt(simTime() + app->process(next), next);
			}

			delete adata;

		}

		delete msg;

	}

}

void Processor::sendRequest(ProcessorData* pdata) {

	ArbiterRequestMsg* req = new ArbiterRequestMsg();
	req->setStage(10000);
	req->setType(proc_req_send);
	req->setVc(0);
	req->setDest(pdata->getDestAddr());
	req->setReqType(dataPacket);
	req->setPortIn(myAddr->lastId());
	req->setSize(1);
	req->setTimestamp(simTime());
	req->setMsgId(pdata->getId());

	send(req, reqOut);
#ifdef ENABLE_HOTSPOT
	ThermalModel::addThermalObjectEnergy("Processor", getId(), 10000*1e-9);
#endif
}

