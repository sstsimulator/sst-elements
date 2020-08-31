
#ifndef _H_VANADIS_INST_ALL
#define _H_VANADIS_INST_ALL

// Arithmetic operations
#include "inst/vadd.h"
#include "inst/vaddi.h"
#include "inst/vaddiu.h"
#include "inst/vsub.h"
#include "inst/vmul.h"
#include "inst/vmuli.h"
#include "inst/vdivmod.h"

// Logical operations
#include "inst/vand.h"
#include "inst/vandi.h"
#include "inst/vor.h"
#include "inst/vxor.h"
#include "inst/vxori.h"

// Shift operations
#include "inst/vsll.h"
#include "inst/vslli.h"
#include "inst/vsra.h"
#include "inst/vsrai.h"
#include "inst/vsrl.h"
#include "inst/vsrli.h"

// Compare operations
#include "inst/vcmplt.h"
#include "inst/vcmplte.h"
#include "inst/vscmp.h"
#include "inst/vscmpi.h"
#include "inst/vbcmpi.h"

// Jumps and PC change instructions
#include "inst/vjump.h"
#include "inst/vbgtzl.h"
#include "inst/vjlr.h"
#include "inst/vjr.h"
#include "inst/vbcmp.h"

// Load instructions
#include "inst/vload.h"
#include "inst/vpartialload.h"

// Store instructions
#include "inst/vstore.h"
#include "inst/vpartialstore.h"

// Fence Instructions
#include "inst/vfence.h"

// Special instructions
#include "inst/vdecodefaultinst.h"
#include "inst/vnop.h"
#include "inst/vsetreg.h"
#include "inst/vsyscall.h"

#endif
