#include "sstdisksim_otf_parser.h"

/******************************************************************************/
sstdisksim_otf_parser::sstdisksim_otf_parser(string filename)
{
  filestream.open(filename.c_str());
}

/******************************************************************************/
sstdisksim_otf_parser::~sstdisksim_otf_parser()
{
  filestream.close();
}

/******************************************************************************/
sstdisksim_event*
sstdisksim_otf_parser::getNextEvent()
{
  sstdisksim_event* ev = new sstdisksim_event();

  ev->done = 0;

  // Todo:  Add reading of events from a file here.
  // Erase this after starting to read from the file
  ev->etype = READ;
  ev->pos = 0;
  ev->count = 1000;
  ev->devno = 0;
  //End erase

  return ev;
}
