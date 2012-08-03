
#include "ORION_Arbiter.h"

#include <stdio.h>
#include <math.h>

#include "statistics.h"

/*============================== arbiter ==============================*/

/* switch cap of request signal (round robin arbiter) */
double ORION_Arbiter::rr_req_cap(double length) {
	double Ctotal = 0;

	/* part 1: gate cap of 2 NOR gates */
	/* FIXME: need actual size */
	Ctotal += 2 * ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	/* part 2: inverter */
	/* FIXME: need actual size */
	Ctotal += ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf) + ORION_Util_draincap(conf->PARM("Wdecinvp"), PCH, 1, conf) +
	ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 0, conf);

	/* part 3: wire cap */
	Ctotal += length * conf->PARM("Cmetal");

	return Ctotal;
}

	/* switch cap of priority signal (round robin arbiter) */
double ORION_Arbiter::rr_pri_cap() {
	double Ctotal = 0;

	/* part 1: gate cap of NOR gate */
	/* FIXME: need actual size */
	Ctotal += ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	return Ctotal;
}

	/* switch cap of grant signal (round robin arbiter) */
double ORION_Arbiter::rr_grant_cap() {
	double Ctotal = 0;

	/* part 1: drain cap of NOR gate */
	/* FIXME: need actual size */
	Ctotal += 2 * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf) + ORION_Util_draincap(
			conf->PARM("WdecNORp"), PCH, 2, conf);

	return Ctotal;
}

/* switch cap of carry signal (round robin arbiter) */
double ORION_Arbiter::rr_carry_cap() {
	double Ctotal = 0;

	/* part 1: drain cap of NOR gate (this block) */
	/* FIXME: need actual size */
	Ctotal += 2 * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf) + ORION_Util_draincap(
			conf->PARM("WdecNORp"), PCH, 2, conf);

	/* part 2: gate cap of NOR gate (next block) */
	/* FIXME: need actual size */
	Ctotal += ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	return Ctotal;
}

	/* switch cap of internal carry getNode(round robin arbiter) */
double ORION_Arbiter::rr_carry_in_cap() {
	double Ctotal = 0;

	/* part 1: gate cap of 2 NOR gates */
	/* FIXME: need actual size */
	Ctotal += 2 * ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	/* part 2: drain cap of NOR gate */
	/* FIXME: need actual size */
	Ctotal += 2 * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf) +
	ORION_Util_draincap(conf->PARM("WdecNORp"), PCH, 2, conf);

	return Ctotal;
}

	/* the "huge" NOR gate in matrix arbiter model is an approximation */
	/* switch cap of request signal (matrix arbiter) */
double ORION_Arbiter::matrix_req_cap(u_int req_width, double length) {
	double Ctotal = 0;

	/* FIXME: all need actual sizes */
	/* part 1: gate cap of NOR gates */
	Ctotal += (req_width - 1) * ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	/* part 2: inverter */
	Ctotal += ORION_Util_draincap(conf->PARM("Wdecinvn"), NCH, 1, conf) + ORION_Util_draincap(conf->PARM("Wdecinvp"), PCH, 1, conf) +
	ORION_Util_gatecap(conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 0, conf);

	/* part 3: gate cap of the "huge" NOR gate */
	Ctotal += ORION_Util_gatecap(conf->PARM("WdecNORn") + conf->PARM("WdecNORp"), 0, conf);

	/* part 4: wire cap */
	Ctotal += length * conf->PARM("Cmetal");

	return Ctotal;
}

	/* switch cap of priority signal (matrix arbiter) */
double ORION_Arbiter::matrix_pri_cap(u_int req_width) {
	double Ctotal = 0;

	/* part 1: gate cap of NOR gates (2 groups) */
	Ctotal += 2 * ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	/* no inverter because priority signal is kept by a flip flop */
	return Ctotal;
}

	/* switch cap of grant signal (matrix arbiter) */
