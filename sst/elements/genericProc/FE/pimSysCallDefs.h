// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef PIMSYSCALLDEFS_H
#define PIMSYSCALLDEFS_H

/* AFR: added */
#define	SS_PIM_FORK		3000
#define	SS_PIM_EXIT		3001
#define	SS_PIM_LOCK     	3002
#define	SS_PIM_UNLOCK		3003
#define	SS_PIM_IS_LOCAL		3004
#define	SS_PIM_ALLOCATE_LOCAL	3005
#define	SS_PIM_MOVE_TO  	3006
#define	SS_PIM_MOVE_AWAY	3007
#define	SS_PIM_QUICK_PRINT	3008
#define	SS_PIM_RESET    	3009
#define SS_PIM_NUMBER           3010
#define SS_PIM_REMAP            3011
#define SS_PIM_REMAP_TO_ADDR    3012
#define SS_PIM_ATOMIC_INCREMENT    3013
#define SS_PIM_EST_STATE_SIZE   3014
#define SS_PIM_READFF           3015
#define SS_PIM_READFE           3016
#define SS_PIM_WRITEEF          3017
#define SS_PIM_FILL_FE          3018
#define SS_PIM_EMPTY_FE         3019
#define SS_PIM_IS_FE_FULL       3020
#define SS_PIM_IS_PRIVATE       3021
#define SS_PIM_TID              3022
#define SS_PIM_REMAP_TO_POLY    3023
#define SS_PIM_TAG_INSTRUCTIONS  3024
#define SS_PIM_TAG_SWITCH  3025
#define SS_PIM_SPAWN_TO_COPROC 3026
#define SS_PIM_SWITCH_ADDR_MODE 3027

#define SS_PIM_WRITE_SPECIAL 3028
#define SS_PIM_WRITE_SPECIAL2 3029
#define SS_PIM_WRITE_SPECIAL3 3030
#define SS_PIM_RW_SPECIAL3 3031
#define SS_PIM_READ_SPECIAL 3032
#define SS_PIM_READ_SPECIAL1 3033
#define SS_PIM_READ_SPECIAL2 3034
#define SS_PIM_READ_SPECIAL3 3035
#define SS_PIM_READ_SPECIAL1_2 3036
#define SS_PIM_READ_SPECIAL1_5 3037
#define SS_PIM_WRITE_SPECIAL5 3038
#define SS_PIM_READ_SPECIAL4 3039
#define SS_PIM_READ_SPECIAL_2 3040

#define SS_PIM_TRYEF           3041

#define SS_PIM_WRITE_SPECIAL4 3042
#define SS_PIM_READ_SPECIAL1_6 3043
#define SS_PIM_WRITE_SPECIAL6 3044

#define SS_PIM_BULK_EMPTY_FE 3045
#define SS_PIM_BULK_FILL_FE 3046

#define SS_PIM_WRITE_SPECIAL7 3047
#define SS_PIM_READ_SPECIAL1_7 3048

#define SS_PIM_RAND 3049

#define SS_PIM_FFILE_RD 3050

#define SS_PIM_MALLOC 3051
#define SS_PIM_FREE 3052

#define SS_PIM_WRITE_MEM 3053

#define SS_PIM_ATOMIC_DECREMENT 3054
#define SS_PIM_EXIT_FREE 3055
#define SS_PIM_SPAWN_TO_LOCALE_STACK 3056

#define SS_PIM_MEM_REGION_CREATE 3057

#define SS_PIM_MEM_REGION_GET 3058

#define SS_PIM_TRACE 3061

#define SS_PIM_SPAWN_TO_LOCALE_STACK_STOPPED 3062
#define SS_PIM_START_STOPPED_THREAD 3063

#define SS_PIM_CORE_NUM_GET 3064
#define SS_PIM_AMO 3065
#define SS_PIM_MATVEC 3066

#endif
