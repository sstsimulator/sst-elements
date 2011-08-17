#include "ORION_Util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Hamming distance table */
static u_char h_tab[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1,
		2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2,
		3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1,
		2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3,
		4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
		4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2,
		3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
		3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
		5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
		4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4,
		5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };


int ORION_Util_init(void) {
	u_int i;

	/* initialize Hamming distance table */
	for (i = 0; i < 256; i++)
		h_tab[i] = ORION_Util_Hamming_slow(i, 0, 0xFF);

	return 0;
}

u_int ORION_Util_Hamming_slow(LIB_Type_max_uint old_val,
		LIB_Type_max_uint new_val, LIB_Type_max_uint mask) {
	/* old slow code, I don't understand the new fast code though */
	/* LIB_Type_max_uint dist;
	 u_int Hamming = 0;

	 dist = ( old_val ^ new_val ) & mask;
	 mask = (mask >> 1) + 1;

	 while ( mask ) {
	 if ( mask & dist ) Hamming ++;
	 mask = mask >> 1;
	 }

	 return Hamming; */
#define TWO(k) (BIGONE << (k))
#define CYCL(k) (BIGNONE/(1 + (TWO(TWO(k)))))
#define BSUM(x,k) ((x)+=(x) >> TWO(k), (x) &= CYCL(k))
	LIB_Type_max_uint x;

	x = (old_val ^ new_val) & mask;
	x = (x & CYCL(0)) + ((x >> TWO(0)) & CYCL(0));
	x = (x & CYCL(1)) + ((x >> TWO(1)) & CYCL(1));
	BSUM(x,2);
	BSUM(x,3);
	BSUM(x,4);
	BSUM(x,5);

	return x;
}

/* assume LIB_Type_max_uint is u_int64_t */
u_int ORION_Util_Hamming(LIB_Type_max_uint old_val, LIB_Type_max_uint new_val,
		LIB_Type_max_uint mask) {
	union {
		LIB_Type_max_uint x;
		u_char id[8];
	} u;
	u_int rval;

	u.x = (old_val ^ new_val) & mask;

	rval = h_tab[u.id[0]];
	rval += h_tab[u.id[1]];
	rval += h_tab[u.id[2]];
	rval += h_tab[u.id[3]];
	rval += h_tab[u.id[4]];
	rval += h_tab[u.id[5]];
	rval += h_tab[u.id[6]];
	rval += h_tab[u.id[7]];

	return rval;
}

u_int ORION_Util_Hamming_group(LIB_Type_max_uint d1_new,
		LIB_Type_max_uint d1_old, LIB_Type_max_uint d2_new,
		LIB_Type_max_uint d2_old, u_int width, u_int n_grp) {
	u_int rval = 0;
	LIB_Type_max_uint g1_new, g1_old, g2_new, g2_old, mask;

	mask = HAMM_MASK(width);

	while (n_grp--) {
		g1_new = d1_new & mask;
		g1_old = d1_old & mask;
		g2_new = d2_new & mask;
		g2_old = d2_old & mask;

		if (g1_new != g1_old || g2_new != g2_old)
			rval++;

		d1_new >>= width;
		d1_old >>= width;
		d2_new >>= width;
		d2_old >>= width;
	}

	return rval;
}

u_int ORION_Util_logtwo(LIB_Type_max_uint x) {
	u_int rval = 0;

	while (x >> rval && rval < sizeof(LIB_Type_max_uint) << 3)
		rval++;
	if (x == (BIGONE << rval - 1))
		rval--;

	return rval;
}

/*#
 * this routine takes the number of rows and cols of an array structure and
 * attemps to make it more of a reasonable circuit structure by trying to
 * make the number of rows and cols as close as possible.  (scaling both by
 * factors of 2 in opposite directions)
 *
 * Parameters:
 *   rows -- # of array rows
 *   cols -- # of array cols
 *
 * Return value: (if positive) scale factor which is the amount that the rows
 *               should be divided by and the cols should be multiplied by.
 *               (if negative) scale factor which is the amount that the cols
 *               should be divided by and the rows should be multiplied by.
 */
