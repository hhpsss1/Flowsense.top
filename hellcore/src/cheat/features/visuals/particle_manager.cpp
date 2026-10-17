#include "particle_manager.h"

#include <context.h>
#include <math/math.h>
#include <cheat/config/config_system.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/csgo_input.h>
#include <sdk/interfaces/resource_system.h>
#include <sdk/datatypes/c_buffer_string.h>

const char* get_particle_name( int type ) {
	if ( type < 0 || type >= particle_max )
		return nullptr;
		
	switch ( type ) {
	case particle_firework_1:
		return "particles/entity/env_explosion/explosion_hegrenade_h.vpcf";
	case particle_firework_2:
		return "particles/inferno_fx/explosion_incend_air_falling.vpcf";
	case particle_feathers:
		return "particles/critters/chicken/chicken_gone_feathers_fire.vpcf";
	case particle_explosion:
		return "particles/explosions_fx/explosion_basic.vpcf";
	default:
		return nullptr;
	}
}

void c_particle_mgr::create_particle_at_pos( const vec3_t& position, int type ) {
	if ( !g_interfaces->m_global_vars || !g_interfaces->m_game_particle_manager || !g_interfaces->m_particle_system_mgr )
		return;

	const char* particle_name = get_particle_name( type );
	if ( !particle_name )
		return;

	particle_info_t info {};

	if ( !g_interfaces->m_game_particle_manager->create_particle_effect( &info.m_effect_index, particle_name ) )
		return;

	particle_information_t particle_info = {};
	g_interfaces->m_game_particle_manager->set_particle_data( info.m_effect_index, e_particle_setting::particle_setting_info, &particle_info );

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data( info.m_effect_index, e_particle_setting::particle_setting_position, &effect_position );

	info.m_positions = new vec3_t[1];
	info.m_times = new float[1];

	info.m_positions[0] = position;

	info.m_data.m_positions = info.m_positions;
	info.m_data.m_times = info.m_times;

	g_interfaces->m_particle_system_mgr->create_snapshot( &info.m_snapshot_handle );
	if ( !g_interfaces->m_game_particle_manager->init_effect( info.m_effect_index, 0, &info.m_snapshot_handle ) ) {
		delete[] info.m_positions;
		delete[] info.m_times;
		return;
	}

	info.m_snapshot_handle->draw( 1, &info.m_data );

	g_interfaces->m_game_particle_manager->release_particle_index( info.m_effect_index );

	delete[] info.m_positions;
	delete[] info.m_times;

	beam_object_t obj {};
	obj.m_time_added = g_interfaces->m_global_vars->m_curtime;
	obj.m_effect_index = info.m_effect_index;

	m_active_beams.emplace_back( obj );
}

void c_particle_mgr::create_particle_beam( const vec3_t& start, const vec3_t& end, hellcolor color, float time, float width ) {
	if (!g_interfaces->m_global_vars || !g_interfaces->m_game_particle_manager || !g_interfaces->m_particle_system_mgr)
		return;

	particle_info_t tracer_info{};
	particle_color_t beam_color = { color.Value.x * 255.f, color.Value.y * 255.f, color.Value.z * 255.f };

	if (!g_interfaces->m_game_particle_manager->create_particle_effect(&tracer_info.m_effect_index, xx("particles/entity/spectator_utility_trail.vpcf")))
        return;

	g_interfaces->m_game_particle_manager->set_particle_data( tracer_info.m_effect_index, e_particle_setting::particle_setting_color, &beam_color );

	particle_information_t beam_info{};
	beam_info.m_time = time;
	beam_info.m_width = width;
	beam_info.m_alpha = color.Value.w;
	g_interfaces->m_game_particle_manager->set_particle_data( tracer_info.m_effect_index, e_particle_setting::particle_setting_info, &beam_info );

	constexpr int total_points = 5;
	std::vector<vec3_t> positions;
	positions.reserve( total_points );

	vec3_t direction = end - start;
	vec3_t middle = start + direction * 0.5f;

	positions.push_back( start );                                 /* start*/
	positions.push_back( start.lerp( middle, 0.05f ) );             /* near start*/
	positions.push_back( middle );                                /* middle */
	positions.push_back( middle.lerp( end, 0.95f ) );               /* near end*/
	positions.push_back( end );                                   /*  end*/

	tracer_info.m_data.m_positions = positions.data( );

	g_interfaces->m_particle_system_mgr->create_snapshot( &tracer_info.m_snapshot_handle );
	if (!tracer_info.m_snapshot_handle || !g_interfaces->m_game_particle_manager->init_effect(tracer_info.m_effect_index, 0, &tracer_info.m_snapshot_handle)) {
        g_interfaces->m_game_particle_manager->destroy_particle(tracer_info.m_effect_index, true, true);
        g_interfaces->m_game_particle_manager->release_particle_index(tracer_info.m_effect_index);
        return;
    }

	tracer_info.m_snapshot_handle->draw( total_points, &tracer_info.m_data );

	beam_object_t obj{};
	obj.m_time_added = g_interfaces->m_global_vars->m_curtime;
	obj.m_duration = time;
	obj.m_effect_index = tracer_info.m_effect_index;
	m_active_beams.emplace_back( obj );
}

