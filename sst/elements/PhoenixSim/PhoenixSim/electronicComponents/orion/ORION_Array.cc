/*
 * FIXME: (2) can I use u_int in .xml file?
 * 	  (3) address decomposition does not consider column selector and sub-array now
 *
 * NOTES: (1) power stats function should not be called before its functional counterpart
 *            except data_write
 *
 * TODO:  (2) How to deal with RF's like AH/AL/AX?  Now I assume output with 0 paddings.
 *        (1) make _dec handle both row dec and col dec, no wordline
 *        (3) move wordline stats to _read and _write
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "ORION_Array.h"

/* local macros */
#define IS_DIRECT_MAP( info )		((info)->assoc == 1)
#define IS_FULLY_ASSOC( info )		((info)->n_set == 1 && (info)->assoc > 1)
#define IS_WRITE_THROUGH( info )	(! (info)->write_policy)
#define IS_WRITE_BACK( info )		((info)->write_policy)

/* sufficient (not necessary) condition */
#define HAVE_TAG( info )		((info)->tag_mem_model)
#define HAVE_USE_BIT( info )		((info)->use_bit_width)
#define HAVE_COL_DEC( info )		((info)->col_dec_model)
#define HAVE_COL_MUX( info )		((info)->mux_model)

void bzero(char *parg, int c) {
	char *p = (char *) parg;

	while (c-- != 0)
		*p++ = 0;
}

