// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    string* gpu_api_to_string(enum GpuApi_t api) {
        string *str;
        switch (api) {
            case GPU_REG_FAT_BINARY:
                str = new string("GPU_REG_FAT_BINARY");
                break;
            case GPU_REG_FAT_BINARY_RET:
                str = new string("GPU_REG_FAT_BINARY_RET");
                break;
            case GPU_REG_FUNCTION:
                str = new string("GPU_REG_FUNCTION");
                break;
            case GPU_REG_FUNCTION_RET:
                str = new string("GPU_REG_FUNCTION_RET");
                break;
            case GPU_MEMCPY:
                str = new string("GPU_MEMCPY");
                break;
            case GPU_MEMCPY_RET:
                str = new string("GPU_MEMCPY_RET");
                break;
            case GPU_CONFIG_CALL:
                str = new string("GPU_CONFIG_CALL");
                break;
            case GPU_CONFIG_CALL_RET:
                str = new string("GPU_CONFIG_CALL_RET");
                break;
            case GPU_SET_ARG:
                str = new string("GPU_SET_ARG");
                break;
            case GPU_SET_ARG_RET:
                str = new string("GPU_SET_ARG_RET");
                break;
            case GPU_LAUNCH:
                str = new string("GPU_LAUNCH");
                break;
            case GPU_LAUNCH_RET:
                str = new string("GPU_LAUNCH_RET");
                break;
            case GPU_FREE:
                str = new string("GPU_FREE");
                break;
            case GPU_FREE_RET:
                str = new string("GPU_FREE_RET");
                break;
            case GPU_GET_LAST_ERROR:
                str = new string("GPU_GET_LAST_ERROR");
                break;
            case GPU_GET_LAST_ERROR_RET:
                str = new string("GPU_GET_LAST_ERROR_RET");
                break;
            case GPU_MALLOC:
                str = new string("GPU_MALLOC");
                break;
            case GPU_MALLOC_RET:
                str = new string("GPU_MALLOC_RET");
                break;
            case GPU_REG_VAR:
                str = new string("GPU_REG_VAR");
                break;
            case GPU_REG_VAR_RET:
                str = new string("GPU_REG_VAR_RET");
                break;
            case GPU_MAX_BLOCK:
                str = new string("GPU_MAX_BLOCK");
                break;
            case GPU_MAX_BLOCK_RET:
                str = new string("GPU_MAX_BLOCK_RET");
                break;
            default:
                str = new string("Unknown cuda calls");
                break;
        }
        return str;
    }

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