void c_particle_mgr::update() {
	if (!g_interfaces->m_global_vars || !g_interfaces->m_game_particle_manager)
		return;

	auto it = m_active_beams.begin();
	while (it != m_active_beams.end()) {
		auto& beam = *it;
		float current_time = g_interfaces->m_global_vars->m_curtime;
		float delta = fabsf(current_time - beam.m_time_added);

		if (delta > beam.m_duration) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
			it = m_active_beams.erase(it);
		}
		else {
			++it;
		}
	}
}

void c_particle_mgr::run_world_weather(hellcolor color) {
    auto* manager = g_interfaces->m_game_particle_manager;
    auto* pawn = g_ctx->m_local_pawn;
    auto* node = pawn ? pawn->m_pGameSceneNode() : nullptr;
    if (!GET_VAR(bool, VISUALS_PATH(m_enable_world_weather)) || !g_interfaces->m_engine ||
        !g_interfaces->m_engine->in_game()) { release_world_weather(); return; }
    if (!manager || !node || !g_interfaces->m_resource_system) return;
    const auto origin = node->m_vecAbsOrigin();
    if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z)) return;
    const int type = GET_VAR(int, VISUALS_PATH(m_weather_index));
    if (type < 0 || type > 1) { release_world_weather(); return; }
    if (manager != m_weather_owner || type != m_weather_type || m_b_new_round_callback) {
        release_world_weather();
        m_weather_owner = manager;
        m_weather_type = type;
        m_b_new_round_callback = false;
    }
    if (m_weather_effects.empty()) {
        if (GetTickCount64() < m_weather_retry) return;
        m_weather_retry = GetTickCount64() + 500;
        const char* path = type == 0 ? "particles/rain_fx/rain.vpcf" : "particles/rain_fx/ash.vpcf";
        auto* resource = g_interfaces->m_resource_system->precache_resource(path, 0x66637076ull);
        if (!resource || !resource->data) return;
        for (int i = 0; i < 4; ++i) {
            unsigned int index = 0;
            if (manager->create_particle_effect(&index, path) && index != 0 && index != 0xFFFFFFFFu)
                m_weather_effects.push_back(index);
        }
    }
    const float distance = type == 0 ? 300.f : 200.f;
    const vec3_t positions[4] = {
        origin + vec3_t(distance, 0.f, 256.f), origin + vec3_t(-distance, 0.f, 256.f),
        origin + vec3_t(0.f, distance, 256.f), origin + vec3_t(0.f, -distance, 256.f)
    };
    vec3_t density(500.f, 0.f, 0.f);
    particle_color_t tint{color.Value.x, color.Value.y, color.Value.z};
    for (size_t i = 0; i < m_weather_effects.size(); ++i) {
        auto position = positions[i];
        manager->set_particle_data(m_weather_effects[i], 0, &position);
        if (type == 1) {
            manager->set_particle_data(m_weather_effects[i], 16, &position);
            manager->set_particle_data(m_weather_effects[i], 1, &tint);
        }
        manager->set_particle_data(m_weather_effects[i], 2, &density);
    }
}
void c_particle_mgr::release_world_weather() {
    if (m_weather_owner && m_weather_owner == g_interfaces->m_game_particle_manager)
        for (auto index : m_weather_effects) {
            m_weather_owner->destroy_particle(index, true, true);
            m_weather_owner->release_particle_index(index);
        }
    m_weather_effects.clear();
    m_weather_owner = nullptr;
    m_weather_type = -1;
    m_weather_retry = 0;
}

