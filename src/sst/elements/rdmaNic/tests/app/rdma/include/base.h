#ifndef _BASE_H
#define _BASE_H

#include <rdmaNicHostInterface.h>
void writeCmd( NicCmd* cmd );
void base_init();
int base_n_pes(); 
int base_my_pe();
void base_make_progress();

#endif
