/*
 * DRAM_Cfg.h
 *
 *  Created on: Feb 18, 2009
 *      Author: SolidSnake
 */

#ifndef DRAM_CFG_H_
#define DRAM_CFG_H_

#include <omnetpp.h>
#include <map>

using namespace std;

class DRAM_Cfg {
public:
	DRAM_Cfg(int n, int pC);
	virtual ~DRAM_Cfg();


	virtual void init();

	virtual int getNumDRAM()= 0;

	int getAccessId(int coreId);    //takes a coreId, returns the access core to the right dram
	int getAccessCore(int dramId);  //takes a dramId, returns the access core. basically just uses the accessMap

	int procConc;

protected:

	virtual int get_bank_id(int nodeId, int x, int y) = 0;
	virtual int get_access_node(int dramId) = 0;


	int numX;
	int numY;

	map<int, int> bankMap;
	map<int, int> accessMap;



};

#endif /* DRAM_CFG_H_ */
