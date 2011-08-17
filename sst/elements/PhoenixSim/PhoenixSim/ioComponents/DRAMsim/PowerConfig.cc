#include "PowerConfig.h"
#include "PowerCounter.h"

#define MEM_POWER_GLOBAL 0
#define MEM_POWER_PERIODIC 1

PowerConfig::PowerConfig(DRAM_config* cfg, Memory_Controller** memC) {
	system_config = cfg;
	dram_controller = memC;
	initialize_power_config();
}

void PowerConfig::initialize_power_config() {

	//configured for Micron DDR3 2Gb x8
	max_VDD = 1.575;
	min_VDD = 1.425;
	VDD = 1.5;
	P_per_DQ = .0052;
	density = 128;
	DQ = 8;
	DQS = 2;
	IDD0 = .120;
	IDD2P = .025;
	IDD2F = .060;
	IDD3P = .050;
	IDD3N = .080;
	IDD4R = .225;
	IDD4W = .265;
	IDD5 = .285;
	t_CK = 1.875;
	t_RC = 50.625;
	t_RFC_min = 160;
	t_REFI = 7.8;
	print_period = 400;

	calculate_power_values();

	last_print_time = 0;

	//dump_power_config(stdout);
}

void PowerConfig::calculate_power_values() {
	VDD_scale = (VDD * VDD) / (max_VDD * max_VDD);
	freq_scale = (t_CK * (float) system_config->get_dram_frequency() / 2)
			/ 1000.0; /*** FIX ME : use_freq/spec_freq***/

	p_PRE_PDN = IDD2P * max_VDD * VDD_scale;
	p_PRE_STBY = IDD2F * max_VDD * VDD_scale * freq_scale;
	p_ACT_PDN = IDD3P * max_VDD * VDD_scale;
	p_ACT_STBY = IDD3N * max_VDD * VDD_scale * freq_scale;
	p_WR = (IDD4W - IDD3N) * max_VDD * VDD_scale * freq_scale;
	p_RD = (IDD4R - IDD3N) * max_VDD * VDD_scale * freq_scale;
	p_DQ = P_per_DQ * (DQ + DQS) * freq_scale;
	p_REF = (IDD5 - IDD3N) * max_VDD * VDD_scale * freq_scale;

	if (system_config->get_dram_type() == DDRSDRAM)
		p_ACT = (float) (IDD0 - IDD3N) * max_VDD * VDD_scale * freq_scale;
	else
		// DDR2
		p_ACT = (float) (IDD0 - ((IDD3N * (float) system_config->t_ras + IDD2F
				* (float) (system_config->t_rc - system_config->t_ras))
				/ system_config->t_rc)) * max_VDD * VDD_scale * freq_scale;

	return;
}

void PowerConfig::dump_power_config(FILE *stream) {

	fprintf(stream,
			"  Operating Current: One Bank Active-Precharge : IDD0 %d\n", IDD0);
	fprintf(stream, "  Precharge Power Down Current : IDD2P %d\n", IDD2P);
	fprintf(stream, "  Precharge Standby Current : IDD3P %d\n", IDD3P);
	fprintf(stream, "  Active Power Down Current : IDD2F %d\n", IDD2F);
	fprintf(stream, "  Active Standby Current : IDD3N %d\n", IDD3N);
	fprintf(stream, "  Operating Burst Read  Current : IDD4R %d\n", IDD4R);
	fprintf(stream, "  Operating Burst Write Current : IDD4W %d\n", IDD4W);
	fprintf(stream, "  Operating Burst Refresh Current : IDD5 %d\n", IDD5);
	fprintf(stream, "  t_RFC_min: %f ns\n", t_RFC_min);

	if (system_config->dram_type == FBD_DDR2) {
		fprintf(
				stream,
				"  Idle Current, single or last DIMM L0 state : ICC_Idle_0 %f\n",
				ICC_Idle_0);
		fprintf(stream,
				"  Idle Current, first DIMM L0 state : ICC_Idle_1 %f\n",
				ICC_Idle_1);
		fprintf(stream,
				"  Idle Current, DRAM power down L0 state : ICC_Idle_2 %f\n",
				ICC_Idle_2);
		fprintf(stream, "  Active Power L0 state : ICC_Active_1 %f\n",
				ICC_Active_1);
		fprintf(stream, "  Active Power L0 state : ICC_Active_2 %f\n",
				ICC_Active_2);

	}
	fprintf(stream, "  p_PRE_PDN   %f\n", p_PRE_PDN);
	fprintf(stream, "  p_PRE_STBY  %f\n", p_PRE_STBY);
	fprintf(stream, "  p_ACT_PDN   %f\n", p_ACT_PDN);
	fprintf(stream, "  p_ACT_STBY  %f\n", p_ACT_STBY);
	fprintf(stream, "  p_ACT       %f\n", p_ACT);
	fprintf(stream, "  p_WR        %f\n", p_WR);
	fprintf(stream, "  p_RD        %f\n", p_RD);
	fprintf(stream, "  p_REF       %f\n", p_REF);
	fprintf(stream, "  p_DQ        %f\n", p_DQ);
}