int ORION_Array::init(ORION_Array_Info *i) {
	u_int rows, cols, ports, dec_width, n_bitline_pre, n_colsel_pre;
	double wordline_len, bitline_len, tagline_len, matchline_len;
	double wordline_cmetal, bitline_cmetal;
	double Cline, pre_size, comp_pre_size;

	info = i;

	I_static = 0;

	if (info->arr_buf_type == SRAM) {
		/* sanity check */
		if (info->read_ports == 0)
			info->share_rw = 0;
		if (info->share_rw) {
			info->data_end = 2;
			info->tag_end = 2;
		}

		if (info->share_rw)
			ports = info->read_ports;
		else
			ports = info->read_ports + info->write_ports;

		/* data array unit length wire cap */
		if (ports > 1) {
			/* 3x minimal spacing */
			wordline_cmetal = info->conf->PARM("CC3M3metal");
			bitline_cmetal = info->conf->PARM("CC3M2metal");
		} else if (info->data_end == 2) {
			/* wordline infinite spacing, bitline 3x minimal spacing */
			wordline_cmetal = info->conf->PARM("CM3metal");
			bitline_cmetal = info->conf->PARM("CC3M2metal");
		} else {
			/* both infinite spacing */
			wordline_cmetal = info->conf->PARM("CM3metal");
			bitline_cmetal = info->conf->PARM("CM2metal");
		}

		info->data_arr_width = 0;
		info->tag_arr_width = 0;
		info->data_arr_height = 0;
		info->tag_arr_height = 0;

		/* BEGIN: data array power initialization */
		if ((dec_width = ORION_Util_logtwo(info->n_set))) {
			/* row decoder power initialization */
			row_dec.init(info->row_dec_model, dec_width, info->conf);

			/* row decoder precharging power initialization */
			//if ( is_dynamic_dec( info->row_dec_model ))
			/* FIXME: need real pre_size */
			//SIM_array_pre_init( &row_dec_pre, info->row_dec_pre_model, 0 );

			rows = info->n_set / info->data_ndbl / info->data_nspd;
			cols = info->blk_bits * info->assoc * info->data_nspd
					/ info->data_ndwl;

			bitline_len = rows * (info->conf->PARM("RegCellHeight") + ports * info->conf->PARM("WordlineSpacing"));
			if (info->data_end == 2)
				wordline_len = cols * (info->conf->PARM("RegCellWidth") + 2 * ports
						* info->conf->PARM("BitlineSpacing"));
			else
				/* info->data_end == 1 */
				wordline_len = cols * (info->conf->PARM("RegCellWidth")
				+ (2 * ports - info->read_ports) * info->conf->PARM("BitlineSpacing"));
			info->data_arr_width = wordline_len;
			info->data_arr_height = bitline_len;

			/* compute precharging size */
			/* FIXME: should consider n_pre and pre_size simultaneously */
			Cline = rows * ORION_Util_draincap(info->conf->PARM("Wmemcellr"), NCH, 1, info->conf)
					+ bitline_cmetal * bitline_len;
			pre_size = ORION_Util_driver_size(Cline, info->conf->PARM("Period") / 8, info->conf);
			/* WHS: ?? compensate for not having an nmos pre-charging */
			pre_size += pre_size * info->conf->PARM("Wdecinvn") / info->conf->PARM("Wdecinvp");

			/* bitline power initialization */
			n_bitline_pre
					= O_arr_pre::n_pre_drain(info->data_bitline_pre_model);
			n_colsel_pre
					= (info->data_n_share_amp > 1) ? O_arr_pre::n_pre_drain(
							info->data_colsel_pre_model) : 0;
			data_bitline.init(info->data_bitline_model, info->share_rw,
					info->data_end, rows, bitline_len * bitline_cmetal,
					info->data_n_share_amp, n_bitline_pre, n_colsel_pre,
					pre_size, info->outdrv_model, info->conf);
			/* static power */
			//std::cout << "Istatic: " << I_static << endl;
			I_static += data_bitline.I_static * cols * info->write_ports;
			//std::cout << "bitline: " << I_static << endl;

			/* bitline precharging power initialization */
			data_bitline_pre.init(info->data_bitline_pre_model, pre_size, info->conf);
			/* static power */
			I_static += data_bitline_pre.I_static * cols * info->read_ports;
			//std::cout << "bitline_pre: " << I_static << endl;
			/* bitline column selector precharging power initialization */
			if (info->data_n_share_amp > 1)
				data_colsel_pre.init(info->data_colsel_pre_model, pre_size, info->conf);

			/* sense amplifier power initialization */
			data_amp.init(info->data_amp_model, info->conf);
		} else {
			/* info->n_set == 1 means this array is fully-associative */
			rows = info->assoc;
			cols = info->blk_bits;

			/* WHS: no read wordlines or bitlines */
			bitline_len = rows * (info->conf->PARM("RegCellHeight") + info->write_ports
					* info->conf->PARM("WordlineSpacing"));
			wordline_len = cols * (info->conf->PARM("RegCellWidth") + 2 * info->write_ports
					* info->conf->PARM("BitlineSpacing"));
			info->data_arr_width = wordline_len;
			info->data_arr_height = bitline_len;

			/* bitline power initialization */
			data_bitline.init(info->data_bitline_model, 0, info->data_end,
					rows, bitline_len * bitline_cmetal, 1, 0, 0, 0,
					info->outdrv_model, info->conf);
		}

		//if(I_static > 1){
		//	std::cout << "I_static : " << I_static << "datamem: " << data_mem.I_static << " rows: " << rows << " cols: " << cols << endl;

		//	opp_error("1");
		//}

		/* wordline power initialization */
		data_wordline.init(info->data_wordline_model, info->share_rw, cols,
				wordline_len * wordline_cmetal, info->data_end, info->conf);
		/* static power */
		//std::cout << "before wordline: " << I_static << endl;
		I_static += data_wordline.I_static * rows * ports;

		//std::cout << "after wordline: " << I_static << endl;

		if ((dec_width = ORION_Util_logtwo(info->n_item))) {
			/* multiplexor power initialization */
			mux.init(info->mux_model, info->n_item, info->assoc, info->conf);

			/* column decoder power initialization */
			col_dec.init(info->col_dec_model, dec_width, info->conf);

			/* column decoder precharging power initialization */
			//if ( is_dynamic_dec( info->col_dec_model ))
			/* FIXME: need real pre_size */
			//SIM_array_pre_init( &col_dec_pre, info->col_dec_pre_model, 0 );
		}

		/* memory cell power initialization */

		//std::cout << "data_mem: " << data_mem.I_static << endl;
		data_mem.init(info->data_mem_model, info->read_ports,
				info->write_ports, info->share_rw, info->data_end, info->conf);
		//std::cout << "data_mem2: " << data_mem.I_static << endl;
		/* static power */
		I_static += data_mem.I_static * rows * cols;

		//std::cout << "I_static : " << I_static << " ports: " << ports << " dataword: " << data_wordline.I_static << " datamem: " << data_mem.I_static << " rows: " << rows << " cols: " << cols << endl;


		//if(I_static > 1){
		//	std::cout << "I_static : " << I_static << " ports: " << ports << " dataword: " << data_wordline.I_static << " datamem: " << data_mem.I_static << " rows: " << rows << " cols: " << cols << endl;
		//		opp_error("2");
		//	}

		/* output driver power initialization */
		outdrv.init(info->outdrv_model, info->data_width, info->conf);
		/* END: data array power initialization */

		/* BEGIN: legacy */
		//GLOB(e_const)[CACHE_PRECHARGE] = ndwl * ndbl * SIM_array_precharge_power( PARM( n_pre_gate ), pre_size, 10 ) * cols * ((( n_share_amp > 1 ) ? ( 1.0 / n_share_amp ):0 ) + 1 );
		/* END: legacy */

		/* BEGIN: tag array power initialization */
		/* assume a tag array must have memory cells */
		if (info->tag_mem_model) {
			if (info->n_set > 1) {
				/* tag array unit length wire cap */
				if (ports > 1) {
					/* 3x minimal spacing */
					wordline_cmetal = info->conf->PARM("CC3M3metal");
					bitline_cmetal = info->conf->PARM("CC3M2metal");
				} else if (info->data_end == 2) {
					/* wordline infinite spacing, bitline 3x minimal spacing */
					wordline_cmetal = info->conf->PARM("CM3metal");
					bitline_cmetal = info->conf->PARM("CC3M2metal");
				} else {
					/* both infinite spacing */
					wordline_cmetal = info->conf->PARM("CM3metal");
					bitline_cmetal = info->conf->PARM("CM2metal");
				}

				rows = info->n_set / info->tag_ndbl / info->tag_nspd;
				cols = info->tag_line_width * info->assoc * info->tag_nspd
						/ info->tag_ndwl;

				bitline_len = rows * (info->conf->PARM("RegCellHeight") + ports * info->conf->PARM("WordlineSpacing"));
				if (info->tag_end == 2)
					wordline_len = cols * (info->conf->PARM("RegCellWidth") + 2 * ports
							* info->conf->PARM("BitlineSpacing"));
				else
					/* info->tag_end == 1 */
					wordline_len = cols * (info->conf->PARM("RegCellWidth") + (2 * ports
							- info->read_ports) * info->conf->PARM("BitlineSpacing"));
				info->tag_arr_width = wordline_len;
				info->tag_arr_height = bitline_len;

				/* compute precharging size */
				/* FIXME: should consider n_pre and pre_size simultaneously */
				Cline = rows * ORION_Util_draincap(info->conf->PARM("Wmemcellr"), NCH, 1, info->conf)
						+ bitline_cmetal * bitline_len;
				pre_size = ORION_Util_driver_size(Cline, info->conf->PARM("Period") / 8, info->conf);
				/* WHS: ?? compensate for not having an nmos pre-charging */
				pre_size += pre_size * info->conf->PARM("Wdecinvn") / info->conf->PARM("Wdecinvp");

				/* bitline power initialization */
				n_bitline_pre = O_arr_pre::n_pre_drain(
						info->tag_bitline_pre_model);
				n_colsel_pre
						= (info->tag_n_share_amp > 1) ? O_arr_pre::n_pre_drain(
								info->tag_colsel_pre_model) : 0;
				tag_bitline.init(info->tag_bitline_model, info->share_rw,
						info->tag_end, rows, bitline_len * bitline_cmetal,
						info->tag_n_share_amp, n_bitline_pre, n_colsel_pre,
						pre_size, SIM_NO_MODEL, info->conf);

				/* bitline precharging power initialization */
				tag_bitline_pre.init(info->tag_bitline_pre_model, pre_size, info->conf);
				/* bitline column selector precharging power initialization */
				if (info->tag_n_share_amp > 1)
					tag_colsel_pre.init(info->tag_colsel_pre_model, pre_size, info->conf);

				/* sense amplifier power initialization */
				tag_amp.init(info->tag_amp_model, info->conf);

				/* prepare for comparator initialization */
				tagline_len = matchline_len = 0;
				comp_pre_size = info->conf->PARM("Wcomppreequ");
			} else { /* info->n_set == 1 */
				/* cam cells are big enough, so infinite spacing */
				wordline_cmetal = info->conf->PARM("CM3metal");
				bitline_cmetal = info->conf->PARM("CM2metal");

				rows = info->assoc;
				/* FIXME: operations of valid bit, use bit and dirty bit are not modeled */
				cols = info->tag_addr_width;

				bitline_len = rows * (info->conf->PARM("CamCellHeight") + ports * info->conf->PARM("WordlineSpacing")
						+ info->read_ports * info->conf->PARM("MatchlineSpacing"));
				if (info->tag_end == 2)
					wordline_len = cols * (info->conf->PARM("CamCellWidth") + 2 * ports
							* info->conf->PARM("BitlineSpacing") + 2 * info->read_ports
							* info->conf->PARM("TaglineSpacing"));
				else
					/* info->tag_end == 1 */
					wordline_len = cols * (info->conf->PARM("CamCellWidth") + (2 * ports
							- info->read_ports) * info->conf->PARM("BitlineSpacing") + 2
							* info->read_ports * info->conf->PARM("TaglineSpacing"));
				info->tag_arr_width = wordline_len;
				info->tag_arr_height = bitline_len;

				if (tag_bitline.is_rw_bitline()) {
					/* compute precharging size */
					/* FIXME: should consider n_pre and pre_size simultaneously */
					Cline = rows * ORION_Util_draincap(info->conf->PARM("Wmemcellr"), NCH, 1, info->conf)
							+ bitline_cmetal * bitline_len;
					pre_size = ORION_Util_driver_size(Cline, info->conf->PARM("Period") / 8, info->conf);
					/* WHS: ?? compensate for not having an nmos pre-charging */
					pre_size += pre_size * info->conf->PARM("Wdecinvn") / info->conf->PARM("Wdecinvp");

					/* bitline power initialization */
					n_bitline_pre = O_arr_pre::n_pre_drain(
							info->tag_bitline_pre_model);
					tag_bitline.init(info->tag_bitline_model, info->share_rw,
							info->tag_end, rows, bitline_len * bitline_cmetal,
							1, n_bitline_pre, 0, pre_size, SIM_NO_MODEL, info->conf);

					/* bitline precharging power initialization */
					tag_bitline_pre.init(info->tag_bitline_pre_model, pre_size, info->conf);

					/* sense amplifier power initialization */
					tag_amp.init(info->tag_amp_model, info->conf);
				} else {
					/* bitline power initialization */
					tag_bitline.init(info->tag_bitline_model, 0, info->tag_end,
							rows, bitline_len * bitline_cmetal, 1, 0, 0, 0,
							SIM_NO_MODEL, info->conf);
				}

				/* memory cell power initialization */
				tag_attach_mem.init(info->tag_attach_mem_model,
						info->read_ports, info->write_ports, info->share_rw,
						info->tag_end, info->conf);

				/* prepare for comparator initialization */
				tagline_len = bitline_len;
				matchline_len = wordline_len;
				comp_pre_size = info->conf->PARM("Wmatchpchg");
			}

			/* wordline power initialization */
			tag_wordline.init(info->tag_wordline_model, info->share_rw, cols,
					wordline_len * wordline_cmetal, info->tag_end, info->conf);

			/* comparator power initialization */
			comp.init(info->comp_model, info->tag_addr_width, info->assoc,
					O_arr_pre::n_pre_drain(info->comp_pre_model),
					matchline_len, tagline_len, info->conf);

			/* comparator precharging power initialization */
			comp_pre.init(info->comp_pre_model, comp_pre_size, info->conf);

			/* memory cell power initialization */
			tag_mem.init(info->tag_mem_model, info->read_ports,
					info->write_ports, info->share_rw, info->tag_end, info->conf);
		}
		/* END: tag array power initialization */

		/* BEGIN: legacy */
		/* bitline precharging energy */
		//GLOB(e_const)[CACHE_PRECHARGE] += ntwl * ntbl * SIM_array_precharge_power( PARM( n_pre_gate ), pre_size, 10 ) * cols * ((( n_share_amp > 1 ) ? ( 1.0 / n_share_amp ):0 ) + 1 );
		/* comparator precharging energy */
		//GLOB(e_const)[CACHE_PRECHARGE] += LPARM_cache_associativity * SIM_array_precharge_power( 1, Wcomppreequ, 1 );
		/* END: legacy */

	} else if (info->arr_buf_type == REGISTER) {
		ff.init(NEG_DFF, 0, info->conf);
		I_static = ff.I_static * info->data_width * info->n_set;
	}

	//if(I_static > 1){
	//		opp_error("23");
	//	}

	return 0;
}