int ORION_Util_squarify(int rows, int cols) {
	int scale_factor = 0;

	if (rows == cols)
		return 1;

	while (rows > cols) {
		rows = rows / 2;
		cols = cols * 2;
		if (rows <= cols)
			return (1 << scale_factor);

		scale_factor++;
	}

	while (cols > rows) {
		rows = rows * 2;
		cols = cols / 2;
		if (cols <= rows)
			return -(1 << scale_factor);

		scale_factor++;
	}
}

double ORION_Util_driver_size(double driving_cap, double desiredrisetime, ORION_Tech_Config *conf) {
	double nsize, psize;
	double Rpdrive;

	Rpdrive = desiredrisetime / (driving_cap * log(conf->PARM("VSINV")) * -1.0);
	psize = ORION_Util_restowidth(Rpdrive, PCH, conf );
	nsize = ORION_Util_restowidth(Rpdrive, NCH, conf );
	if (psize > conf->PARM("Wworddrivemax")) {
		psize = conf->PARM("Wworddrivemax");
	}
	if (psize < 4.0 * conf->PARM("LSCALE"))
		psize = 4.0 * conf->PARM("LSCALE");

	return (psize);
}

/*----------------------------------------------------------------------*/

double ORION_Util_gatecap(double width, double wirelength, ORION_Tech_Config *conf) /* returns gate capacitance in Farads */
//double width;		/* gate width in um (length is Leff) */
//double wirelength;	/* poly wire length going to gate in lambda */
{

	double overlapCap;
	double gateCap;
	double l = 0.1525;


	return (width * conf->PARM("Leff") * conf->PARM("Cgate") + wirelength * conf->PARM("Cpolywire") * conf->PARM("Leff")
			* conf->PARM("SCALE_T"));
	/* return(width*Leff*PARM(Cgate)); */
	/* return(width*CgateLeff+wirelength*Cpolywire*Leff);*/
}

double ORION_Util_gatecappass(double width, double wirelength, ORION_Tech_Config *conf) /* returns gate capacitance in Farads */
//double width;           /* gate width in um (length is Leff) */
//double wirelength;      /* poly wire length going to gate in lambda */
{
	return (ORION_Util_gatecap(width, wirelength, conf ));
	/* return(width*Leff*PARM(Cgatepass)+wirelength*Cpolywire*Leff); */
}

/*----------------------------------------------------------------------*/

/* Routine for calculating drain capacitances.  The draincap routine
 * folds transistors larger than 10um */
double ORION_Util_draincap(double width, int nchannel, int stack, ORION_Tech_Config *conf) /* returns drain cap in Farads */
//double width;		/* um */
//int nchannel;		/* whether n or p-channel (boolean) */
//int stack;		/* number of transistors in series that are on */
{
	double Cdiffside, Cdiffarea, Coverlap, cap;

	double overlapCap;
	double swAreaUnderGate;
	double area_peri;
	double diffArea;
	double diffPeri;
	double l = 0.4 * conf->PARM("LSCALE");

	diffArea = l * width;
	diffPeri = 2 * l + 2 * width;


	Cdiffside = (nchannel) ? conf->PARM("Cndiffside") : conf->PARM("Cpdiffside");
	Cdiffarea = (nchannel) ? conf->PARM("Cndiffarea") : conf->PARM("Cpdiffarea");
	Coverlap = (nchannel) ? (conf->PARM("Cndiffovlp") + conf->PARM("Cnoxideovlp")) : (conf->PARM(
			"Cpdiffovlp") + conf->PARM("Cpoxideovlp"));
	/* calculate directly-connected (non-stacked) capacitance */
	/* then add in capacitance due to stacking */
	if (width >= 10) {
		cap = 3.0 * conf->PARM("Leff") * width / 2.0 * Cdiffarea + 6.0
		* conf->PARM("Leff") * Cdiffside
				+ width * Coverlap;
		cap += (double) (stack - 1) * (conf->PARM("Leff") * width * Cdiffarea + 4.0 * conf->PARM("Leff")
				* Cdiffside + 2.0 * width * Coverlap);
	} else {
		cap = 3.0 * conf->PARM("Leff") * width * Cdiffarea +
		(6.0 * conf->PARM("Leff") + width) * Cdiffside
				+ width * Coverlap;
		cap += (double) (stack - 1) * (conf->PARM("Leff") * width * Cdiffarea + 2.0 * conf->PARM("Leff")
				* Cdiffside + 2.0 * width * Coverlap);
	}
	return (cap * conf->PARM("SCALE_T"));
}