double ORION_Arbiter::matrix_grant_cap(u_int req_width) {
	/* drain cap of the "huge" NOR gate */
	return (req_width * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf)
			+ ORION_Util_draincap(conf->PARM("WdecNORp"), PCH, req_width, conf));
}

/* switch cap of internal getNode(matrix arbiter) */
double ORION_Arbiter::matrix_int_cap() {
	double Ctotal = 0;

	/* part 1: drain cap of NOR gate */
	Ctotal += 2 * ORION_Util_draincap(conf->PARM("WdecNORn"), NCH, 1, conf) + ORION_Util_draincap(
			conf->PARM("WdecNORp"), PCH, 2, conf);

	/* part 2: gate cap of the "huge" NOR gate */
	Ctotal += ORION_Util_gatecap(conf->PARM("WdecNORn")+ conf->PARM("WdecNORp"), 0, conf);

	return Ctotal;
}

int ORION_Arbiter::clear_stat() {
	n_chg_req = n_chg_grant = n_chg_mint = 0;
	n_chg_carry = n_chg_carry_in = 0;

	queue.clear_stat();
	pri_ff.clear_stat();

	return 0;
}

int ORION_Arbiter::init(int arbiter_model, int ff_model, u_int req,
		double length, ORION_Tech_Config *con) {

	conf = con;

	n_chg_grant = 0;
	n_chg_req = 0;
	n_chg_carry = 0;
	n_chg_carry_in = 0;

	n_chg_mint = 0;

	I_static = 0;

	arb_bus.init(RESULT_BUS, IDENT_ENC, req, req, req, 1, 20, 0, conf);

	if ((model = arbiter_model) && arbiter_model < ARBITER_MAX_MODEL) {
		req_width = req;
		clear_stat();
		/* redundant field */
		mask = HAMM_MASK(req_width);

		switch (arbiter_model) {
		case RR_ARBITER:
			e_chg_req = rr_req_cap(length) / 2 * conf->PARM("EnergyFactor");
			/* two grant signals switch together, so no 1/2 */
			e_chg_grant = rr_grant_cap() * conf->PARM("EnergyFactor");
			e_chg_carry = rr_carry_cap() / 2 * conf->PARM("EnergyFactor");
			e_chg_carry_in = rr_carry_in_cap() / 2 * conf->PARM("EnergyFactor");
			e_chg_mint = 0;

			I_static = 0;
			/* NOR */
			I_static += (6 * req_width * ((conf->PARM("WdecNORp")*TABS::NOR2_TAB[0]
					+ conf->PARM("WdecNORn")*(TABS::NOR2_TAB[1] + TABS::NOR2_TAB[2] + TABS::NOR2_TAB[3]))/4));

			/* inverter */
			I_static += 2 * req_width * ((conf->PARM("Wdecinvn")*TABS::NMOS_TAB[0]
					+ conf->PARM("Wdecinvp")*TABS::PMOS_TAB[0])/2);
			/* DFF */
			I_static += (req_width * conf->PARM("Wdff")*TABS::DFF_TAB[0]);

			if (pri_ff.init(ff_model, rr_pri_cap(), conf))
				return -1;
			break;

		case MATRIX_ARBITER:
			e_chg_req = matrix_req_cap(req_width, length) / 2 * conf->PARM("EnergyFactor");
			/* 2 grant signals switch together, so no 1/2 */
			e_chg_grant = matrix_grant_cap(req_width) * conf->PARM("EnergyFactor");
			e_chg_mint = matrix_int_cap() / 2 * conf->PARM("EnergyFactor");
			e_chg_carry = e_chg_carry_in = 0;

			I_static = 0;
			/* NOR */
			I_static += ((2 * req_width - 1) * req_width) * ((conf->PARM("WdecNORp")*TABS::NOR2_TAB[0]
			              + conf->PARM("WdecNORn")*(TABS::NOR2_TAB[1] + TABS::NOR2_TAB[2] + TABS::NOR2_TAB[3]))/4);
			/* inverter */
			I_static += req_width * ((conf->PARM("Wdecinvn")*TABS::NMOS_TAB[0] +
					conf->PARM("Wdecinvp")*TABS::PMOS_TAB[0])/2);
			/* DFF */
			I_static += (req_width * (req_width - 1) / 2) * (conf->PARM("Wdff")*TABS::DFF_TAB[0]);

			if (pri_ff.init(ff_model, matrix_pri_cap(req_width), conf))
				return -1;
			break;

		default: /* some error handler */
			break;
		}

		return 0;
	} else
		return -1;
}

