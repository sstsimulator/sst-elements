/*
 * DRAM_Cfg_Periphery.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: Gilbert
 */

#include "DRAM_Cfg_Periphery.h"

DRAM_Cfg_Periphery::DRAM_Cfg_Periphery(int n, int pC) : DRAM_Cfg(n, pC) {

}

DRAM_Cfg_Periphery::~DRAM_Cfg_Periphery() {

}


int DRAM_Cfg_Periphery::get_bank_id(int coreId, int x, int y){
	if(x == 2){  //it's in the very middle
		return coreId;
	}

	int numDRAM = x * 2 + (y-2)*2;

	if(coreId < x)  //it's on the top
		return coreId;
	else if(coreId >= x*(y-1))  //it's on the bottom
		return numDRAM - x + (coreId - x*(y-1));
	else if(coreId % x == 0){  //it's on the left side
		int row = coreId / x;
		return x + (row-1) * 2;
	}else if(coreId % x == x-1){  //it's on the right side
		int row = coreId / x;
		return x + (row-1) * 2 + 1;
	}else{  //recurse
		int row = coreId / x;
		int res = get_bank_id(coreId - x - row - (row-1), x-2, y-2);

		//coreId was modified, so fix the result back
		int fix = 1;
		if(row == y-2)
			fix += 6;
		else if(row == 2){
			fix += 3;
		}

		return res + fix;

	}

}

int DRAM_Cfg_Periphery::getNumDRAM(){
	return 2*numX + 2*(numY-2);
}


int DRAM_Cfg_Periphery::get_access_node(int dramId){
	int numDRAM = numX * 2 + (numY-2)*2;

	if(dramId < numX)  //it's on the top
		return dramId;
	else if(dramId >= numDRAM - numX)  //it's on the bottom
		return numX * numY - (numDRAM - dramId);
	else{  //it's on the side
		int less = dramId - numX;
		return (less % 2) * (numX -1) + numX + (int)(less / 2) * numX;
	}

}