void c_particle_mgr::play_particle_effect(c_cs_player_pawn* pawn) {
 if(!pawn || !GET_VAR(bool,VISUALS_PATH(m_enabled_kill_effects)) || !g_interfaces->m_game_particle_manager) return;
 const int type=GET_VAR(int,VISUALS_PATH(m_kill_effects_type));
 const char* name=type==particle_glow_stars ? "bin/glow_stars.vpcf" : type==particle_sakura2 ? "bin/sakura_red.vpcf" : type==particle_sakura3 ? "bin/sakura_purple.vpcf" : nullptr;
 auto* node=pawn->m_pGameSceneNode();
 if(!name || !node) return;
 c_buffer_string resource(name,'fcpv');
 g_interfaces->m_resource_system->blocking_load_resource_by_name(&resource,"");
 unsigned index=0;
 if(!g_interfaces->m_game_particle_manager->create_particle_effect(&index,name)) return;
 auto position=node->m_vecAbsOrigin();
 g_interfaces->m_game_particle_manager->set_particle_data(index,0,&position);
 m_vec_effect_indexes.push_back(index);
}

void c_particle_mgr::clear_all_particles() {
    release_world_weather();
	if (!g_interfaces->m_game_particle_manager) {
        m_active_beams.clear();
        m_circle_beams.clear();
        m_vec_effect_indexes.clear();
        m_autopeek_effect_index = 0;
        return;
    }

	std::vector<beam_object_t> beams_to_clear;
	beams_to_clear.swap(m_active_beams);

	for (auto& beam : beams_to_clear) {
		if (beam.m_effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
		}
	}

	std::vector<beam_object_t> circle_beams_to_clear;
	circle_beams_to_clear.swap(m_circle_beams);

	for (auto& beam : circle_beams_to_clear) {
		if (beam.m_effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
		}
	}

	std::vector<unsigned int> effects_to_clear;
	effects_to_clear.swap(m_vec_effect_indexes);

	for (unsigned int& effect_index : effects_to_clear) {
		if (effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(effect_index);
		}
	}

	clean_autopeek_effect();
}

void c_particle_mgr::create_shot_sparks(const vec3_t& position, hellcolor color) {
	if (!g_interfaces->m_game_particle_manager)
		return;

	c_buffer_string shot_sparks("bin/shot_sparks.vpcf", 'fcpv');
	g_interfaces->m_resource_system->blocking_load_resource_by_name(&shot_sparks, "");
	unsigned int effect_index = 0;

	particle_info_t info{};
	if (!g_interfaces->m_game_particle_manager->create_particle_effect(&effect_index, "bin/shot_sparks.vpcf"))
		return;
	particle_color_t spark_color = { color.Value.x, color.Value.y, color.Value.z };
	g_interfaces->m_game_particle_manager->set_particle_data(effect_index, e_particle_setting::particle_setting_color, &spark_color);

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data(effect_index, e_particle_setting::particle_setting_position, &effect_position);
}

void c_particle_mgr::create_autopeek_effect(const vec3_t& position, hellcolor color) {
	if (!g_interfaces->m_game_particle_manager)
		return;

	if (m_autopeek_effect_index != 0)
		return;

	c_buffer_string autopeek_effect("bin/autopeek.vpcf", 'fcpv');
	g_interfaces->m_resource_system->blocking_load_resource_by_name(&autopeek_effect, "");

	particle_info_t info{};
	if (!g_interfaces->m_game_particle_manager->create_particle_effect(&m_autopeek_effect_index, "bin/autopeek.vpcf"))
		return;
	particle_color_t effect_color = { color.Value.x, color.Value.y, color.Value.z };
	g_interfaces->m_game_particle_manager->set_particle_data(m_autopeek_effect_index, 2U, &effect_color);

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data(m_autopeek_effect_index, e_particle_setting::particle_setting_position, &effect_position);
}

void c_particle_mgr::clean_autopeek_effect() {
	if (!g_interfaces->m_game_particle_manager || m_autopeek_effect_index == 0)
		return;

	g_interfaces->m_game_particle_manager->destroy_particle(m_autopeek_effect_index, true, true);
	g_interfaces->m_game_particle_manager->release_particle_index(m_autopeek_effect_index);
	m_autopeek_effect_index = 0;
}