int ORION_Arbiter::record(LIB_Type_max_uint new_req, u_int new_grant) {
	int nchgreq = 0;
	int nchggrant = 0;
	int nchgmint = 0;
	int prinswitch = 0;
	int nchgcarry = 0;
	int nchgcarryin = 0;

	switch (model) {
	case MATRIX_ARBITER:
		nchgreq = ORION_Util_Hamming(new_req, curr_req, mask);
		n_chg_req += nchgreq;
		if(new_grant >= 0)
			nchggrant = new_grant != curr_grant;
		n_chg_grant += nchggrant;
		/* FIXME: approximation */
		nchgmint = (req_width - 1) * req_width / 2;
		n_chg_mint += nchgmint;
		/* priority registers */
		/* FIXME: use average instead */
		prinswitch = (req_width - 1) / 2;
		pri_ff.n_switch += prinswitch;

		break;

	case RR_ARBITER:
		nchgreq = ORION_Util_Hamming(new_req, curr_req, mask);
		n_chg_req += nchgreq;
		if(new_grant >= 0)
			nchggrant = new_grant != curr_grant;
		n_chg_grant += nchggrant;
		/* FIXME: use average instead */
		nchgcarry = req_width / 2;
		n_chg_carry += nchgcarry;
		nchgcarryin = req_width / 2 - 1;
		if(req_width>1)
			n_chg_carry_in += nchgcarryin;
		/* priority registers */
		prinswitch = 2;
		pri_ff.n_switch += prinswitch;
		break;

	case QUEUE_ARBITER:
		break;

	default: /* some error handler */
		break;
	}

	double busE = arb_bus.report();
	arb_bus.record(new_grant);

	curr_req = new_req;
	curr_grant = new_grant;



	return 0;
}

double ORION_Arbiter::report() {

	/*std::cout << "n_chg_req " << n_chg_req << endl;
	std::cout << "e_chg_req " << e_chg_req << endl;
	std::cout << "n_chg_grant " << n_chg_grant << endl;
	std::cout << "e_chg_grant " << e_chg_grant << endl;
	std::cout << "n_chg_carry " << n_chg_carry << endl;
	std::cout << "e_chg_carry " << e_chg_carry << endl;
	std::cout << "n_chg_carry_in " << n_chg_carry_in << endl;
	std::cout << "e_chg_carry_in " << e_chg_carry_in << endl;
	std::cout << "arb_bus " << arb_bus.report() << endl;*/
	//std::cout << "pri_ff.report(time) " << pri_ff.report(time) << endl;


	double ret = 0;

	switch (model) {
		case MATRIX_ARBITER:
			//fprintf(stdout, "arbiter: chg_req(%d) E_chg_req(%e) chg_grant(%d) E_chg_grant(%e) chg_mint(%d) E_chg_mint(%e) pri_ff(%e) pri_ff.n_switch(%d) pri_ff.e_switch(%e)\n",  n_chg_req,  e_chg_req,  n_chg_grant,  e_chg_grant, n_chg_mint, e_chg_mint, pri_ff.report(), pri_ff.n_switch, pri_ff.e_switch);
			ret = (n_chg_req * e_chg_req + n_chg_grant * e_chg_grant + n_chg_mint
					* e_chg_mint + arb_bus.report() + pri_ff.report());
			break;

		case RR_ARBITER:
			ret = (n_chg_req * e_chg_req + n_chg_grant * e_chg_grant + n_chg_carry
					* e_chg_carry + n_chg_carry_in * e_chg_carry_in
					+ arb_bus.report() + pri_ff.report());
			break;
		default:
			return -1;
	}


	return ret;

}

