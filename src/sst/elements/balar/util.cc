// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdint.h>
#include <algorithm>
#include "util.h"
#include <stdexcept>

using namespace std;

namespace SST {
namespace BalarComponent {
    /**
     * @brief Trim the whitespace for a given string
     *
     * @param s
     * @return std::string&
     */
    std::string& trim(std::string& s) {
        // Credit: https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring

        // Remove upto none space ch from left
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) { return !std::isspace(ch); }));

        // From right
        s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) { return !std::isspace(ch); }).base(), s.end());

        return s;
    }

    /**
     * @brief Return a vector of string separated by delim
     *
     * @param s
     * @param delim
     * @return std::vector<std::string>
     */
    std::vector<std::string> split(std::string& s, const std::string& delim) {
        std::vector<std::string> arr;
        size_t delim_length = delim.length();
        while (!s.empty()) {
            size_t pos = s.find(delim);
            if (pos == std::string::npos) {
                // All done
                arr.push_back(s);
                s.clear();
            } else {
                std::string sub = s.substr(0, pos);
                s = s.substr(pos + delim_length);
                arr.push_back(sub);
            }
        }
        return arr;
    }

    /**
     * @brief Construct a map from a vector of std string
     *        where each element is separated by delim
     *
     * @param vec
     * @param delim
     * @return std::map<std::string, std::string>
     */
    std::map<std::string, std::string> map_from_vec(std::vector<std::string> vec, const std::string& delim) {
        std::map<std::string, std::string> map;
        size_t delim_length = delim.length();
        for (auto it = vec.begin(); it < vec.end(); it++) {
            // Split each element into key-val by delim
            std::string str = *it;
            size_t pos = str.find(delim);
            if (pos == std::string::npos) {
                // Treat entire string as key
                map.insert({str, std::string("")});
            } else {
                std::string key = str.substr(0, pos);
                std::string val = str.substr(pos + delim_length);
                map.insert({key, val});
            }
        }
        return map;
    }
}
}
