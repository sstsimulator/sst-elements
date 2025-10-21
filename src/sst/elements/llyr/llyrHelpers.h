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


#ifndef _LLYR_HELPERS
#define _LLYR_HELPERS

#include <ostream>
#include <iterator>
#include <vector>
#include <string>
#include <list>

#include "llyrTypes.h"
#include "mappers/csvParser.h"

namespace SST {
namespace Llyr {

inline opType const getOptype(std::string &opString)
{
    opType operation;

    // transform to make opString case insensitive
    std::transform(opString.begin(), opString.end(), opString.begin(),
                   [](unsigned char c){ return std::toupper(c); }
    );

    if( opString == "ROUTE" )
        operation = ROUTE;
    else if( opString == "ANY" )
        operation = ANY;
    else if( opString == "ANY_MEM" )
        operation = ANY_MEM;
    else if( opString == "LD" )
        operation = LD;
    else if( opString == "LDADDR" )
        operation = LDADDR;
    else if( opString == "STREAM_LD" )
        operation = STREAM_LD;
    else if( opString == "ST" )
        operation = ST;
    else if( opString == "STADDR" )
        operation = STADDR;
    else if( opString == "STREAM_ST" )
        operation = STREAM_ST;
    else if( opString == "ALLOCA" )
        operation = ALLOCA;
    else if( opString == "ANY_LOGIC" )
        operation = ANY_LOGIC;
    else if( opString == "AND" )
        operation = AND;
    else if( opString == "OR" )
        operation = OR;
    else if( opString == "XOR" )
        operation = XOR;
    else if( opString == "NOT" )
        operation = NOT;
    else if( opString == "SLL" )
        operation = SLL;
    else if( opString == "SLR" )
        operation = SLR;
    else if( opString == "ROL" )
        operation = ROL;
    else if( opString == "ROR" )
        operation = ROR;
    else if( opString == "EQ" )
        operation = EQ;
    else if( opString == "EQ_IMM" )
        operation = EQ_IMM;
    else if( opString == "NE" )
        operation = NE;
    else if( opString == "UGT" )
        operation = UGT;
    else if( opString == "UGT_IMM" )
        operation = UGT_IMM;
    else if( opString == "UGE" )
        operation = UGE;
    else if( opString == "UGE_IMM" )
        operation = UGE_IMM;
    else if( opString == "SGT" )
        operation = SGT;
    else if( opString == "SGT_IMM" )
        operation = SGT_IMM;
    else if( opString == "SGE" )
        operation = SGE;
    else if( opString == "ULT" )
        operation = ULT;
    else if( opString == "ULE" )
        operation = ULE;
    else if( opString == "ULE_IMM" )
        operation = ULE_IMM;
    else if( opString == "SLT" )
        operation = SLT;
    else if( opString == "SLT_IMM" )
        operation = SLT_IMM;
    else if( opString == "SLE" )
        operation = SLE;
    else if( opString == "AND_IMM" )
        operation = AND_IMM;
    else if( opString == "OR_IMM" )
        operation = OR_IMM;
    else if( opString == "ANY_INT" )
        operation = ANY_INT;
    else if( opString == "ADD" )
        operation = ADD;
    else if( opString == "SUB" )
        operation = SUB;
    else if( opString == "MUL" )
        operation = MUL;
    else if( opString == "DIV" )
        operation = DIV;
    else if( opString == "REM" )
        operation = REM;
    else if( opString == "ADDCONST" )
        operation = ADDCONST;
    else if( opString == "SUBCONST" )
        operation = SUBCONST;
    else if( opString == "MULCONST" )
        operation = MULCONST;
    else if( opString == "DIVCONST" )
        operation = DIVCONST;
    else if( opString == "REMCONST" )
        operation = REMCONST;
    else if( opString == "INC" )
        operation = INC;
    else if( opString == "INC_RST" )
        operation = INC_RST;
    else if( opString == "ACC" )
        operation = ACC;
    else if( opString == "ANY_FP" )
        operation = ANY_FP;
    else if( opString == "FADD" )
        operation = FADD;
    else if( opString == "FSUB" )
        operation = FSUB;
    else if( opString == "FMUL" )
        operation = FMUL;
    else if( opString == "FDIV" )
        operation = FDIV;
    else if( opString == "FMatMul" )
        operation = FMatMul;
    else if( opString == "ANY_CP" )
        operation = ANY_CP;
    else if( opString == "TSIN" )
        operation = TSIN;
    else if( opString == "TCOS" )
        operation = TCOS;
    else if( opString == "TTAN" )
        operation = TTAN;
    else if( opString == "DUMMY" )
        operation = DUMMY;
    else if( opString == "BUFFER" )
        operation = BUFFER;
    else if( opString == "REPEATER" )
        operation = REPEATER;
    else if( opString == "ROS" )
        operation = ROS;
    else if( opString == "RNE" )
        operation = RNE;
    else if( opString == "ROZ" )
        operation = ROZ;
    else if( opString == "ROO" )
        operation = ROO;
    else if( opString == "ONEONAND" )
        operation = ONEONAND;
    else if( opString == "GATED_ONE" )
        operation = GATED_ONE;
    else if( opString == "MERGE" )
        operation = MERGE;
    else if( opString == "FILTER" )
        operation = FILTER;
    else if( opString == "SEL" )
        operation = SEL;
    else if( opString == "RET" )
        operation = RET;
    else
        operation = OTHER;

    return operation;
}

inline std::string const getOpString(const opType &op)
{
    std::string operation;

    if( op == ROUTE )
        operation = "ROUTE";
    else if( op == ANY )
        operation = "ANY";
    else if( op == ANY_MEM )
        operation = "ANY_MEM";
    else if( op == LD )
        operation = "LD";
    else if( op == LDADDR )
        operation = "LDADDR";
    else if( op == STREAM_LD )
        operation = "STREAM_LD";
    else if( op == ST )
        operation = "ST";
    else if( op == STADDR )
        operation = "STADDR";
    else if( op == STREAM_ST )
        operation = "STREAM_ST";
    else if( op == ALLOCA )
        operation = "ALLOCA";
    else if( op == ANY_LOGIC )
        operation = "ANY_LOGIC";
    else if( op == AND )
        operation = "AND";
    else if( op == OR )
        operation = "OR";
    else if( op == XOR )
        operation = "XOR";
    else if( op == NOT )
        operation = "NOT";
    else if( op == SLL )
        operation = "SLL";
    else if( op == SLR )
        operation = "SLR";
    else if( op == ROL )
        operation = "ROL";
    else if( op == ROR )
        operation = "ROR";
    else if( op == EQ )
        operation = "EQ";
    else if( op == EQ_IMM )
        operation = "EQ_IMM";
    else if( op == NE )
        operation = "NE";
    else if( op == UGT )
        operation = "UGT";
    else if( op == UGT_IMM )
        operation = "UGT_IMM";
    else if( op == UGE )
        operation = "UGE";
    else if( op == UGE_IMM )
        operation = "UGE_IMM";
    else if( op == SGT )
        operation = "SGT";
    else if( op == SGT_IMM )
        operation = "SGT_IMM";
    else if( op == SGE )
        operation = "SGE";
    else if( op == ULT )
        operation = "ULT";
    else if( op == ULE )
        operation = "ULE";
    else if( op == ULE_IMM )
        operation = "ULE_IMM";
    else if( op == SLT )
        operation = "SLT";
    else if( op == SLT_IMM )
        operation = "SLT_IMM";
    else if( op == SLE )
        operation = "SLE";
    else if( op == AND_IMM )
        operation = "AND_IMM";
    else if( op == OR_IMM )
        operation = "OR_IMM";
    else if( op == ANY_INT )
        operation = "ANY_INT";
    else if( op == ADD )
        operation = "ADD";
    else if( op == SUB )
        operation = "SUB";
    else if( op == MUL )
        operation = "MUL";
    else if( op == DIV )
        operation = "DIV";
    else if( op == REM )
        operation = "REM";
    else if( op == ADDCONST )
        operation = "ADDCONST";
    else if( op == SUBCONST )
        operation = "SUBCONST";
    else if( op == MULCONST )
        operation = "MULCONST";
    else if( op == DIVCONST )
        operation = "DIVCONST";
    else if( op == REMCONST )
        operation = "REMCONST";
    else if( op == INC )
        operation = "INC";
    else if( op == INC_RST )
        operation = "INC_RST";
    else if( op == ACC )
        operation = "ACC";
    else if( op == ANY_FP )
        operation = "ANY_FP";
    else if( op == FADD )
        operation = "FADD";
    else if( op == FSUB )
        operation = "FSUB";
    else if( op == FMUL )
        operation = "FMUL";
    else if( op == FDIV )
        operation = "FDIV";
    else if( op == FMatMul )
        operation = "FMatMul";
    else if( op == ANY_CP )
        operation = "ANY_CP";
    else if( op == TSIN )
        operation = "TSIN";
    else if( op == TCOS )
        operation = "TCOS";
    else if( op == TTAN )
        operation = "TTAN";
    else if( op == DUMMY )
        operation = "DUMMY";
    else if( op == BUFFER )
        operation = "BUFFER";
    else if( op == ROS )
        operation = "ROS";
    else if( op == RNE )
        operation = "RNE";
    else if( op == ROZ )
        operation = "ROZ";
    else if( op == ROO )
        operation = "ROO";
    else if( op == ONEONAND )
        operation = "ONEONAND";
    else if( op == GATED_ONE )
        operation = "GATED_ONE";
    else if( op == MERGE )
        operation = "MERGE";
    else if( op == FILTER )
        operation = "FILTER";
    else if( op == REPEATER )
        operation = "REPEATER";
    else if( op == SEL )
        operation = "SEL";
    else if( op == RET )
        operation = "RET";
    else
        operation = "OTHER";

    return operation;
}

inline void printHardwareNode(HardwareNode* hardwareNode, std::ostream& os)
{
    os << "**** PE " << hardwareNode->pe_id_ << " ****" << std::endl;
    os << "JOB:    " << hardwareNode->job_id_ << std::endl;
    os << "consts: " << std::endl;
    slist_t::const_iterator cit = hardwareNode->const_list_->begin();
    for( ; cit!=hardwareNode->const_list_->end(); cit++ ) {
        os << *cit << std::endl;
    }

    os << "inputs: " << std::endl;
    std::list< PairPE >::const_iterator pit = hardwareNode->input_list_->begin();
    for( ; pit!=hardwareNode->input_list_->end(); pit++ ) {
        pair_t p = *pit;
        os << p.first << ',' << p.second << std::endl;
    }

    os << "OP:     " << hardwareNode->op_ << std::endl;
    os << "outputs: " << std::endl;
    pit = hardwareNode->output_list_->begin();
    for( ; pit!=hardwareNode->output_list_->end(); pit++ ) {
        pair_t p = *pit;
        os << p.first << ',' << p.second << std::endl;
    }

    os << "routes: " << std::endl;
    std::list< TriplePE >::const_iterator tlit = hardwareNode->route_list_->begin();
    for( ; tlit!=hardwareNode->route_list_->end(); tlit++ ) {
        triple_t t = *tlit;
        os << std::get<0>(t) << "," << std::get<1>(t) << "," << std::get<2>(t) << std::endl;
    }

    os << std::endl;
}

inline std::list< std::string >* process_single_level(std::string str, char delim)
{
    CSVParser csvData(str, delim);
    const auto& data = csvData.get_data();
    std::list< std::string >* result = new std::list< std::string >;

    for( const auto& row : data ) {
        for( const auto& cell : row ) {
            if( std::string(cell) != "" ) {
                result->push_back(std::string(cell));
            }
        }
    }

    return result;
}

inline PairPE process_pair(std::string str, char delim)
{
    CSVParser csvData(str, delim);
    const auto& data = csvData.get_data();
    assert(data[0].size() == 2);
    std::string field1 = std::string(data[0][0]);
    std::string field2 = std::string(data[0][1]);

    return PairPE(field1, std::stoi(field2));
}

inline TriplePE process_triple(std::string str, char delim)
{
    CSVParser csvData(str, delim);
    const auto& data = csvData.get_data();
    assert(data[0].size() == 3);
    std::string field1 = std::string(data[0][0]);
    std::string field2 = std::string(data[0][1]);
    std::string field3 = std::string(data[0][2]);

    return TriplePE(field1, std::stoi(field2), std::stoi(field3));
}

inline PairEdge* process_edge_row(const std::vector< std::string >& row)
{
    const std::string from_pe = std::string(row[1]);
    const std::string to_pe   = std::string(row[2]);
    std::cout << "EDGE: " << from_pe << " " << to_pe << std::endl;

    return new PairEdge(from_pe, to_pe);
}

inline HardwareNode* process_node_row(const std::vector< std::string >& row)
{
    HardwareNode* hardwareNode = new HardwareNode;

    // TODO: figure out lists of lists
    hardwareNode->pe_id_ = std::string(row[1]);
    hardwareNode->job_id_ = std::string(row[2]);
    hardwareNode->op_ = std::string(row[5]);
    hardwareNode->const_list_ = process_single_level(std::string(row[3]), ';');

    // parse inputs
    std::list< PairPE >* input_pairs = new std::list< PairPE >;
    std::list< std::string >* input_str = process_single_level(std::string(row[4]), ';');
    std::list< std::string >::const_iterator sit = input_str->begin();
    for( ; sit!=input_str->end(); sit++ ) {
        std::string s = *sit;
        if( s.length() > 1 ) {
            pair_t p = process_pair(s, ',');
            input_pairs->push_back(p);
        }
    }
    hardwareNode->input_list_ = input_pairs;

    // parse outputs
    std::list< PairPE >* output_pairs = new std::list< PairPE >;
    std::list< std::string >* output_str   = process_single_level(std::string(row[6]), ';');
    sit = output_str->begin();
    for( ; sit!=output_str->end(); sit++ ) {
        std::string s = *sit;
        if( s.length() > 1 ) {
            pair_t p = process_pair(s, ',');
            output_pairs->push_back(p);
        }
    }
    hardwareNode->output_list_ = output_pairs;

    // parse routes -- Note that the way the csv parsing works, there may not be an 8th entry
    std::list< TriplePE >* routes_triples = new std::list< TriplePE >;
    if( row.size() >= 8 ) {
        std::list< std::string >* routes_str   = process_single_level(std::string(row[7]), ';');
        sit = routes_str->begin();
        for( ; sit!=routes_str->end(); sit++ ) {
            std::string s = *sit;
            if( s.length() > 1 ) {
                TriplePE t = process_triple(s, ',');
                routes_triples->push_back(t);
            }
        }
    }
    hardwareNode->route_list_ = routes_triples;

    return hardwareNode;
}

}//Llyr
}//SST

#endif // _LLYR_HELPERS
