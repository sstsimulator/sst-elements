#ifndef _ORION_CROSSBAR_H
#define _ORION_CROSSBAR_H

#include <malloc.h>

#include "ORION_Config.h"
#include "ORION_Util.h"

class ORION_Crossbar {
public:

	~ORION_Crossbar() {
		free(in_data);
		free(out_data);
		free(out_ports);
	}

	int record(int io, LIB_Type_max_uint new_data, u_int in_port,
			u_int out_port);
	int record(int io, u_int in_port, u_int out_port, int size); //assumes 0.5 switching probability
	int init(int model, u_int n_in, u_int n_out, u_int in_seg, u_int out_seg,
			u_int data_width, u_int degree, int connect_type, int trans_type,
			double in_len, double out_len, ORION_Tech_Config *con);
	double stat_energy(int max_avg);
	double report();
	double report_static_power();

	double in_cap(double wire_cap, u_int n_out, double n_seg, int connect_type,
			int trans_type, double *Nsize);
	double out_cap(double length, u_int n_in, double n_seg, int connect_type,
			int trans_type, double *Nsize);
	double io_cap(double length);
	double int_cap(u_int degree, int connect_type, int trans_type);
	double ctr_cap(double length, u_int data_width, int prev_ctr, int next_ctr,
			u_int degree, int connect_type, int trans_type);

	int model;
	u_int n_in;
	u_int n_out;
	u_int in_seg; /* only used by segmented crossbar */
	u_int out_seg; /* only used by segmented crossbar */
	u_int data_width;
	u_int degree; /* only used by multree crossbar */
	int connect_type;
	int trans_type; /* only used by transmission gate connection */
	LIB_Type_max_uint n_chg_in;
	LIB_Type_max_uint n_chg_int;
	LIB_Type_max_uint n_chg_out;
	LIB_Type_max_uint n_chg_ctr;
	double e_chg_in;
	double e_chg_int;
	double e_chg_out;
	double e_chg_ctr;
	/* redundant field */
	u_int depth; /* only used by multree crossbar */
	LIB_Type_max_uint mask;
	double I_static;

	LIB_Type_max_uint *in_data;
	LIB_Type_max_uint *out_data;
	u_int *out_ports; //what input ports they're connected to

	ORION_Tech_Config *conf;
};

#endif
