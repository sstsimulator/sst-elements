/*
 * DRAM_Cfg_Periphery.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: Gilbert
 */

#include "DRAM_Cfg_AllNodes.h"

DRAM_Cfg_AllNodes::DRAM_Cfg_AllNodes(int n, int pC) : DRAM_Cfg(n, pC) {

}

DRAM_Cfg_AllNodes::~DRAM_Cfg_AllNodes() {

}


int DRAM_Cfg_AllNodes::get_bank_id(int nodeId, int x, int y){
	return nodeId;
}

int DRAM_Cfg_AllNodes::getNumDRAM(){
	return numX*numY;
}


int DRAM_Cfg_AllNodes::get_access_node(int dramId){
	return dramId;

}
