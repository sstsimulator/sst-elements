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

/*
A utility class for writing output files in the same format as the N2A C simulator.
This code is not used by the C runtime. Instead, it is a simplified version
for use by those who wish to write C++ code compatible with OutputParser.
This is a pure header implementation. No need to build/link extra libraries.
*/


#ifndef output_holder_h
#define output_holder_h


#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <time.h>


inline void split (const std::string & source, const std::string & delimiter, std::string & first, std::string & second)
{
    int index = source.find (delimiter);
    if (index == std::string::npos)
    {
        first = source;
        second.clear ();
    }
    else
    {
        std::string temp = source;  // Make a copy of source, in case source is also one of the destination strings.
        first = temp.substr (0, index);
        second = temp.substr (index + delimiter.size ());
    }
}

inline std::string trim (const std::string & source)
{
    std::string::size_type begin = source.find_first_not_of (" \t\r\n");
    std::string::size_type end   = source.find_last_not_of  (" \t\r\n");
    if (begin == std::string::npos  ||  end == std::string::npos) return "";
    return source.substr (begin, end - begin + 1);
}

inline void replace_all (std::string & target, char find, char replace)
{
    int count = target.size ();
    for (int i = 0; i < count; i++)
    {
        if (target[i] == find) target[i] = replace;
    }
}

class OutputHolder
{
public:
    std::string                                      fileName;
    std::string                                      columnFileName;
    bool                                             raw;             ///< Indicates that column is an exact index.
    std::ostream *                                   out;
    std::unordered_map<std::string,int>              columnMap;
    std::vector<std::map<std::string,std::string> *> columnMode;
    std::vector<float>                               columnValues;
    int                                              columnsPrevious; ///< Number of columns written in previous cycle.
    bool                                             traceReceived;   ///< Indicates that at least one column was touched during the current cycle.
    float                                            t;

    OutputHolder (const std::string & fileName)
    :   fileName (fileName)
    {
        columnsPrevious = 0;
        traceReceived   = false;
        t               = 0;
        raw             = false;

        if (fileName.empty ())
        {
            out = &std::cout;
            columnFileName = "out.columns";
        }
        else
        {
            out = new std::ofstream (fileName.c_str ());
            columnFileName = fileName + ".columns";
        }
    }

    ~OutputHolder ()
    {
        if (out)
        {
            writeTrace ();
            out->flush ();
            if (out != &std::cout) delete out;

            writeModes ();
        }
        for (auto it : columnMode) delete it;
    }

    /// Subroutine for other trace() functions.
    void trace (float now)
    {
        // Detect when time changes and dump any previously traced values.
        if (now > t)
        {
            writeTrace ();
            t = now;
        }

        if (! traceReceived)  // First trace for this cycle
        {
            if (columnValues.empty ())  // slip $t into first column
            {
                columnMap["$t"] = 0;
                columnValues.push_back (t);
                columnMode.push_back (new std::map<std::string,std::string>);
            }
            else
            {
                columnValues[0] = t;
            }
            traceReceived = true;
        }
    }

    /// Subroutine for other trace() functions.
    void addMode (const char * mode)
    {
        std::map<std::string,std::string> * result = new std::map<std::string,std::string>;
        columnMode.push_back (result);
        if (mode)
        {
            std::string rest = mode;
            std::string hint;
            while (! rest.empty ())
            {
                split (rest, ",", hint, rest);
                hint = trim (hint);
                std::string key;
                std::string value;
                split (hint, "=", key, value);
                if (key == "timeScale")
                {
                    std::map<std::string,std::string> * c = columnMode[0];
                    (*c)["scale"] = value;
                }
                else if (key == "ymin"  ||  key == "ymax"  ||  key == "xmin"  ||  key == "xmax")
                {
                    std::map<std::string,std::string> * c = columnMode[0];
                    (*c)[key] = value;
                }
                else
                {
                    (*result)[key] = value;
                }
            }
        }
    }

    void trace (float now, const std::string & column, float value, const char * mode = 0)
    {
        trace (now);

        std::unordered_map<std::string, int>::iterator result = columnMap.find (column);
        if (result == columnMap.end ())
        {
            columnMap[column] = columnValues.size ();
            columnValues.push_back ((float) value);
            addMode (mode);
        }
        else
        {
            columnValues[result->second] = (float) value;
        }
    }

    void trace (float now, float column, float value, const char * mode = 0)
    {
        trace (now);

        char buffer[32];
        int index;  // Only used for "raw" mode.
        if (raw)
        {
            index = (int) round (column);
            snprintf (buffer, 32, "%i", index);
        }
        else
        {
            snprintf (buffer, 32, "%g", column);
        }
        std::string columnName = buffer;

        std::unordered_map<std::string, int>::iterator result = columnMap.find (columnName);
        if (result == columnMap.end ())
        {
            if (raw)
            {
                index++;  // column index + offset for time column
                columnValues.resize (index, NAN);  // add any missing columns before the one we are about to create
            }
            columnMap[columnName] = columnValues.size ();
            columnValues.push_back ((float) value);
            addMode (mode);
        }
        else
        {
            columnValues[result->second] = (float) value;
        }
    }

    void writeTrace ()
    {
        if (! traceReceived  ||  ! out) return;  // Don't output anything unless at least one value was set.

        const int count = columnValues.size ();
        const int last  = count - 1;

        // Write headers if new columns have been added
        if (count > columnsPrevious)
        {
            if (! raw)
            {
                std::vector<std::string> headers (count);
                for (auto it : columnMap) headers[it.second] = it.first;

                (*out) << headers[0];  // Should be $t
                int i = 1;
                for (; i < columnsPrevious; i++)
                {
                    (*out) << "\t";
                }
                for (; i < count; i++)
                {
                    (*out) << "\t";
                    std::string header (headers[i]);  // deep copy
                    replace_all (header, ' ', '_');
                    (*out) << header;
                }
                (*out) << std::endl;
            }
            columnsPrevious = count;
            writeModes ();
        }

        // Write values
        for (int i = 0; i <= last; i++)
        {
            float & c = columnValues[i];
            if (! std::isnan (c)) (*out) << c;
            if (i < last) (*out) << "\t";
            c = NAN;
        }
        (*out) << std::endl;

        traceReceived = false;
    }

    void writeModes ()
    {
        std::ofstream mo (columnFileName.c_str ());
        mo << "N2A.schema=2\n";
        for (auto it : columnMap)
        {
            int i = it.second;
            mo << i << ":" << it.first << "\n";
            auto mode = columnMode[i];
            for (auto nv : *mode) mo << " " << nv.first << ":" << nv.second << "\n";
        }
        // mo should automatically flush and close here
    }
};


#endif
