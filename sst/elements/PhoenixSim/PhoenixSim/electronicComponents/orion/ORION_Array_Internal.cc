/*%
 * This file defines atomic-level and structural-level power models for array
 * structure, e.g. cache, RF, etc.
 *
 * Authors: Hangsheng Wang
 *
 * NOTES: (1) use Wmemcellr for shared r/w because Wmemcellr > Wmemcellw
 *        (2) end == 1 implies register file
 *
 * TODO:  (1) add support for "RatCellHeight"
 *        (2) add support for dynamic decoders
 *
 * FIXME: (1) should use different pre_size for different precharging circuits
 *        (2) should have a lower bound when auto-sizing drivers
 *        (3) sometimes no output driver
 */

#include <stdio.h>
#include <math.h>
#include "ORION_Array_Internal.h"
#include "ORION_Config.h"
#include "ORION_Util.h"






/*============================== decoder ==============================*/

/*#
 * compute switching cap when decoder changes output (select signal)
 *
 * Parameters:
 *   n_input -- fanin of 1 gate of last level decoder
 *
 * Return value: switching cap
 *
 * NOTES: 2 select signals switch, so no 1/2
 */
double O_dec::select_cap( u_int n_input )
{
  double Ctotal = 0;

  /* FIXME: why? */
  // if ( numstack > 5 ) numstack = 5;

  /* part 1: drain cap of last level decoders */
  Ctotal = n_input * ORION_Util_draincap( conf->PARM("WdecNORn"), NCH, 1, conf ) +
  ORION_Util_draincap( conf->PARM("WdecNORp"), PCH, n_input, conf  );

  /* part 2: output inverter */
  /* WHS: 20 should go to PARM */
  Ctotal += ORION_Util_draincap( conf->PARM("Wdecinvn"), NCH, 1, conf  ) +
  ORION_Util_draincap( conf->PARM("Wdecinvp"), PCH, 1, conf ) +
            ORION_Util_gatecap( conf->PARM("Wdecinvn") + conf->PARM("Wdecinvp"), 20, conf  );

  return Ctotal;
}


/*#
 * compute switching cap when 1 input bit of decoder changes
 *
 * Parameters:
 *   n_gates -- fanout of 1 addr signal
 *
 * Return value: switching cap
 *
 * NOTES: both addr and its complement change, so no 1/2
 */
double O_dec::chgaddr_cap( u_int n_gates )
{
  double Ctotal;

  /* stage 1: input driver */
  Ctotal = ORION_Util_draincap( conf->PARM("Wdecdrivep"), PCH, 1 , conf ) +
  ORION_Util_draincap( conf->PARM("Wdecdriven"), NCH, 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wdecdrivep"), 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wdecdriven"), 1 , conf );
  /* inverter to produce complement addr, this needs 1/2 */
  /* WHS: assume Wdecinv(np) for this inverter */
  Ctotal += ( ORION_Util_draincap( conf->PARM("Wdecinvp"), PCH, 1 , conf ) +
		  ORION_Util_draincap( conf->PARM("Wdecinvn"), NCH, 1 , conf ) +
	      ORION_Util_gatecap( conf->PARM("Wdecinvp"), 1 , conf ) +
	      ORION_Util_gatecap( conf->PARM("Wdecinvn"), 1 , conf )) / 2;

  /* stage 2: gate cap of level-1 decoder */
  /* WHS: 10 should go to PARM */
  Ctotal += n_gates * ORION_Util_gatecap( conf->PARM("Wdec3to8n") + conf->PARM("Wdec3to8p"), 10 , conf );

  return Ctotal;
}


/*#
 * compute switching cap when 1st-level decoder changes output
 *
 * Parameters:
 *   n_in_1st -- fanin of 1 gate of 1st-level decoder
 *   n_in_2nd -- fanin of 1 gate of 2nd-level decoder
 *   n_gates  -- # of gates of 2nd-level decoder, i.e.
 *               fanout of 1 gate of 1st-level decoder
 *
 * Return value: switching cap
 *
 * NOTES: 2 complementary signals switch, so no 1/2
 */
double O_dec::chgl1_cap( u_int n_in_1st, u_int n_in_2nd, u_int n_gates )
{
  double Ctotal;

  /* part 1: drain cap of level-1 decoder */
  Ctotal = n_in_1st * ORION_Util_draincap( conf->PARM("Wdec3to8p"), PCH, 1, conf  )
  + ORION_Util_draincap( conf->PARM("Wdec3to8n"), NCH, n_in_1st, conf  );

  /* part 2: gate cap of level-2 decoder */
  /* WHS: 40 and 20 should go to PARM */
  Ctotal += n_gates * ORION_Util_gatecap( conf->PARM("WdecNORn")
		  + conf->PARM("WdecNORp"), n_in_2nd * 40 + 20, conf  );

  return Ctotal;
}


int O_dec::clear_stat()
{
  n_chg_output = n_chg_l1 = n_chg_addr = 0;

  return 0;
}


/*#
 * initialize decoder
 *
 * Parameters:
 *   dec    -- decoder structure
 *   model  -- decoder model type
 *   n_bits -- decoder width
 *
 * Side effects:
 *   initialize dec structure if model type is valid
 *
 * Return value: -1 if model type is invalid
 *               0 otherwise
 */
int O_dec::init(  int m, u_int n_b, ORION_Tech_Config* con )
{
	conf = con;
  if ((model = m) && m < DEC_MAX_MODEL) {
    n_bits = n_b;
    /* redundant field */
    addr_mask = HAMM_MASK(n_bits);

	n_chg_output = 0;
	n_chg_addr = 0;
	n_chg_l1 = 0;

    clear_stat();
    e_chg_output = e_chg_l1 = e_chg_addr = 0;

    /* compute geometry parameters */
    if ( n_bits >= 4 ) {		/* 2-level decoder */
      /* WHS: inaccurate for some n_bits */
      n_in_1st = ( n_bits == 4 ) ? 2:3;
      n_out_0th = BIGONE << ( n_in_1st - 1 );
      n_in_2nd = (u_int)ceil((double)n_bits / n_in_1st );
      n_out_1st = BIGONE << ( n_bits - n_in_1st );
    }
    else if ( n_bits >= 2 ) {	/* 1-level decoder */
      n_in_1st = n_bits;
      n_out_0th = BIGONE << ( n_bits - 1 );
      n_in_2nd = n_out_1st = 0;
    }
    else {			/* no decoder basically */
      n_in_1st = n_in_2nd = n_out_0th = n_out_1st = 0;
    }

    /* compute energy constants */
    if ( n_bits >= 2 ) {
      e_chg_l1 = chgl1_cap( n_in_1st, n_in_2nd, n_out_1st ) * conf->PARM("EnergyFactor");
      if ( n_bits >= 4 )
        e_chg_output = select_cap( n_in_2nd ) * conf->PARM("EnergyFactor");
    }
    e_chg_addr = chgaddr_cap( n_out_0th ) * conf->PARM("EnergyFactor");

    return 0;
  }
  else
    return -1;
}


