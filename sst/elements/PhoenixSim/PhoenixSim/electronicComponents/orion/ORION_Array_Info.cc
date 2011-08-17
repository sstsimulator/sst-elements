#include "ORION_Array_Info.h"

int ORION_Array_Info::init(int buf_type, int is_fifo, u_int n_read_port,
		u_int n_write_port, u_int n_entry, u_int line_width, int outdrv,
		ORION_Tech_Config* con) {
	/* ==================== set parameters ==================== */

	conf = con;

	arr_buf_type = buf_type;

	/* general parameters */
	share_rw = 0;
	read_ports = n_read_port;
	write_ports = n_write_port;
	n_set = n_entry;
	blk_bits = line_width;
	assoc = 1;
	data_width = line_width;
	data_end = conf->PARM("data_end");

	/* no sub-array partition */
	data_ndwl = 1;
	data_ndbl = 1;
	data_nspd = 1;

	data_n_share_amp = 1;

	/* model parameters */
	if (is_fifo) {
		row_dec_model = SIM_NO_MODEL;
		row_dec_pre_model = SIM_NO_MODEL;
	} else {
		row_dec_model = conf->PARM("row_dec_model");
		row_dec_pre_model = conf->PARM("row_dec_pre_model");
	}
	data_wordline_model = conf->PARM("wordline_model");
	data_bitline_model = conf->PARM("bitline_model");
	data_bitline_pre_model = conf->PARM("bitline_pre_model");
	data_mem_model = conf->PARM("mem_model");
	if (conf->PARM("data_end") == 2) {
		data_amp_model = conf->PARM("amp_model");
	} else {
		data_amp_model = SIM_NO_MODEL;
	} /* PARM(data_end) == 2 */
	if (outdrv)
		outdrv_model = conf->PARM("outdrv_model");
	else
		outdrv_model = SIM_NO_MODEL;

	data_colsel_pre_model = SIM_NO_MODEL;
	col_dec_model = SIM_NO_MODEL;
	col_dec_pre_model = SIM_NO_MODEL;
	mux_model = SIM_NO_MODEL;

	/* FIXME: not true for shared buffer */
	/* no tag array */
	tag_wordline_model = SIM_NO_MODEL;
	tag_bitline_model = SIM_NO_MODEL;
	tag_bitline_pre_model = SIM_NO_MODEL;
	tag_mem_model = SIM_NO_MODEL;
	tag_attach_mem_model = SIM_NO_MODEL;
	tag_amp_model = SIM_NO_MODEL;
	tag_colsel_pre_model = SIM_NO_MODEL;
	comp_model = SIM_NO_MODEL;
	comp_pre_model = SIM_NO_MODEL;

	/* ==================== set flags ==================== */
	write_policy = 0; /* no dirty bit */

	/* ==================== compute redundant fields ==================== */
	n_item = blk_bits / data_width;
	eff_data_cols = blk_bits * assoc * data_nspd;

	/* ==================== call back functions ==================== */
	get_entry_valid_bit = NULL;
	get_entry_dirty_bit = NULL;
	get_entry_tag = NULL;
	get_set_tag = NULL;
	get_set_use_bit = NULL;

	/* initialize state variables */
	//if (read_port) SIM_buf_port_state_init(info, read_port, read_ports);
	//if (write_port) SIM_buf_port_state_init(info, write_port, write_ports);

	return 0;
}
