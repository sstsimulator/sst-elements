#ifndef _SSTDISKSIM_OTF_PARSER_H
#define _SSTDISKSIM_OTF_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <iostream>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <string>
#include "sstdisksim_event.h"

using namespace std;

class sstdisksim_otf_parser {

 public:
  sstdisksim_otf_parser(string filename);
  ~sstdisksim_otf_parser();
  sstdisksim_event* getNextEvent();

 private:
  ofstream filestream;
};

#endif /* _SSTDISKSIM_OTF_PARSER_H */
