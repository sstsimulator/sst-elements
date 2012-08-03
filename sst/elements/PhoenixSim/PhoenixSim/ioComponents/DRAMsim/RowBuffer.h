


class RowBuffer {

public:

	RowBuffer();

	int status; 			/* Three choices. IDLE, OPEN, PENDING */  
	int completion_time;		/* this is used with counters to decide how long to keep a row buffer open */
  	int rank_id;
	int row_id;
  	int bank_id;


};

