/*
 * activeelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef ACTIVEELEMENT_H_
#define ACTIVEELEMENT_H_

#include <vector>
#include "basicelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"
#include "thermalmodel.h"

class ActiveElement : public virtual BasicElement
{
	public:
		ActiveElement();
		virtual ~ActiveElement();
	protected:
		int currState;
		int gateIdFromRouter;
		int numOfStates;
		simtime_t lastSwitchTimestamp;
		vector<simtime_t> cumulativeTime;
 		vector< vector<int> > switchCount;
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);

		//virtual int AccessRoutingTable(int index, PacketStat *ps = NULL);
		void InitializeActiveElement();
		virtual void SetState(int newState);
		virtual void SetRoutingTable() = 0;
		virtual bool StateChangeDestroysSignal(int newState, PacketStat *ps);
		virtual double GetEnergyDissipation(int stateBefore, int stateAfter);
		virtual double GetPowerLevel(int state);

	private:
		StatObject* E_dynamic;
		StatObject* E_count;
 		bool CheckAndHandleControlMessage(cMessage *msg);
 		void HandleControlMessage(ElementControlMessage *msg);
 		int Route(int index);
		void ChangeState(int newState);
		void AddStaticEnergyDissipation(int state,double lastDuration);
		void AddDynamicEnergyDissipation(int stateBefore, int stateAfter);
};

#endif /* ACTIVEELEMENT_H_ */
