
#include "./gen.h"

void genBus( FILE* fp, char *name, int sstRank, linkName_t links[], int busWidth, float busClock )
{
    fprintf(fp, "%s<component name=%s type=Bus rank=%d >\n", tab(fp,1), name, sstRank );
    fprintf(fp, "%s    <params include=busParams>\n",tab(fp,1));
    fprintf(fp, "%s    <width> %d </width>\n",tab(fp,1),busWidth);
    fprintf(fp, "%s    <clock> %.3fGhz </clock>\n",tab(fp,1),busClock);
    fprintf(fp, "%s    </params>\n",tab(fp,1));
    int pos = 0;
    while ( strlen(links[pos]) ) {
        fprintf(fp, "%s    <link name=%s port=port latency=$lat />\n", tab(fp,1), links[pos] );
        ++pos;
    }
    fprintf(fp, "%s</component>\n",tab(fp,1));
}