/*#
 * record decoder power stats
 *
 * Parameters:
 *   dec       -- decoder structure
 *   prev_addr -- previous input
 *   curr_addr -- current input
 *
 * Side effects:
 *   update counters in dec structure
 *
 * Return value: 0
 */
int O_dec::record(  LIB_Type_max_uint prev_addr, LIB_Type_max_uint curr_addr )
{

	//GRH: fix this if you're going to use it.  local variables named same as class variables
 /* u_int n_chg_bits, n_chg_l1 = 0, n_chg_output = 0;
  u_int i;
  LIB_Type_max_uint mask;

  // compute Hamming distance
  n_chg_bits = ORION_Util_Hamming( prev_addr, curr_addr, addr_mask );
  if ( n_chg_bits ) {
    if ( n_bits >= 4 ) {		// 2-level decoder
      /// WHS: inaccurate for some n_bits
      n_chg_output ++;
      // count addr group changes
      mask = HAMM_MASK(n_in_1st);
      for ( i = 0; i < n_in_2nd; i ++ ) {
        if ( ORION_Util_Hamming( prev_addr, curr_addr, mask ))
          n_chg_l1 ++;
        mask = mask << n_in_1st;
      }
    }
    else if ( n_bits >= 2 ) {	// 1-level decoder
      n_chg_l1 ++;
    }

    n_chg_addr += n_chg_bits;
    n_chg_l1 += n_chg_l1;
    n_chg_output += n_chg_output;
  }*/

  return -10000000;
}


/*#
 * report decoder power stats
 *
 * Parameters:
 *   dec -- decoder structure
 *
 * Return value: total energy consumption of this decoder
 *
 * TODO: add more report functionality, currently only total energy is reported
 */
double O_dec::report(  )
{
  double Etotal;

  Etotal = n_chg_output * e_chg_output + n_chg_l1 * e_chg_l1 +
	   n_chg_addr * e_chg_addr;

  /* bonus energy for dynamic decoder :) */
  //if ( is_dynamic_dec( model )) Etotal += Etotal;

  return Etotal;
}

/*============================== decoder ==============================*/



/*============================== wordlines ==============================*/

/*#
 * compute wordline switching cap
 *
 * Parameters:
 *   cols           -- # of pass transistors, i.e. # of bitlines
 *   wordlinelength -- length of wordline
 *   tx_width       -- width of pass transistor
 *
 * Return value: switching cap
 *
 * NOTES: upon address change, one wordline 1->0, another 0->1, so no 1/2
 */
double O_wordline::cap( u_int cols, double wire_cap, double tx_width )
{
  double Ctotal, Cline, psize, nsize;

  /* part 1: line cap, including gate cap of pass tx's and metal cap */
  Ctotal = Cline = ORION_Util_gatecappass( tx_width, conf->PARM("BitWidth") / 2 - tx_width, conf ) * cols + wire_cap;

  /* part 2: input driver */
  psize = ORION_Util_driver_size( Cline, conf->PARM("Period") / 16 , conf );
  nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
  /* WHS: 20 should go to PARM */
  Ctotal += ORION_Util_draincap( nsize, NCH, 1 , conf ) + ORION_Util_draincap( psize, PCH, 1, conf  ) +
            ORION_Util_gatecap( nsize + psize, 20, conf  );

  return Ctotal;
}


int O_wordline::clear_stat()
{
 n_read = n_write = 0;

  return 0;
}


/*#
 * initialize wordline
 *
 * Parameters:
 *   wordline -- wordline structure
 *   model    -- wordline model type
 *   share_rw -- 1 if shared R/W wordlines, 0 if separate R/W wordlines
 *   cols     -- # of array columns, NOT # of bitlines
 *   wire_cap -- wordline wire capacitance
 *   end      -- end of bitlines
 *
 * Return value: -1 if invalid model type
 *               0 otherwise
 *
 * Side effects:
 *   initialize wordline structure if model type is valid
 *
 * TODO: add error handler
 */
int O_wordline::init( int m, int s_rw, u_int cols, double wire_cap, u_int end, ORION_Tech_Config* con )
{
	conf= con;

  if ((model = m) && m < WORDLINE_MAX_MODEL) {
    clear_stat();

	n_read = 0;
	n_write = 0;

    switch ( model ) {
      case CAM_RW_WORDLINE:
  	   e_read = cam_cap( cols * end, wire_cap, conf->PARM("Wmemcellr") ) * conf->PARM("EnergyFactor");
           if ( share_rw = s_rw )
             e_write = e_read;
           else
             /* write bitlines are always double-ended */
             e_write = cam_cap( cols * 2, wire_cap, conf->PARM("Wmemcellw") ) * conf->PARM("EnergyFactor");
           break;

      case CAM_WO_WORDLINE:	/* only have write wordlines */
    	   share_rw = 0;
	   e_read = 0;
	   e_write = cam_cap( cols * 2, wire_cap, conf->PARM("Wmemcellw") ) * conf->PARM("EnergyFactor");
	   break;

      case CACHE_WO_WORDLINE:	/* only have write wordlines */
	   share_rw = 0;
	   e_read = 0;
	   e_write = cap( cols * 2, wire_cap, conf->PARM("Wmemcellw") ) * conf->PARM("EnergyFactor");
	   break;

      case CACHE_RW_WORDLINE:
	   e_read = cap( cols * end, wire_cap, conf->PARM("Wmemcellr") ) * conf->PARM("EnergyFactor");
           if ( share_rw = s_rw )
             e_write = e_read;
           else
             e_write = cap( cols * 2, wire_cap, conf->PARM("Wmemcellw") ) * conf->PARM("EnergyFactor");

	   /* static power */
	   /* input driver */
	   I_static = (conf->PARM("Woutdrivern") * TABS::NMOS_TAB[0] + conf->PARM("Woutdriverp") *
			   TABS::PMOS_TAB[0]) / conf->PARM("TECH_POINT") * 100;
	   break;

      default:
    	  break;/* some error handler */
    }

    return 0;
  }
  else
    return -1;
}


/* each time one wordline 1->0, another wordline 0->1, so no 1/2 */
double  O_wordline::cam_cap( u_int cols, double wire_cap, double tx_width )
{
  double Ctotal, Cline, psize, nsize;

  /* part 1: line cap, including gate cap of pass tx's and metal cap */
  Ctotal = Cline = ORION_Util_gatecappass( tx_width, 2 , conf ) * cols + wire_cap;

  /* part 2: input driver */
  psize = ORION_Util_driver_size( Cline, conf->PARM("Period") / 8, conf  );
  nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
  /* WHS: 20 should go to PARM */
  Ctotal += ORION_Util_draincap( nsize, NCH, 1 , conf ) + ORION_Util_draincap( psize,  PCH, 1, conf  ) +
            ORION_Util_gatecap( nsize + psize, 20, conf  );

  return Ctotal;
}

