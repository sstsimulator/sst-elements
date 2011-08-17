#ifndef SCHEDULE_ITEM_H_
#define SCHEDULE_ITEM_H_

#include "messages_m.h"

using namespace std;

class ScheduleItem {
public:
	enum SCHEDULE_TYPE {
		SEND = 0,
		RX,
		MEM_ROW_CMD,
		MEM_LCL_READ,
		MEM_REM_READ,
		MEM_WRITE,
		MEM_WRITE_DATA
	};

	ScheduleItem(SCHEDULE_TYPE t, ProcessorData* p) {
		type = t;
		procData = p;
		cntrlSetup = false;
		isXY = false;
		sentThisSlot = false;

	}

	virtual ~ScheduleItem() {
		//std::cout << "ScheduleItem destructor" << endl;
		delete procData;
	}

	SCHEDULE_TYPE type;
	ProcessorData* procData;

	bool isXY;

	bool cntrlSetup;

	int myTimeSlot;

	bool sentThisSlot;

	static simtime_t cntrlPeriod;
	static int numWavelengths;
	static double dataRate;

	virtual bool done() = 0;
	virtual bool notifyProc() = 0;
	virtual void sendingBits(int bits) = 0;

	virtual int getSendSize() {
		return procData->getSize();
	}

	virtual simtime_t getWaitTime() {
		return 2 * cntrlPeriod;
	}

	virtual bool repeat() {
		return false;
	} //returns true if another item can be executed in the same slot

	virtual ProcessorData* getMsg() {
		return procData;
	}
	virtual void setNewSize(int s) {
		procData->setSize(s);
	}

	virtual int getRingCntrl() = 0;

	virtual ProcessorData* getRescheduleMsg() {
		return NULL;
	}

};

//----------------------- subclasses ---------------------


class ScheduleItem_Send: public ScheduleItem {
public:
	ScheduleItem_Send(ProcessorData* p) :
		ScheduleItem(SEND, p) {

	}

	virtual ~ScheduleItem_Send() {
		//delete procData;
	}

	virtual bool repeat() {
		return true;
	}

	virtual int getRingCntrl() {
		int ret = pow(2, 4);
		return ret;
	}

	virtual bool done() {
		return procData->getSize() <= 0;
	}

	virtual bool notifyProc() {
		return true;
	}

	virtual void sendingBits(int bits) {
		procData->setSize(procData->getSize() - bits);
	}

};

class ScheduleItem_Rx: public ScheduleItem {
public:
	ScheduleItem_Rx() :
		ScheduleItem(RX, NULL) {
	}

	virtual int getRingCntrl() {
		int ret = pow(2, 2);
		return ret;
	}

	virtual bool done() {
		return true;
	}

	virtual bool notifyProc() {
		return false;
	}

	virtual void sendingBits(int bits) {

	}

	virtual ProcessorData* getMsg() {
		return NULL;
	}

	virtual int getSendSize() {
		return 0;
	}

	virtual simtime_t getWaitTime() {
		return 0;
	}

};

//----------------- memory schedule items ----------
class ScheduleItem_Mem: public ScheduleItem {
public:

	int bankId;
	int row;

	int maxBurst;

	int size; //in cols
	int accessed; //in cols

	static int rows;
	static int cols;
	static int banks;
	static int chips;
	static int arrays;

	DRAM_CntrlMsg* cmd;

	ScheduleItem_Mem(SCHEDULE_TYPE t, ProcessorData* p, int b, int r, int s,
			int max) :
		ScheduleItem(t, p) {
		bankId = b;
		row = r;
		size = s;
		accessed = 0;
		cmd = NULL;
		maxBurst = max;
	}

	virtual int getSendSize() {
		return 128;
	}
};

class ScheduleItem_RowCmd: public ScheduleItem_Mem {
public:
	ScheduleItem_RowCmd(ProcessorData* p, int b, int r, int s, int max) :
		ScheduleItem_Mem(MEM_ROW_CMD, p, b, r, s, max) {

		sent = false;
	}

	bool sent;

	virtual ~ScheduleItem_RowCmd() {
		//std::cout << "RowCmd destructor" << endl;
		if (cmd != NULL)
			delete cmd;
		//delete procData;
	}

	virtual ProcessorData* getMsg() {
		if (cmd == NULL) {
			cmd = new DRAM_CntrlMsg();
			cmd->setType(Row_Access);
			cmd->setRow(row);
			cmd->setWriteEn(false);
			cmd->setBank(bankId);
			cmd->setCoreAddr(procData->getSrcAddr());
			cmd->setSize(128);
		}

		return cmd;

	}

