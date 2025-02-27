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
    // TODO Fix this so it is returning const string to avoid potential
    // TODO memory leak
    string* cuda_api_to_string(enum CudaAPI_t api) {
        string *str;
        switch (api) {
            case CUDA_REG_FAT_BINARY:
                str = new string("CUDA_REG_FAT_BINARY");
                break;
            case CUDA_REG_FUNCTION:
                str = new string("CUDA_REG_FUNCTION");
                break;
            case CUDA_MEMCPY:
                str = new string("CUDA_MEMCPY");
                break;
            case CUDA_CONFIG_CALL:
                str = new string("CUDA_CONFIG_CALL");
                break;
            case CUDA_SET_ARG:
                str = new string("CUDA_SET_ARG");
                break;
            case CUDA_LAUNCH:
                str = new string("CUDA_LAUNCH");
                break;
            case CUDA_FREE:
                str = new string("CUDA_FREE");
                break;
            case CUDA_GET_LAST_ERROR:
                str = new string("CUDA_GET_LAST_ERROR");
                break;
            case CUDA_MALLOC:
                str = new string("CUDA_MALLOC");
                break;
            case CUDA_REG_VAR:
                str = new string("CUDA_REG_VAR");
                break;
            case CUDA_MAX_BLOCK:
                str = new string("CUDA_MAX_BLOCK");
                break;
            case CUDA_PARAM_CONFIG:
                str = new string("CUDA_PARAM_CONFIG");
                break;
            case CUDA_THREAD_SYNC:
                str = new string("CUDA_THREAD_SYNC");
                break;
            case CUDA_GET_ERROR_STRING:
                str = new string("CUDA_GET_ERROR_STRING");
                break;
            case CUDA_MEMSET:
                str = new string("CUDA_MEMSET");
                break;
            case CUDA_MEMCPY_TO_SYMBOL:
                str = new string("CUDA_MEMCPY_TO_SYMBOL");
                break;
            case CUDA_MEMCPY_FROM_SYMBOL:
                str = new string("CUDA_MEMCPY_FROM_SYMBOL");
                break;
            case CUDA_SET_DEVICE:
                str = new string("CUDA_SET_DEVICE");
                break;
            case CUDA_CREATE_CHANNEL_DESC:
                str = new string("CUDA_CREATE_CHANNEL_DESC");
                break;
            case CUDA_BIND_TEXTURE:
                str = new string("CUDA_BIND_TEXTURE");
                break;
            case CUDA_REG_TEXTURE:
                str = new string("CUDA_REG_TEXTURE");
                break;
            case CUDA_GET_DEVICE_COUNT:
                str = new string("CUDA_GET_DEVICE_COUNT");
                break;
            case CUDA_FREE_HOST:
                str = new string("CUDA_FREE_HOST");
                break;
            case CUDA_MALLOC_HOST:
                str = new string("CUDA_MALLOC_HOST");
                break;
            case CUDA_MEMCPY_ASYNC:
                str = new string("CUDA_MEMCPY_ASYNC");
                break;
            case CUDA_GET_DEVICE_PROPERTIES:
                str = new string("CUDA_GET_DEVICE_PROPERTIES");
                break;
            case CUDA_SET_DEVICE_FLAGS:
                str = new string("CUDA_SET_DEVICE_FLAGS");
                break;
            case CUDA_STREAM_CREATE:
                str = new string("CUDA_STREAM_CREATE");
                break;
            case CUDA_STREAM_DESTROY:
                str = new string("CUDA_STREAM_DESTROY");
                break;
            case CUDA_EVENT_CREATE:
                str = new string("CUDA_EVENT_CREATE");
                break;
            case CUDA_EVENT_CREATE_WITH_FLAGS:
                str = new string("CUDA_EVENT_CREATE_WITH_FLAGS");
                break;
            case CUDA_EVENT_RECORD:
                str = new string("CUDA_EVENT_RECORD");
                break;
            case CUDA_EVENT_SYNCHRONIZE:
                str = new string("CUDA_EVENT_SYNCHRONIZE");
                break;
            case CUDA_EVENT_ELAPSED_TIME:
                str = new string("CUDA_EVENT_ELAPSED_TIME");
                break;
            case CUDA_EVENT_DESTROY:
                str = new string("CUDA_EVENT_DESTROY");
                break;
            case CUDA_DEVICE_GET_ATTRIBUTE:
                str = new string("CUDA_DEVICE_GET_ATTRIBUTE");
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
