
#ifndef _H_VANADIS_CACHE
#define _H_VANADIS_CACHE

#include <list>
#include <unordered_map>
#include <cstdint>
#include <type_traits>

namespace SST {
namespace Vanadis {

template< typename I, typename T >
class VanadisCache {
public:
	VanadisCache( const size_t cache_entries ) {
		reset( cache_entries );
	}

	~VanadisCache() {
		ordering_q.clear();
		data_values.clear();
	}

	void clear() {
		ordering_q.clear();
		data_values.clear();
	}

	void reset( const size_t cache_entries ) {
		clear();

		max_entries = cache_entries;
                data_values.reserve( cache_entries );
	}

	bool contains( const I& value ) const {
		return (data_values.find( value ) != data_values.end());
	}

	T find( const I& key ) {
		send_key_to_front( key );
		return data_values.find( key )->second;
	}

	void store( const I& key, T value ) {
		if( contains( key ) ) {
			send_key_to_front( key );
		} else {
			kill_lru_key();
			data_values.insert( std::pair<I, T>( key, value ) );
			ordering_q.push_front( key );
		}
	}

	void touch( const I& key ) {
		if( contains(key) ) {
			send_key_to_front( key );
		}
	}

	size_t size() {
		return data_values.size();
	}

	size_t capacity() {
		return max_entries;
	}

private:
	void kill_lru_key() {
		// if we aren't full yet, then keep entries otherwise we will
		// throw away
		if( ordering_q.size() < max_entries ) {
			return;
		}

		I remove_key = ordering_q.back();
		ordering_q.pop_back();

		auto find_key = data_values.find( remove_key );
		delete find_key->second;
		data_values.erase( find_key );
	}

	void send_key_to_front( const I& key ) {
		bool found_key = false;

                for( auto order_itr = ordering_q.begin(); order_itr != ordering_q.end(); ) {
                        if( key == (*order_itr) ) {
                                ordering_q.erase( order_itr );
                                found_key = true;
				break;
                        } else {
                                order_itr++;
                        }
                }

                if( found_key ) {
                        ordering_q.push_front( key );
                }
	}

	size_t max_entries;
	std::list< I > ordering_q;
	std::unordered_map<I, T> data_values;

};

}
}

#endif
