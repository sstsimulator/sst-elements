/*
 * basicelement.cc
 *
 *  Created on: Oct 22, 2009
 *      Author: Johnnie Chan
 */

#include "basicelement.h"

BasicElement::BasicElement()
{
}

BasicElement::~BasicElement()
{
}

void BasicElement::initialize()
{
	InitializeBasicElement();
	Setup();
}

void BasicElement::InitializeBasicElement()
{
	currPacketStat = NULL;

	if(hasPar("groupLabel"))
	{
		groupLabel = par("groupLabel").stringValue();
	}
	else
	{
		groupLabel = "Default";
	}

	if(hasPar("numOfPorts"))
	{
		numOfPorts = par("numOfPorts");
	}
	else
	{
		throw cRuntimeError("numOfPorts was not specified");
	}

	routingTable.resize(numOfPorts, -1);
	gateIdIn.resize(numOfPorts, -2);
	gateIdOut.resize(numOfPorts, -2);
	portType.resize(numOfPorts, inout);
	inNoise.resize(numOfPorts, false);

	currPackets.resize(numOfPorts, map<double, PacketStat*>());
	currOutputPackets.resize(numOfPorts, map<double, PacketStat*>());
	lastMessageTimestamp.resize(numOfPorts);

#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==0
	if(hasPar("SizeWidth") && hasPar("SizeHeight") && hasPar("PositionLeftX") && hasPar("PositionBottomY"))
	{
		SizeWidth = par("SizeWidth");
        SizeHeight = par("SizeHeight");
		PositionLeftX = par("PositionLeftX");
        PositionBottomY = par("PositionBottomY");
        ThermalModel::registerThermalObject(groupLabel.c_str(),getId(), SizeWidth, SizeHeight, PositionLeftX, PositionBottomY);
	}
	else
	{
		throw cRuntimeError("HotSpot Simulator requires definition of SizeWidth, SizeHeight, PositionLeftX, and PositionBottomY");
	}
#endif
#endif

	SetRoutingTable();
}

void BasicElement::finish()
{
}

void BasicElement::handleMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	if(!CheckAndHandlePhotonicMessage(msg))
	{
		delete msg;
		throw cRuntimeError("Error 004: cMessage arrived on the invalid gateId, %d", arrivalGate);
	}
}

int BasicElement::Route(int index)
{
	return routingTable[index];
}

bool BasicElement::CheckAndHandlePhotonicMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	for (int i = 0; i < numOfPorts; i++)
	{
		if (arrivalGate == gateIdIn[i])
		{
			HandlePhotonicMessage((PhotonicMessage*) msg, i);
			return true;
		}
	}
	return false;
}

void BasicElement::HandlePhotonicMessage(PhotonicMessage *msg, int index) {
	int msgType = msg->getMsgType();
	PacketStat *msgPktStat = (PacketStat*) (msg->getPacketStat());
	pair<map<double, PacketStat*>::iterator, bool> ret;
	int destPort = AccessRoutingTable(index, msgPktStat);

	double wavelength = msgPktStat->wavelength;

	if (msgType == TXstart)
	{


		debug(getFullPath(), "Photonic Message received", UNIT_OPTICS);

		PacketStat *newPacketStat = new PacketStat(msgPktStat);
		ret = currPackets[index].insert(pair<double, PacketStat*> (wavelength, newPacketStat));
		if (ret.second == false)
		{
			throw cRuntimeError("Error : Wavelength collision on input.");
		}

		ret = currOutputPackets[destPort].insert(pair<double, PacketStat*> (wavelength, newPacketStat));
		if (ret.second == false)
		{
			throw cRuntimeError("Error : Wavelength collision on output.");
		}

		lastMessageTimestamp[index][wavelength] = simTime();
		newPacketStat->next = msgPktStat;
		if (newPacketStat->prev != NULL)
		{
			newPacketStat->prev->next = newPacketStat;
		}
		msgPktStat->prev = newPacketStat;


		ApplyInsertionLoss(newPacketStat, index, destPort, wavelength);
		ApplyCrosstalk(msgPktStat, index, destPort, wavelength);

	}
	else if (msgType == TXstop)
	{
		currPackets[index][wavelength]->next->prev = NULL;
		PacketStat* p = currPackets[index][wavelength];
		currPackets[index].erase(wavelength);
		currOutputPackets[destPort].erase(wavelength);
		delete p;
	}
	else
	{
		throw cRuntimeError("Photonic message was recieved in element with no msgType defined");
	}
	sendDelayed(msg, GetLatency(index, destPort), gateIdOut[destPort]);
}


int BasicElement::AccessRoutingTable(int index, PacketStat *ps)
{
	return Route(index);
}

void BasicElement::ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength)
{
	PacketStat *frontps = localps->next;
	double tempIL = GetInsertionLoss(indexIn, indexOut, wavelength);
	localps->currInsertionLoss = tempIL;
	frontps->signalPower -= tempIL;

	frontps->propagationLoss += GetPropagationLoss(indexIn, indexOut, wavelength);
	frontps->crossingLoss += GetCrossingLoss(indexIn, indexOut, wavelength);
	frontps->bendingLoss += GetBendingLoss(indexIn, indexOut, wavelength);

	map<double, double>::iterator it;
	double wavelengthIL;
	for(it = frontps->wavelengthNoisePower.begin(); it != frontps->wavelengthNoisePower.end(); it++)
	{
		wavelengthIL = GetInsertionLoss(indexIn, indexOut, it->first);
		it->second *= dBtoLinear(-wavelengthIL);
	}
	frontps->laserNoisePower *= dBtoLinear(-tempIL);
	frontps->messageNoisePower *= dBtoLinear(-tempIL);
}