/*#
 * record wordline power stats
 *
 * Parameters:
 *   wordline -- wordline structure
 *   rw       -- 1 if write operation, 0 if read operation
 *   n_switch -- switching times
 *
 * Return value: 0
 *
 * Side effects:
 *   update counters of wordline structure
 */
int O_wordline::record( int rw, LIB_Type_max_uint n_switch )
{
  if ( rw ) n_write += n_switch;
  else n_read += n_switch;

  return 0;
}


/*#
 * report wordline power stats
 *
 * Parameters:
 *   wordline -- wordline structure
 *
 * Return value: total energy consumption of all wordlines of this array
 *
 * TODO: add more report functionality, currently only total energy is reported
 */
double O_wordline::report( )
{
  return ( n_read * e_read +
           n_write * e_write + I_static * conf->PARM("Vdd") * conf->PARM("Period") * conf->PARM("SCALE_S"));
}

/*============================== wordlines ==============================*/



/*============================== bitlines ==============================*/

/*#
 * compute switching cap of reading 1 separate bitline column
 *
 * Parameters:
 *   rows          -- # of array rows, i.e. # of wordlines
 *   wire_cap      -- bitline wire capacitance
 *   end           -- end of bitlines
 *   n_share_amp   -- # of columns who share one sense amp
 *   n_bitline_pre -- # of precharge transistor drains for 1 bitline column
 *   n_colsel_pre  -- # of precharge transistor drains for 1 column selector, if any
 *   pre_size      -- width of precharge transistors
 *   outdrv_model  -- output driver model type
 *
 * Return value: switching cap
 *
 * NOTES: one bitline 1->0, then 0->1 on next precharging, so no 1/2
 */
double O_bitline::column_read_cap(u_int rows, double wire_cap, u_int e, u_int n_share_amp, u_int n_bitline_pre, u_int n_colsel_pre, double pre_size, int outdrv_model)
{
  double Ctotal;

  /* part 1: drain cap of precharge tx's */
  Ctotal = n_bitline_pre * ORION_Util_draincap( pre_size, PCH, 1, conf  );

  /* part 2: drain cap of pass tx's */
  Ctotal += rows * ORION_Util_draincap( conf->PARM("Wmemcellr"), NCH, 1 , conf );

  /* part 3: metal cap */
  Ctotal += wire_cap;

  /* part 4: column selector or bitline inverter */
  if ( e == 1 ) {		/* bitline inverter */
    /* FIXME: magic numbers */
    Ctotal += ORION_Util_gatecap( conf->PARM("MSCALE") * ( 29.9 + 7.8 ), 0, conf  ) +
	      ORION_Util_gatecap( conf->PARM("MSCALE") * ( 47.0 + 12.0), 0, conf  );
  }
  else if ( n_share_amp > 1 ) {	/* column selector */
    /* drain cap of pass tx's */
    Ctotal += ( n_share_amp + 1 ) * ORION_Util_draincap( conf->PARM("Wbitmuxn"), NCH, 1, conf  );
    /* drain cap of column selector precharge tx's */
    Ctotal += n_colsel_pre * ORION_Util_draincap( pre_size, PCH, 1, conf  );
    /* FIXME: no way to count activity factor on gates of column selector */
  }

  /* part 5: gate cap of sense amplifier or output driver */
  if (e == 2)			/* sense amplifier */
    Ctotal += 2 * ORION_Util_gatecap( conf->PARM("WsenseQ1to4"), 10 , conf );
  else if (outdrv_model)	/* end == 1, output driver */
    Ctotal += ORION_Util_gatecap( conf->PARM("Woutdrvnandn"), 1, conf  )
    + ORION_Util_gatecap( conf->PARM("Woutdrvnandp"), 1, conf  ) +
              ORION_Util_gatecap( conf->PARM("Woutdrvnorn"), 1, conf  )
              + ORION_Util_gatecap( conf->PARM("Woutdrvnorp"), 1, conf  );

  return Ctotal;
}


/*#
 * compute switching cap of selecting 1 column selector
 *
 * Parameters:
 *
 * Return value: switching cap
 *
 * NOTES: select one, deselect another, so no 1/2
 */
double O_bitline::column_select_cap( void )
{
  return ORION_Util_gatecap( conf->PARM("Wbitmuxn"), 1 , conf );
}


/*#
 * compute switching cap of writing 1 separate bitline column
 *
 * Parameters:
 *   rows          -- # of array rows, i.e. # of wordlines
 *   wire_cap      -- bitline wire capacitance
 *
 * Return value: switching cap
 *
 * NOTES: bit and bitbar switch simultaneously, so no 1/2
 */
double O_bitline::column_write_cap( u_int rows, double wire_cap )
{
  double Ctotal, psize, nsize;

  /* part 1: line cap, including drain cap of pass tx's and metal cap */
  Ctotal = rows * ORION_Util_draincap( conf->PARM("Wmemcellw"), NCH, 1, conf  ) + wire_cap;

  /* part 2: write driver */
  psize = ORION_Util_driver_size( Ctotal, conf->PARM("Period") / 8 , conf );
  nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
  Ctotal += ORION_Util_draincap( psize, PCH, 1, conf  ) + ORION_Util_draincap( nsize, NCH, 1, conf  ) +
            ORION_Util_gatecap( psize + nsize, 1 , conf );

  return Ctotal;
}


/* one bitline switches twice in one cycle, so no 1/2 */
double O_bitline::share_column_write_cap( u_int rows, double wire_cap, u_int n_share_amp, u_int n_bitline_pre, double pre_size )
{
  double Ctotal, psize, nsize;

  /* part 1: drain cap of precharge tx's */
  Ctotal = n_bitline_pre * ORION_Util_draincap( pre_size, PCH, 1 , conf );

  /* part 2: drain cap of pass tx's */
  Ctotal += rows * ORION_Util_draincap( conf->PARM("Wmemcellr"), NCH, 1 , conf );

  /* part 3: metal cap */
  Ctotal += wire_cap;

  /* part 4: column selector or sense amplifier */
  if ( n_share_amp > 1 ) Ctotal += ORION_Util_draincap( conf->PARM("Wbitmuxn"), NCH, 1 , conf );
  else Ctotal += 2 * ORION_Util_gatecap( conf->PARM("WsenseQ1to4"), 10, conf  );

  /* part 5: write driver */
  psize = ORION_Util_driver_size( Ctotal, conf->PARM("Period") / 8, conf  );
  nsize = psize * conf->PARM("Wdecinvn") / conf->PARM("Wdecinvp");
  /* WHS: omit gate cap of driver due to modeling difficulty */
  Ctotal += ORION_Util_draincap( psize, PCH, 1, conf  ) + ORION_Util_draincap( nsize, NCH, 1, conf  );

  return Ctotal;
}


