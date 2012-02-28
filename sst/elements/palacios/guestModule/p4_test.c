#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define NPAGES 1

/* this is a test program that opens the mmap_drv.
   It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness.
   You need a device special file to access the driver.
   The device special file is called 'node' and searched
   in the current directory.
   To create it
   - load the driver
     'insmod mmap_mod.o'
   - find the major number assigned to the driver
     'grep mmapdrv /proc/devices'
   - and create the special file (assuming major number 254)
     'mknod node c 254 0'
*/

int main(void)
{
  int fd;
  unsigned int *vadr;

  int len = NPAGES * getpagesize();

  if ((fd=open("node", O_RDWR|O_SYNC))<0)
  {
      perror("open");
      exit(-1);
  }

  vadr = mmap(0, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  
  if (vadr == MAP_FAILED)
  {
          perror("mmap");
          exit(-1);
  }


  printf("%#x\n",vadr[0]);
  vadr[1] = 0x12345678;
    
  close(fd);
  return(0);
}