/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */
double ORION_Util_restowidth(double res, int nchannel, ORION_Tech_Config *conf) /* returns width in um */
//double res;            /* resistance in ohms */
//int nchannel;          /* whether N-channel or P-channel */
{
	double restrans;

	restrans = (nchannel) ? conf->PARM("Rnchannelon") : conf->PARM("Rpchannelon");

	return (restrans / res);
}

int ORION_Util_print_stat_energy(char *path, double Energy, int print_flag) {
	u_int path_len;

	if (print_flag) {
		if (path) {
			path_len = strlen(path);
			strcat(path, ": %g\n");
			fprintf(stderr, path, Energy);
			ORION_Util_res_path(path, path_len);
		} else
			fprintf(stderr, "%g\n", Energy);
	}

	return 0;
}

/*
 * SIDE EFFECT: change string dest
 *
 * NOTES: use SIM_power_res_path to restore dest
 */
char *ORION_Util_strcat(char *dest, char *src) {
	if (dest) {
		strcat(dest, "/");
		strcat(dest, src);
	}

	return dest;
}

int ORION_Util_res_path(char *path, u_int id) {
	if (path)
		path[id] = 0;

	return 0;
}

u_int ORION_Util_strlen(char *s) {
	if (s)
		return strlen(s);
	else
		return 0;
}



/*============================== instruction selection logic ==============================*/

/* no 1/2 due to precharging */
double ORION_Sel::anyreq_cap(u_int width) {
	double Ctotal;

	/* part 1: drain cap of precharging gate */
	Ctotal = ORION_Util_draincap(conf->PARM("WSelORprequ"), PCH, 1, conf );

	/* part 2: drain cap of NOR gate */
	Ctotal += width * ORION_Util_draincap(conf->PARM("WSelORn"), NCH, 1, conf );

	/* part 3: inverter cap */
	/* WHS: no size information, assume decinv */
	Ctotal += ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf ) + ORION_Util_draincap(
			conf->PARM("Wdecinvp"), PCH, 1, conf ) + ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 1, conf );

	return Ctotal;
}

/* no 1/2 due to req precharging */
double ORION_Sel::chgreq_cap() {
	/* gate cap of priority encoder */
	return (ORION_Util_gatecap(conf->PARM("WSelPn") + conf->PARM("WSelPp"), 1, conf ));
}

/* switching cap when priority encoder generates 1 (some req is 1) */
/* no 1/2 due to req precharging */
double ORION_Sel::enc_cap(u_int level) {
	double Ctotal;

	/* part 1: drain cap of priority encoder */
	Ctotal = level * ORION_Util_draincap(conf->PARM("WSelPn"), NCH, 1, conf ) + ORION_Util_draincap(
			conf->PARM("WSelPp"), PCH, level, conf );

	/* part 2: gate cap of next-level NAND gate */
	/* WHS: 20 should go to PARM */
	Ctotal += ORION_Util_gatecap(conf->PARM("WSelEnn") + conf->PARM("WSelEnp"), 20, conf );

	return Ctotal;
}

/* no 1/2 due to req precharging */
double ORION_Sel::grant_cap(u_int width) {
	double Ctotal;

	/* part 1: drain cap of NAND gate */
	Ctotal = ORION_Util_draincap(conf->PARM("WSelEnn"), NCH, 2, conf ) + 2 * ORION_Util_draincap(
			conf->PARM("WSelEnp"), PCH, 1, conf );

	/* part 2: inverter cap */
	/* WHS: no size information, assume decinv */
	Ctotal += ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf ) + ORION_Util_draincap(
			conf->PARM("Wdecinvp"), PCH, 1, conf ) + ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 1, conf );

	/* part 3: gate cap of enable signal */
	/* grant signal is enable signal to the lower-level arbiter */
	/* WHS: 20 should go to PARM */
	Ctotal += width * ORION_Util_gatecap(conf->PARM("WSelEnn") + conf->PARM("WSelEnp"), 20, conf );

	return Ctotal;
}

