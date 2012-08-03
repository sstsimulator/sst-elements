/*
 * activeelement.cpp
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "activeelement.h"

ActiveElement::ActiveElement() {
}

ActiveElement::~ActiveElement() {
}

void ActiveElement::initialize() {
	InitializeBasicElement();
	InitializeActiveElement();
	Setup();
}

void ActiveElement::InitializeActiveElement() {
	gateIdFromRouter = -2;
	currState = 0;
	SetRoutingTable();
	WATCH(currState);
	lastSwitchTimestamp = 0;
	if (hasPar("numOfStates")) {
		numOfStates = par("numOfStates");
	} else {
		throw cRuntimeError("numOfStates was not specified");
	}

	cumulativeTime.resize(numOfStates, 0);
	vector<int> tempSwitchCount(numOfStates, 0);
	switchCount.resize(numOfStates, tempSwitchCount);

	E_dynamic = NULL;
	if (numOfStates > 1) {
		E_dynamic = Statistics::registerStat(GetName(),
				StatObject::ENERGY_EVENT, "photonic");
	}
	E_count
			= Statistics::registerStat(GetName(), StatObject::COUNT, "photonic");


#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==1
	if(hasPar("SizeWidth") && hasPar("SizeHeight") && hasPar("PositionLeftX") && hasPar("PositionBottomY"))
	{
		SizeWidth = par("SizeWidth");
        SizeHeight = par("SizeHeight");
		PositionLeftX = par("PositionLeftX");
        PositionBottomY = par("PositionBottomY");
        ThermalModel::registerThermalObject(groupLabel.c_str(), getId(), SizeWidth, SizeHeight, PositionLeftX, PositionBottomY);
	}
	else
	{
		throw cRuntimeError("HotSpot Simulator requires definition of SizeWidth, SizeHeight, PositionLeftX, and PositionBottomY");
	}
#endif
#endif
}

int ActiveElement::Route(int index) {
	return routingTable[index];
}

double ActiveElement::GetEnergyDissipation(int stateBefore, int stateAfter) {
	return 0;
}

double ActiveElement::GetPowerLevel(int state) {
	return 0;
}

bool ActiveElement::CheckAndHandleControlMessage(cMessage *msg) {
	int arrivalGate = msg->getArrivalGateId();
	if (arrivalGate == gateIdFromRouter) {
		//debug(getFullPath(), "Control Message received", UNIT_OPTICS);
		HandleControlMessage(check_and_cast<ElementControlMessage*> (msg));
		return true;
	}
	return false;
}

void ActiveElement::handleMessage(cMessage *msg) {
	int arrivalGate = msg->getArrivalGateId();
	if (!CheckAndHandlePhotonicMessage(msg) && !CheckAndHandleControlMessage(
			msg)) {
		delete msg;
		throw cRuntimeError(
				"Error 004: cMessage arrived on the invalid gateId, %d",
				arrivalGate);
	}
}

bool ActiveElement::StateChangeDestroysSignal(int newState, PacketStat *ps)
{
	return true;
}

void ActiveElement::HandleControlMessage(ElementControlMessage *msg)
{
	int st = msg->getState();
	map<double , PacketStat*>::iterator iter;
	if (st != currState)
	{
		for (int i = 0; i < numOfPorts; i++)
		{
			for(iter = currPackets[i].begin() ; iter != currPackets[i].end() ; iter++)
			{
				if (StateChangeDestroysSignal(st,(*iter).second))
				{
					throw cRuntimeError("Switching state while signals present in device.");
				}
			}
		}
		ChangeState(st);
	}
	delete msg;
}

void ActiveElement::SetState(int newState) {
	currState = newState;
	SetRoutingTable();
}

void ActiveElement::ChangeState(int newState)
{
	debug(getFullPath(), "setting state to ", newState, UNIT_OPTICS);
	cumulativeTime[currState] += simTime() - lastSwitchTimestamp;
	switchCount[currState][newState]++;
	AddStaticEnergyDissipation(currState,
			(simTime() - lastSwitchTimestamp).dbl());
	AddDynamicEnergyDissipation(currState, newState);
	lastSwitchTimestamp = simTime();
	SetState(newState);
}

void ActiveElement::AddStaticEnergyDissipation(int state, double lastDuration) {
	if (numOfStates > 1) {
		E_dynamic->track(GetPowerLevel(state) * lastDuration);

#ifdef ENABLE_HOTSPOT

#if HOTSPOT_GRANULARITY==1
		ThermalModel::addThermalObjectEnergy(groupLabel.c_str(), getId(), GetPowerLevel(state) * lastDuration);
#endif
#endif
	}

}

void ActiveElement::AddDynamicEnergyDissipation(int stateBefore, int stateAfter) {
	if (numOfStates > 1) {
		E_dynamic->track(GetEnergyDissipation(stateBefore, stateAfter));
#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==1
		ThermalModel::addThermalObjectEnergy(groupLabel.c_str(), getId(), GetEnergyDissipation(stateBefore, stateAfter));
#endif
#endif
	}
}
