#ifndef _ORION_ARRAY_INTERNAL_H
#define _ORION_ARRAY_INTERNAL_H

#include <sys/types.h>
#include "ORION_Config.h"

#define SIM_ARRAY_READ		0
#define SIM_ARRAY_WRITE		1

#define SIM_ARRAY_RECOVER	1

/* read/write */
#define SIM_ARRAY_RW		0
/* only write */
#define SIM_ARRAY_WO		1


/* Used to pass values around the program */
typedef struct {
   int cache_size;
   int number_of_sets;
   int associativity;
   int block_size;
} time_parameter_type;

typedef struct {
   double access_time,cycle_time;
   int best_Ndwl,best_Ndbl;
   int best_Nspd;
   int best_Ntwl,best_Ntbl;
   int best_Ntspd;
   double decoder_delay_data,decoder_delay_tag;
   double dec_data_driver,dec_data_3to8,dec_data_inv;
   double dec_tag_driver,dec_tag_3to8,dec_tag_inv;
   double wordline_delay_data,wordline_delay_tag;
   double bitline_delay_data,bitline_delay_tag;
   double sense_amp_delay_data,sense_amp_delay_tag;
   double senseext_driver_delay_data;
   double compare_part_delay;
   double drive_mux_delay;
   double selb_delay;
   double data_output_delay;
   double drive_valid_delay;
   double precharge_delay;
} time_result_type;


class O_dec {
public:
	int init( int model, u_int n_bits, ORION_Tech_Config* con );
	int record(LIB_Type_max_uint prev_addr, LIB_Type_max_uint curr_addr);
	double report();
	int clear_stat();
	double select_cap( u_int n_input );
	double chgaddr_cap( u_int n_gates );
	double chgl1_cap( u_int n_in_1st, u_int n_in_2nd, u_int n_gates );


	int model;
	u_int n_bits;
	LIB_Type_max_uint n_chg_output;
	LIB_Type_max_uint n_chg_addr;
	LIB_Type_max_uint n_chg_l1;
	double e_chg_output;
	double e_chg_addr;
	double e_chg_l1;
	u_int n_in_1st;
	u_int n_in_2nd;
	u_int n_out_0th;
	u_int n_out_1st;
	/* redundant field */
	LIB_Type_max_uint addr_mask;

	ORION_Tech_Config *conf;
};

class O_wordline {
public:
	int init( int model, int share_rw, u_int cols, double wire_cap, u_int end, ORION_Tech_Config* con);
	int record(int rw, LIB_Type_max_uint n_switch );
	double report();
	int clear_stat();

	double cap( u_int cols, double wire_cap, double tx_width );
	double cam_cap( u_int cols, double wire_cap, double tx_width );

	int model;
	int share_rw;
	LIB_Type_max_uint n_read;
	LIB_Type_max_uint n_write;
	double e_read;
	double e_write;
	double I_static;

	ORION_Tech_Config *conf;
};

class O_bitline {
public:



	int init( int model, int share_rw, u_int end, u_int rows, double wire_cap,
			u_int n_share_amp, u_int n_bitline_pre,
			u_int n_colsel_pre, double pre_size, int outdrv_model, ORION_Tech_Config* con);
	int record( int rw, u_int cols,  LIB_Type_max_uint new_value);
	int record( int rw, u_int cols, LIB_Type_max_uint old_value, LIB_Type_max_uint new_value);  //non state saving version
	double report();
	int clear_stat();

	double column_read_cap(u_int rows, double wire_cap, u_int end, u_int n_share_amp, u_int n_bitline_pre, u_int n_colsel_pre, double pre_size, int outdrv_model);
	double column_select_cap( void );
	double column_write_cap( u_int rows, double wire_cap );
	double share_column_write_cap( u_int rows, double wire_cap, u_int n_share_amp, u_int n_bitline_pre, double pre_size );
	double share_column_read_cap( u_int rows, double wire_cap, u_int n_share_amp, u_int n_bitline_pre, u_int n_colsel_pre, double pre_size );
	int is_rw_bitline( );

	LIB_Type_max_uint curr_val;

	int model;
	int share_rw;
	u_int end;
	LIB_Type_max_uint n_col_write;
	LIB_Type_max_uint n_col_read;
	LIB_Type_max_uint n_col_sel;
	double e_col_write;
	double e_col_read;
	double e_col_sel;
	double I_static;

	ORION_Tech_Config *conf;
};

class O_amp {
public:
	int init( int m, ORION_Tech_Config* con);
	int record(u_int cols);
	double report();
	int clear_stat();

