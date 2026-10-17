#pragma once

#include <includes.h>
#include <sdk/interfaces/game_event.h>

class c_visual_events {
public:
	void on_fire_game_event( c_game_event* event_ptr );
	void create_delayed_tracer();
	vec3_t m_last_impact_pos = vec3_t(0, 0, 0);
	vec3_t m_last_shoot_pos = vec3_t(0, 0, 0);
	float m_last_impact_time = 0.f;
};
inline auto g_visual_events = std::make_unique<c_visual_events>( );