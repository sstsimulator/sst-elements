#include "ORION_Crossbar.h"
#include <math.h>
#include <stdio.h>

/*============================== crossbar ==============================*/

double ORION_Crossbar::in_cap(double wire_cap, u_int n_out, double n_seg,
		int connect_type, int trans_type, double *Nsize) {
	double Ctotal = 0, Ctrans, psize, nsize;

	/* part 1: wire cap */
	Ctotal += wire_cap;

	/* part 2: drain cap of transmission gate or gate cap of tri-state gate */
	if (connect_type == TRANS_GATE) {
		/* FIXME: resizing strategy */
		nsize = Nsize ? *Nsize : conf->PARM("Wmemcellr");
		psize = nsize * conf->PARM("Wdecinvp") / conf->PARM("Wdecinvn");
		Ctrans = ORION_Util_draincap(nsize, NCH, 1, conf );
		if (trans_type == NP_GATE)
			Ctrans += ORION_Util_draincap(psize, PCH, 1, conf );
	} else if (connect_type == TRISTATE_GATE) {
		Ctrans = ORION_Util_gatecap(conf->PARM("Woutdrvnandn")+ conf->PARM("Woutdrvnandp"), 0, conf ) +
		ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf );
	}
	else {/* some error handler */}

	Ctotal += n_out * Ctrans;

	/* segmented crossbar */
	if (n_seg> 1) {
		Ctotal *= (n_seg + 1)/(n_seg * 2);
		/* input capacitance of tri-state buffer */
		Ctotal += (n_seg + 2)*(n_seg - 1)/(n_seg * 2)*(ORION_Util_gatecap(conf->PARM("Woutdrvnandn")
				+ conf->PARM("Woutdrvnandp"), 0, conf ) +
				ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf ));
		/* output capacitance of tri-state buffer */
		Ctotal += (n_seg - 1) / 2 * (ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf ) +
				ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ));
	}

	/* part 3: input driver */
	/* FIXME: how to specify timing? */
	psize = ORION_Util_driver_size(Ctotal, conf->PARM("Period") / 3, conf );
	nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
	Ctotal += ORION_Util_draincap(nsize, NCH, 1, conf ) + ORION_Util_draincap(psize, PCH, 1, conf ) +
	ORION_Util_gatecap(nsize + psize, 0, conf );

	return Ctotal / 2;
}

double ORION_Crossbar::out_cap(double length, u_int n_in, double n_seg,
		int connect_type, int trans_type, double *Nsize) {
	double Ctotal = 0, Ctrans, psize, nsize;

	/* part 1: wire cap */
	Ctotal += conf->PARM("CC3metal") * length;

	/* part 2: drain cap of transmission gate or tri-state gate */
	if (connect_type == TRANS_GATE) {
		/* FIXME: resizing strategy */
		if (Nsize) {
			/* FIXME: how to specify timing? */
			psize = ORION_Util_driver_size(Ctotal, conf->PARM("Period") / 3, conf );
			*Nsize = nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
		} else {
			nsize = conf->PARM("Wmemcellr");
			psize = nsize * conf->PARM("Wdecinvp") / conf->PARM("Wdecinvn");
		}
		Ctrans = ORION_Util_draincap(nsize, NCH, 1, conf );
		if (trans_type == NP_GATE)
			Ctrans += ORION_Util_draincap(psize, PCH, 1, conf );
	} else if (connect_type == TRISTATE_GATE) {
		Ctrans
				= ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf )
						+ ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf );
	} else {/* some error handler */
	}

	Ctotal += n_in * Ctrans;

	/* segmented crossbar */
	if (n_seg > 1) {
		Ctotal *= (n_seg + 1) / (n_seg * 2);
		/* input capacitance of tri-state buffer */
		Ctotal
				+= (n_seg + 2) * (n_seg - 1) / (n_seg * 2)
						* (ORION_Util_gatecap(conf->PARM("Woutdrvnandn")+ conf->PARM("Woutdrvnandp"), 0, conf ) +
						ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf ));
						/* output capacitance of tri-state buffer */
						Ctotal += (n_seg - 1) / 2 * (ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf ) +
								ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ));
					}

					/* part 3: output driver */
					Ctotal += ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf ) +
					ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ) +
					ORION_Util_gatecap(conf->PARM("Woutdrivern") + conf->PARM("Woutdriverp"), 0, conf );

					return Ctotal / 2;
				}

		/* cut-through crossbar only supports 4x4 now */