int ORION_Sel::init(int m, u_int w, ORION_Tech_Config *con) {
	u_int i;

	conf = con;
	if ((model = m) && m < SEL_MAX_MODEL) {
		width = w;

		n_anyreq = n_chgreq = n_grant = 0;
		for (i = 0; i < width; i++)
			n_enc[i] = 0;

		e_anyreq = anyreq_cap(width) * conf->PARM("EnergyFactor");
		e_chgreq = chgreq_cap() * conf->PARM("EnergyFactor");
		e_grant = grant_cap(width);

		for (i = 0; i < width; i++)
			e_enc[i] = enc_cap(i + 1);

		return 0;
	} else
		return -1;
}

/* WHS: maybe req should be of type LIB_Type_max_uint and use BIGONE instead of 1 */
int ORION_Sel::record(u_int req, int en, int last_level) {
	u_int i;
	int pre_grant = 0;

	if (req) {
		if (!last_level)
			n_anyreq++;

		/* assume LSB has the highest priority */
		for (i = 0; i < width; i++) {
			if (req & 1 << i) {
				n_chgreq += width - i;

				if (!pre_grant) {
					pre_grant = 1;

					n_enc[i]++;
					if (en)
						n_grant++;
				}
			}
		}
	}

	return 0;
}

double ORION_Sel::report() {
	u_int i;
	double Etotal;

	Etotal = n_anyreq * e_anyreq + n_chgreq * e_chgreq + n_grant * e_grant;

	for (i = 0; i < width; i++)
		Etotal += n_enc[i] * e_enc[i];

	return Etotal;
}

/* BEGIN: legacy */
/* WHS: 4 should go to PARM */
// while(win_entries > 4)
//   {
//     win_entries = (int)ceil((double)win_entries / 4.0);
//     num_arbiter += win_entries;
//   }
// Ctotal += PARM(ruu_issue_width) * num_arbiter*(Cor+Cpencode);
/* END: legacy */

/*============================== instruction selection logic ==============================*/

/*============================== bus (link) ==============================*/

int ORION_Bus::bitwidth(int encoding, u_int data_width, u_int grp_width) {
	if (encoding && encoding < BUS_MAX_ENC)
		switch (encoding) {
		case IDENT_ENC:
		case TRANS_ENC:
			return data_width;
		case BUSINV_ENC:
			return data_width + data_width / grp_width + (data_width
					% grp_width ? 1 : 0);
		default: /* some error handler */
			break;
		}
	else
		return -1;
}

/*
 * this function is provided to upper layers to compute the exact binary bus representation
 * only correct when grp_width divides data_width
 */
LIB_Type_max_uint ORION_Bus::state(LIB_Type_max_uint new_data) {
	LIB_Type_max_uint mask_bus, mask_data;
	LIB_Type_max_uint new_state = 0;
	LIB_Type_max_uint old_state = curr_state;
	u_int done_width = 0;

	switch (encoding) {
	case IDENT_ENC:
		return new_data;
	case TRANS_ENC:
		return new_data ^ curr_data;

	case BUSINV_ENC:
		/* FIXME: this function should be re-written for boundary checking */
		mask_data = (BIGONE << grp_width) - 1;
		mask_bus = (mask_data << 1) + 1;

		while (data_width > done_width) {
			if (ORION_Util_Hamming(old_state & mask_bus, new_data & mask_data,
					mask_bus) > grp_width / 2)
				new_state += (~(new_data & mask_data) & mask_bus) << done_width
						+ done_width / grp_width;
			else
				new_state += (new_data & mask_data) << done_width + done_width
						/ grp_width;

			done_width += grp_width;
			old_state >>= grp_width + 1;
			new_data >>= grp_width;
		}

		return new_state;

	default: /* some error handler */
		break;
	}
}

