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


#ifndef _H_EMBER_SHMEM_REDUCTION
#define _H_EMBER_SHMEM_REDUCTION

#include <type_traits>
#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template<class TYPE>
typename std::enable_if<std::is_integral<TYPE>::value,void>::type
checkType( ) { }

template<class TYPE>
typename std::enable_if<std::is_floating_point<TYPE>::value,void>::type
checkType( ) { assert(0); }

template <class TYPE>
class EmberShmemReductionGenerator : public EmberShmemGenerator {


    enum Op { AND, OR, XOR, SUM, PROD, MAX, MIN } m_op;
	enum Type { Int, Long, LongLong, Float, Double } m_type;
public:
	EmberShmemReductionGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemReduction" ), m_phase(0) 
	{ 
        m_printResults = params.find<bool>("arg.printResults", false );
        m_nelems = params.find<int>("arg.nelems", 1 );
        m_opName = params.find<std::string>("arg.op", "SUM");
		m_nelems = params.find<int>("arg.nelems", 1 );	

        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free(tmp); 

		if ( 0 == m_type_name.compare( "int" ) ) {
			m_type = Int;
		} else if ( 0 == m_type_name.compare( "long" ) ) {
			m_type = Long;
		} else if ( 0 == m_type_name.compare( "long long" ) ) {
			m_type = LongLong;
		} else if ( 0 == m_type_name.compare( "float" ) ) {
			m_type = Float;
		} else if ( 0 ==  m_type_name.compare( "double" ) ) {
			m_type = Double;
		} else {
			assert(0);
		}

        if ( 0 == m_opName.compare("SUM") ) {
            m_op = SUM;
        } else if ( 0 == m_opName.compare("PROD") ) {
            m_op = PROD;
        } else if ( 0 == m_opName.compare("MAX") ) {
            m_op = MAX;
        } else if ( 0 == m_opName.compare("MIN") ) {
            m_op = MIN;
        }else if ( 0 == m_opName.compare("AND") ) {
			checkType<TYPE>();
            m_op = AND;
        } else if ( 0 == m_opName.compare("OR") ) {
			checkType<TYPE>();
            m_op = OR;
        } else if ( 0 == m_opName.compare("XOR") ) {
			checkType<TYPE>();
            m_op = XOR;
        } else {
            assert(0);
        }
    }

	TYPE calcSrc( Op op, int pe, int v ) {
		int shift = (sizeof(TYPE) * 8 )/ 2;
		TYPE value;
		switch( op ) {
		case AND:
			if ( m_num_pes / 2  == pe ) {
				value = 1L << (pe + shift) | 1 << v; 
			} else {
				value = -1;
			} 
			break;

		case OR:
			if ( m_num_pes / 2  == pe ) {
				value = 1L << (pe + shift) | 1 << v; 
			} else {
				value = 0;
			} 
			break;

		case XOR:
			value = ( 1L << ( pe + shift) ) | ( 1 << v ); 
			
			break;

		case MAX:
		case MIN:
		case SUM:
		case PROD:
			value = pe + 1 + v + 1; 
			break;
		}	
		return value;
	}

	template<class T>
	typename std::enable_if<std::is_floating_point<T>::value,T>::type
	calcResultLOGICAL( Op op, int num_pes, int v ) {
		assert(0);
	}

	template<class T>
	typename std::enable_if<std::is_integral<T>::value,T>::type
	calcResultLOGICAL( Op op, int num_pes, int v ) {
		int shift = (sizeof(TYPE) * 8 )/ 2;
		T result;
		switch ( op ) {
		case AND:
		case OR:
			result = 1L << ( ( m_num_pes / 2 ) + shift ) | 1 << v; 
			break;

		case XOR:
			result = 1L << shift | 1 << v; 

			for ( int i = 1; i< num_pes; i++ ) {
				TYPE tmp = 1L << (  i  + shift ) | 1 << v; 
				result ^= tmp; 
			}
			break;
        default: 
            assert(0);
		}
		return result;
	}

	template<class T>
	TYPE calcResult( Op op, int num_pes, int v ) {
		TYPE result = 0;
		switch( op ) {

		case MAX: 
			result = num_pes - 1 + 1 + v  + 1;
			break;

		case MIN: 
			result = 0 + 1 + v  + 1;
			break;

		case SUM:
			for ( int i = 0; i< num_pes; i++ ) {
				result += i + 1 +  v + 1;
			}
			break;

		case PROD:
			result = 1;
			for ( int i = 0; i< num_pes; i++ ) {
				result *=  i + 1 + v + 1;
			}
			break;

		case AND:
		case OR:
		case XOR:
			result = calcResultLOGICAL<T>( op, num_pes, v ); 
			break;
		}	
		return result; 
	}

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
          case 0: 
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            break;