double ORION_Crossbar::io_cap(double length) {
	double Ctotal = 0, psize, nsize;

	/* part 1: wire cap */
	Ctotal += conf->PARM("CC3metal") * length;

	/* part 2: gate cap of tri-state gate */
	Ctotal += 2 * (ORION_Util_gatecap(conf->PARM("Woutdrvnandn")+ conf->PARM("Woutdrvnandp"), 0, conf ) +
	ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf ));

	/* part 3: drain cap of tri-state gate */
	Ctotal += 2 * (ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf )
			+ ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ));

	/* part 4: input driver */
	/* FIXME: how to specify timing? */
	psize = ORION_Util_driver_size(Ctotal, conf->PARM("Period") * 0.8, conf );
	nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
	Ctotal += ORION_Util_draincap(nsize, NCH, 1, conf ) + ORION_Util_draincap(psize, PCH, 1, conf ) +
	ORION_Util_gatecap(nsize + psize, 0, conf );

	/* part 5: output driver */
	Ctotal += ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf ) +
	ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ) +
	ORION_Util_gatecap(conf->PARM("Woutdrivern") + conf->PARM("Woutdriverp"), 0, conf );

	/* HACK HACK HACK */
	/* this HACK is to count a 1:4 mux and a 4:1 mux, so we have a 5x5 crossbar */
	return Ctotal / 2 * 1.32;
}

double ORION_Crossbar::int_cap(u_int degree, int connect_type, int trans_type) {
	double Ctotal = 0, Ctrans;

	if (connect_type == TRANS_GATE) {
		/* part 1: drain cap of transmission gate */
		/* FIXME: Wmemcellr and resize */
		Ctrans = ORION_Util_draincap(conf->PARM("Wmemcellr"), NCH, 1, conf );
		if (trans_type == NP_GATE)
			Ctrans += ORION_Util_draincap(conf->PARM("Wmemcellr")* conf->PARM("Wdecinvp") / conf->PARM("Wdecinvn"),
			PCH, 1, conf );
			Ctotal += (degree + 1) * Ctrans;
		} else if (connect_type == TRISTATE_GATE) {
			/* part 1: drain cap of tri-state gate */
			Ctotal += degree * (ORION_Util_draincap(conf->PARM("Woutdrivern"), NCH, 1, conf )
					+ ORION_Util_draincap(conf->PARM("Woutdriverp"), PCH, 1, conf ));

			/* part 2: gate cap of tri-state gate */
			Ctotal += ORION_Util_gatecap(conf->PARM("Woutdrvnandn")+ conf->PARM("Woutdrvnandp"), 0, conf ) +
			ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf );
		}
		else {/* some error handler */}

		return Ctotal / 2;
	}

			/* FIXME: segment control signals are not handled yet */
double ORION_Crossbar::ctr_cap(double length, u_int data_width, int prev_ctr,
		int next_ctr, u_int degree, int connect_type, int trans_type) {
	double Ctotal = 0, Cgate;

	/* part 1: wire cap */
	Ctotal = conf->PARM("Cmetal") * length;

	/* part 2: gate cap of transmission gate or tri-state gate */
	if (connect_type == TRANS_GATE) {
		/* FIXME: Wmemcellr and resize */
		Cgate = ORION_Util_gatecap(conf->PARM("Wmemcellr"), 0, conf );
		if (trans_type == NP_GATE)
			Cgate += ORION_Util_gatecap(conf->PARM("Wmemcellr")* conf->PARM("Wdecinvp") / conf->PARM("Wdecinvn"), 0, conf );
		}
		else if (connect_type == TRISTATE_GATE) {
			Cgate = ORION_Util_gatecap(conf->PARM("Woutdrvnandn") + conf->PARM("Woutdrvnandp"), 0, conf ) +
			ORION_Util_gatecap(conf->PARM("Woutdrvnorn") + conf->PARM("Woutdrvnorp"), 0, conf );
		}
		else {/* some error handler */}

		Ctotal += data_width * Cgate;

		/* part 3: inverter */
		if (!(connect_type == TRANS_GATE && trans_type == N_GATE && !prev_ctr))
    /* FIXME: need accurate size, use minimal size for now */
    Ctotal += ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf ) + ORION_Util_draincap(conf->PARM("Wdecinvp"), PCH, 1, conf ) +
	      ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 0, conf );

  /* part 4: drain cap of previous level control signal */
  if (prev_ctr)
    /* FIXME: need actual size, use decoder data for now */
    Ctotal += degree * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf ) +
    ORION_Util_draincap(conf->PARM("WdecNORp"), PCH, degree, conf );

  /* part 5: gate cap of next level control signal */
  if (next_ctr)
    /* FIXME: need actual size, use decoder data for now */
    Ctotal += ORION_Util_gatecap(conf->PARM("WdecNORn") + conf->PARM("WdecNORp"), degree * 40 + 20, conf );

  return Ctotal;
}