/* one bitline switches twice in one cycle, so no 1/2 */
double O_bitline::share_column_read_cap( u_int rows, double wire_cap, u_int n_share_amp, u_int n_bitline_pre, u_int n_colsel_pre, double pre_size )
{
  double Ctotal;

  /* part 1: same portion as write */
  Ctotal = share_column_write_cap( rows, wire_cap, n_share_amp, n_bitline_pre, pre_size );

  /* part 2: column selector and sense amplifier */
  if ( n_share_amp > 1 ) {
    /* bottom part of drain cap of pass tx's */
    Ctotal += n_share_amp * ORION_Util_draincap( conf->PARM("Wbitmuxn"), NCH, 1 , conf );
    /* drain cap of column selector precharge tx's */
    Ctotal += n_colsel_pre * ORION_Util_draincap( pre_size, PCH, 1, conf );

    /* part 3: gate cap of sense amplifier */
    Ctotal += 2 * ORION_Util_gatecap( conf->PARM("WsenseQ1to4"), 10, conf  );
  }

  return Ctotal;
}


int O_bitline::clear_stat()
{
  n_col_write = n_col_read = n_col_sel = 0;

  return 0;
}


int O_bitline::init( int m, int s_rw, u_int e, u_int rows, double wire_cap,
		u_int n_share_amp, u_int n_bitline_pre,
		u_int n_colsel_pre, double pre_size, int outdrv_model, ORION_Tech_Config* con)
{
	conf = con;
  if ((model = m) && m < BITLINE_MAX_MODEL) {
    end = e;
    clear_stat();


	n_col_write = 0;
	n_col_read = 0;
	n_col_sel = 0;

    switch ( model ) {
      case RW_BITLINE:
	   if ( end == 2 )
             e_col_sel = column_select_cap() * conf->PARM("EnergyFactor");
	   else		/* end == 1 implies register file */
             e_col_sel = 0;

           if ( share_rw = s_rw ) {
	     /* shared bitlines are double-ended, so SenseEnergyFactor */
             e_col_read = share_column_read_cap( rows, wire_cap, n_share_amp, n_bitline_pre, n_colsel_pre, pre_size )
             * conf->PARM("SenseEnergyFactor");
             e_col_write = share_column_write_cap( rows, wire_cap, n_share_amp, n_bitline_pre, pre_size )
             * conf->PARM("EnergyFactor");
           }
           else {
             e_col_read = column_read_cap(rows, wire_cap, end, n_share_amp, n_bitline_pre, n_colsel_pre, pre_size,
            		 outdrv_model) * (end == 2 ? conf->PARM("SenseEnergyFactor") : conf->PARM("EnergyFactor"));
             e_col_write = column_write_cap( rows, wire_cap ) * conf->PARM("EnergyFactor");

	     /* static power */
	     I_static = 2 * (conf->PARM("Wdecinvn") * TABS::NMOS_TAB[0] + conf->PARM("Wdecinvp") * TABS::PMOS_TAB[0]) /
	     conf->PARM("TECH_POINT") * 100;
           }

           break;

      case WO_BITLINE:	/* only have write bitlines */
  	   share_rw = 0;
           e_col_sel = e_col_read = 0;
           e_col_write = column_write_cap( rows, wire_cap ) * conf->PARM("EnergyFactor");
	   break;

      default:	/* some error handler */
	break;
    }

    return 0;
  }
  else
    return -1;
}


int O_bitline::is_rw_bitline( )
{
  return ( model == RW_BITLINE );
}


/* WHS: no way to count activity factor on column selector gates */
int O_bitline::record( int rw, u_int cols, LIB_Type_max_uint new_value )
{
  /* FIXME: should use variable rather than computing each time */
  LIB_Type_max_uint mask = HAMM_MASK(cols);

  if ( rw ) {	/* write */
    if ( share_rw )	/* share R/W bitlines */
      n_col_write += cols;
    else			/* separate R/W bitlines */
      n_col_write += ORION_Util_Hamming( curr_val, new_value, mask );

	curr_val = new_value;
  }
  else {	/* read */
    if ( end == 2 )	/* double-ended bitline */
      n_col_read += cols;
    else			/* single-ended bitline */
      /* WHS: read ~new_value due to the bitline inverter */
      n_col_read += ORION_Util_Hamming( mask, ~new_value, mask );
  }



  return 0;
}

/* WHS: no way to count activity factor on column selector gates */
int O_bitline::record( int rw, u_int cols, LIB_Type_max_uint old_value, LIB_Type_max_uint new_value )
{
  /* FIXME: should use variable rather than computing each time */
  LIB_Type_max_uint mask = HAMM_MASK(cols);

  if ( rw ) {	/* write */
    if ( share_rw )	/* share R/W bitlines */
      n_col_write += cols;
    else			/* separate R/W bitlines */
      n_col_write += ORION_Util_Hamming( old_value, new_value, mask );

  }
  else {	/* read */
    if ( end == 2 )	/* double-ended bitline */
      n_col_read += cols;
    else			/* single-ended bitline */
      /* WHS: read ~new_value due to the bitline inverter */
      n_col_read += ORION_Util_Hamming( mask, ~new_value, mask );
  }



  return 0;
}


double O_bitline::report( )
{
  return ( n_col_write * e_col_write +
	   n_col_read * e_col_read +
	   n_col_sel * e_col_sel + I_static * conf->PARM("Vdd") * conf->PARM("Period") * conf->PARM("SCALE_S"));
}

/*============================== bitlines ==============================*/



/*============================== sense amplifier ==============================*/

/* estimate senseamp power dissipation in cache structures (Zyuban's method) */
double O_amp::energy( void )
{
  return ( conf->PARM("Vdd") / 8 * conf->PARM("Period") * conf->PARM( "amp_Idsat" ));
}


int O_amp::clear_stat()
{
  n_access = 0;

  return 0;
}


int O_amp::init(  int m , ORION_Tech_Config* con)
{
	conf = con;
  if ((model = m) && m < AMP_MAX_MODEL) {
    clear_stat();
    e_access = energy();

	n_access = 0;

    return 0;
  }
  else
    return -1;
}


int O_amp::record(  u_int cols )
{
  n_access += cols;

  return 0;
}


double O_amp::report()
{
  return ( n_access * e_access);
}

/*============================== sense amplifier ==============================*/



/*============================== tag comparator ==============================*/

