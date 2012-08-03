#ifndef _DOMAINCHANGER_H_
#define _DOMAINCHANGER_H_

#include <omnetpp.h>
#include "messages_m.h"

class DomainChanger : public cSimpleModule
{
	public:
		DomainChanger();
		~DomainChanger();
	protected:
		virtual void initialize();
		virtual void finish();
		virtual void handleMessage(cMessage* msg);

	private:
		cQueue buffer; // buffer for electronic to photonic conversion
		cQueue bufferOE; // buffer for photonic to electronic conversion
		cMessage *etimer;
		cMessage *ptimer; // bufferOE timer
		simtime_t startTime, nextTime;
		int numOfWavelengths;
		double photonicBitRate;
		bool useWDM;

		int gateId_electronicConnectionIn;
		int gateId_electronicConnectionOutIn;
		int gateId_electronicConnectionOut;
		int gateId_photonicConnectionInBase;
		int gateId_photonicConnectionOutBase;

		bool inPmsg;
		bool inAmsg;
		bool inEmsg;

		void handlePhotonicMessage(PhotonicMessage *pmsg);
		void handleElectronicMessage();
		void checkAndIssueAckOrElectronicMessage();
		void checkAndIssueBufferOE();
		void handleBufferOETimer();

		void handleElectronicTimer();


};



#endif //_DOMAINCHANGER_H_