int ORION_Crossbar::init(int m, u_int in, u_int out, u_int in_s, u_int out_s,
		u_int dwidth, u_int de, int c_type, int t_type, double in_len,
		double out_len, ORION_Tech_Config *con) {
	double in_length, out_length, ctr_length, Nsize, in_wire_cap;

	conf = con;

	if ((model = m) && m < CROSSBAR_MAX_MODEL) {
		n_in = in;
		n_out = out;
		in_seg = 0;
		out_seg = 0;
		data_width = dwidth;
		degree = de;
		connect_type = c_type;
		trans_type = t_type;
		/* redundant field */
		mask = HAMM_MASK(data_width);

		n_chg_in = n_chg_int = n_chg_out = n_chg_ctr = 0;

		in_data = (LIB_Type_max_uint*) malloc(sizeof(LIB_Type_max_uint) * n_in);
		out_data = (LIB_Type_max_uint*) malloc(sizeof(LIB_Type_max_uint)
				* n_out);
		out_ports = (u_int*) malloc(sizeof(u_int) * n_out);

		for (int i = 0; i < n_in; i++)
			in_data[i] = 0;

		for (int i = 0; i < n_out; i++) {
			out_data[i] = 0;
			out_ports[i] = 300000;
		}

		switch (model) {
		case MATRIX_CROSSBAR:
			in_seg = in_s;
			out_seg = out_s;

			/* FIXME: need accurate spacing */
			in_length = n_out * data_width * conf->PARM("CrsbarCellWidth");
			out_length = n_in * data_width * conf->PARM("CrsbarCellHeight");
			if (in_length < in_len)
				in_length = in_len;
			if (out_length < out_len)
				out_length = out_len;
			ctr_length = in_length / 2;
			// if (req_len) *req_len = in_length;

			in_wire_cap = in_length * conf->PARM("CC3metal");

			e_chg_out = out_cap(out_length, n_in, out_seg, connect_type,
					trans_type, &Nsize) * conf->PARM("EnergyFactor");
			e_chg_in = in_cap(in_wire_cap, n_out, in_seg, connect_type,
					trans_type, &Nsize) * conf->PARM("EnergyFactor");
			/* FIXME: wire length estimation, really reset? */
			/* control signal should reset after transmission is done, so no 1/2 */
			e_chg_ctr = ctr_cap(ctr_length, data_width, 0, 0, 0, connect_type,
					trans_type) * conf->PARM("EnergyFactor");
			e_chg_int = 0;

			/* static power */
			I_static = 0;
			/*crossbar connector */
			if (connect_type == TRANS_GATE) { /*transmission gate*/
				double nsize, psize;

				nsize = conf->PARM("Wmemcellr");
				I_static += (nsize * TABS::NMOS_TAB[0]) * n_in * n_out * data_width;

				if (trans_type == NP_GATE) {
					psize = nsize * conf->PARM("Wdecinvp") / conf->PARM("Wdecinvn");
					I_static += (psize * TABS::PMOS_TAB[0]) * n_in * n_out
							* data_width;
				}
			} else if (connect_type == TRISTATE_GATE) { /* tri-state buffers */
				/* tri-state buffers */
				I_static += ((conf->PARM("Woutdrvnandp") * (TABS::NAND2_TAB[0]
						+ TABS::NAND2_TAB[1] + TABS::NAND2_TAB[2])
						+ conf->PARM("Woutdrvnandn") * TABS::NAND2_TAB[3]) / 4 + (conf->PARM("Woutdrvnorp")
				* TABS::NOR2_TAB[0] + conf->PARM("Woutdrvnorn") * (TABS::NOR2_TAB[1]
						+ TABS::NOR2_TAB[2] + TABS::NOR2_TAB[3])) / 4
						+ conf->PARM("Woutdrivern") * TABS::NMOS_TAB[0] + conf->PARM("Woutdriverp")
						* TABS::PMOS_TAB[0]) * n_in * n_out * data_width;
			}
			/* input driver */
			I_static += (conf->PARM("Wdecinvn") * TABS::NMOS_TAB[0] + conf->PARM("Wdecinvp")
					* TABS::PMOS_TAB[0]) * n_in * data_width;
			/* output driver */
			I_static += (conf->PARM("Woutdrivern") * TABS::NMOS_TAB[0] + conf->PARM("Woutdriverp")
					* TABS::PMOS_TAB[0]) * n_out * data_width;
			/* control signal inverter */
			I_static += (conf->PARM("Wdecinvn") * TABS::NMOS_TAB[0] + conf->PARM("Wdecinvp")
					* TABS::PMOS_TAB[0]) * n_in * n_out;

			break;

		case MULTREE_CROSSBAR:
			/* input wire horizontal segment length */
			in_length = n_in * data_width * conf->PARM("CrsbarCellWidth") * (n_out / 2);
			in_wire_cap = in_length * conf->PARM("CCmetal");
			/* input wire vertical segment length */
			in_length = n_in * data_width * (5 * conf->PARM("Lamda")) * (n_out / 2);
			in_wire_cap += in_length * conf->PARM("CC3metal");

			ctr_length = n_in * data_width * conf->PARM("CrsbarCellWidth") * (n_out / 2) / 2;

			e_chg_out = out_cap(0, degree, 0, connect_type, trans_type, NULL)
					* conf->PARM("EnergyFactor");
			e_chg_in = in_cap(in_wire_cap, n_out, 0, connect_type, trans_type,
					NULL) * conf->PARM("EnergyFactor");
			e_chg_int = int_cap(degree, connect_type, trans_type)
					* conf->PARM("EnergyFactor");

			/* redundant field */
			depth = (u_int)ceil(log((double) n_in) / log((double) degree));

			/* control signal should reset after transmission is done, so no 1/2 */
			if (depth == 1)
				/* only one level of control signal */
				e_chg_ctr = ctr_cap(ctr_length, data_width, 0, 0, degree,
						connect_type, trans_type) * conf->PARM("EnergyFactor");
			else {
				/* first level and last level control signals */
				e_chg_ctr = ctr_cap(ctr_length, data_width, 0, 1, degree,
						connect_type, trans_type) * conf->PARM("EnergyFactor") + ctr_cap(0,
						data_width, 1, 0, degree, connect_type, trans_type)
						* conf->PARM("EnergyFactor");
				/* intermediate control signals */
				if (depth > 2)
					e_chg_ctr += (depth - 2) * ctr_cap(0, data_width, 1, 1,
							degree, connect_type, trans_type) * conf->PARM("EnergyFactor");
			}

			/* static power */
			I_static = 0;
			/* input driver */
			I_static += (conf->PARM("Wdecinvn") * TABS::NMOS_TAB[0] + conf->PARM("Wdecinvp")
					* TABS::PMOS_TAB[0]) * n_in * data_width;
			/* output driver */
			I_static += (conf->PARM("Woutdrivern") * TABS::NMOS_TAB[0] + conf->PARM("Woutdriverp")
					* TABS::PMOS_TAB[0]) * n_out * data_width;
			/* mux */
			I_static += (conf->PARM("WdecNORp") * TABS::NOR2_TAB[0] + conf->PARM("WdecNORn")
					* (TABS::NOR2_TAB[1] + TABS::NOR2_TAB[2]
							+ TABS::NOR2_TAB[3])) / 4 * (2 * n_in - 1) * n_out
					* data_width;
			/* control signal inverter */
			I_static += (conf->PARM("Wdecinvn") * TABS::NMOS_TAB[0] + conf->PARM("Wdecinvp")
					* TABS::PMOS_TAB[0]) * n_in * n_out;

			break;

		case CUT_THRU_CROSSBAR:
			/* only support 4x4 now */
			in_length = 2 * data_width * MAX(conf->PARM("CrsbarCellWidth"), conf->PARM("CrsbarCellHeight"));
			ctr_length = in_length / 2;

			e_chg_in = io_cap(in_length) * conf->PARM("EnergyFactor");
			e_chg_out = 0;
			/* control signal should reset after transmission is done, so no 1/2 */
			e_chg_ctr = ctr_cap(ctr_length, data_width, 0, 0, 0, TRISTATE_GATE,
					0) * conf->PARM("EnergyFactor");
			e_chg_int = 0;
			break;

		default: /* some error handler */
			break;
		}

		return 0;
	} else
		return -1;
}

