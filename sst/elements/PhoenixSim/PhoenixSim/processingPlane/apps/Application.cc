/*
 * Application.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "Application.h"
#include "statistics.h"


double Application::param1;
double Application::param2;
double Application::param3;
double Application::param4;

string Application::param5;

double Application::ePerOp;

double Application::clockPeriod;

long Application::bytesTransferred = 0;
int Application::msgsRx = 0;
double Application::latencyTime = 0;
bool Application::statsReported = false;

SizeDistribution* Application::sizeDist;

Application::Application(int i, int n, DRAM_Cfg* cfg) {
	id = i;

	numOfProcs = n;

	dramcfg = cfg;

	bytesTransferred = 0;
}

Application::~Application() {

}

void Application::finish() {

	if (!statsReported) {
		//Statistics::statLog << "Avg Bandwidth:, " << bytesTransferred
		//		/ SIMTIME_DBL(currentTime) << endl;
		//Statistics::statLog << "Avg Latency:, " << latencyTime / msgsRx << endl;

		statsReported = true;
	}
}

