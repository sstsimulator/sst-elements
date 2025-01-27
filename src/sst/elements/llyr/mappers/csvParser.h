// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CSV_PARSER_H
#define _CSV_PARSER_H

#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "../llyrTypes.h"

namespace SST {
namespace Llyr {

class CSVParser {
public:
    CSVParser(char delimiter = ',') : delimiter_(delimiter)
    {
    }

    CSVParser(const std::string& stringIn, char delimiter = ',') : delimiter_(delimiter)
    {
        read_csv_file(stringIn);
    }

    const std::vector< std::vector< std::string > >& get_data() const { return data_; }
    const std::vector< std::string >& operator[](uint32_t index) const { return data_[index]; }

private:
    char delimiter_;
    std::vector< std::vector< std::string > > data_;

    void read_csv_file(const std::string& stringIn)
    {
        // is this a csv file or a row
        auto const extension = stringIn.find_last_of('.');
        if( extension != std::string::npos ) {
            std::string fileExtension = stringIn.substr(extension + 1);

            if( fileExtension.compare("csv") == 0 ) {
                std::ifstream file(stringIn);

                if( !file.is_open() ) {
                    std::cerr << "File not found: " << stringIn << std::endl;
                    return;
                }

                std::string line;
                while (std::getline(file, line)) {
                    data_.push_back(parse_line(line));
                }
            }
        } else {
            data_.push_back(parse_line(stringIn));
        }
    }

    std::vector<std::string> parse_line(const std::string& line)
    {
        std::vector< std::string > row;
        std::stringstream ss(line);
        std::string cell;

        while( std::getline(ss, cell, delimiter_ ) ) {
            row.push_back(cell);
        }

        return row;
    }
};

}// namespace Llyr
}// namespace SST

#endif // _CSV_PARSER_H
