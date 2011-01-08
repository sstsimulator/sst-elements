// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
  static int tmpCount = 0;
  sstdisksim_event* ev = new sstdisksim_event();
  ev->completed = 0;

  if ( tmpCount > 100 )
  {
    ev->etype = DISKSIMEND;
    return ev;
  }

  tmpCount++;
  

  // Todo:  Add reading of events from a file here.
  // Erase this after starting to read from the file
  if ( tmpCount % 3 == 0 )
    ev->etype = DISKSIMREAD;
  else
    ev->etype = DISKSIMWRITE;
  ev->pos = 0;
  ev->count = 1000;
  ev->devno = 0;
  //End erase

  return ev;
}