void BasicElement::ApplyCrosstalk(PacketStat *frontps, int indexIn, int indexOut, double wavelength)
{
	map<double, PacketStat*>::iterator it, it2, it3, it4;
	int wavelengthDestination;
//	bool found;
	double crosstalkedPower;

	//iterate through input ports
	for(int i = 0; i < numOfPorts; i++)
	{
		// Crosstalk that occurs on same wavelength
		// If there are signals coming in AND if not calculating for the same input port
		if(i != indexIn)
		{
			// Does this wavelength exist on the channel currently being checking
			it = currPackets[i].find(wavelength);

			// if wavelength was found
			if(it != currPackets[i].end())
			{
				// For currently examined signal, add crosstalk from other messages present in the element
				frontps->messageNoisePower += dBtoLinear(-GetInsertionLoss(i, indexOut, wavelength)) * dBtoLinear(it->second->signalPower);


					//std::cout<<"front "<<getFullName()<<" "<<dBtoLinear(-GetInsertionLoss(i, indexOut, wavelength))<<" * "<<dBtoLinear(it->second->signalPower)<<" = "<<dBtoLinear(-GetInsertionLoss(i, indexOut, wavelength)) * dBtoLinear(it->second->signalPower)<<" => "<<frontps->messageNoisePower<<endl;


				// For other messages present in the device, add crosstalk from the examined signal
				PacketStat *ps = (*it).second;
				double applyIL = dBtoLinear(-GetInsertionLoss(indexIn, AccessRoutingTable(i,ps), wavelength));
				double orig_power = dBtoLinear(frontps->signalPower); // noise power not added, assumed to be negligible
				while (ps->next != NULL)
				{
					ps = ps->next;
					ps->messageNoisePower += applyIL * orig_power;
					applyIL *= dBtoLinear(-(ps->currInsertionLoss));
				}

					//std::cout<<getFullName()<<" "<<applyIL<<" * "<<orig_power<<" = "<<applyIL * orig_power<<" => "<<ps->messageNoisePower<<endl;
					//std::cout<<indexIn<<"-->"<<indexOut<<" : "<<i<<endl;

			}

		}
	}

	// cross wavelength crosstalk calculation

	// for each output port

	for(int i = 0; i < numOfPorts; i++)
	{
		crosstalkedPower = dBtoLinear(-GetInsertionLoss(indexIn, i, wavelength)) * dBtoLinear(frontps->signalPower);
		// other than the one that the message is going out on
		if(i != indexOut)
		{
			// if same wavelength isn't found on this output
			it = currOutputPackets[i].find(wavelength);
			if(it == currOutputPackets[i].end() && !currOutputPackets[i].empty())
			{

				// then add the cross-wavelength crosstalk power to all wavelenth messages equally
				for (it2 = currOutputPackets[i].begin(); it2 != currOutputPackets[i].end(); ++it2)
				{

						PacketStat *ps = it2->second;
						double currNoisePower = crosstalkedPower;
						while (ps->next != NULL)
						{
							ps = ps->next;
							ps->addWavelengthNoisePower(wavelength, currNoisePower);
							currNoisePower *= dBtoLinear(-(ps->currInsertionLoss));
						}
						//std::cout<<getFullName()<<" forward adding "<<currNoisePower<< " @ "<<it3->first<<endl;

				}
			}
		}
	}

	// for each input port
	for(int i = 0 ; i < numOfPorts ; i++)
	{
		for(it3 = currPackets[i].begin(); it3 != currPackets[i].end(); ++it3)
		{
			wavelengthDestination = AccessRoutingTable(i, it3->second);
			if(wavelengthDestination != indexOut)
			{
				it4 = currOutputPackets[indexOut].find(it3->first);
				if(it4 == currOutputPackets[indexOut].end())
				{
					//std::cout<<getFullName()<<" adding "<<dBtoLinear(-GetInsertionLoss(i, indexOut, it3->first)) * dBtoLinear(it3->second->signalPower)<< " @ "<<it3->first<<endl;
					frontps->addWavelengthNoisePower(it3->first, dBtoLinear(-GetInsertionLoss(i, indexOut, it3->first)) * dBtoLinear(it3->second->signalPower));
				}
			}
		}
	}

	// cleanup routine
	for(it = currOutputPackets[indexOut].begin(); it != currOutputPackets[indexOut].end(); ++it)
	{
		if(it->first != wavelength)
		{
			it->second->resetWavelengthNoisePower(wavelength);

			frontps->resetWavelengthNoisePower(it->first);
		}
	}

}

double BasicElement::GetInsertionLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	IL += GetPropagationLoss(indexIn, indexOut, wavelength);
	IL += GetCrossingLoss(indexIn, indexOut, wavelength);
	IL += GetBendingLoss(indexIn, indexOut, wavelength);
	IL += GetGain(indexIn, indexOut, wavelength);
	return IL;
}

double BasicElement::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double BasicElement::GetBendingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double BasicElement::GetCrossingLoss(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double BasicElement::GetGain(int indexIn, int indexOut, double wavelength)
{
	return 0;
}

double BasicElement::GetLatency(int indexIn, int indexOut)
{
	return 0;
}

string BasicElement::GetName()
{
	return groupLabel;
}