	virtual int getRingCntrl() {
		int ret = pow(2, 1);
		return ret;
	}

	virtual bool done() {
		return accessed >= size;
		//return true;
	}

	virtual bool notifyProc() {
		return false;
	}

	virtual void sendingBits(int bits) {
		accessed += min(maxBurst, cols); //assumes always starting at col 0!!!!

		row++;
		if (row >= rows)
			row = 0;

		delete cmd;
		cmd = NULL;
	}
};

class ScheduleItem_ReadData: public ScheduleItem_Mem {
public:
	ScheduleItem_ReadData(ProcessorData* p, int b, int r, int c, int u, int max) :
		ScheduleItem_Mem(MEM_LCL_READ, p, b, r, u, max) {

		startingCol = c;
		lastBurst = 0;
		sent = false;

		cmd = new DRAM_CntrlMsg();
		cmd->setType(Col_Access);
		cmd->setCol((accessed == 0) ? startingCol : 0);
		int bu = min(cols - cmd->getCol(), min(size - accessed, maxBurst));
		cmd->setBurst(bu);
		lastBurst = bu;
		cmd->setWriteEn(false);
		cmd->setBank(bankId);

		MemoryAccess* mem = (MemoryAccess*) procData->getEncapsulatedPacket();
		cmd->setLastAccess(accessed + bu >= size);
		//cmd->setLastAccess(bu * chips * arrays >= mem->getAccessSize());
		cmd->setCreationTime(mem->getCreationTime());
		cmd->setSize(128);
		cmd->setData((long) (mem));

	}

	ScheduleItem_ReadData(SCHEDULE_TYPE t, ProcessorData* p, int b, int r,
			int c, int u, int max) :
		ScheduleItem_Mem(t, p, b, r, u, max) {

		startingCol = c;
		lastBurst = 0;
		sent = false;

		cmd = new DRAM_CntrlMsg();
		cmd->setType(Col_Access);
		cmd->setCol((accessed == 0) ? startingCol : 0);
		int bu = min(cols - cmd->getCol(), min(size - accessed, maxBurst));
		cmd->setBurst(bu);
		lastBurst = bu;
		cmd->setWriteEn(false);
		cmd->setBank(bankId);

		MemoryAccess* mem = (MemoryAccess*) procData->getEncapsulatedPacket();
		cmd->setLastAccess(accessed + bu >= size);
		//cmd->setLastAccess(bu * chips * arrays >= mem->getAccessSize());
		cmd->setCreationTime(mem->getCreationTime());
		cmd->setSize(128);
		cmd->setData((long) (mem));
	}

	virtual ~ScheduleItem_ReadData() {
		//std::cout << "ReadData destructor" << endl;
		if (cmd != NULL)
			delete cmd;
		//delete procData;
	}

	int startingCol; //first col on the first row

	int lastBurst;

	bool sent;

	virtual ProcessorData* getMsg() {
		if (cmd == NULL) {
			cmd = new DRAM_CntrlMsg();
			cmd->setType(Col_Access);
			cmd->setCol((accessed == 0) ? startingCol : 0);
			int b = min(cols - cmd->getCol(), min(size - accessed, maxBurst));
			cmd->setBurst(b);
			lastBurst = b;
			cmd->setWriteEn(false);
			cmd->setBank(bankId);

			MemoryAccess* mem =
					(MemoryAccess*) procData->getEncapsulatedPacket();
			cmd->setLastAccess(accessed + b >= size);
			//cmd->setLastAccess(b * chips * arrays >= mem->getAccessSize());
			cmd->setCreationTime(mem->getCreationTime());
			cmd->setSize(128);
			cmd->setData((long) (mem));
		}

		return cmd;

	}

	virtual int getRingCntrl() {
		int ret = (int) pow(2, 0) | (int) pow(2, 1);
		return ret;
	}

	virtual bool done() {
		return accessed >= size;
		//return true;
	}

	virtual bool notifyProc() {
		return false;
	}

	virtual void sendingBits(int bits) {
		accessed += lastBurst;

		delete cmd;
		cmd = NULL;
	}