double ORION_Arbiter::report_static_power(){
	return I_static * conf->PARM("Vdd") * conf->PARM("SCALE_S") + pri_ff.report_static_power();

}

/* stat over one cycle */
/* info is only used by queuing arbiter */
double ORION_Arbiter::stat_energy(int max_avg) {
	/*double Eavg = 0, Estruct, Eatomic;
	int next_depth, next_next_depth;
	double total_pri, n_chg_pri, n_grant;
	u_int path_len, next_path_len;
	int n_req = req_width;

	// energy cycle distribution

	if (n_req >= 1)
		n_grant = 1;
	else
		n_grant = 1.0 / ceil(1.0 / n_req);

	switch (model) {
	case RR_ARBITER:
		// FIXME: we may overestimate request switch
		Eatomic = e_chg_req * n_req;

		Eavg += Eatomic;

		Eatomic = e_chg_grant * n_grant;

		Eavg += Eatomic;

		// assume carry signal propagates half length in average case
		// carry does not propagate in maximum case, i.e. all carrys go down
		Eatomic = e_chg_carry * req_width * (max_avg ? 1 : 0.5) * n_grant;

		Eavg += Eatomic;

		Eatomic = e_chg_carry_in * (req_width * (max_avg ? 1 : 0.5) - 1)
				* n_grant;

		Eavg += Eatomic;

		// priority registers
		Estruct = 0;

		Eatomic = pri_ff.e_switch * 2 * n_grant;

		Estruct += Eatomic;

		Eatomic = pri_ff.e_keep_0 * (req_width - 2 * n_grant);

		Estruct += Eatomic;

		Eatomic = pri_ff.e_clock * req_width;

		Estruct += Eatomic;

		Eavg += Estruct;
		break;

	case MATRIX_ARBITER:
		total_pri = req_width * (req_width - 1) * 0.5;
		// assume switch probability 0.5 for priorities
		n_chg_pri = (req_width - 1) * (max_avg ? 1 : 0.5);

		// FIXME: we may overestimate request switch
		Eatomic = e_chg_req * n_req;

		Eavg += Eatomic;

		Eatomic = e_chg_grant * n_grant;

		Eavg += Eatomic;

		// priority registers
		Estruct = 0;

		Eatomic = pri_ff.e_switch * n_chg_pri * n_grant;

		Estruct += Eatomic;

		// assume 1 and 0 are uniformly distributed
		if (pri_ff.e_keep_0 >= pri_ff.e_keep_1 || !max_avg) {
			Eatomic = pri_ff.e_keep_0 * (total_pri - n_chg_pri * n_grant)
					* (max_avg ? 1 : 0.5);

			Estruct += Eatomic;
		}

		if (pri_ff.e_keep_0 < pri_ff.e_keep_1 || !max_avg) {
			Eatomic = pri_ff.e_keep_1 * (total_pri - n_chg_pri * n_grant)
					* (max_avg ? 1 : 0.5);

			Estruct += Eatomic;
		}

		Eatomic = pri_ff.e_clock * total_pri;

		Estruct += Eatomic;

		Eavg += Estruct;

		// based on above assumptions
		if (max_avg)
			// min(p,n/2)(n-1) + 2(n-1)
			Eatomic = e_chg_mint * (MIN(n_req, req_width * 0.5) + 2) * (req_width - 1);
		else
			// p(n-1)/2 + (n-1)/2
			Eatomic = e_chg_mint * (n_req + 1) * (req_width - 1) * 0.5;

		Eavg += Eatomic;
		break;

	case QUEUE_ARBITER:
		// FIXME: what if n_req > 1?
		Eavg = queue.stat_energy(n_req, n_grant, max_avg, Period);

		break;

	default: // some error handler
		break;
	}

	return Eavg;

	*/

	return -1000;  //don't use this function
}

/*============================== arbiter ==============================*/
