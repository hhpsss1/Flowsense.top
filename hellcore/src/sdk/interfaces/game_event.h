#pragma once

#include <utils/memory.h>

#include <sdk/entity/pawn.h>
#include <sdk/entity/controller.h>
#include <sdk/datatypes/murmur_hash.h>

struct event_hash_t
{
	event_hash_t( const char* string )
	{
		m_name = string;
		m_hash = MurmurHash2( string, static_cast< int >( std::strlen( string ) ), 0x31415926 );
	}

	uint32_t m_hash;
	const char* m_name;
};

class c_game_event
{
public:
	virtual ~c_game_event( ) { };

	const char* get_name( )
	{
		return g_memory->call_virtual<const char*>( this, 1 );
	}

	bool get_bool( const char* event_name, bool def = false )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<bool>( this, 6, hash, def );
	}

	int get_int( const char* event_name, int def = 0 )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<int>( this, 7, hash, def );
	}

	uint64_t get_uint64( const char* event_name, uint64_t def = 0 )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<uint64_t>( this, 8, hash, def );
	}

	float get_float( const char* event_name, float def = 0.0f )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<float>( this, 9, hash, def );
	}

	const char* get_string( const char* event_name, const char* def = "" )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<const char*>( this, 10, hash, def );
	}

	const void* get_pointer( const char* event_name )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<const void*>( this, 11, hash );
	}

	int get_player_index( const char* event_name )
	{
		const event_hash_t hash( event_name );
		int index{};
		g_memory->call_virtual<void>( this, 14, hash, &index );
		return index;
	}

	c_cs_player_controller* get_player_controller( const char* event_name )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<c_cs_player_controller*>( this, 16, hash );
	}

	c_cs_player_pawn* get_player_pawn( const char* event_name )
	{
		const event_hash_t hash( event_name );
		return g_memory->call_virtual<c_cs_player_pawn*>( this, 17, hash );
	}

	void set_bool( const char* event_name, bool value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 20, hash, value );
	}

	void set_int( const char* event_name, int value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 21, hash, value );
	}
	
	void set_uint64( const char* event_name, uint64_t value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 22, hash, value );
	}

	void set_float( const char* event_name, float value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 23, hash, value );
	}

	void set_string( const char* event_name, const char* value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 24, hash, value );
	}

	void set_ptr( const char* event_name, const void* value )
	{
		const event_hash_t hash( event_name );
		g_memory->call_virtual<void>( this, 25, hash, value );
	}
private:
	PAD( 0x60 );
};
static_assert( sizeof( c_game_event ) == 0x68 );