double ORION_Array::report() {
	double epart, etotal = 0;

	if (info->row_dec_model) {
		epart = row_dec.report();
		//fprintf(stderr, "row decoder: %g\n", epart);
		etotal += epart;
	}
	if (info->col_dec_model) {
		epart = col_dec.report();
		//fprintf(stderr, "col decoder: %g\n", epart);
		etotal += epart;
	}
	if (info->data_wordline_model) {
		epart = data_wordline.report();
		//fprintf(stderr, "data wordline: %g\n", epart);
		etotal += epart;
	}
	if (info->tag_wordline_model) {
		epart = tag_wordline.report();
		//fprintf(stderr, "tag wordline: %g\n", epart);
		etotal += epart;
	}
	if (info->data_bitline_model) {
		epart = data_bitline.report();
		//fprintf(stderr, "data bitline: %g\n", epart);
		etotal += epart;
	}
	if (info->data_bitline_pre_model) {
		epart = data_bitline_pre.report();
		//fprintf(stderr, "data bitline precharge: %g\n", epart);
		etotal += epart;
	}
	if (info->tag_bitline_model) {
		epart = tag_bitline.report();
		//fprintf(stderr, "tag bitline: %g\n", epart);
		etotal += epart;
	}
	if (info->data_mem_model) {
		epart = data_mem.report();
		//fprintf(stderr, "data memory: %g\n", epart);
		etotal += epart;
	}
	if (info->tag_mem_model) {
		epart = tag_mem.report();
		//fprintf(stderr, "tag memory: %g\n", epart);
		etotal += epart;
	}
	if (info->data_amp_model) {
		epart = data_amp.report();
		//fprintf(stderr, "data amp: %g\n", epart);
		etotal += epart;
	}
	if (info->tag_amp_model) {
		epart = tag_amp.report();
		//fprintf(stderr, "tag amp: %g\n", epart);
		etotal += epart;
	}
	if (info->comp_model) {
		epart = comp.report();
		//fprintf(stderr, "comparator: %g\n", epart);
		etotal += epart;
	}
	if (info->mux_model) {
		epart = mux.report();
		//fprintf(stderr, "multiplexor: %g\n", epart);
		etotal += epart;
	}
	if (info->outdrv_model) {
		epart = outdrv.report();
		//fprintf(stderr, "output driver: %g\n", epart);
		etotal += epart;
	}
	/* ignore other precharging for now */

	//fprintf(stderr, "total energy: %g\n", etotal);

	etotal += I_static * info->conf->PARM("Vdd") * info->conf->PARM("Period") * info->conf->PARM("SCALE_S");

	return etotal;
}