double ORION_Bus::resultbus_cap() {
	double Cline, reg_height;

	/* compute size of result bus tags */
	reg_height = conf->PARM("RUU_size") * (conf->PARM("RegCellHeight") + conf->PARM("WordlineSpacing") * 3 * conf->PARM(
			"ruu_issue_width"));

	/* assume num alu's = ialu */
	/* FIXME: generate a more detailed result bus network model */
	/* WHS: 3200 should go to PARM */
	/* WHS: use minimal pitch for buses */
	Cline = conf->PARM("CCmetal") * (reg_height + 0.5 * conf->PARM("res_ialu") * 3200 * conf->PARM("LSCALE"));

	/* or use result bus length measured from 21264 die photo */
	// Cline = CCmetal * 3.3 * 1000;

	return Cline;
}

double ORION_Bus::generic_bus_cap(u_int n_snd, u_int n_rcv, double length,
		double time) {
	double Ctotal = 0;
	double n_size, p_size;

	/* part 1: wire cap */
	/* WHS: use minimal pitch for buses */
	Ctotal += conf->PARM("CC2metal") * length;

	if ((n_snd == 1) && (n_rcv == 1)) {
		/* directed bus if only one sender and one receiver */

		/* part 2: repeater cap */
		/* FIXME: ratio taken from Raw, does not scale now */
		n_size = conf->PARM("Lamda") * 10;
		p_size = n_size * 2;

		Ctotal += ORION_Util_gatecap(n_size + p_size, 0, conf ) + ORION_Util_draincap(
				n_size, NCH, 1, conf ) + ORION_Util_draincap(p_size, PCH, 1, conf );

		n_size *= 2.5;
		p_size *= 2.5;

		Ctotal += ORION_Util_gatecap(n_size + p_size, 0, conf ) + ORION_Util_draincap(
				n_size, NCH, 1, conf ) + ORION_Util_draincap(p_size, PCH, 1, conf );
	} else {
		/* otherwise, broadcasting bus */

		/* part 2: input cap */
		/* WHS: no idea how input interface is, use an inverter for now */
		Ctotal += n_rcv * ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 0, conf );

		/* part 3: output driver cap */
		if (time) {
			p_size = ORION_Util_driver_size(Ctotal, time, conf );
			n_size = p_size / 2;
		} else {
			p_size = conf->PARM("Wbusdrvp");
			n_size = conf->PARM("Wbusdrvn");
		}

		Ctotal += n_snd * (ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf )
				+ ORION_Util_draincap(conf->PARM("Wdecinvp"), PCH, 1, conf ));
	}

	return Ctotal;
}

/*
 * width -> in bits

 * n_snd -> # of senders
 * n_rcv -> # of receivers
 * time  -> rise and fall time, 0 means using default transistor sizes
 * grp_width only matters for BUSINV_ENC
 */
int ORION_Bus::init(int m, int enc, u_int width, u_int g_width, u_int n_snd,
		u_int n_rcv, double length, double time, ORION_Tech_Config *con) {
	conf= con;
	if ((model = m) && m < BUS_MAX_MODEL) {
		data_width = width;
		grp_width = g_width;
		n_switch = 0;

		curr_data = 0;
		curr_state = 0;



		switch (model) {
		case RESULT_BUS:
			/* assume result bus uses identity encoding */
			encoding = IDENT_ENC;
			e_switch = resultbus_cap() / 2 * conf->PARM("EnergyFactor");
			break;

		case GENERIC_BUS:
			if ((encoding = encoding) && encoding < BUS_MAX_ENC) {
				e_switch = generic_bus_cap(n_snd, n_rcv, length, time) / 2
						* conf->PARM("EnergyFactor");
				/* sanity check */
				if (!grp_width || grp_width > width)
					grp_width = width;
			} else
				return -1;
			break;

		default: /* some error handler */
			break;
		}

		bit_width = bitwidth(encoding, width, grp_width);
		bus_mask = HAMM_MASK(bit_width);

		/* BEGIN: legacy */
		// npreg_width = ORION_Util_logtwo(PARM(RUU_size));
		/* assume ruu_issue_width result busses -- power can be scaled linearly
		 * for number of result busses (scale by writeback_access) */
		// static_af = 2 * (PARM(data_width) + npreg_width) * PARM(ruu_issue_width) * PARM(AF);
		/* END: legacy */

		return 0;
	} else
		return -1;
}

