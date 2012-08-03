#ifndef _ORION_ARRAY_INFO_H
#define _ORION_ARRAY_INFO_H

#include <sys/types.h>
#include "ORION_Config.h"

class ORION_Array_Info {

public:

	int init(int buf_type, int is_fifo, u_int n_read_port, u_int n_write_port,
			u_int n_entry, u_int line_width, int outdrv, ORION_Tech_Config *con);

	/*array type parameter*/
	int arr_buf_type;
	/* parameters shared by data array and tag array */
	int share_rw;
	u_int read_ports;
	u_int write_ports;
	u_int n_set;
	u_int blk_bits;
	u_int assoc;
	int row_dec_model;
	/* parameters specific to data array */
	u_int data_width;
	int col_dec_model;
	int mux_model;
	int outdrv_model;
	/* parameters specific to tag array */
	u_int tag_addr_width;
	u_int tag_line_width;
	int comp_model;
	/* data set of common parameters */
	u_int data_ndwl;
	u_int data_ndbl;
	u_int data_nspd;
	u_int data_n_share_amp;
	u_int data_end;
	int data_wordline_model;
	int data_bitline_model;
	int data_amp_model;
	int data_mem_model;
	/* tag set of common parameters */
	u_int tag_ndwl;
	u_int tag_ndbl;
	u_int tag_nspd;
	u_int tag_n_share_amp;
	u_int tag_end;
	int tag_wordline_model;
	int tag_bitline_model;
	int tag_amp_model;
	int tag_mem_model;
	int tag_attach_mem_model;
	/* parameters for precharging */
	int row_dec_pre_model;
	int col_dec_pre_model;
	int data_bitline_pre_model;
	int tag_bitline_pre_model;
	int data_colsel_pre_model;
	int tag_colsel_pre_model;
	int comp_pre_model;
	/* some redundant fields */
	u_int n_item; /* # of items in a block */
	u_int eff_data_cols; /* # of data columns driven by one wordline */
	u_int eff_tag_cols; /* # of tag columns driven by one wordline */
	/* flags used by prototype array model */
	u_int use_bit_width;
	u_int valid_bit_width;
	int write_policy; /* 1 if write back (have dirty bits), 0 otherwise */
	/* call back functions */
	/* these fields have no physical counterparts, they are
	 * here just because this is the most convenient place */
	u_int (*get_entry_valid_bit)(void*);
	u_int (*get_entry_dirty_bit)(void*);
	u_int (*get_set_use_bit)(void*, int);
	LIB_Type_max_uint (*get_entry_tag)(void*);
	LIB_Type_max_uint (*get_set_tag)(void*, int);
	/* fields which will be filled by initialization */
	double data_arr_width;
	double tag_arr_width;
	double data_arr_height;
	double tag_arr_height;

	ORION_Tech_Config *conf;
};

#endif