/* FIXME: MULTREE_CROSSBAR record missing */
int ORION_Crossbar::record(int io, LIB_Type_max_uint new_data, u_int in_port,
		u_int out_port) {
	switch (model) {
	case MATRIX_CROSSBAR:
		if (io) { /* input port */
			n_chg_in += ORION_Util_Hamming(new_data, in_data[in_port], mask);
			in_data[in_port] = new_data;
		} else { /* output port */
			n_chg_out += ORION_Util_Hamming(new_data, out_data[out_port], mask);
			n_chg_ctr += in_port != out_ports[out_port];
			out_ports[out_port] = in_port;
			out_data[out_port] = new_data;
		}
		break;

	case CUT_THRU_CROSSBAR:
		if (io) { /* input port */
			n_chg_in += ORION_Util_Hamming(new_data, out_data[out_port], mask);
			in_data[in_port] = new_data;
		} else { /* output port */
			n_chg_ctr += in_port != out_ports[out_port];
			out_ports[out_port] = in_port;
			out_data[out_port] = new_data;
		}
		break;

	case MULTREE_CROSSBAR:
		break;

	default: /* some error handler */
		break;
	}

	return 0;
}

/* FIXME: MULTREE_CROSSBAR record missing */
int ORION_Crossbar::record(int io, u_int in_port, u_int out_port, int size) {
	switch (model) {
	case MATRIX_CROSSBAR:
		if (io) { /* input port */
			n_chg_in += size * data_width * 0.5;

		} else { /* output port */
			n_chg_out += size * data_width * 0.5;
			n_chg_ctr += in_port != out_ports[out_port];
			out_ports[out_port] = in_port;

		}
		break;

	case CUT_THRU_CROSSBAR:
		if (io) { /* input port */
			n_chg_in += size * data_width * 0.5;

		} else { /* output port */
			n_chg_ctr += in_port != out_ports[out_port];
			out_ports[out_port] = in_port;

		}
		break;

	case MULTREE_CROSSBAR:
		break;

	default: /* some error handler */
		break;
	}

	return 0;
}

