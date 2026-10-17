#include "event_manager.h"

#include <core/interfaces/interfaces.h>
#include <cheat/features/visuals/visual_events.h>

void c_event_listener::setup( const std::deque<const char*>& events )
{
	if ( events.empty( ) )
		return;

	for ( auto& event_name : events )
		g_interfaces->m_game_event_manager->add_listener( this, event_name );
}

void c_event_listener::fire_game_event( c_game_event* event_ptr )
{
	if ( !event_ptr )
		return;

	g_visual_events->on_fire_game_event( event_ptr );
}