	/*virtual ProcessorData* getRescheduleMsg() {

		MemoryAccess* mem = (MemoryAccess*) procData->getEncapsulatedPacket();
		if (lastBurst * chips * arrays < mem->getAccessSize()) {
			mem = (MemoryAccess*) procData->decapsulate();

			ProcessorData* pdata = procData->dup();
			mem->setAccessSize(mem->getAccessSize() - lastBurst * chips
					* arrays);
			pdata->encapsulate(mem);
			return pdata;

		} else
			return NULL;
	}*/

	/*virtual simtime_t getWaitTime() {
	 return 2 * cntrlPeriod + (lastBurst * chips * arrays) / (dataRate
	 * numWavelengths);
	 }*/

};

class ScheduleItem_ReadData_Remote: public ScheduleItem_ReadData {
public:
	ScheduleItem_ReadData_Remote(ProcessorData* p, int b, int r, int c, int u,
			int max) :
		ScheduleItem_ReadData(MEM_REM_READ, p, b, r, c, u, max) {
	}

	virtual ~ScheduleItem_ReadData_Remote() {
		//if (cmd != NULL)
		//	delete cmd;
		//delete procData;
	}

	virtual int getRingCntrl() {
		int ret = pow(2, 1);
		return ret;
	}
};

class ScheduleItem_WriteCmd: public ScheduleItem_Mem {
public:
	ScheduleItem_WriteCmd(ProcessorData* p, int b, int r, int c, int u, int max) :
		ScheduleItem_Mem(MEM_WRITE, p, b, r, u, max) {

		startingCol = c;
		lastBurst = 0;

		sent = false;
	}

	virtual ~ScheduleItem_WriteCmd() {
		if (cmd != NULL)
			delete cmd;
		//delete procData;
	}

	int startingCol;
	int lastBurst;

	bool sent;

	virtual ProcessorData* getMsg() {
		if (cmd == NULL) {
			cmd = new DRAM_CntrlMsg();
			cmd->setType(Col_Access);
			cmd->setCol((accessed == 0) ? startingCol : 0);
			int b = min(cols - cmd->getCol(), min(size - accessed, maxBurst));
			cmd->setBurst(b);
			lastBurst = b;
			cmd->setWriteEn(true);
			cmd->setBank(bankId);
			cmd->setLastAccess(accessed + b >= size);
			cmd->setCreationTime(
					((MemoryAccess*) procData->getEncapsulatedPacket())->getCreationTime());
			cmd->setSize(128);
		}

		return cmd;

	}

	virtual int getRingCntrl() {
		int ret = pow(2, 1);
		return ret;
	}

	virtual bool done() {
		return accessed >= size;
	}

	virtual bool notifyProc() {
		return false;
	}

	virtual void sendingBits(int bits) {
		accessed += lastBurst;

		delete cmd;
		cmd = NULL;
	}

	virtual bool repeat() {
		return true;
	}

	virtual simtime_t getWaitTime() {
		return cntrlPeriod;
	}

};

class ScheduleItem_WriteData: public ScheduleItem_Mem {
public:
	ScheduleItem_WriteData(ProcessorData* p, int b, int r, int c, int u,
			int max) :
		ScheduleItem_Mem(MEM_WRITE_DATA, p, b, r, u, max) {

		col = c;
		lastBurst = 0;
		sent = false;
	}

	virtual ~ScheduleItem_WriteData() {
		if (cmd != NULL)
			delete cmd;
		//delete procData;
	}

	int col;
	int lastBurst;

	bool sent;

	virtual int getRingCntrl() {
		int ret = pow(2, 1);
		return ret;
	}

	virtual bool done() {
		return accessed >= size;
	}

	virtual bool notifyProc() {
		return true;
	}

	virtual void sendingBits(int bits) {
		accessed += lastBurst;
	}

	virtual ProcessorData* getMsg() {
		if (cmd == NULL) {
			cmd = new DRAM_CntrlMsg();
			cmd->setType(Write_Data);
			cmd->setCol((accessed == 0) ? col : 0);
			int b = min(cols - cmd->getCol(), min(size - accessed, maxBurst));
			cmd->setBurst(b);
			lastBurst = b;
			cmd->setWriteEn(true);
			cmd->setBank(bankId);
			cmd->setLastAccess(accessed + b >= size);
			cmd->setCreationTime(
					((MemoryAccess*) procData->getEncapsulatedPacket())->getCreationTime());
			cmd->setSize(b * chips * arrays);
		}

		return cmd;

	}

	virtual int getSendSize() {
		return lastBurst * chips * arrays;
	}

};

#endif
