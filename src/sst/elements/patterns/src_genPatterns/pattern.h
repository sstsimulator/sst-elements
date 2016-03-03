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

#ifndef _PATTERN_H_
#define _PATTERN_H_


int read_pattern_file(FILE *fp_pattern, int verbose);
void disp_pattern_params(void);
char *pattern_name(void);
void pattern_params(FILE *out);

#endif /* _PATTERN_H_ */
