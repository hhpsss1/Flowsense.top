#pragma once
#include <memory>

class c_utl_memory_pool
{
public:
	int count( ) const
	{
		return blocks_allowed;
	}

	int peak_count( ) const
	{
		return peak_alloc;
	}

	int block_size( ) const
	{
		return blocks_size;
	}
protected:
	class c_blob
	{
	public:
		c_blob* prev;
		c_blob* next;
		int num_bytes;
		char data[1];
		char padding[3];
	};

	int blocks_size;
	int blocks_per_blob;
	int glow_mode;
	int blocks_allowed;
	int peak_alloc;

	unsigned short alignment;
	unsigned short num_blobs;

	void* head_of_free_list;
	void* alloc_owner;
	c_blob blob_head;
};

using utl_ts_hash_handle = uintptr_t;

inline unsigned hash_int_conventional( const int n )
{
	unsigned hash = 0xAAAAAAAA + (n & 0xFF);
	hash = (hash << 5) + hash + ((n >> 8) & 0xFF);
	hash = (hash << 5) + hash + ((n >> 16) & 0xFF);
	hash = (hash << 5) + hash + ((n >> 24) & 0xFF);

	return hash;
}

template<int bucket_count, class tKey = uintptr_t>
class c_utl_ts_hash_generic
{
public:
	static int hash( const tKey& Key, int bucket_mask )
	{
		int hash = hash_int_conventional( uintptr_t( Key ) );
		if ( bucket_count <= UINT16_MAX )
		{
			hash ^= (hash >> 16);
		}

		if ( bucket_count <= UINT8_MAX )
		{
			hash ^= (hash >> 8);
		}

		return (hash & bucket_mask);
	}

	static bool compare( const tKey& lhs, const tKey& rhs )
	{
		return lhs == rhs;
	}
};

template<class element, int bucket_count, class key = uintptr_t, class hash_funcs = c_utl_ts_hash_generic<bucket_count, key>, int alignment = 0>
class c_utl_ts_hash
{
	static constexpr int bucket_mask = bucket_count - 1;
public:
	static constexpr utl_ts_hash_handle invalid_handle( )
	{
		return static_cast<utl_ts_hash_handle>(0);
	}

	utl_ts_hash_handle find( key ui_key )
	{
		int bucket = hash_funcs::hash( ui_key, bucket_count );
		const hash_bucket_t& hashBucket = buckets[bucket];
		const utl_ts_hash_handle hash_out = find( ui_key, hashBucket.first, nullptr );
		return hash_out ? hash_out : find( ui_key, hashBucket.first_uncommited, hashBucket.first );
	}

	int count( ) const
	{
		return entry_memory.count( );
	}

	int get_elements( int first_element, int count, utl_ts_hash_handle* handles ) const
	{
		int index = 0;
		for ( int bucket_index = 0; bucket_index < bucket_count; bucket_index++ )
		{
			const hash_bucket_t& bucket = buckets[bucket_index];

			hash_fixed_data* element = bucket.first_uncommited;

			for ( ; element; element = element->get_next( ) )
			{
				if ( --first_element >= 0 )
					continue;

				handles[index++] = reinterpret_cast<utl_ts_hash_handle>(element);

				if ( index >= count )
					return index;
			}
		}

		return index;
	}

	element get_element( utl_ts_hash_handle hash )
	{
		return ((hash_fixed_data*)(hash))->get_data( );
	}

	const element& get_element( utl_ts_hash_handle hash ) const
	{
		return reinterpret_cast<hash_fixed_data*>(hash)->game_data;
	}

	element& operator[]( utl_ts_hash_handle hash )
	{
		return reinterpret_cast<hash_fixed_data*>(hash)->game_data;
	}

	const element& operator[]( utl_ts_hash_handle hash ) const
	{
		return reinterpret_cast<hash_fixed_data*>(hash)->game_data;
	}

	key get_id( utl_ts_hash_handle hHash ) const
	{
		return reinterpret_cast<hash_fixed_data*>(hHash)->ui_key;
	}

private:
	template<typename data>
	struct hash_fixed_data_internal
	{
		key ui_key;
		hash_fixed_data_internal<data>* next;
		data game_data;

		data get_data( )
		{
			return *(data*)(uintptr_t( this ) + 0x10);
		}

		hash_fixed_data_internal<data>* get_next( )
		{
			return *(hash_fixed_data_internal<data>**)(uintptr_t( this ) + 0x8);
		}
	};

	using hash_fixed_data = hash_fixed_data_internal<element>;

	struct hash_bucket_t
	{
	private:
		[[ maybe_unused ]]
		char m_pad0[0x18];
	public:
		hash_fixed_data* first;
		hash_fixed_data* first_uncommited;
	};

	utl_ts_hash_handle find( key uiKey, hash_fixed_data* first, hash_fixed_data* last )
	{
		for ( hash_fixed_data* element = first; element != last; element = element->next )
		{
			if ( hash_funcs::compare( element->ui_key, uiKey ) )
				return reinterpret_cast<utl_ts_hash_handle>(element);
		}

		return invalid_handle( );
	}

	c_utl_memory_pool entry_memory;
	char pad[0x40];
	hash_bucket_t buckets[bucket_count];
	bool needs_commit;
};