int ORION_Array::clear_stat() {
	row_dec.clear_stat();
	col_dec.clear_stat();
	data_wordline.clear_stat();
	tag_wordline.clear_stat();
	data_bitline.clear_stat();
	tag_bitline.clear_stat();
	data_mem.clear_stat();
	tag_mem.clear_stat();
	tag_attach_mem.clear_stat();
	data_amp.clear_stat();
	tag_amp.clear_stat();
	comp.clear_stat();
	mux.clear_stat();
	outdrv.clear_stat();
	row_dec_pre.clear_stat();
	col_dec_pre.clear_stat();
	data_bitline_pre.clear_stat();
	tag_bitline_pre.clear_stat();
	data_colsel_pre.clear_stat();
	tag_colsel_pre.clear_stat();
	comp_pre.clear_stat();

	return 0;
}

/* for now we simply initialize all fields to 0, which should not
 * add too much error if the program runtime is long enough :) */
PortState::PortState(ORION_Array_Info *info) {
	if (IS_FULLY_ASSOC( info ) || !(info->share_rw)) {
		bzero(data_line, int(data_line_size));
	}

	tag_line = 0;
	row_addr = 0;
	col_addr = 0;
	tag_addr = 0;

}

SetState::SetState(ORION_Array_Info *info) {
	entry = NULL;
	entry_set = NULL;

	if (IS_FULLY_ASSOC( info )) {
		write_flag = 0;
		write_back_flag = 0;
	}

}

