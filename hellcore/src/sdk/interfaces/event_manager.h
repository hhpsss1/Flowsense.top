#pragma once

#include "game_event.h"

#include <deque>

class c_game_event_listener2
{
public:
	virtual ~c_game_event_listener2( ) {}
	virtual void fire_game_event( c_game_event* event ) = 0;
	virtual int get_event_debug_id( )
	{
		return m_debug_id;
	}
private:
	int m_debug_id;
};

class c_event_listener : public c_game_event_listener2
{
public:
	void setup( const std::deque<const char*>& events );

	virtual void fire_game_event( c_game_event* event_ptr ) override;
	virtual int get_event_debug_id( ) override
	{
		return 42;
	}
};

inline auto g_event_listener = std::make_unique<c_event_listener>( );

class c_game_event_manager2
{
public:
	virtual ~c_game_event_manager2( ) {}

	int load_events_from_file( const char* file_name )
	{
		return g_memory->call_virtual<int>( this, 1, file_name );
	}

	void reset( )
	{
		g_memory->call_virtual<void>( this, 2 );
	}

	bool add_listener( c_game_event_listener2* listener, const char* name, bool server_side = false )
	{
		return g_memory->call_virtual<bool>( this, 3, listener, name, server_side );
	}

	bool find_listener( c_game_event_listener2* listener, const char* listener_name )
	{
		return g_memory->call_virtual<bool>( this, 4, listener, listener_name );
	}

	void remove_listener( c_game_event_listener2* listener )
	{
		g_memory->call_virtual<void>( this, 5, listener );
	}
};