// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_OBERON_FP64_VAL
#define _H_SST_OBERON_FP64_VAL

namespace SST {
namespace Oberon {

class OberonFP64ExprValue : public OberonExpressionValue {

        private:
                double value;

        public:
               	OberonFP64ExprValue(double v);
                int64_t getAsInt64();
                double  getAsFP64();

};

}
}

#endif