/* eval switches twice per cycle, so no 1/2 */
/* WHS: assume eval = 1 when no cache operation */
double O_comp::base_cap( void )
{
  /* eval tx's: 4 inverters */
  return ( ORION_Util_draincap( conf->PARM("Wevalinvp"), PCH, 1 , conf ) +
		  ORION_Util_draincap( conf->PARM("Wevalinvn"), NCH, 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wevalinvp"), 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wevalinvn"), 1 , conf ) +
           ORION_Util_draincap( conf->PARM("Wcompinvp1"), PCH, 1 , conf ) +
           ORION_Util_draincap( conf->PARM("Wcompinvn1"), NCH, 1 , conf ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvp1"), 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvn1"), 1 , conf ) +
           ORION_Util_draincap( conf->PARM("Wcompinvp2"), PCH, 1, conf  ) +
           ORION_Util_draincap( conf->PARM("Wcompinvn2"), NCH, 1 , conf ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvp2"), 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvn2"), 1, conf  ) +
           ORION_Util_draincap( conf->PARM("Wcompinvp3"), PCH, 1, conf  ) +
           ORION_Util_draincap( conf->PARM("Wcompinvn3"), NCH, 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvp3"), 1, conf  ) +
	   ORION_Util_gatecap( conf->PARM("Wcompinvn3"), 1 , conf ));
}


/* no 1/2 for the same reason with SIM_array_comp_base_cap */
double O_comp::match_cap(  )
{
  return ( n_bits * ( ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 1 , conf ) +
		  ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 2, conf  )));
}


/* upon mismatch, select signal 1->0, then 0->1 on next precharging, so no 1/2 */
double O_comp::mismatch_cap( u_int n_pre )
{
  double Ctotal;

  /* part 1: drain cap of precharge tx */
  Ctotal = n_pre * ORION_Util_draincap( conf->PARM("Wcomppreequ"), PCH, 1, conf  );

  /* part 2: nor gate of valid output */
  Ctotal += ORION_Util_gatecap( conf->PARM("WdecNORn"), 1 , conf ) +
  ORION_Util_gatecap( conf->PARM("WdecNORp"), 3 , conf );

  return Ctotal;
}


/* upon miss, valid output switches twice in one cycle, so no 1/2 */
double O_comp::miss_cap( u_int assoc )
{
  /* drain cap of valid output */
  return ( assoc * ORION_Util_draincap( conf->PARM("WdecNORn"), NCH, 1, conf  ) +
		  ORION_Util_draincap( conf->PARM("WdecNORp"), PCH, assoc, conf  ));
}


/* no 1/2 for the same reason as base_cap */
double O_comp::bit_match_cap( void )
{
  return ( 2 * ( ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 1, conf  ) +
		  ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 2, conf  )));
}


/* no 1/2 for the same reason as base_cap */
double O_comp::bit_mismatch_cap( void )
{
  return ( 3 * ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 1 , conf ) +
		  ORION_Util_draincap( conf->PARM("Wcompn"), NCH, 2 , conf ));
}


/* each addr bit drives 2 nmos pass transistors, so no 1/2 */
double O_comp::chgaddr_cap( void )
{
  return ( ORION_Util_gatecap( conf->PARM("Wcompn"), 1 , conf ));
}


int O_comp::clear_stat()
{
  n_access = n_miss = n_chg_addr = n_match = 0;
  n_mismatch = n_bit_match = n_bit_mismatch = 0;

  return 0;
}


int O_comp::init( int m, u_int bits, u_int ass, u_int n_pre,
		double matchline_len, double tagline_len, ORION_Tech_Config* con )
{
	conf = con;
  if ((model = m) && m < COMP_MAX_MODEL) {
    n_bits = bits;
    assoc = ass;
    /* redundant field */
    comp_mask = HAMM_MASK(n_bits);
	n_access = 0;
	n_match = 0;
	n_mismatch = 0;
	n_miss = 0;
	n_bit_match = 0;
	n_bit_mismatch = 0;
	n_chg_addr = 0;



    clear_stat();

    switch ( model ) {
      case CACHE_COMP:
           e_access = base_cap() * conf->PARM("EnergyFactor");
           e_match = match_cap( ) * conf->PARM("EnergyFactor");
           e_mismatch = mismatch_cap( n_pre ) * conf->PARM("EnergyFactor");
           e_miss = miss_cap( assoc ) * conf->PARM("EnergyFactor");
           e_bit_match = bit_match_cap() * conf->PARM("EnergyFactor");
           e_bit_mismatch = bit_mismatch_cap() * conf->PARM("EnergyFactor");
           e_chg_addr = chgaddr_cap() * conf->PARM("EnergyFactor");
	   break;

      case CAM_COMP:
           e_access = e_match = e_chg_addr = 0;
           e_bit_match = e_bit_mismatch = 0;
	   /* energy consumption of tagline */
	   e_chg_addr = cam_tagline_cap( assoc, tagline_len ) * conf->PARM("EnergyFactor");
           e_mismatch = cam_mismatch_cap( n_bits, n_pre, matchline_len ) * conf->PARM("EnergyFactor");
           e_miss = cam_miss_cap( assoc ) * conf->PARM("EnergyFactor");
	   break;

      default:	/* some error handler */
	break;
    }

    return 0;
  }
  else
    return -1;
}


int O_comp::global_record(  LIB_Type_max_uint prev_value, LIB_Type_max_uint curr_value, int miss )
{
  if ( miss ) n_miss ++;

  switch ( model ) {
    case CACHE_COMP:
         n_access ++;
         n_chg_addr += ORION_Util_Hamming( prev_value, curr_value, comp_mask ) * assoc;
         break;

    case CAM_COMP:
         n_chg_addr += ORION_Util_Hamming( prev_value, curr_value, comp_mask );
	 break;

    default:	/* some error handler */
	break;
  }

  return 0;
}


/* recover means prev_tag will recover on next cycle, e.g. driven by sense amplifier */
/* return value: 1 if miss, 0 if hit */
int O_comp::local_record(  LIB_Type_max_uint prev_tag, LIB_Type_max_uint curr_tag, LIB_Type_max_uint input, int recover )
{
  u_int H_dist;
  int mismatch;

  if ( mismatch = ( curr_tag != input )) n_mismatch ++;

  /* for cam, input changes are reflected in memory cells */
  if ( model == CACHE_COMP ) {
    if ( recover )
      n_chg_addr += 2 * ORION_Util_Hamming( prev_tag, curr_tag, comp_mask );
    else
      n_chg_addr += ORION_Util_Hamming( prev_tag, curr_tag, comp_mask );

    if ( mismatch ) {
      H_dist = ORION_Util_Hamming( curr_tag, input, comp_mask );
      n_bit_mismatch += H_dist;
      n_bit_match += n_bits - H_dist;
    }
    else n_match ++;
  }

  return mismatch;
}


