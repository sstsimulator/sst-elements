
/*-------------------------------------------------------------------------
 *                             ORION 2.0
 *
 *         					Copyright 2009
 *  	Princeton University, and Regents of the University of California
 *                         All Rights Reserved
 *
 *
 *  ORION 2.0 was developed by Bin Li at Princeton University and Kambiz Samadi at
 *  University of California, San Diego. ORION 2.0 was built on top of ORION 1.0.
 *  ORION 1.0 was developed by Hangsheng Wang, Xinping Zhu and Xuning Chen at
 *  Princeton University.
 *
 *  If your use of this software contributes to a published paper, we
 *  request that you cite our paper that appears on our website
 *  http://www.princeton.edu/~peh/orion.html
 *
 *  Permission to use, copy, and modify this software and its documentation is
 *  granted only under the following terms and conditions.  Both the
 *  above copyright notice and this permission notice must appear in all copies
 *  of the software, derivative works or modified versions, and any portions
 *  thereof, and both notices must appear in supporting documentation.
 *
 *  This software may be distributed (but not offered for sale or transferred
 *  for compensation) to third parties, provided such third parties agree to
 *  abide by the terms and conditions of this notice.
 *
 *  This software is distributed in the hope that it will be useful to the
 *  community, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *-----------------------------------------------------------------------*/

#ifndef _SIM_PARAMETER_H
#define _SIM_PARAMETER_H

//#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "sim_includes.h"


#define u_int unsigned int
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#define LIB_Type_max_uint       uint64_t
#define LIB_Type_max_int        int64_t

/*Useful macros, wrappers and functions */
#define __INSTANCE__ mainpe__power
#define GLOBDEF(t,n) t mainpe__power___ ## n
#define GLOB(n) mainpe__power___ ## n
#define FUNC(n, args...) mainpe__power___ ## n (args)
#define FUNCPTR(n)  mainpe__power___ ## n
//#define PARM(x) PARM_ ## x


#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif

#define NEXT_DEPTH(d)	((d) > 0 ? (d) - 1 : (d))

#define BIGNUM	(1e30)
#define BIGONE	((LIB_Type_max_uint)1)
#define BIGNONE	((LIB_Type_max_uint)-1)
#define HAMM_MASK(w)	((u_int)w < (sizeof(LIB_Type_max_uint) << 3) ? (BIGONE << (w)) - 1 : BIGNONE)

#define FALSE	0
#define TRUE	1
/* Used to communicate with the horowitz model */
#define RISE 1
#define FALL 0
#define NCH  1
#define PCH  0

#define MAX_ENERGY	1
#define AVG_ENERGY	0
/*End useful macros, wrappers and functions */

#define	SIM_NO_MODEL	0

enum TRANS_TYPES {
	LVT = 1, NVT = 2, HVT = 3
};

class ORION_Tech_Config {

public:
	int technology; //in nm;  90, 65, 45, 32

	TRANS_TYPES trans_type;

	double voltage;
	double frequency;



	ORION_Tech_Config(int tech, int type, double volt,
			double freq);

	void initParams();

	double PARM(string name);

private:
	map<string, double> params;
	void initTABS();
};

/* static stuff */
class TABS{
public:

	static double NMOS_TAB[1];
	static double PMOS_TAB[1];
	static double NAND2_TAB[4];
	static double NOR2_TAB[4];
	static double DFF_TAB[1];

};

/* End available models for router and link */

/* User-defined router with selected tech node and operating freq and Vdd
 * CAUTION: Please do not alter the location of this SIM_port.h file, sequency tricky
 */

#ifdef ORION_VERSION_2
//#include "ORION_2_Params/ORION_port.h"
/*End user-defined router with selected tech node and operating freq and Vdd */

/* Useful macros related to parameters defined by users */
//#define Vdd             (PARM(Vdd))
//#define Period          ((double)1/(double)PARM(Freq))

//#define Powerfactor		((PARM(Freq))*Vdd*Vdd)
//#define EnergyFactor	(Vdd*Vdd)
/* End useful macros related to parameters defined by users */

/* Technology file */
//#include "ORION_2_Params/ORION_Technology.h"
/* End technology file */

#endif

#ifdef ORION_VERSION_1

#include "ORION_1_Params/SIM_port.h"
/*End user-defined router with selected tech node and operating freq and Vdd */

ORION 1.0 not supported

/* Useful macros related to parameters defined by users */

#define Period          ((double)1/(double)PARM(Freq))

#define EnergyFactor	(Vdd*Vdd)
/* End useful macros related to parameters defined by users */
#include "ORION_1_Params/SIM_power_test.h"
/* Technology file */
#include "ORION_1_Params/SIM_power.h"
/* End technology file */

#endif

/* link wire type model*/
#define	LOCAL			1
#define INTERMEDIATE	2
#define GLOBAL			3

/* link width spacing type */
#define SWIDTH_SSPACE	1  	//singleWidthSingleSpace
#define SWIDTH_DSPACE	2  	//singleWidthDoubleSpace
#define DWIDTH_SSPACE	3  	//doubleWidthSingleSpace
#define DWIDTH_DSPACE	4   //doubleWidthDoubleSpace

/* link buffering scheme type */
#define MIN_DELAY	1
#define STAGGERED	2