/* record row decoder and wordline activity */
/* only used by non-fully-associative array, but we check it anyway */
int ORION_Array::dec(PortState *port, LIB_Type_max_uint row_addr, int rw) {
	if (!IS_FULLY_ASSOC( info )) {
		/* record row decoder stats */
		if (info->row_dec_model) {
			row_dec.record(port->row_addr, row_addr);

			/* update state */
			port->row_addr = row_addr;
		}

		/* record wordline stats */
		data_wordline.record(rw, info->data_ndwl);
		if (HAVE_TAG( info ))
			tag_wordline.record(rw, info->tag_ndwl);

		return 0;
	} else
		return -1;
}

/* record read data activity (including bitline and sense amplifier) */
/* only used by non-fully-associative array, but we check it anyway */
/* data only used by RF array */
int ORION_Array::data_read(LIB_Type_max_uint data) {
	if (info->data_end == 1) {
		data_bitline.record(SIM_ARRAY_READ, info->eff_data_cols, data);

		return 0;
	} else if (!IS_FULLY_ASSOC( info )) {
		data_bitline.record(SIM_ARRAY_READ, info->eff_data_cols, 0);
		data_amp.record(info->eff_data_cols);

		return 0;
	} else
		return -1;
}

/* record write data bitline and memory cell activity */
/* assume no alignment restriction on write, so (u_char *) */
/* set only used by fully-associative array */
/* data_line only used by fully-associative or RF array */
// data_line = value on the bitline
// old_data = old value of mem cell
// new_data = new value going in mem cell
int ORION_Array::data_write(SetState *set, u_int n_item, u_char *new_data) {
	u_int i;

	/* record bitline stats */
	if (IS_FULLY_ASSOC( info )) {
		/* wordline should be driven only once */
		if (!set->write_flag) {
			data_wordline.record(SIM_ARRAY_WRITE, 1);
			set->write_flag = 1;
		}

		/* for fully-associative array, data bank has no read
		 * bitlines, so bitlines not written have no activity */
		for (i = 0; i < n_item; i++) {

			data_bitline.record(SIM_ARRAY_WRITE, 8, new_data[i]);

			/* update state */
			//data_line[i] = new_data[i];
			//data_line_bus->record(new_data[i]);
		}
	} else if (info->share_rw) {
		//fprintf(stdout, "dw1\n");
		/* there is some subtlety here: write width may not be as wide as block size,
		 * bitlines not written are actually read, but column selector should be off,
		 * so read energy per bitline is the same as write energy per bitline */
		data_bitline.record(SIM_ARRAY_WRITE, info->eff_data_cols, 0, 0);
		//fprintf(stdout, "dw2\n");
		/* write in all sub-arrays if direct-mapped, which implies 1 cycle write latency,
		 * in those sub-arrays wordlines are not driven, so only n items columns switch */
		if (IS_DIRECT_MAP( info ) && info->data_ndbl > 1)
			data_bitline.record(SIM_ARRAY_WRITE, n_item * 8 * (info->data_ndbl
					- 1), 0, 0);
		//fprintf(stdout, "dw3\n");
	} else { /* separate R/W bitlines */
		/* same arguments as in the previous case apply here, except that when we say
		 * read_energy = write_energy, we omit the energy of write driver gate cap */
		//fprintf(stdout, "dw4\n");
		for (i = 0; i < n_item; i++) {
			data_bitline.record(SIM_ARRAY_WRITE, 8, new_data[i]);

			//fprintf(stdout, "dw5\n");
			/* update state */
			//data_line_bus->record(new_data[i]);
		}
	}
	//fprintf(stdout, "dw6\n");
	/* record memory cell stats */
	for (i = 0; i < n_item; i++)
		data_mem.record(new_data[i], 8);

	//fprintf(stdout, "dw7\n");

	return 0;
}