int ORION_Bus::record(LIB_Type_max_uint new_data) {
	LIB_Type_max_uint new_st = state(new_data);
	n_switch += ORION_Util_Hamming(new_st, curr_state, bus_mask);
	curr_data = new_data;
	curr_state = new_st;

	return 0;
}

double ORION_Bus::report() {
	return (n_switch * e_switch);
}

/*============================== bus (link) ==============================*/

/*============================== flip flop ==============================*/

/* this model is based on the gate-level design given by Randy H. Katz "Contemporary Logic Design"
 * Figure 6.24, node numbers (1-6) are assigned to all gate outputs, left to right, top to bottom
 *
 * We should have pure cap functions and leave the decision of whether or not to have coefficient
 * 1/2 in init function.
 */
double ORION_FF::node_cap(u_int fan_in, u_int fan_out) {
	double Ctotal = 0;

	/* FIXME: all need actual sizes */
	/* part 1: drain cap of NOR gate */
	Ctotal += fan_in * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf )
			+ ORION_Util_draincap(conf->PARM("WdecNORp"), PCH, fan_in, conf );

	/* part 2: gate cap of NOR gates */
	Ctotal += fan_out * ORION_Util_gatecap(conf->PARM("WdecNORn") + conf->PARM("WdecNORp"), 0, conf );

	return Ctotal;
}

double ORION_FF::clock_cap() {
	/* gate cap of clock load */
	return (2 * ORION_Util_gatecap(conf->PARM("WdecNORn") + conf->PARM("WdecNORp"), 0, conf ));
}

int ORION_FF::clear_stat() {
	n_switch = n_keep_1 = n_keep_0 = n_clock = 0;

	return 0;
}

int ORION_FF::init(int m, double load, ORION_Tech_Config *con) {
	double c1, c2, c3, c4, c5, c6;

	conf = con;

	n_switch = 0;
	n_clock = 0;
	n_keep_0 = 0;
	n_keep_1 = 0;

	if ((model = m) && m < FF_MAX_MODEL) {
		switch (model) {
		case NEG_DFF:
			clear_stat();

			/* node 5 and node 6 are identical to node 1 in capacitance */
			c1 = c5 = c6 = node_cap(2, 1);
			c2 = node_cap(2, 3);
			c3 = node_cap(3, 2);
			c4 = node_cap(2, 3);

			e_switch = (c4 + c1 + c2 + c3 + c5 + c6 + load) / 2 * conf->PARM("EnergyFactor");
			/* no 1/2 for e_keep and e_clock because clock signal switches twice in one cycle */
			e_keep_1 = c3 * conf->PARM("EnergyFactor");
			e_keep_0 = c2 * conf->PARM("EnergyFactor");
			e_clock = clock_cap() * conf->PARM("EnergyFactor");

			/* static power */
			I_static = (conf->PARM("WdecNORp") * TABS::NOR2_TAB[0] + conf->PARM("WdecNORn")
					* (TABS::NOR2_TAB[1] + TABS::NOR2_TAB[2]
							+ TABS::NOR2_TAB[3])) / 4 * 6 / conf->PARM("TECH_POINT")
					* 100;
			break;

		default: /* some error handler */
			break;
		}

		return 0;
	} else
		return -1;
}

double ORION_FF::report() {
	return (e_switch * n_switch + e_clock * n_clock + e_keep_0 * n_keep_0
			+ e_keep_1 * n_keep_1);
}

double ORION_FF::report_static_power() {
	return I_static * conf->PARM("Vdd") * conf->PARM("SCALE_S");
}

/*============================== flip flop ==============================*/