double ORION_Crossbar::report() {
	return (n_chg_in * e_chg_in + n_chg_out * e_chg_out + n_chg_int * e_chg_int
			+ n_chg_ctr * e_chg_ctr);
}

double ORION_Crossbar::report_static_power() {
	return I_static * conf->PARM("Vdd") * conf->PARM("SCALE_S");
}

double ORION_Crossbar::stat_energy(int max_avg) {
	double Eavg = 0, Eatomic, Estatic;
	int next_depth;
	u_int path_len;

	/* if (n_data > n_out) {
	 fprintf(stderr, "%s: overflow\n", path);
	 n_data = n_out;
	 }*/

	// next_depth = NEXT_DEPTH(print_depth);
	// path_len = ORION_Util_strlen(path);

	switch (model) {
	case MATRIX_CROSSBAR:
	case CUT_THRU_CROSSBAR:
	case MULTREE_CROSSBAR:
		/* assume 0.5 data switch probability */
		Eatomic = e_chg_in * data_width * (max_avg ? 1 : 0.5) * n_out;
		// ORION_Util_print_stat_energy(ORION_Util_strcat(path, "input"), Eatomic, next_depth);
		// ORION_Util_res_path(path, path_len);
		Eavg += Eatomic;

		Eatomic = e_chg_out * data_width * (max_avg ? 1 : 0.5) * n_out;
		// ORION_Util_print_stat_energy(ORION_Util_strcat(path, "output"), Eatomic, next_depth);
		//  ORION_Util_res_path(path, path_len);
		Eavg += Eatomic;

		Eatomic = e_chg_ctr * n_out;
		// ORION_Util_print_stat_energy(ORION_Util_strcat(path, "control"), Eatomic, next_depth);
		// ORION_Util_res_path(path, path_len);
		Eavg += Eatomic;

		if (model == MULTREE_CROSSBAR && depth > 1) {
			Eatomic = e_chg_int * data_width * (depth - 1)
					* (max_avg ? 1 : 0.5) * n_out;
			//   ORION_Util_print_stat_energy(ORION_Util_strcat(path, "internal node"), Eatomic, next_depth);
			//    ORION_Util_res_path(path, path_len);
			Eavg += Eatomic;
		}

		/* static power */
		Estatic = I_static * conf->PARM("Vdd") * conf->PARM("Period") * conf->PARM("SCALE_S");
		// ORION_Util_print_stat_energy(ORION_Util_strcat(path, "static energy"), Estatic, next_depth);
		//   ORION_Util_res_path(path, path_len);

		Eavg += Estatic;

		break;

	default: /* some error handler */
		break;
	}

	//ORION_Util_print_stat_energy(path, Eavg, print_depth);

	return Eavg;
}

/*============================== crossbar ==============================*/