/*
 * This function essentially computes the Power Consumption of a given rank on
 * a given channel
 * duration for you to select whether you want global or periodic power info!
 */

void PowerConfig::do_power_calc(tick_t now, int chan_id, int rank_id,
		PowerMeasure *meas_ptr, int type) {

	float sum_WR_percent = 0;
	float sum_RD_percent = 0;
	int j;
	PowerCounter *r_p_info_ptr;

	if (type == MEM_POWER_GLOBAL) {
		r_p_info_ptr = &(dram_controller[chan_id]->rank[rank_id].r_p_gblinfo);
		// We need to take care of making sure that n_ACT is bounded
	} else
		r_p_info_ptr = &(dram_controller[chan_id]->rank[rank_id].r_p_info);

	//Avoiding divided by zero
	if (r_p_info_ptr->RAS_count == 0)
		r_p_info_ptr->RAS_count = 1;

	if ((system_config->dram_type == DDR2 || system_config->dram_type
			== FBD_DDR2 || system_config->dram_type
			== DDR3)) {
		//Calculate sum of RD/WR for the whole DRAM system
		for (j = 0; j < system_config->rank_count; j++) {
			if (type == MEM_POWER_GLOBAL) {
				sum_WR_percent
						+= (float) dram_controller[chan_id]->rank[j].r_p_gblinfo.sum_per_WR
								/ (float) (dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										> 0 ? dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										: 1); //Ohm: change RAS_count to RAS_sum
				sum_RD_percent
						+= (float) dram_controller[chan_id]->rank[j].r_p_gblinfo.sum_per_RD
								/ (float) (dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										> 0 ? dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										: 1);
			} else {
				sum_WR_percent
						+= (float) dram_controller[chan_id]->rank[j].r_p_info.sum_per_WR
								/ (float) (dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										> 0 ? dram_controller[chan_id]->rank[j].r_p_info.RAS_sum
										: 1);
				sum_RD_percent
						+= (float) dram_controller[chan_id]->rank[j].r_p_info.sum_per_RD
								/ (float) (dram_controller[chan_id]->rank[j].r_p_gblinfo.RAS_sum
										> 0 ? dram_controller[chan_id]->rank[j].r_p_info.RAS_sum
										: 1);
			}
		}
	}

	// Power Calculation
	// Activate Power
	// In memory clock cycles RAS_sum = # of cycles that RAS has been active
	//
	meas_ptr->t_ACT = (float) r_p_info_ptr->RAS_sum
			/ ((float) r_p_info_ptr->RAS_count);

	if (meas_ptr->t_ACT == 0.0)
		meas_ptr->p_ACT = 0; // No ACT issued in the last print period
	else {
		if (system_config->dram_type == DDRSDRAM)
			//		  meas_ptr->p_ACT = (float)p_ACT * (t_RC/(float)(t_CK * meas_ptr->t_ACT));
			meas_ptr->p_ACT = (float) p_ACT * (system_config->t_rc
					/ (float) (meas_ptr->t_ACT));

		else
			// DDR2
			//	  		meas_ptr->p_ACT = (float)p_ACT * (t_RC/(float)(t_CK * meas_ptr->t_ACT));
			meas_ptr->p_ACT = (float) p_ACT * (system_config->t_rc
					/ (float) (meas_ptr->t_ACT));
	}
	// Background Power
	meas_ptr->CKE_LO_PRE = 1.0 - ((float) r_p_info_ptr->cke_hi_pre
			/ (float) (r_p_info_ptr->bnk_pre > 0 ? r_p_info_ptr->bnk_pre : 1));
	if (type == MEM_POWER_PERIODIC) {
		if ((float) ((now - last_print_time) - r_p_info_ptr->bnk_pre) > 0) {
			meas_ptr->CKE_LO_ACT = 1.0
					- ((float) r_p_info_ptr->cke_hi_act / (float) ((now
							- last_print_time) - r_p_info_ptr->bnk_pre));
		}else{
			meas_ptr->CKE_LO_ACT = 0;
		}

		meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre / ((float) (now
				- last_print_time));

		//meas_ptr->BNK_PRE = (float)  r_p_info_ptr->bnk_pre/((float)r_p_info_ptr->RAS_sum);
	} else {
		meas_ptr->CKE_LO_ACT = 1.0 - ((float) r_p_info_ptr->cke_hi_act
				/ (float) (now - r_p_info_ptr->bnk_pre));

		meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre / ((float) (now));

		//meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre/( (float)(r_p_info_ptr->RAS_sum));
	}
	if (meas_ptr->CKE_LO_PRE < 0)
		meas_ptr->CKE_LO_PRE = 0;
	if (meas_ptr->CKE_LO_ACT < 0)
		meas_ptr->CKE_LO_ACT = 0;

	meas_ptr->p_PRE_PDN = p_PRE_PDN * meas_ptr->BNK_PRE * meas_ptr->CKE_LO_PRE;
	meas_ptr->p_PRE_STBY = p_PRE_STBY * meas_ptr->BNK_PRE * (1.0
			- meas_ptr->CKE_LO_PRE);
	meas_ptr->p_PRE_MAX = p_PRE_STBY * meas_ptr->BNK_PRE * 1.0; /* The max if there was no power down */
	meas_ptr->p_ACT_PDN = p_ACT_PDN * (1 - meas_ptr->BNK_PRE)
			* meas_ptr->CKE_LO_ACT;
	meas_ptr->p_ACT_STBY = p_ACT_STBY * (1 - meas_ptr->BNK_PRE) * (1.0
			- meas_ptr->CKE_LO_ACT);
	meas_ptr->p_ACT_MAX = p_ACT_STBY * (1 - meas_ptr->BNK_PRE) * (1.0);
	// Read/Write Power
	if (r_p_info_ptr->RAS_sum > 0) {
		meas_ptr->WR_percent = (float) r_p_info_ptr->sum_per_WR
				/ (float) r_p_info_ptr->RAS_sum;
		meas_ptr->RD_percent = (float) r_p_info_ptr->sum_per_RD
				/ (float) r_p_info_ptr->RAS_sum;
	} else {
		meas_ptr->WR_percent = 0;
		meas_ptr->RD_percent = 0;
	}

	meas_ptr->p_WR = p_WR * meas_ptr->WR_percent;
	meas_ptr->p_RD = p_RD * meas_ptr->RD_percent;
	meas_ptr->p_DQ = p_DQ * meas_ptr->RD_percent;

	meas_ptr->p_REF = 0, meas_ptr->p_TOT = 0, meas_ptr->p_TOT_MAX = 0;

	//reset p_AMB to zero to prevent printing junk in non-AMB DRAM
	meas_ptr->p_AMB = 0;
	meas_ptr->p_AMB_IDLE = 0;
	meas_ptr->p_AMB_ACT = 0;
	meas_ptr->p_AMB_PASS_THROUGH = 0;

	if (system_config->dram_type == DDRSDRAM) {
		//ignore the refresh power for now
		//p_REF = (IDD5 - IDD2P) * max_VDD * VDD_scale;
		meas_ptr->p_REF = p_REF * r_p_info_ptr->refresh_count;

		meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY
				+ meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT
				+ meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ
				+ meas_ptr->p_REF;
		// At this point, we have to total power per chip
		// need to calculate the total power of eack rank
		int chip_count = (system_config->channel_width * 8) / DQ; //number of chips per rank FIXME
		meas_ptr->p_TOT *= chip_count; // Total power for a rank
		meas_ptr->p_TOT_MAX *= chip_count; // Total power for a rank
	} else if (system_config->dram_type == DDR2 || system_config->dram_type
			== DDR3) {
		//ignore the refresh power for now
		//p_REF = (IDD5 - IDD3N) * max_VDD * t_RFC_min/(t_REFI * 1000) * VDD_scale; // Assume auto-refresh
		float p_dq_RD = 0;
		float p_dq_WR = 0;
		float p_dq_RDoth = 0;
		float p_dq_WRoth = 0;
		float termRDsch = 0;
		float termWRsch = 0;

		if (system_config->rank_count == 1) { /* FIXME What about 2/3? */
			p_dq_RD = 1.1;
			p_dq_WR = 8.2;

		} else { // rank_count == 4
			p_dq_RD = 1.5;
			p_dq_WR = 0;
			p_dq_RDoth = 13.1;
			p_dq_WRoth = 14.6;

			//calculate termRDsch and terWRsch
			// *** FIXME : I don't know how to calculate this ***
			// Average teh RD/WR percent of the other DRAMs
			termRDsch = (sum_RD_percent - meas_ptr->RD_percent)
					/ (system_config->channel_count * system_config->rank_count
							- 1);
			termWRsch = (sum_WR_percent - meas_ptr->WR_percent)
					/ (system_config->channel_count * system_config->rank_count
							- 1);
		}
		// Let's calculate the DQ and termination power!
		meas_ptr->p_DQ = p_dq_RD * (DQ + DQS) * meas_ptr->RD_percent;
		float p_termW = p_dq_WR * (DQ + DQS + 1) * meas_ptr->WR_percent;
		float p_termRoth = p_dq_RDoth * (DQ + DQS) * termRDsch;
		float p_termWoth = p_dq_WRoth * (DQ + DQS + 1) * termWRsch;

		if (r_p_info_ptr->refresh_count > 0) {
			if (type == MEM_POWER_PERIODIC) {
				meas_ptr->p_REF = p_REF * (float) t_RFC_min / ((now
						- last_print_time) / ((system_config->memory_frequency
						/ 1000.0) * r_p_info_ptr->refresh_count));
			} else {
				meas_ptr->p_REF = p_REF * (float) t_RFC_min / ((now)
						/ ((system_config->memory_frequency / 1000.0)
								* r_p_info_ptr->refresh_count));

			}
		}

		meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY
				+ meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT
				+ meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ
				+ meas_ptr->p_REF + p_termW + p_termRoth + p_termWoth;
		meas_ptr->p_TOT_MAX = meas_ptr->p_PRE_MAX + meas_ptr->p_ACT_MAX
				+ meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD
				+ meas_ptr->p_DQ + meas_ptr->p_REF + p_termW + p_termRoth
				+ p_termWoth;
		// At this point, we have to total power per chip
		// need to calculate the total power of eack rank
		int chip_count = (system_config->channel_width * 8) / DQ; //number of chips per rank FIXME
		meas_ptr->p_TOT *= chip_count; // Total power for a rank
		meas_ptr->p_TOT_MAX *= chip_count; // Total power for a rank

	} else if (system_config->dram_type == FBD_DDR2) {
		/* FIXME How do we take care of termination and DQ ? */
		/* FIXME Paramterize these assuming one rank per dimm */
		float p_dq_RD = 1.1;
		float p_dq_WR = 8.2;
		meas_ptr->p_DQ = p_dq_RD * (DQ + DQS) * meas_ptr->RD_percent + p_dq_WR
				* (DQ + DQS + 1) * meas_ptr->WR_percent;
		if (r_p_info_ptr->refresh_count > 0) {
			if (type == MEM_POWER_PERIODIC) {
				meas_ptr->p_REF = p_REF * (float) t_RFC_min / ((now
						- last_print_time) / ((system_config->memory_frequency
						/ 1000.0) * r_p_info_ptr->refresh_count));
			} else {
				meas_ptr->p_REF = p_REF * (float) t_RFC_min / ((now)
						/ ((system_config->memory_frequency / 1000.0)
								* r_p_info_ptr->refresh_count));
			}
		}
		/* Calculate p_AMB */
		// Note that if the system is running using traces we need to bound
		// amb_busy and amb_data_pass_through
		if (r_p_info_ptr->amb_busy > now)
			r_p_info_ptr->amb_busy = now;
		if (r_p_info_ptr->amb_data_pass_through > now)
			r_p_info_ptr->amb_data_pass_through = now;

		meas_ptr->p_AMB_ACT = VCC * ICC_Active_1 * r_p_info_ptr->amb_busy
				* 1000;
		meas_ptr->p_AMB_PASS_THROUGH = VCC * ICC_Active_2
				* r_p_info_ptr->amb_data_pass_through * 1000; /*ICC is in A */

		if (type == MEM_POWER_PERIODIC) {
			if (rank_id == system_config->rank_count - 1) {
				meas_ptr->p_AMB_IDLE = VCC * ICC_Idle_0 * (now
						- last_print_time - r_p_info_ptr->amb_busy
						- r_p_info_ptr->amb_data_pass_through) * 1000; /*ICC is in A */
			} else {
				meas_ptr->p_AMB_IDLE = VCC * ICC_Idle_1 * (now
						- last_print_time - r_p_info_ptr->amb_busy
						- r_p_info_ptr->amb_data_pass_through) * 1000; /*ICC is in A */
			}
			meas_ptr->p_AMB = meas_ptr->p_AMB_IDLE + meas_ptr->p_AMB_ACT
					+ meas_ptr->p_AMB_PASS_THROUGH; /*ICC is in A */
			meas_ptr->p_AMB = meas_ptr->p_AMB / (now - last_print_time);
			meas_ptr->p_AMB_IDLE /= (now - last_print_time);
			meas_ptr->p_AMB_ACT /= (now - last_print_time);
			meas_ptr->p_AMB_PASS_THROUGH /= (now - last_print_time);
		} else {
			if (rank_id == system_config->rank_count - 1) {
				meas_ptr->p_AMB_IDLE = VCC * ICC_Idle_0 * (now
						- r_p_info_ptr->amb_busy
						- r_p_info_ptr->amb_data_pass_through) * 1000; /*ICC is in A */
			} else {
				meas_ptr->p_AMB_IDLE = VCC * ICC_Idle_1 * (now
						- r_p_info_ptr->amb_busy
						- r_p_info_ptr->amb_data_pass_through) * 1000; /*ICC is in A */
			}
			meas_ptr->p_AMB = meas_ptr->p_AMB_IDLE + meas_ptr->p_AMB_ACT
					+ meas_ptr->p_AMB_PASS_THROUGH; /*ICC is in A */
			meas_ptr->p_AMB = meas_ptr->p_AMB / (now);
			meas_ptr->p_AMB_IDLE = meas_ptr->p_AMB_IDLE / (now);
			meas_ptr->p_AMB_ACT /= (now);
			meas_ptr->p_AMB_PASS_THROUGH /= (now);
		}

		meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY
				+ meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT
				+ meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ;
		meas_ptr->p_TOT_MAX = meas_ptr->p_PRE_MAX + meas_ptr->p_ACT_MAX
				+ meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD
				+ meas_ptr->p_DQ;
		// At this point, we have to total power per chip
		// need to calculate the total power of eack rank
		meas_ptr->p_TOT *= chip_count; // Total power for a rank
		meas_ptr->p_TOT_MAX *= chip_count; // Total power for a rank
		meas_ptr->p_TOT += meas_ptr->p_AMB;
		meas_ptr->p_TOT_MAX += meas_ptr->p_AMB;
	} else {
		meas_ptr->p_TOT = 0; // not support other types of DRAM
	}

}

