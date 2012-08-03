


class Bus {

public:
	Bus(){completion_time = 0;};


	int		status;		/* IDLE, BUSY */
	tick_t		completion_time;/* time when the bus will be free */
	int		current_rank_id;
	int		command;	/* if this is a command bus, the command on this bus now */

};

