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