void PowerConfig::mem_initialize_power_collection(char *filename) {
	pwr_filename = filename;
	if (filename != NULL) {
		if ((power_file_ptr = fopen(filename, "a")) == NULL) {
			fprintf(stdout,"Cannot open power stats file %s\n", filename);
			exit(0);
		}
	} else {
		power_file_ptr = stdout;
	}
	last_print_time = 0;
}

void PowerConfig::power_update_freq_based_values(int freq) {
	freq_scale = (t_CK * (float) system_config->get_dram_frequency() / 2)
			/ 1000.0; /*** FIX ME : use_freq/spec_freq***/
}

void PowerConfig::update(int theint) {
	power_update_freq_based_values(theint);
}

//Ohm--Add 3 functions
//print_power_stats() : calculate and print power every a period of time.
//check_cke_hi_pre() : check if the current tick is in precharge state with CKE enable
//check_cke_hi_act(): check if the current tick is in active state with CKE enable
void PowerConfig::print_power_stats(tick_t now) {
	//FILE *fp;
	int chan_id, rank_id;

	int chip_count = (system_config->channel_width * 8) / DQ; //number of chips per rank FIXME

	if (now < last_print_time + print_period || power_file_ptr == NULL) {
		return;
	}
#ifdef DEBUG_POWER
	fprintf(stdout,"\n===================print_power_stats====================\n");
#endif
	//assert (dram_power->power_file_ptr != NULL);
	/* This stuff should not be calculated every time fix it */
	t_RC = (float) (system_config->t_rc * t_CK / 2); // spec t_RC in nanosecond unit FIXME
	//DQ = density * 1024 * 1024 / (system_config->row_count
	//		* system_config->bank_count * system_config->col_count); /* FIXME : need to do calculation during configure not all the time */

	for (chan_id = 0; chan_id < system_config->channel_count; chan_id++) {
		for (rank_id = 0; rank_id < system_config->rank_count; rank_id++) {
			PowerMeasure meas_tmp;
			do_power_calc(now, chan_id, rank_id, &meas_tmp, MEM_POWER_PERIODIC);
			if (rank_id == 0)
				fprintf(power_file_ptr, "%llu    ", now);
			else
				fprintf(power_file_ptr, "            ");

			fprintf(
					power_file_ptr,
					"ch[%d] ra[%d] p_PRE_PDN[%5.2f] p_PRE_STBY[%5.2f] p_ACT_PDN[%5.2f]  p_ACT_STBY[%5.2f] p_ACT[%5.2f] p_WR[%5.2f] p_RD[%5.2f]  p_REF[%5.2f] p_AMB_IDLE[%5.2f] p_AMB_ACT[%5.2f] p_AMB_PASS_THROUGH[%5.2f] p_AMB[%5.2f] p_TOT[%5.2f] P_TOT_MAX[%5.2f] access[%d] \n",
					chan_id,
					rank_id,
					meas_tmp.p_PRE_PDN,
					meas_tmp.p_PRE_STBY,
					meas_tmp.p_ACT_PDN,
					meas_tmp.p_ACT_STBY,
					meas_tmp.p_ACT,
					meas_tmp.p_WR,
					meas_tmp.p_RD,
					meas_tmp.p_REF,
					meas_tmp.p_AMB_IDLE,
					meas_tmp.p_AMB_ACT,
					meas_tmp.p_AMB_PASS_THROUGH,
					meas_tmp.p_AMB,
					meas_tmp.p_TOT,
					meas_tmp.p_TOT_MAX,
					dram_controller[chan_id]->rank[rank_id].r_p_info.dram_access);

		}
		fprintf(power_file_ptr, "\n");
	}
	for (chan_id = 0; chan_id < system_config->channel_count; chan_id++) {
		for (rank_id = 0; rank_id < system_config->rank_count; rank_id++) {
			dram_controller[chan_id]->rank[rank_id].r_p_info.bnk_pre = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.cke_hi_pre = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.cke_hi_act = 0;
			//dram_controller[chan_id]->rank[rank_id].last_RAS_time = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.RAS_count = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.RAS_sum = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.sum_per_RD = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.sum_per_WR = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.dram_access = 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.amb_busy
					= dram_controller[chan_id]->rank[rank_id].r_p_info.amb_busy_spill_over;
			dram_controller[chan_id]->rank[rank_id].r_p_info.amb_data_pass_through
					= dram_controller[chan_id]->rank[rank_id].r_p_info.amb_data_pass_through_spill_over;
			dram_controller[chan_id]->rank[rank_id].r_p_info.amb_busy_spill_over
					= 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.amb_data_pass_through_spill_over
					= 0;
			dram_controller[chan_id]->rank[rank_id].r_p_info.refresh_count = 0;
		}
	}
	last_print_time = now;
}