double O_comp::report( )
{
  return ( n_access * e_access + n_match * e_match +
           n_mismatch * e_mismatch + n_miss * e_miss +
	   n_bit_match * e_bit_match + n_chg_addr * e_chg_addr +
	   n_bit_mismatch * e_bit_mismatch );
}

/* tag and tagbar switch simultaneously, so no 1/2 */
double O_comp::cam_tagline_cap( u_int rows, double taglinelength )
{
  double Ctotal;

  /* part 1: line cap, including drain cap of pass tx's and metal cap */
  Ctotal = rows * ORION_Util_gatecap( conf->PARM("Wcomparen2"), 2 , conf ) + conf->PARM("CC3M2metal") * taglinelength;

  /* part 2: input driver */
  Ctotal += ORION_Util_draincap( conf->PARM("Wcompdrivern"), NCH, 1, conf  ) +
  ORION_Util_draincap( conf->PARM("Wcompdriverp"), PCH, 1 , conf ) +
	    ORION_Util_gatecap( conf->PARM("Wcompdrivern") + conf->PARM("Wcompdriverp"), 1, conf  );

  return Ctotal;
}


/* upon mismatch, matchline 1->0, then 0->1 on next precharging, so no 1/2 */
double O_comp::cam_mismatch_cap( u_int n_bits, u_int n_pre, double matchline_len )
{
  double Ctotal;

  /* part 1: drain cap of precharge tx */
  Ctotal = n_pre * ORION_Util_draincap( conf->PARM("Wmatchpchg"), PCH, 1, conf  );

  /* part 2: drain cap of comparator tx */
  Ctotal += n_bits * ( ORION_Util_draincap( conf->PARM("Wcomparen1"), NCH, 1, conf  ) +
		  ORION_Util_draincap( conf->PARM("Wcomparen1"), NCH, 2 , conf ));

  /* part 3: metal cap of matchline */
  Ctotal += conf->PARM("CC3M3metal") * matchline_len;

  /* FIXME: I don't understand the Wattch code here */
  /* part 4: nor gate of valid output */
  Ctotal += ORION_Util_gatecap( conf->PARM("Wmatchnorn") + conf->PARM("Wmatchnorp"), 10, conf  );

  return Ctotal;
}


/* WHS: subtle difference of valid output between cache and inst window:
 *   fully-associative cache: nor all matchlines of the same port
 *   instruction window:      nor all matchlines of the same tag line */
/* upon miss, valid output switches twice in one cycle, so no 1/2 */
double O_comp::cam_miss_cap( u_int assoc )
{
  /* drain cap of valid output */
  return ( assoc * ORION_Util_draincap( conf->PARM("Wmatchnorn"), NCH, 1, conf  ) +
		  ORION_Util_draincap( conf->PARM("Wmatchnorp"), PCH, assoc, conf  ));
}

/*============================== tag comparator ==============================*/



/*============================== multiplexor ==============================*/

/* upon mismatch, 1 output of nor gates 1->0, then 0->1 on next cycle, so no 1/2 */
double O_mux::mismatch_cap( u_int n_nor_gates )
{
  double Cmul;

  /* stage 1: inverter */
  Cmul = ORION_Util_draincap( conf->PARM("Wmuxdrv12n"), NCH, 1, conf  ) +
  ORION_Util_draincap( conf->PARM("Wmuxdrv12p"), PCH, 1 , conf ) +
	 ORION_Util_gatecap( conf->PARM("Wmuxdrv12n"), 1 , conf ) +
	 ORION_Util_gatecap( conf->PARM("Wmuxdrv12p"), 1 , conf );

  /* stage 2: nor gates */
  /* gate cap of nor gates */
  Cmul += n_nor_gates * ( ORION_Util_gatecap( conf->PARM("WmuxdrvNORn"), 1, conf )
		  + ORION_Util_gatecap( conf->PARM("WmuxdrvNORp"), 1, conf  ));
  /* drain cap of nor gates, only count 1 */
  Cmul += ORION_Util_draincap( conf->PARM("WmuxdrvNORp"), PCH, 2, conf  ) + 2
  * ORION_Util_draincap( conf->PARM("WmuxdrvNORn"), NCH, 1 , conf );

  /* stage 3: output inverter */
  Cmul += ORION_Util_gatecap( conf->PARM("Wmuxdrv3n"), 1, conf  ) +
  ORION_Util_gatecap( conf->PARM("Wmuxdrv3p"), 1, conf ) +
	  ORION_Util_draincap( conf->PARM("Wmuxdrv3n"), NCH, 1 , conf ) +
	  ORION_Util_draincap( conf->PARM("Wmuxdrv3p"), PCH, 1 , conf );

  return Cmul;
}


/* 2 nor gates switch gate signals, so no 1/2 */
/* WHS: assume address changes won't propagate until matched or mismatched */
double O_mux::chgaddr_cap( void )
{
  return ( ORION_Util_gatecap( conf->PARM("WmuxdrvNORn"), 1, conf  ) +
		  ORION_Util_gatecap( conf->PARM("WmuxdrvNORp"), 1 , conf ));
}


int O_mux::clear_stat()
{
  n_mismatch = n_chg_addr = 0;

  return 0;
}


int O_mux::init( int m, u_int n_gates, u_int ass, ORION_Tech_Config* con )
{
	conf = con;
  if ((model = m) && m < MUX_MAX_MODEL) {
    assoc = ass;

	n_mismatch = 0;
	n_chg_addr = 0;




    clear_stat();

    e_mismatch = mismatch_cap( n_gates ) * conf->PARM("EnergyFactor");
    e_chg_addr = chgaddr_cap() * conf->PARM("EnergyFactor");

    return 0;
  }
  else
    return -1;
}


int O_mux::record(  LIB_Type_max_uint prev_addr, LIB_Type_max_uint curr_addr, int miss )
{
  if ( prev_addr != curr_addr )
    n_chg_addr += assoc;

  if ( miss )
    n_mismatch += assoc;
  else
    n_mismatch += assoc - 1;

  return 0;
}


double O_mux::report(  )
{
  return ( n_mismatch * e_mismatch + n_chg_addr * e_chg_addr);
}

/*============================== multiplexor ==============================*/



/*============================== output driver ==============================*/

/* output driver should be disabled somehow when no access occurs, so no 1/2 */
double O_out::select_cap( u_int data_width )
{
  double Ctotal;

  /* stage 1: inverter */
  Ctotal = ORION_Util_gatecap( conf->PARM("Woutdrvseln"), 1, conf  ) +
  ORION_Util_gatecap( conf->PARM("Woutdrvselp"), 1, conf  ) +
	   ORION_Util_draincap( conf->PARM("Woutdrvseln"), NCH, 1, conf  ) +
	   ORION_Util_draincap( conf->PARM("Woutdrvselp"), PCH, 1 , conf );

  /* stage 2: gate cap of nand gate and nor gate */
  /* only consider 1 gate cap because another and drain cap switch depends on data value */
  Ctotal += data_width *( ORION_Util_gatecap( conf->PARM("Woutdrvnandn"), 1, conf  )
		  + ORION_Util_gatecap( conf->PARM("Woutdrvnandp"), 1, conf  ) +
                          ORION_Util_gatecap( conf->PARM("Woutdrvnorn"), 1 , conf )
                          + ORION_Util_gatecap( conf->PARM("Woutdrvnorp"), 1 , conf ));

  return Ctotal;
}


