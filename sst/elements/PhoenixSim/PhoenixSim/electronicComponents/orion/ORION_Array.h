#ifndef _ORION_ARRAY_H
#define _ORION_ARRAY_H



#include "ORION_Array_Info.h"
#include "ORION_Array_Internal.h"
#include "ORION_Util.h"



/*@
 * data type: array port state
 *
 *  - row_addr       -- input to row decoder
 *    col_addr       -- input to column decoder, if any
 *    tag_addr       -- input to tag comparator
 * $+ tag_line       -- value of tag bitline
 *  # data_line_size -- size of data_line in char
 *  # data_line      -- value of data bitline
 *
 * legend:
 *   -: only used by non-fully-associative array
 *   +: only used by fully-associative array
 *   #: only used by fully-associative array or RF array
 *   $: only used by write-through array
 *
 * NOTE:
 *   (1) *_addr may not necessarily be an address
 *   (2) data_line_size is the allocated size of data_line in simulator,
 *       which must be no less than the physical size of data line
 *   (3) each instance of module should define an instance-specific data
 *       type with non-zero-length data_line and cast it to this type
 */
class PortState{
public:
	PortState(ORION_Array_Info *info );


	LIB_Type_max_uint row_addr;
	LIB_Type_max_uint col_addr;
	LIB_Type_max_uint tag_addr;
	LIB_Type_max_uint tag_line;
	u_int data_line_size;
	char data_line[0];
};

/*@
 * data type: array set state
 *
 *   entry           -- pointer to some entry structure if an entry is selected for
 *                      r/w, NULL otherwise
 *   entry_set       -- pointer to corresponding set structure
 * + write_flag      -- 1 if entry is already written once, 0 otherwise
 * + write_back_flag -- 1 if entry is already written back, 0 otherwise
 *   valid_bak       -- valid bit of selected entry before operation
 *   dirty_bak       -- dirty bit of selected entry, if any, before operation
 *   tag_bak         -- tag of selected entry before operation
 *   use_bak         -- use bits of all entries before operation
 *
 * legend:
 *   +: only used by fully-associative array
 *
 * NOTE:
 *   (1) entry is interpreted by modules, if some module has no "entry structure",
 *       then make sure this field is non-zero if some entry is selected
 *   (2) tag_addr may not necessarily be an address
 *   (3) each instance of module should define an instance-specific data
 *       type with non-zero-length use_bit and cast it to this type
 */
class SetState{
public:
	SetState(ORION_Array_Info *info );


	void *entry;
	void *entry_set;
	int write_flag;
	int write_back_flag;
	u_int valid_bak;
	u_int dirty_bak;
	LIB_Type_max_uint tag_bak;
	u_int use_bak[0];
} ;



class ORION_Array {

public:


	ORION_Array(){} ;


	/* prototype-level record functions */
	int dec( PortState *port, LIB_Type_max_uint row_addr, int rw );
	int read( LIB_Type_max_uint data );
	int data_read(  LIB_Type_max_uint data );
	int data_write( SetState *set, u_int n_item, u_char *new_data );
	int tag_read(  SetState *set );
	int tag_update(  PortState *port, SetState *set );
	int tag_compare( PortState *port, LIB_Type_max_uint tag_input, LIB_Type_max_uint col_addr, SetState *set );
	int output(  u_int data_size, u_int length, void *data_out, void *data_all );



	int init( ORION_Array_Info *info);
	double report( );
	int clear_stat();

	double stat_energy( double n_read, double n_write,  int max_avg);
	double stat_static_power();


	ORION_Array_Info* info;
	O_dec row_dec;
	O_dec col_dec;
	O_wordline data_wordline;
	O_wordline tag_wordline;
	O_bitline data_bitline;
	O_bitline tag_bitline;
	O_mem data_mem;
	/* tag address memory cells */
	O_mem tag_mem;
	/* tag various bits memory cells, different with tag_mem for fully-associative cache */
	O_mem tag_attach_mem;
	O_amp data_amp;
	O_amp tag_amp;
	O_comp comp;
	O_mux mux;
	O_out outdrv;
	O_arr_pre row_dec_pre;
	O_arr_pre col_dec_pre;
	O_arr_pre data_bitline_pre;
	O_arr_pre tag_bitline_pre;
	O_arr_pre data_colsel_pre;
	O_arr_pre tag_colsel_pre;
	O_arr_pre comp_pre;
	ORION_FF ff;
	double I_static;



};



#endif