/* Is rank busy ? AMB busy => true
 * Is rank not busy ? Bundle issued => AMB data pass through
 */
void PowerConfig::update_amb_power_stats(Command *bundle[], int command_count,
		tick_t now) {

	int i;
	tick_t completion_time;
	int chan_id = bundle[0]->chan_id;
	tick_t dram_power_print_time = last_print_time + print_period;

	/* Update busy till */
	for (i = 0; i < command_count && bundle[i] != NULL; i++) {
		completion_time = bundle[i]->get_command_completion_time();
		if (completion_time
				> dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till) {
			dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_last
					= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till;
			dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till
					= completion_time;

			dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_last
					= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_till;
			dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_till
					= completion_time;

			if (dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_last
					> now) {
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.amb_busy
						+= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till
								- dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_last;
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.amb_busy
						+= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_till
								- dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_last;
			} else {
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.amb_busy
						+= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till
								- now;
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.amb_busy
						+= dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_gblinfo.busy_till
								- now;
			}
			//Take care of spill over cycles
			if (dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till
					> dram_power_print_time) {
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.amb_busy
						+= -dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till
								+ dram_power_print_time;
				dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.amb_busy_spill_over
						+= -dram_power_print_time
								+ dram_controller[bundle[i]->chan_id]->rank[bundle[i]->rank_id].r_p_info.busy_till;

			}
		}
	}
	for (i = 0; i < system_config->rank_count; i++) {
		if (dram_controller[chan_id]->rank[i].r_p_info.busy_till < now) {
			dram_controller[chan_id]->rank[i].r_p_info.amb_data_pass_through
					+= system_config->t_bundle;
			dram_controller[chan_id]->rank[i].r_p_gblinfo.amb_data_pass_through
					+= system_config->t_bundle;
			//Take care of spill over cycles
			if (now + system_config->t_bundle > dram_power_print_time) {
				dram_controller[chan_id]->rank[i].r_p_info.amb_data_pass_through_spill_over
						+= now + system_config->t_bundle
								- dram_power_print_time;
				dram_controller[chan_id]->rank[i].r_p_info.amb_data_pass_through
						+= -now - system_config->t_bundle
								+ dram_power_print_time;
			}
		}
	}

}

