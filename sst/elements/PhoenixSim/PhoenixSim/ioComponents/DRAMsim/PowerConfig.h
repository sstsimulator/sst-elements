#ifndef POWERCONFIG__H
#define POWERCONFIG__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "Command.h"
#include "DRAM_config.h"
#include "UpdateInterface.h"
#include "PowerMeasure.h"
#include "Memory_Controller.h"


class PowerConfig : public UpdateInterface{

public:

	PowerConfig(DRAM_config *cfg, Memory_Controller** memCntrl);
	
	float max_VDD;
	float min_VDD;
	float	VDD;
	float P_per_DQ;
	int density;
	int DQ;
	int DQS;
	int IDD0;
	int IDD2P;
	int IDD2F;
	int IDD3P;
	int IDD3N;
	int IDD4R;
	int IDD4W;
	int IDD5;
	float t_CK;
	float t_RC;
	float t_RFC_min;
	float t_REFI;
	float VDD_scale;
	float freq_scale; /* Note this needs to be fixed everytime you change the dram frequency*/
	int	print_period;
	int 	chip_count;
	
	float ICC_Idle_0;
	float ICC_Idle_1;
	float ICC_Idle_2;
	float ICC_Active_1;
	float ICC_Active_2;
	float VCC;
	
	/* Pre-calcualted power values */
	float p_PRE_PDN;
	float p_PRE_STBY;
	float p_ACT_PDN;
	float p_ACT_STBY;
	float p_ACT;
	float p_WR;
	float p_RD;
	float p_REF;
	float p_DQ;


	char* pwr_filename;
	FILE* power_file_ptr;
	tick_t	last_print_time;

	DRAM_config *system_config;
	Memory_Controller **dram_controller;



	/******* implemented ***********/

	virtual void update(int);   //from UpdateInterface

	void initialize_power_config();
	void calculate_power_values();

	void mem_initialize_power_collection( char *filename);
	void power_update_freq_based_values(int freq);
	
	void print_power_stats(tick_t now);
	void print_global_power_stats(FILE *fileout, int time);

	void update_amb_power_stats(Command* bundle[], int command_count , tick_t now);
	
	

	void dump_power_config(FILE *stream);
	void do_power_calc(tick_t now, int chan_id, int rank_id, PowerMeasure *meas_ptr, int type);
	

	/********************************/



};

#endif
