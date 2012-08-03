#ifndef _ORION_ARBITER_H
#define _ORION_ARBITER_H

#include "ORION_Array.h"
#include "ORION_Util.h"

/* carry_in is the internal carry transition node */
class ORION_Arbiter {
public:
	int init(int arbiter_model, int ff_model, u_int req_width, double length, ORION_Tech_Config *con);
	int record(LIB_Type_max_uint new_req, u_int new_grant);
	double report();
	double report_static_power();

	double stat_energy(int max_avg);


	double rr_req_cap(double length);
	double rr_pri_cap();
	double rr_grant_cap();
	double rr_carry_cap();
	double rr_carry_in_cap();
	double matrix_req_cap(u_int req_width, double length);
	double matrix_pri_cap(u_int req_width);
	double matrix_grant_cap(u_int req_width);
	double matrix_int_cap();
	int clear_stat();

	int model;
	u_int req_width;
	LIB_Type_max_uint n_chg_req;
	LIB_Type_max_uint n_chg_grant;
	/* internal node of round robin arbiter */
	LIB_Type_max_uint n_chg_carry;
	LIB_Type_max_uint n_chg_carry_in;
	/* internal node of matrix arbiter */
	LIB_Type_max_uint n_chg_mint;
	double e_chg_req;
	double e_chg_grant;
	double e_chg_carry;
	double e_chg_carry_in;
	double e_chg_mint;
	/* priority flip flop */
	ORION_FF pri_ff;
	/* request queue */
	ORION_Array queue;
	/* redundant field */
	LIB_Type_max_uint mask;

	LIB_Type_max_uint curr_req;
	LIB_Type_max_uint curr_grant;


	double I_static;

	ORION_Bus arb_bus;

	ORION_Tech_Config *conf;
};

#endif

