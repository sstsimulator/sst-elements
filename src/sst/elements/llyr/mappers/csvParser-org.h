// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CSV_PARSER_H
#define _CSV_PARSER_H

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <tuple>

#include "../llyrTypes.h"

typedef std::pair<std::string, size_t> pair_t;
typedef std::tuple<std::string, size_t, size_t> triple_t;

typedef std::list<std::string> slist_t;
typedef std::list<pair_t>   plist_t;
typedef std::list<triple_t> tlist_t;

namespace SST {
namespace Llyr {

// Based on https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
class CSVRow
{
public:
    CSVRow(char delim) : delimiter(delim) {}

    std::string_view operator[](std::size_t index) const
    {
        return std::string_view(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
    }
    std::size_t size() const
    {
        return m_data.size() - 1;
    }
    void process_line()
    {
        m_data.clear();
        m_data.push_back(-1);
        std::string::size_type pos = 0;
        while((pos = m_line.find(delimiter, pos)) != std::string::npos)
        {
            m_data.push_back(pos);
            ++pos;
        }
        // This checks for a trailing comma with no data after it.
        pos   = m_line.size();
        m_data.push_back(pos);
    }
    void readNextRow(std::istream& str)
    {
        std::getline(str, m_line);
        process_line();
    }
    void readNextRow(std::stringstream& str)
    {
        std::getline(str, m_line);
        process_line();
    }
private:
    std::string         m_line;
    std::vector<int>    m_data;
    char                delimiter;
};

void printHardwareNode(HardwareNode* hardwareNode, std::ostream& os)
{
    os << "**** PE " << hardwareNode->pe_id_ << " ****" << std::endl;
//     os << "PE:     " << hardwareNode->pe_id_  << std::endl;
    os << "JOB:    " << hardwareNode->job_id_ << std::endl;
    os << "consts: " << std::endl;
    slist_t::const_iterator cit = hardwareNode->const_list_->begin();
    for (; cit!=hardwareNode->const_list_->end(); cit++) {
        os << *cit << std::endl;
    }
    os << "inputs: " << std::endl;
    std::list< PairPE >::const_iterator pit = hardwareNode->input_list_->begin();
    for (; pit!=hardwareNode->input_list_->end(); pit++) {
        pair_t p = *pit;
        os << p.first << ',' << p.second << std::endl;
    }
    os << "OP:     " << hardwareNode->op_ << std::endl;
    os << "outputs: " << std::endl;
    pit = hardwareNode->output_list_->begin();
    for (; pit!=hardwareNode->output_list_->end(); pit++) {
        pair_t p = *pit;
        os << p.first << ',' << p.second << std::endl;
    }
    os << "routes: " << std::endl;
    std::list< TriplePE >::const_iterator tlit = hardwareNode->route_list_->begin();
    for (; tlit!=hardwareNode->route_list_->end(); tlit++) {
        triple_t t = *tlit;
        os << std::get<0>(t) << "," << std::get<1>(t) << "," << std::get<2>(t) << std::endl;
    }

    os << std::endl;
}

std::istream& operator>>(std::ifstream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

std::stringstream& operator>>(std::stringstream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

std::list< std::string >* process_single_level(std::string str, char delim)
{
    CSVRow row(delim);
    std::stringstream ss(str);
    ss >> row;
    std::list< std::string >* result = new std::list< std::string >;
    for (size_t i = 0; i < row.size(); i++) {
        if( std::string(row[i]) != "" ) {
            result->push_back(std::string(row[i]));
        }
    }
    return result;
}

PairPE process_pair(std::string str, char delim)
{
    CSVRow row(delim);
    std::stringstream ss(str);
    ss >> row;
    assert(row.size() == 2);
    std::string field1 = std::string(row[0]);
    std::string field2 = std::string(row[1]);
    return PairPE(field1, std::stoi(field2));
}

TriplePE process_triple(std::string str, char delim)
{
    CSVRow row(delim);
    std::stringstream ss(str);
    ss >> row;
    assert(row.size() == 3);
    std::string field1 = std::string(row[0]);
    std::string field2 = std::string(row[1]);
    std::string field3 = std::string(row[2]);
    return TriplePE(field1, std::stoi(field2), std::stoi(field3));
}

PairEdge* process_edge_row(CSVRow& row)
{
    std::string from_pe = std::string(row[1]);
    std::string to_pe   = std::string(row[2]);
    std::cout << "EDGE: " << from_pe << " " << to_pe << std::endl;

    return new PairEdge(from_pe, to_pe);
}

HardwareNode* process_node_row(CSVRow& row)
{
    HardwareNode* hardwareNode = new HardwareNode;

    // TODO: figure out lists of lists
    hardwareNode->pe_id_ = std::string(row[1]);
    hardwareNode->job_id_ = std::string(row[2]);
    hardwareNode->op_ = std::string(row[5]);
    hardwareNode->const_list_  = process_single_level(std::string(row[3]), ';');

    // parse inputs
    std::list< std::string >* input_str = process_single_level(std::string(row[4]), ';');
    std::list< PairPE >* input_pairs = new std::list< PairPE >;
    std::list< std::string >::const_iterator sit = input_str->begin();
    for (; sit!=input_str->end(); sit++) {
        std::string s = *sit;
        if (s.length() > 1) {
            pair_t p = process_pair(s, ',');
            input_pairs->push_back(p);
        }
    }
    hardwareNode->input_list_ = input_pairs;

    // parse outputs
    std::list< std::string >* output_str   = process_single_level(std::string(row[6]), ';');
    std::list< PairPE >* output_pairs = new std::list< PairPE >;
    sit = output_str->begin();
    for (; sit!=output_str->end(); sit++) {
        std::string s = *sit;
        if (s.length() > 1) {
            pair_t p = process_pair(s, ',');
            output_pairs->push_back(p);
        }
    }
    hardwareNode->output_list_ = output_pairs;

    // parse routes
    std::list< std::string >* routes_str   = process_single_level(std::string(row[7]), ';');
    std::list< TriplePE >* routes_triples = new std::list< TriplePE >;
    sit = routes_str->begin();
    for (; sit!=routes_str->end(); sit++) {
        std::string s = *sit;
        if (s.length() > 1) {
            TriplePE t = process_triple(s, ',');
            routes_triples->push_back(t);
        }
    }
    hardwareNode->route_list_ = routes_triples;

    return hardwareNode;
}

}// namespace Llyr
}// namespace SST

#endif // _CSV_PARSER_H
