#include "utilities.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* 
 * Build up cycle display for wave.
 */
void build_wave(char *buffer_string, int transaction_id, Command *this_c, int action){
  int buffer_len;
  int print_detail;

  //print_detail = FALSE;
  print_detail = TRUE;
  buffer_len = strlen(buffer_string);
  switch(this_c->command){
  case RAS:
    sprintf(&buffer_string[buffer_len],"RAS ");
    break;
  case CAS:
    sprintf(&buffer_string[buffer_len],"CAS ");
    break;
  case CAS_WITH_DRIVE:
    sprintf(&buffer_string[buffer_len],"CWD ");
    break;
  case CAS_AND_PRECHARGE:
    sprintf(&buffer_string[buffer_len],"CAP ");
    break;
  case CAS_WRITE:
    sprintf(&buffer_string[buffer_len],"CAW ");
    break;
  case CAS_WRITE_AND_PRECHARGE:
    sprintf(&buffer_string[buffer_len],"CWP ");
    break;
  case RETIRE:
    sprintf(&buffer_string[buffer_len],"RET ");
    break;
  case PRECHARGE:
    sprintf(&buffer_string[buffer_len],"PRE ");
    break;
  case PRECHARGE_ALL:
    sprintf(&buffer_string[buffer_len],"PRA ");
    break;
  case RAS_ALL:
    sprintf(&buffer_string[buffer_len],"RSA ");
    break;
  case DATA:
    sprintf(&buffer_string[buffer_len],"DAT ");
    break;
  case DRIVE:
    sprintf(&buffer_string[buffer_len],"DRV ");
    break;
  case REFRESH:
    sprintf(&buffer_string[buffer_len],"REF ");
    break;
  case REFRESH_ALL:
    sprintf(&buffer_string[buffer_len],"RFA ");
    break;
  }
  buffer_len += 4;
  if(print_detail){
    sprintf(&buffer_string[buffer_len],"tid[%llu] ch[%d] ra[%d] ba[%d] ro[%X] co[%X] ref[%d] ",
	    (long long unsigned int)this_c->tran_id,
	    this_c->chan_id,
	    this_c->rank_id,
	    this_c->bank_id,
	    this_c->row_id,
	    this_c->col_id,
		this_c->refresh);
    buffer_len = strlen(buffer_string);
  }
  switch(action){
  case MEM_STATE_SCHEDULED:
    sprintf(&buffer_string[buffer_len],"SCHEDULED : ");
    break;
  case XMIT2PROC:
    sprintf(&buffer_string[buffer_len],"XMIT2PROC : ");
    break;
  case PROC2DATA:
    sprintf(&buffer_string[buffer_len],"PROC2DATA : ");
    break;
  case MEM_STATE_COMPLETED:
    sprintf(&buffer_string[buffer_len],"COMPLETED : ");
    break;
  case XMIT2AMBPROC:
    sprintf(&buffer_string[buffer_len],"XMIT2AMBPROC : ");
    break;
  case AMBPROC2AMBCT:
    sprintf(&buffer_string[buffer_len],"AMBPROC2AMBCT : ");
    break;
  case AMBPROC2DATA:
    sprintf(&buffer_string[buffer_len],"AMBPROC2DATA : ");
    break;
  case AMBCT2PROC:
    sprintf(&buffer_string[buffer_len],"AMBCT2PROC : ");
    break;
  case AMBCT2DATA:
    sprintf(&buffer_string[buffer_len],"AMBCT2DATA : ");
  case DATA2AMBDOWN:
    sprintf(&buffer_string[buffer_len],"DATA2AMBDOWN : ");
    break;
  case AMBDOWN2DATA:
    sprintf(&buffer_string[buffer_len],"AMBDOWN2DATA : ");
    break;
  }
}
