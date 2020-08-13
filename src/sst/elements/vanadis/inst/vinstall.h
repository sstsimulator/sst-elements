
#ifndef _H_VANADIS_INST_ALL
#define _H_VANADIS_INST_ALL

// Arithmetic operations
#include "inst/vadd.h"
#include "inst/vaddi.h"
#include "inst/vsub.h"

// Logical operations
#include "inst/vand.h"
#include "inst/vor.h"
#include "inst/vxor.h"

// Shift operations
#include "inst/vsll.h"
#include "inst/vslli.h"
#include "inst/vsra.h"
#include "inst/vsrai.h"
#include "inst/vsrl.h"
#include "inst/vsrli.h"

// Jumps and PC change instructions
#include "inst/vjump.h"

// Load instructions
#include "inst/vload.h"

// Store instructions
#include "inst/vstore.h"

// Special instructions
#include "inst/vdecodefaultinst.sh"

#endif