          case 1:
            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d nelems=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str());
            }
            {
                size_t buffer_size = 3 * sizeof(long);    // for pSync
                buffer_size += m_nelems * sizeof(TYPE);   // for source
                buffer_size += m_nelems * sizeof(TYPE);   // for dest
                enQ_malloc( evQ, &m_pSync, buffer_size );
            }
			break;

		  case 2:
            bzero( &m_pSync.at<long>(0), sizeof(long) * 3);

			m_src = m_pSync.offset<long>(3);
			for ( int i = 0; i < m_nelems; i++ ) { 
            	m_src.at<TYPE>(i) = calcSrc( m_op, m_my_pe, i ); 
			}

            m_dest = m_src.offset<TYPE>(m_nelems );
            m_dest.at<TYPE>(0) = 0;
            enQ_barrier_all( evQ );

            break;

          case 3:
            switch ( m_op ) { 
            case AND:
				switch ( m_type ) {
				case Int:
                	enQ_int_and_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_and_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default: 
                    assert(0);
				}
                break;
            case OR:
				switch ( m_type ) {
				case Int:
                	enQ_int_or_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_or_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default: 
                    assert(0);
				}
                break;
            case XOR:
				switch ( m_type ) {
				case Int:
                	enQ_int_xor_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_xor_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default:
                    assert(0);
				}
                break;
            case SUM:
				switch ( m_type ) {
				case Int:
                	enQ_int_sum_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_sum_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Float:
                	enQ_float_sum_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Double:
                	enQ_double_sum_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default:
                    assert(0);
				}
                break;
            case PROD:
				switch ( m_type ) {
				case Int:
                	enQ_int_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
                	break;
				case Long:
                	enQ_long_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
                	break;
				case LongLong:
                	enQ_longlong_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
                	break;
				case Float:
                	enQ_float_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
                	break;
				case Double:
                	enQ_double_prod_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
                	break;
                default:
                    assert(0);
				}
				break;

            case MIN:
				switch ( m_type ) {
				case Int:
                	enQ_int_min_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_min_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Float:
                	enQ_float_min_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Double:
                	enQ_double_min_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default:
                    assert(0);
				}
                break;
            case MAX:
				switch ( m_type ) {
				case Int:
                	enQ_int_max_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Long:
                	enQ_long_max_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Float:
                	enQ_float_max_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
				case Double:
                	enQ_double_max_to_all( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
					break;
                default:
                    assert(0);
				}
                break;
            }
            break;

          case 4:
           	for ( int i = 0; i < m_nelems; i++ ) { 
				TYPE result = calcResult<TYPE>( m_op, m_num_pes, i); 
            	if ( m_printResults ) {
					std::stringstream tmp;
					tmp << " got="<< m_dest.at<TYPE>(i) << " want=" <<  result;
					printf("%d:%s: %s\n",m_my_pe,getMotifName().c_str(),tmp.str().c_str());
					
				}
            	assert( m_dest.at<TYPE>(i) == result );
			}
            ret = true;
            if ( 0 == m_my_pe ) {
                printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
            }

        }
        ++m_phase;

        return ret;
	}

  private:
	bool m_printResults;
	std::string m_type_name;
    std::string m_opName;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    TYPE m_result;
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

class EmberShmemReductionIntGenerator : public EmberShmemReductionGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemReductionIntGenerator,
        "ember",
        "ShmemReductionIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM reduction int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    ) 

public:
    EmberShmemReductionIntGenerator( SST::Component* owner, Params& params ) : 
        EmberShmemReductionGenerator(owner,  params) { } 
};

class EmberShmemReductionLongGenerator : public EmberShmemReductionGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemReductionLongGenerator,
        "ember",
        "ShmemReductionLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM reduction long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    ) 

public:
    EmberShmemReductionLongGenerator( SST::Component* owner, Params& params ) : 
        EmberShmemReductionGenerator(owner,  params) { } 
};

class EmberShmemReductionLongLongGenerator : public EmberShmemReductionGenerator<long long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemReductionLongLongGenerator,
        "ember",
        "ShmemReductionLongLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM reduction long long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    ) 

public:
    EmberShmemReductionLongLongGenerator( SST::Component* owner, Params& params ) : 
        EmberShmemReductionGenerator(owner,  params) { } 
};

class EmberShmemReductionDoubleGenerator : public EmberShmemReductionGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemReductionDoubleGenerator,
        "ember",
        "ShmemReductionDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM reduction double",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    ) 

public:
    EmberShmemReductionDoubleGenerator( SST::Component* owner, Params& params ) : 
        EmberShmemReductionGenerator(owner,  params) { } 
};

class EmberShmemReductionFloatGenerator : public EmberShmemReductionGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemReductionFloatGenerator,
        "ember",
        "ShmemReductionFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM reduction float",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    ) 

public:
    EmberShmemReductionFloatGenerator( SST::Component* owner, Params& params ) : 
        EmberShmemReductionGenerator(owner,  params) { } 
};

}
}

#endif