/* record read tag activity (including bitline and sense amplifier) */
/* only used by non-RF array */
/* set only used by fully-associative array */
int ORION_Array::tag_read(SetState *set) {
	if (IS_FULLY_ASSOC( info )) {
		/* the only reason to read a fully-associative array tag is writing back */
		tag_wordline.record(SIM_ARRAY_READ, 1);
		set->write_back_flag = 1;
	}

	tag_bitline.record(SIM_ARRAY_READ, info->eff_tag_cols, 0, 0);
	tag_amp.record(info->eff_tag_cols);

	return 0;
}

/* record write tag bitline and memory cell activity */
/* WHS: assume update of use bit, valid bit, dirty bit and tag will be coalesced */
/* only used by non-RF array */
/* port only used by fully-associative array */
int ORION_Array::tag_update(PortState *port, SetState *set) {
	u_int i;
	LIB_Type_max_uint curr_tag = 0;
	O_mem *tag_attach;

	/* get current tag */
	if (set->entry)
		curr_tag = (*info->get_entry_tag)(set->entry);

	if (IS_FULLY_ASSOC( info ))
		tag_attach = &tag_attach_mem;
	else
		tag_attach = &tag_mem;

	/* record tag bitline stats */
	if (IS_FULLY_ASSOC( info )) {
		if (set->entry && curr_tag != set->tag_bak) {
			/* shared wordline should be driven only once */
			if (!set->write_back_flag)
				tag_wordline.record(SIM_ARRAY_WRITE, 1);

			/* WHS: value of tag_line doesn't matter if not write_through */
			tag_bitline.record(SIM_ARRAY_WRITE, info->eff_tag_cols,
					port->tag_line, curr_tag);
			/* update state */
			if (IS_WRITE_THROUGH( info ))
				port->tag_line = curr_tag;
		}
	} else {
		/* tag update cannot occur at the 1st cycle, so no other sub-arrays */
		tag_bitline.record(SIM_ARRAY_WRITE, info->eff_tag_cols, 0, 0);
	}

	/* record tag memory cell stats */
	if (HAVE_USE_BIT( info ))
		for (i = 0; i < info->assoc; i++)
			tag_attach->record(set->use_bak[i], (*info->get_set_use_bit)(
					set->entry_set, i), info->use_bit_width);

	if (set->entry) {
		tag_attach->record(set->valid_bak, (*info->get_entry_valid_bit)(
				set->entry), info->valid_bit_width);
		tag_mem.record(set->tag_bak, curr_tag, info->tag_addr_width);

		if (IS_WRITE_BACK( info ))
			tag_attach->record(set->dirty_bak, (*info->get_entry_dirty_bit)(
					set->entry), 1);
	}

	return 0;
}

/* record tag compare activity (including tag comparator, column decoder and multiplexor) */
/* NOTE: this function may be called twice during ONE array operation, remember to update
 *       states at the end so that call to *_record won't add erroneous extra energy */
/* only used by non-RF array */
int ORION_Array::tag_compare(PortState *port, LIB_Type_max_uint tag_input,
		LIB_Type_max_uint col_addr, SetState *set) {
	int miss = 0;
	u_int i;

	/* record tag comparator stats */
	for (i = 0; i < info->assoc; i++) {
		/* WHS: sense amplifiers output 0 when idle */
		if (comp.local_record(0, (*info->get_set_tag)(set->entry_set, i),
				tag_input, SIM_ARRAY_RECOVER ))
			miss = 1;
	}

	comp.global_record(port->tag_addr, tag_input, miss);

	/* record column decoder stats */
	if (HAVE_COL_DEC( info ))
		col_dec.record(port->col_addr, col_addr);

	/* record multiplexor stats */
	if (HAVE_COL_MUX( info ))
		mux.record(port->col_addr, col_addr, miss);

	/* update state */
	port->tag_addr = tag_input;
	if (HAVE_COL_DEC( info ))
		port->col_addr = col_addr;

	return 0;
}

