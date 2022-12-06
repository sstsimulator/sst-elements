#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define PATH_NAME "read_write_test.txt"

char* data =
"The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems \
where the ISA, microarchitecture, and memory interact with the programming model and communications system. \
The package provides two novel capabilities. The first is a fully modular design that enables extensive \
exploration of an individual system parameter without the need for intrusive changes to the simulator. \
The second is a parallel simulation environment based on MPI. This provides a high level of performance \
and the ability to look at large systems. The framework has been successfully used to model concepts ranging \
from processing in memory to conventional processors connected by conventional network interfaces and running MPI.";

void writeData( const char *pathname );
void readData( const char* pathname );

static FILE* output; 

int main( int argc, char* argv[] ) {
    output=stdout; 
    writeData( PATH_NAME );
    readData( PATH_NAME );
    if ( unlink( PATH_NAME ) ) {
        printf("unlink '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    }
    fprintf(output,"unlink '%s' succeeded\n",PATH_NAME);
    return 0;
}

void writeData( const char* pathname ) {
    int lens[] = { 10, 20, 10000 };
    int offset = 0;
    int pos = 0;

    int fd = open( pathname, O_WRONLY|O_CREAT, S_IRWXU );
    if ( fd == -1) {
        fprintf(output,"%s() open '%s' failed, error %s \n",__func__,pathname,strerror(errno));
        exit(-1);
    }
    fprintf(output,"%s() open( '%s' ) succeeded\n",__func__,pathname);

    while( strlen((char*) data + offset) ) {
        int tmp = lens[pos] < strlen((char*) data + offset) ? lens[pos]: strlen((char*) data + offset);
        int ret = write( fd, data + offset, tmp );

        if( ret != tmp) {
            printf("write failed, error %s \n",strerror(errno));
            exit(-1);
        }
        printf("write %d bytes\n",tmp);
        
        offset += tmp;
        ++pos;
    }
    
    if ( close( fd ) ) {
        fprintf(output,"%s() close failed, %s",__func__,strerror(errno));
        exit(-1);
    }
    fprintf(output,"%s() close '%s' succeeded\n",__func__,pathname);
}

void readData( const char* pathname ) {
    char buf[10000];
    int lens[] = { 10, 20, 10000 };
    int offset = 0;
    int pos = 0;
    int numRead;

    int fd = open( pathname, O_RDONLY|O_CREAT, S_IRWXU );
    if ( fd == -1) {
        fprintf(output,"%s() open '%s' failed, error %s \n",__func__,pathname,strerror(errno));
        exit(-1);
    }
    fprintf(output,"%s() open '%s' succeeded\n",__func__,pathname);

    while( ( numRead = read( fd, buf, lens[pos++] ) ) > 0 ) {
        printf("read %d bytes\n",numRead);
        fprintf(stderr,"%s",buf);
    }

    if ( close( fd ) ) {
        fprintf(output,"%s() close failed, %s",__func__,strerror(errno));
        exit(-1);
    }
    fprintf(output,"%s() close '%s' succeeded\n",__func__,pathname);
}
