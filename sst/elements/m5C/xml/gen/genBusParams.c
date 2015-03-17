// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

static char *str =
"<busParams>\n\
    <clock> 1.200Ghz </clock>\n\
    <responder_set> false </responder_set>\n\
    <block_size> 64 </block_size>\n\
    <bus_id> 0 </bus_id>\n\
    <header_cycles> 1 </header_cycles>\n\
    <width> 64 </width>\n\
</busParams>\n";


void genBusParams( FILE *fp )
{
    fprintf( fp, "%s",str);
}