/* record output driver activity */
/* assume alignment restriction on read, so specify data_size */
/* WHS: it's really a mess to use data_size to specify data type */
/* data_all only used by non-RF and non-fully-associative array */
/* WHS: don't support 128-bit or wider integer */
int ORION_Array::output(u_int data_size, u_int length, void *data_out,
		void *data_all) {
	u_int i, j;

	/* record output driver stats */
	for (i = 0; i < length; i++) {
		switch (data_size) {
		case 1:
			outdrv.global_record(((uint8_t *) data_out)[i]);
			break;
		case 2:
			outdrv.global_record(((uint16_t *) data_out)[i]);
			break;
		case 4:
			outdrv.global_record(((uint32_t *) data_out)[i]);
			break;
		case 8:
			outdrv.global_record(((uint64_t *) data_out)[i]);
			break;
		default: /* some error handler */
			break;
		}
	}

	if (!IS_FULLY_ASSOC( info ) && data_all != NULL) {
		for (i = 0; i < info->assoc; i++)
			for (j = 0; j < info->n_item; j++)
				/* sense amplifiers output 0 when idle */
				switch (data_size) {
				case 1:
					outdrv.local_record(0, ((uint8_t **) data_all)[i][j],
							SIM_ARRAY_RECOVER );
					break;
				case 2:
					outdrv.local_record(0, ((uint16_t **) data_all)[i][j],
							SIM_ARRAY_RECOVER );
					break;
				case 4:
					outdrv.local_record(0, ((uint32_t **) data_all)[i][j],
							SIM_ARRAY_RECOVER );
					break;
				case 8:
					outdrv.local_record(0, ((uint64_t **) data_all)[i][j],
							SIM_ARRAY_RECOVER );
					break;
				default: /* some error handler */
					break;
				}
	}

	return 0;
}

