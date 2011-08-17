/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPTORNADO_H_
#define APPTORNADO_H_

#include <omnetpp.h>
#include <assert.h>
#include "Application.h"

class AppTornado: public Application { //does poisson arrival, exponential size
public:
	AppTornado(int i, int n, double arr, int s);
	virtual ~AppTornado();

	virtual simtime_t process(ApplicationData* pdata); //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg(); //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata); //data has arrived for the application
	virtual ApplicationData* msgCreated(ApplicationData* pdata);

	int toId(int row, int col);

private:
	double arrival;
	int size;
	int numMessages;
	int progress;

	int neighbors;
	map<int, int> possibles;

	int row;
	int col;

};

#endif /* APPRANDOM_H_ */
