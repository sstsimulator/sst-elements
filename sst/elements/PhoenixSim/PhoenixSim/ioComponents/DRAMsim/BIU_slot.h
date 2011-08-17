#ifndef BIU_SLOT__H
#define BIU_SLOT__H

#include "DRAMaddress.h"
#include "constants.h"


class BIU_slot {

public:

	BIU_slot(){};

	int		status;		/* INVALID, VALID, SCHEDULED, CRITICAL_WORD_RECEIVED or COMPLETED */
	int		thread_id;
	int		rid;		/* request id, or transaction id given by CPU */
	tick_t		start_time;	/* start time of transaction. unit is given in CPU ticks*/
	int		critical_word_ready;	/* critical word ready flag */
	int		callback_done;	/* make sure that callback is only performed once */
	DRAMaddress         address;	/* Entire DRAMaddress structure */
	int		access_type;	/* read/write.. etc */
	int		priority;	/* -1 = most important, BIG_NUM = least important */

	RELEASE_FN_TYPE callback_fn;    /* callback function to simulator */
	void            *mp;





};

#endif