/* WHS: assume data changes won't propagate until enabled */
double O_out::chgdata_cap( void )
{
  return (( ORION_Util_gatecap( conf->PARM("Woutdrvnandn"), 1, conf  )
		  + ORION_Util_gatecap( conf->PARM("Woutdrvnandp"), 1, conf  ) +
            ORION_Util_gatecap( conf->PARM("Woutdrvnorn"), 1 , conf )
            + ORION_Util_gatecap( conf->PARM("Woutdrvnorp"), 1, conf  )) / 2 );
}


/* no 1/2 for the same reason as outdrv_select_cap */
double O_out::outdata_cap( u_int value )
{
  double Ctotal;

  /* stage 1: drain cap of nand gate or nor gate */
  if ( value )
    /* drain cap of nand gate */
    Ctotal = ORION_Util_draincap( conf->PARM("Woutdrvnandn"), NCH, 2 , conf ) + 2
    * ORION_Util_draincap( conf->PARM("Woutdrvnandp"), PCH, 1 , conf );
  else
    /* drain cap of nor gate */
    Ctotal = 2 * ORION_Util_draincap( conf->PARM("Woutdrvnorn"), NCH, 1, conf  )
    + ORION_Util_draincap( conf->PARM("Woutdrvnorp"), PCH, 2 , conf );

  /* stage 2: gate cap of output inverter */
  if ( value )
    Ctotal += ORION_Util_gatecap( conf->PARM("Woutdriverp"), 1, conf  );
  else
    Ctotal += ORION_Util_gatecap( conf->PARM("Woutdrivern"), 1 , conf );

  /* drain cap of output inverter should be included into bus cap */
  return Ctotal;
}


int O_out::clear_stat()
{
  n_select = n_chg_data = 0;
  n_out_0 = n_out_1 = 0;

  return 0;
}


int O_out::init( int m, u_int item, ORION_Tech_Config* con )
{
	conf = con;
  if ((model = m) && m < OUTDRV_MAX_MODEL) {
    item_width = item;
    /* redundant field */
    out_mask = HAMM_MASK(item_width);

	n_select = 0;
	n_chg_data = 0;
	n_out_1 = 0;
	n_out_0 = 0;

    clear_stat();

    e_select = select_cap( item_width ) * conf->PARM("EnergyFactor");
    e_out_1 = outdata_cap( 1 ) * conf->PARM("EnergyFactor");
    e_out_0 = outdata_cap( 0 ) * conf->PARM("EnergyFactor");

    switch ( model ) {
      case CACHE_OUTDRV:
           e_chg_data = chgdata_cap() * conf->PARM("EnergyFactor");
	   break;

      case CAM_OUTDRV:
	   /* input changes are reflected in memory cells */
      case REG_OUTDRV:
	   /* input changes are reflected in bitlines */
           e_chg_data = 0;
	   break;

      default:	/* some error handler */
	break;
    }

    return 0;
  }
  else
    return -1;
}


int O_out::global_record( LIB_Type_max_uint data )
{
  u_int n_1;

  n_select ++;

  n_1 = ORION_Util_Hamming( data, 0, out_mask );

  n_out_1 += n_1;
  n_out_0 += item_width - n_1;

  return 0;
}


/* recover means prev_data will recover on next cycle, e.g. driven by sense amplifier */
/* NOTE: this function SHOULD not be called by a fully-associative cache */
int O_out::local_record(  LIB_Type_max_uint prev_data, LIB_Type_max_uint curr_data, int recover )
{
  if ( recover )
    n_chg_data += 2 * ORION_Util_Hamming( prev_data, curr_data, out_mask );
  else
    n_chg_data += ORION_Util_Hamming( prev_data, curr_data, out_mask );

  return 0;
}


double O_out::report(  )
{
  return ( n_select * e_select + n_chg_data * e_chg_data +
	   n_out_1 * e_out_1 + n_out_0 * e_out_0);
}

/*============================== output driver ==============================*/



/*============================== memory cell ==============================*/

/* WHS: use Wmemcella and Wmemcellbscale to compute tx width of memory cell */
double O_mem::cap( u_int read_ports, u_int write_ports, int share_rw, u_int e )
{
  double Ctotal;

  /* part 1: drain capacitance of pass transistors */
  Ctotal = ORION_Util_draincap( conf->PARM("Wmemcellr"), NCH, 1, conf  ) * read_ports * e / 2;
  if ( ! share_rw )
    Ctotal += ORION_Util_draincap( conf->PARM("Wmemcellw"), NCH, 1, conf  ) * write_ports;

  /* has coefficient ( 1/2 * 2 ) */
  /* part 2: drain capacitance of memory cell */
  Ctotal += ORION_Util_draincap( conf->PARM("Wmemcella"), NCH, 1, conf  )
  + ORION_Util_draincap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), PCH, 1, conf  );

  /* has coefficient ( 1/2 * 2 ) */
  /* part 3: gate capacitance of memory cell */
  Ctotal += ORION_Util_gatecap( conf->PARM("Wmemcella"), 1, conf  )
  + ORION_Util_gatecap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), 1 , conf );

  return Ctotal;
}


int O_mem::clear_stat()
{
  n_switch = 0;

  return 0;
}