	double energy( void );


	int model;
	LIB_Type_max_uint n_access;
	double e_access;

	ORION_Tech_Config *conf;
} ;

class O_comp {
public:
	int init(int m, u_int bits, u_int ass, u_int n_pre, double matchline_len, double tagline_len, ORION_Tech_Config* con);
	int local_record(  LIB_Type_max_uint prev_tag, LIB_Type_max_uint curr_tag, LIB_Type_max_uint input, int recover );
	int global_record(  LIB_Type_max_uint prev_value, LIB_Type_max_uint curr_value, int miss );
	double report();
	int clear_stat();

	double cam_miss_cap( u_int assoc );
	double cam_mismatch_cap( u_int n_bits, u_int n_pre, double matchline_len );
	double cam_tagline_cap( u_int rows, double taglinelength );

	double base_cap( void );
	double match_cap( );
	double mismatch_cap( u_int n_pre );
	double miss_cap( u_int assoc );
	double bit_match_cap( void );
	double bit_mismatch_cap( void );
	double chgaddr_cap( void );


	int model;
	u_int n_bits;
	u_int assoc;
	LIB_Type_max_uint n_access;
	LIB_Type_max_uint n_match;
	LIB_Type_max_uint n_mismatch;
	LIB_Type_max_uint n_miss;
	LIB_Type_max_uint n_bit_match;
	LIB_Type_max_uint n_bit_mismatch;
	LIB_Type_max_uint n_chg_addr;
	double e_access;
	double e_match;
	double e_mismatch;
	double e_miss;
	double e_bit_match;
	double e_bit_mismatch;
	double e_chg_addr;
	/* redundant field */
	LIB_Type_max_uint comp_mask;

	ORION_Tech_Config *conf;
};



class O_mux {
public:
	int init(int m, u_int n_gates, u_int ass, ORION_Tech_Config* con);
	int record( LIB_Type_max_uint prev_addr, LIB_Type_max_uint curr_addr, int miss);
	double report();
	int clear_stat();

	double mismatch_cap( u_int n_nor_gates );
	double chgaddr_cap( void );


	int model;
	u_int assoc;
	LIB_Type_max_uint n_mismatch;
	LIB_Type_max_uint n_chg_addr;
	double e_mismatch;
	double e_chg_addr;

	ORION_Tech_Config *conf;
};

class O_out {
public:
	int init(int m, u_int item, ORION_Tech_Config* con);
	int local_record(  LIB_Type_max_uint prev_data, LIB_Type_max_uint curr_data, int recover );
	int global_record( LIB_Type_max_uint data );
	double report();
	int clear_stat();

	double outdata_cap( u_int value );
	double chgdata_cap( void );
	double select_cap( u_int data_width );


	int model;
	u_int item_width;
	LIB_Type_max_uint n_select;
	LIB_Type_max_uint n_chg_data;
	LIB_Type_max_uint n_out_1;
	LIB_Type_max_uint n_out_0;
	double e_select;
	double e_chg_data;
	double e_out_1;
	double e_out_0;
	/* redundant field */
	LIB_Type_max_uint out_mask;

	ORION_Tech_Config *conf;
};

class O_mem {
public:
	void init( int m, u_int read_ports, u_int write_ports, int share_rw, u_int e, ORION_Tech_Config* con);
	int record(  LIB_Type_max_uint new_value, u_int width);
	int record( LIB_Type_max_uint old_value, LIB_Type_max_uint new_value, u_int width); //non state tracking version
	double report();
	int clear_stat();

	double cap( u_int read_ports, u_int write_ports, int share_rw, u_int e );

	double cam_tag_cap( u_int read_ports, u_int write_ports, int share_rw, u_int end, int only_write );
	double cam_data_cap( u_int read_ports, u_int write_ports );

	LIB_Type_max_uint curr_data;

	int model;
	u_int end;
	LIB_Type_max_uint n_switch;
	double e_switch;
	double I_static;

	ORION_Tech_Config *conf;
} ;

class O_arr_pre{
public:
	int init(int m, double pre_siz, ORION_Tech_Config* con);
	int record( LIB_Type_max_uint n_charge );
	double report();
	int clear_stat();

	double cap( double width, double length );

	/* return # of precharging gates per column */
	static u_int n_pre_gate( int model );

	/* return # of precharging drains per line */
	static u_int n_pre_drain( int model );


	int model;
	LIB_Type_max_uint n_charge;
	double e_charge;
	double I_static;

	ORION_Tech_Config *conf;
};






#endif