void PowerConfig::print_global_power_stats(FILE *fileout, int dram_current_time) {
	int chan_id, rank_id;

	tick_t now = (tick_t) dram_current_time;

	/* This stuff should not be calculated every time fix it */
	t_RC = (float) (system_config->t_rc * t_CK / 2); // spec t_RC in nanosecond unit FIXME
	DQ = density * 1024 * 1024 / (system_config->row_count
			* system_config->bank_count * system_config->col_count); /* FIXME : need to do calculation during configure not all the time */
	if (system_config->get_dram_type() == DDRSDRAM)
		p_ACT = (float) (IDD0 - IDD3N) * max_VDD * VDD_scale;
	else
		// DDR2
		p_ACT = (float) (IDD0 - ((IDD3N * (float) system_config->t_ras + IDD2F
				* (float) (system_config->t_rc - system_config->t_ras))
				/ system_config->t_rc)) * max_VDD * VDD_scale;
	fprintf(fileout, " ---------- POWER STATS ------------------------\n");
	for (chan_id = 0; chan_id < system_config->channel_count; chan_id++) {
		for (rank_id = 0; rank_id < system_config->rank_count; rank_id++) {
			PowerMeasure meas_tmp;
			do_power_calc(now, chan_id, rank_id, &meas_tmp, MEM_POWER_GLOBAL);
			if (rank_id == 0)
				fprintf(fileout, "%llu    ", now);
			else
				fprintf(fileout, "            ");

			fprintf(
					fileout,
					"ch[%d] ra[%d] p_PRE_PDN[%5.2f] p_PRE_STBY[%5.2f] p_ACT_PDN[%5.2f]  p_ACT_STBY[%5.2f] p_ACT[%5.2f] p_WR[%5.2f] p_RD[%5.2f] p_REF[%5.2f] p_AMB_IDLE[%5.2f] p_AMB_ACT[%5.2f] p_AMB_PASS_THROUGH[%5.2f] p_AMB[%5.2f] p_TOT[%5.2f] P_TOT_MAX[%5.2f] access[%d]\n",
					chan_id,
					rank_id,
					meas_tmp.p_PRE_PDN,
					meas_tmp.p_PRE_STBY,
					meas_tmp.p_ACT_PDN,
					meas_tmp.p_ACT_STBY,
					meas_tmp.p_ACT,
					meas_tmp.p_WR,
					meas_tmp.p_RD,
					meas_tmp.p_REF,
					meas_tmp.p_AMB_IDLE,
					meas_tmp.p_AMB_ACT,
					meas_tmp.p_AMB_PASS_THROUGH,
					meas_tmp.p_AMB,
					meas_tmp.p_TOT,
					meas_tmp.p_TOT_MAX,
					dram_controller[chan_id]->rank[rank_id].r_p_gblinfo.dram_access);

		}
		fprintf(fileout, "\n");
	}
}
