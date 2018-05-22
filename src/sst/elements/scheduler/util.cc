// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace SST {
namespace Scheduler {
namespace Utils {

bool file_exists(const std::string &path) {
    struct stat sb;
    return ( stat(path.c_str(), &sb) == 0 );
}



time_t file_time_last_written(const std::string &path) {
    struct stat sb;
    if ( stat(path.c_str(), &sb) == 0 ) {
        return sb.st_mtime;
    }
    return (time_t)-1;
}

}
}
}