typedef enum {
	MATRIX_CROSSBAR = 1,
	MULTREE_CROSSBAR,
	CUT_THRU_CROSSBAR,
	CROSSBAR_MAX_MODEL
} Crossbar_Model;

typedef enum {
	RR_ARBITER = 1,
	MATRIX_ARBITER,
	QUEUE_ARBITER,
	ARBITER_MAX_MODEL
} Arbiter_Model;

typedef enum {
	SRAM = 1,
	REGISTER,
	BUFFER_MAX_MODEL
} Buffer_Model;


/* connection type */
typedef enum {
	TRANS_GATE,	/* transmission gate connection */
	TRISTATE_GATE,	/* tri-state gate connection */
} Connect;

typedef enum {
	RESULT_BUS = 1,
	GENERIC_BUS,
	BUS_MAX_MODEL
} Bus_Model;

typedef enum {
	GENERIC_SEL = 1,
	SEL_MAX_MODEL
} Sel_Model;

typedef enum {
	NEG_DFF = 1,	/* negative egde-triggered D flip-flop */
	FF_MAX_MODEL
} FF_Model;

typedef enum {
	IDENT_ENC = 1,	/* identity encoding */
	TRANS_ENC,	/* transition encoding */
	BUSINV_ENC,	/* bus inversion encoding */
	BUS_MAX_ENC
} Bus_Enc;



/*@
 * data type: decoder model types
 *
 *   GENERIC_DEC   -- default type
 *   DEC_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	GENERIC_DEC = 1, DEC_MAX_MODEL
} Dec_Model;

/*@
 * data type: multiplexor model types
 *
 *   GENERIC_MUX   -- default type
 *   MUX_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	GENERIC_MUX = 1, MUX_MAX_MODEL
} Mux_Model;

/*@
 * data type: sense amplifier model types
 *
 *   GENERIC_AMP   -- default type
 *   AMP_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	GENERIC_AMP = 1, AMP_MAX_MODEL
} Amp_Model;

/*@
 * data type: wordline model types
 *
 *   CACHE_RW_WORDLINE  -- default type
 *   CACHE_WO_WORDLINE  -- write data wordline only, for fully-associative data bank
 *   CAM_RW_WORDLINE    -- both R/W tag wordlines, for fully-associative write-back
 *                         tag bank
 *   CAM_WO_WORDLINE    -- write tag wordline only, for fully-associative write-through
 *                         tag bank
 *   WORDLINE_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	CACHE_RW_WORDLINE = 1,
	CACHE_WO_WORDLINE,
	CAM_RW_WORDLINE,
	CAM_WO_WORDLINE,
	WORDLINE_MAX_MODEL
} Wordline_Model;

/*@
 * data type: bitline model types
 *
 *   RW_BITLINE	       -- default type
 *   WO_BITLINE        -- write bitline only, for fully-associative data bank and
 *                        fully-associative write-through tag bank
 *   BITLINE_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	RW_BITLINE = 1, WO_BITLINE, BITLINE_MAX_MODEL
} Bitline_Model;

/*@
 * data type: precharge model types
 *
 *   PRE_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	SINGLE_BITLINE = 1, EQU_BITLINE, SINGLE_OTHER, PRE_MAX_MODEL
} Pre_Model;

/*@
 * data type: memory cell model types
 *
 *   NORMAL_MEM     -- default type
 *   CAM_TAG_RW_MEM -- read/write memory cell connected with tag comparator, for
 *                     fully-associative write-back tag bank
 *   CAM_TAG_WO_MEM -- write-only memory cell connected with tag comparator, for
 *                     fully-associative write-through tag bank
 *   CAM_DATA_MEM   -- memory cell connected with output driver, for fully-associative
 *                     data bank
 *   CAM_ATTACH_MEM -- memory cell of fully-associative array valid bit, use bit, etc.
 *   MEM_MAX_MODEL  -- upper bound of model type
 */
typedef enum {
	NORMAL_MEM = 1,
	CAM_TAG_RW_MEM,
	CAM_TAG_WO_MEM,
	CAM_DATA_MEM,
	CAM_ATTACH_MEM,
	MEM_MAX_MODEL
} Mem_Model;

/*@
 * data type: tag comparator model types
 *
 *   CACHE_COMP     -- default type
 *   CAM_COMP       -- cam-style tag comparator, for fully-associative array
 *   COMP_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	CACHE_COMP = 1, CAM_COMP, COMP_MAX_MODEL
} Comp_Model;

/*@
 * data type: output driver model types
 *
 *   CACHE_OUTDRV     -- default type
 *   CAM_OUTDRV       -- output driver connected with memory cell, for fully-associative
 *                       array
 *   REG_OUTDRV       -- output driver connected with bitline, for register files
 *   OUTDRV_MAX_MODEL -- upper bound of model type
 */
typedef enum {
	CACHE_OUTDRV = 1, CAM_OUTDRV, REG_OUTDRV, OUTDRV_MAX_MODEL
} Outdrv_Model;

typedef enum {
	ONE_STAGE_ARB = 1, TWO_STAGE_ARB, VC_SELECT, VC_ALLOCATOR_MAX_MODEL
} VC_Allocator_Model;

#endif /* _SIM_PARAMETER_H */

