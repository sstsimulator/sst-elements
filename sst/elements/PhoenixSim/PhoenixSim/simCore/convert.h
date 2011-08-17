#ifndef _CONVERT_H_
#define _CONVERT_H_

#include <omnetpp.h>
#include "messages_m.h"

PhotonicMessage* EtoO(ElectronicMessage *msg);
ElectronicMessage* OtoE(PhotonicMessage *msg);

ElectronicMessage* newElectronicMessage();

#endif