double ORION_Array::stat_energy(double n_read, double n_write, int max_avg) {
	double Eavg = 0, Eatomic, Estruct = 0 /*, Estatic*/;

	/* hack to mimic central buffer */
	/* packet header probability */
	u_int NP_width, NC_width, cnt_width;
	int share_flag = 0;

	//std::cout << "-------- n_read: " << n_read << ", n_write: " << n_write << endl;

	if (info->arr_buf_type == SRAM) {
		/* decoder */
		if (info->row_dec_model) {
			Estruct = 0;

			/* assume switch probability 0.5 for address bits */
			Eatomic = row_dec.e_chg_addr * row_dec.n_bits * (max_avg ? 1 : 0.5)
					* (n_read + n_write);

			Estruct += Eatomic;

			Eatomic = row_dec.e_chg_output * (n_read + n_write);

			Estruct += Eatomic;

			/* assume all 1st-level decoders change output */
			Eatomic = row_dec.e_chg_l1 * row_dec.n_in_2nd * (n_read + n_write);

			Estruct += Eatomic;

			Eavg += Estruct;
		}

		//std::cout << "e_chg_addr: " << row_dec.e_chg_addr << ", n_bits: "
		//		<< row_dec.n_bits << ", e_chg_output: " << row_dec.e_chg_output
		//		<< ", l1: " << row_dec.e_chg_l1 << ", l2: " << row_dec.n_in_2nd << endl;

		//std::cout << "row_dec: " << Estruct << endl;

		/* wordline */
		Estruct = 0;

		Eatomic = data_wordline.e_read * n_read;

		Estruct += Eatomic;

		Eatomic = data_wordline.e_write * n_write;

		Estruct += Eatomic;

		Eavg += Estruct;

		//std::cout << "wordline: " << Estruct << endl;

		/* bitlines */
		Estruct = 0;

		if (data_bitline.end == 2) {
			Eatomic = data_bitline.e_col_read * info->eff_data_cols * n_read;
			/* dirty hack */
			if (share_flag) {
				Eatomic += data_bitline.e_col_read * (NP_width + NC_width
						+ cnt_width) * n_read;
				/* read free list */
				Eatomic += data_bitline.e_col_read * (NP_width + NC_width
						+ cnt_width) * n_write;
			}
		} else {
			/* assume switch probability 0.5 for single-ended bitlines */
			Eatomic = data_bitline.e_col_read * info->eff_data_cols
					* (max_avg ? 1 : 0.5) * n_read;
			/* dirty hack */
			if (share_flag) {
				/* assume no multicasting, cnt is always 0 */
				Eatomic += data_bitline.e_col_read * (NP_width + NC_width)
						* (max_avg ? 1 : 0.5) * n_read;
				/* read free list */
				Eatomic += data_bitline.e_col_read * (NP_width + NC_width)
						* (max_avg ? 1 : 0.5) * n_write;
			}
		}

		Estruct += Eatomic;

		/* assume switch probability 0.5 for write bitlines */
		Eatomic = data_bitline.e_col_write * info->data_width * (max_avg ? 1
				: 0.5) * n_write;
		/* dirty hack */
		if (share_flag) {
			/* current NP and NC */
			Eatomic += data_bitline.e_col_write * (NP_width + NC_width)
					* (max_avg ? 1 : 0.5) * n_write;
			/* previous NP or NC */
			Eatomic += data_bitline.e_col_write * NP_width
					* (max_avg ? 1 : 0.5) * n_write;
			/* update free list */
			Eatomic += data_bitline.e_col_write * NC_width
					* (max_avg ? 1 : 0.5) * n_read;
		}

		Estruct += Eatomic;

		Eatomic = data_bitline_pre.e_charge * info->eff_data_cols * n_read;
		/* dirty hack */
		if (share_flag) {
			Eatomic += data_bitline_pre.e_charge * (NP_width + NC_width
					+ cnt_width) * n_read;
			/* read free list */
			Eatomic += data_bitline_pre.e_charge * (NP_width + NC_width
					+ cnt_width) * n_write;
		}

		Estruct += Eatomic;

		//std::cout << "bitline: " << Estruct << endl;

		Eavg += Estruct;

		/* memory cells */
		Estruct = 0;

		/* assume switch probability 0.5 for memory cells */
		Eatomic = data_mem.e_switch * info->data_width * (max_avg ? 1 : 0.5)
				* n_write;
		/* dirty hack */
		if (share_flag) {
			/* current NP and NC */
			Eatomic += data_mem.e_switch * (NP_width + NC_width) * (max_avg ? 1
					: 0.5) * n_write;
			/* previous NP or NC */
			Eatomic += data_mem.e_switch * NP_width * (max_avg ? 1 : 0.5)
					* n_write;
			/* update free list */
			Eatomic += data_mem.e_switch * NC_width * (max_avg ? 1 : 0.5)
					* n_read;
		}
		Estruct += Eatomic;

		//std::cout << "mem cell: " << Estruct << endl;

		Eavg += Estruct;

		/* sense amplifier */
		if (info->data_end == 2) {
			Estruct = 0;

			Eatomic = data_amp.e_access * info->eff_data_cols * n_read;
			/* dirty hack */
			if (share_flag) {
				Eatomic += data_amp.e_access
						* (NP_width + NC_width + cnt_width) * n_read;
				/* read free list */
				Eatomic += data_amp.e_access
						* (NP_width + NC_width + cnt_width) * n_write;
			}
			Estruct += Eatomic;

			Eavg += Estruct;

			//std::cout << "sense amp: " << Estruct << endl;
		}

		/* output driver */
		if (info->outdrv_model) {
			Estruct = 0;

			Eatomic = outdrv.e_select * n_read;

			Estruct += Eatomic;

			/* same switch probability as bitlines */
			Eatomic = outdrv.e_chg_data * outdrv.item_width * (max_avg ? 1
					: 0.5) * info->n_item * info->assoc * n_read;

			Estruct += Eatomic;

			/* assume 1 and 0 are uniformly distributed */
			if (outdrv.e_out_1 >= outdrv.e_out_0 || !max_avg) {
				Eatomic = outdrv.e_out_1 * outdrv.item_width * (max_avg ? 1
						: 0.5) * n_read;

				Estruct += Eatomic;
			}

			if (outdrv.e_out_1 < outdrv.e_out_0 || !max_avg) {
				Eatomic = outdrv.e_out_0 * outdrv.item_width * (max_avg ? 1
						: 0.5) * n_read;

				Estruct += Eatomic;
			}

			Eavg += Estruct;

			//std::cout << "out drive: " << Estruct << endl;
		}

	} else if (info->arr_buf_type == REGISTER) {
		double avg_read, avg_write;

		Estruct = 0;

		/*average read energy for one buffer entry*/
		ff.n_clock = info->data_width;

		avg_read = ff.e_clock * ff.n_clock * 0.5;

		/*average write energy for one buffer entry*/
		ff.n_clock = info->data_width;
		ff.n_switch = info->data_width * 0.5;

		avg_write = ff.e_switch * ff.n_switch + ff.e_clock * ff.n_clock;

		/* for each read operation, the energy consists of one read operation and n write
		 * operateion. n means there is n flits in the buffer before read operation.
		 * assume n is info->n_entry * 0.25.
		 */
		if (info->n_set > 1) {
			Eatomic = (avg_read + info->n_set * 0.25 * avg_write) * n_read;
		} else {
			Eatomic = avg_read * n_read;
		}

		Estruct += Eatomic;

		/* write energy */
		Eatomic = avg_write * n_write;

		Estruct += Eatomic;

		Eavg = Estruct;

	}

	return Eavg;
}

double ORION_Array::stat_static_power() {

	if (info->arr_buf_type == SRAM) {

		return I_static * info->conf->PARM("Vdd") * info->conf->PARM("SCALE_S");

	} else if (info->arr_buf_type == REGISTER) {

		return ff.I_static * info->conf->PARM("Vdd") * info->conf->PARM("SCALE_S");
	}

	return -10000;
}