void O_mem::init(  int m, u_int read_ports,
		u_int write_ports, int share_rw, u_int e, ORION_Tech_Config* con )
{
	conf = con;

	n_switch = 0;

	//std::cout << m << ", " << read_ports << ", " << write_ports << ", " << share_rw << ", " << e << endl;

  if ((model = m) && m < MEM_MAX_MODEL) {
    end = e;
    clear_stat();

    switch ( model ) {
      case CAM_TAG_RW_MEM:
	   e_switch = cam_tag_cap( read_ports, write_ports, share_rw, end, SIM_ARRAY_RW ) * conf->PARM("EnergyFactor");
	   break;

      /* FIXME: it's only an approximation using CAM_TAG_WO_MEM to emulate CAM_ATTACH_MEM */
      case CAM_ATTACH_MEM:
      case CAM_TAG_WO_MEM:
	   e_switch = cam_tag_cap( read_ports, write_ports, share_rw, end, SIM_ARRAY_WO ) * conf->PARM("EnergyFactor");
	   break;

      case CAM_DATA_MEM:
	   e_switch = cam_data_cap( read_ports, write_ports ) * conf->PARM("EnergyFactor");
	   break;

      default:	/* NORMAL_MEM */
	   e_switch = cap( read_ports, write_ports, share_rw, end ) * conf->PARM("EnergyFactor");

	   /* static power */
	   I_static = 0;
	   /* memory cell */
	   I_static += (conf->PARM("Wmemcella") * TABS::NMOS_TAB[0] +
			   conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale") * TABS::PMOS_TAB[0]) * 2;
	  // std::cout << I_static << endl;
	   /* read port pass tx */
	   I_static += conf->PARM("Wmemcellr") * TABS::NMOS_TAB[0] * end * read_ports;
	  // std::cout << I_static << endl;
	   /* write port pass tx */
	   if (! share_rw)
	     I_static += conf->PARM("Wmemcellw") * TABS::NMOS_TAB[0] * 2 * write_ports;

	   I_static = I_static / conf->PARM("TECH_POINT") * 100.0;
	  // std::cout << PARM(TECH_POINT) << ", " << I_static << endl;
    }


  }

}


int O_mem::record(  LIB_Type_max_uint new_value, u_int width )
{
  n_switch += ORION_Util_Hamming( curr_data, new_value, HAMM_MASK(width));
	curr_data = new_value;

  return 0;
}


int O_mem::record( LIB_Type_max_uint old_value, LIB_Type_max_uint new_value, u_int width )
{
  n_switch += ORION_Util_Hamming( old_value, new_value, HAMM_MASK(width));


  return 0;
}


double O_mem::report( )
{
  return ( n_switch * e_switch + I_static * conf->PARM("Vdd") * conf->PARM("Period") * conf->PARM("SCALE_S") );
}

/* WHS: use Wmemcella and Wmemcellbscale to compute tx width of memory cell */
double O_mem::cam_tag_cap( u_int read_ports, u_int write_ports, int share_rw, u_int end, int only_write )
{
  double Ctotal;

  /* part 1: drain capacitance of pass transistors */
  if ( only_write )
    Ctotal = ORION_Util_draincap( conf->PARM("Wmemcellw"), NCH, 1, conf  ) * write_ports;
  else {
    Ctotal = ORION_Util_draincap( conf->PARM("Wmemcellr"), NCH, 1 , conf ) * read_ports * end / 2;
    if ( ! share_rw )
      Ctotal += ORION_Util_draincap( conf->PARM("Wmemcellw"), NCH, 1, conf  ) * write_ports;
  }

  /* has coefficient ( 1/2 * 2 ) */
  /* part 2: drain capacitance of memory cell */
  Ctotal += ORION_Util_draincap( conf->PARM("Wmemcella"), NCH, 1, conf  )
  + ORION_Util_draincap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), PCH, 1 , conf );

  /* has coefficient ( 1/2 * 2 ) */
  /* part 3: gate capacitance of memory cell */
  Ctotal += ORION_Util_gatecap( conf->PARM("Wmemcella"), 1, conf  )
  + ORION_Util_gatecap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), 1 , conf );

  /* has coefficient ( 1/2 * 2 ) */
  /* part 4: gate capacitance of comparator */
  Ctotal += ORION_Util_gatecap( conf->PARM("Wcomparen1"), 2 , conf ) * read_ports;

  return Ctotal;
}


double O_mem::cam_data_cap( u_int read_ports, u_int write_ports )
{
  double Ctotal;

  /* has coefficient ( 1/2 * 2 ) */
  /* part 1: drain capacitance of pass transistors */
  Ctotal = ORION_Util_draincap( conf->PARM("Wmemcellw"), NCH, 1, conf  ) * write_ports;

  /* has coefficient ( 1/2 * 2 ) */
  /* part 2: drain capacitance of memory cell */
  Ctotal += ORION_Util_draincap( conf->PARM("Wmemcella"), NCH, 1, conf  )
  + ORION_Util_draincap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), PCH, 1 , conf );

  /* has coefficient ( 1/2 * 2 ) */
  /* part 3: gate capacitance of memory cell */
  Ctotal += ORION_Util_gatecap( conf->PARM("Wmemcella"), 1, conf  )
  + ORION_Util_gatecap( conf->PARM("Wmemcella") * conf->PARM("Wmemcellbscale"), 1, conf  );

  /* part 4: gate capacitance of output driver */
  Ctotal += ( ORION_Util_gatecap( conf->PARM("Woutdrvnandn"), 1 , conf )
		  + ORION_Util_gatecap( conf->PARM("Woutdrvnandp"), 1, conf  ) +
              ORION_Util_gatecap( conf->PARM("Woutdrvnorn"), 1 , conf )
              + ORION_Util_gatecap( conf->PARM("Woutdrvnorp"), 1, conf  )) / 2 * read_ports;

  return Ctotal;
}


/*============================== memory cell ==============================*/



/*============================== precharge ==============================*/

/* consider charge then discharge, so no 1/2 */
double O_arr_pre::cap( double width, double length )
{
  return ORION_Util_gatecap( width, length , conf );
}


/* return # of precharging gates per column */
u_int O_arr_pre::n_pre_gate( int model )
{
  switch ( model ) {
    case SINGLE_BITLINE:	return 2;
    case EQU_BITLINE:		return 3;
    case SINGLE_OTHER:		return 1;
    default:	/* some error handler */
	break;
  }

  return 0;
}


/* return # of precharging drains per line */
u_int O_arr_pre::n_pre_drain( int model )
{
  switch ( model ) {
    case SINGLE_BITLINE:	return 1;
    case EQU_BITLINE:		return 2;
    case SINGLE_OTHER:		return 1;
    default:	/* some error handler */
	break;
  }

  return 0;
}


int O_arr_pre::clear_stat()
{
  n_charge = 0;

  return 0;
}


int O_arr_pre::init(  int m, double pre_size, ORION_Tech_Config* con )
{
	conf = con;

  u_int n_gate;

  n_gate = n_pre_gate(m);
	n_charge = 0;

  if ((model = m) && m < PRE_MAX_MODEL) {
    clear_stat();

    /* WHS: 10 should go to PARM */
    e_charge = cap( pre_size, 10 ) * n_gate * conf->PARM("EnergyFactor");

    /* static power */
    I_static = n_gate * pre_size * TABS::PMOS_TAB[0] / conf->PARM("TECH_POINT") * 100;

    return 0;
  }
  else
    return -1;
}


int O_arr_pre::record(  LIB_Type_max_uint n_charge )
{
  n_charge += n_charge;

  return 0;
}


double O_arr_pre::report(  )
{
  return ( n_charge * e_charge + I_static * conf->PARM("Vdd") * conf->PARM("Period") * conf->PARM("SCALE_S"));
}

/*============================== precharge ==============================*/

