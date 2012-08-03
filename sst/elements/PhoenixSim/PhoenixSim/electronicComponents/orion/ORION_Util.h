#ifndef _ORION_UTIL_H
#define _ORION_UTIL_H

#include <sys/types.h>

#include "ORION_Config.h"


/* maximum level of selection arbiter tree */
#define MAX_SEL_LEVEL	5


/***************/




/*
 * FIXME: cannot handle bus wider than 64-bit
 */
class ORION_Bus {
public:

	int init( int m, int enc, u_int width, u_int grp_width,
			u_int n_snd, u_int n_rcv, double length, double time, ORION_Tech_Config *conf);
	double report();
	int record(LIB_Type_max_uint new_data);

	int bitwidth(int encoding, u_int data_width, u_int grp_width);
	LIB_Type_max_uint state(  LIB_Type_max_uint new_data);  //converts new data into a new state, but does NOT update the current
	double resultbus_cap(void);
	double generic_bus_cap(u_int n_snd, u_int n_rcv, double length, double time);


	LIB_Type_max_uint curr_data;
	LIB_Type_max_uint curr_state;

	int model;
	int encoding;
	u_int data_width;
	u_int grp_width;
	LIB_Type_max_uint n_switch;
	double e_switch;
	/* redundant field */
	u_int bit_width;
	LIB_Type_max_uint bus_mask;

	ORION_Tech_Config *conf;
};

class ORION_Sel {
public:

	int init( int m, u_int w, ORION_Tech_Config *conf);
	int record( u_int req, int en, int last_level);
	double report();

	double anyreq_cap(u_int width);
	double chgreq_cap(void);
	double enc_cap(u_int level);
	double grant_cap(u_int width);


	int model;
	u_int width;
	LIB_Type_max_uint n_anyreq;
	LIB_Type_max_uint n_chgreq;
	LIB_Type_max_uint n_grant;
	LIB_Type_max_uint n_enc[MAX_SEL_LEVEL];
	double e_anyreq;
	double e_chgreq;
	double e_grant;
	double e_enc[MAX_SEL_LEVEL];

	ORION_Tech_Config *conf;
};

class ORION_FF {
public:
	int init( int m, double load, ORION_Tech_Config *conf);
	double report();
	double report_static_power();
	int clear_stat();

	double node_cap(u_int fan_in, u_int fan_out);
	double clock_cap(void);


	int model;
	LIB_Type_max_uint n_switch;
	LIB_Type_max_uint n_keep_1;
	LIB_Type_max_uint n_keep_0;
	LIB_Type_max_uint n_clock;
	double e_switch;
	double e_keep_1;
	double e_keep_0;
	double e_clock;
	double I_static;

	ORION_Tech_Config *conf;
};

/* transmission gate type */
typedef enum {
	N_GATE,
	NP_GATE
} SIM_power_trans_t;



int ORION_Util_init(void);

u_int ORION_Util_Hamming(LIB_Type_max_uint old_val, LIB_Type_max_uint new_val, LIB_Type_max_uint mask);
u_int ORION_Util_Hamming_group(LIB_Type_max_uint d1_new, LIB_Type_max_uint d1_old, LIB_Type_max_uint d2_new, LIB_Type_max_uint d2_old, u_int width, u_int n_grp);
u_int ORION_Util_Hamming_slow( LIB_Type_max_uint old_val, LIB_Type_max_uint new_val, LIB_Type_max_uint mask );

/* some utility routines */
u_int ORION_Util_logtwo(LIB_Type_max_uint x);
int ORION_Util_squarify(int rows, int cols);
double ORION_Util_driver_size(double driving_cap, double desiredrisetime, ORION_Tech_Config *conf);


/* functions from cacti */
double ORION_Util_gatecap(double width, double wirelength, ORION_Tech_Config *conf);
double ORION_Util_gatecappass(double width, double wirelength, ORION_Tech_Config *conf);
double ORION_Util_draincap(double width, int nchannel, int stack, ORION_Tech_Config *conf);
double ORION_Util_restowidth(double res, int nchannel, ORION_Tech_Config *conf);




/* statistical functions */
int ORION_Util_print_stat_energy(char *path, double Energy, int print_flag);
u_int ORION_Util_strlen(char *s);
char *ORION_Util_strcat(char *dest, char *src);
int ORION_Util_res_path(char *path, u_int id);
int ORION_Util_dump_tech_para(void);


#endif

