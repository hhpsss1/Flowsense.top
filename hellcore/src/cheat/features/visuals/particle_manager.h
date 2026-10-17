#pragma once

#include <sdk/interfaces/particle_system.h>
#include <../src/cheat/config/vars.h>

class c_particle_mgr {
public:
	struct beam_object_t {
		float m_time_added;
		float m_duration;
		unsigned int m_effect_index;
	};
public:
	bool m_b_new_round_callback = true;

	void create_particle_at_pos(const vec3_t& position, int type);

	void create_particle_beam(const vec3_t& start, const vec3_t& end, hellcolor color, float time, float width);

	void create_shot_sparks(const vec3_t& position, hellcolor color);

	void create_autopeek_effect(const vec3_t& position, hellcolor color);

	void clean_autopeek_effect();

	void update();

	void run_world_weather(hellcolor color);

	void release_world_weather();

	void play_particle_effect(c_cs_player_pawn* pawn);

	void clear_all_particles();
private:
	std::vector<beam_object_t> m_active_beams;
	std::vector<unsigned int> m_vec_effect_indexes;
    std::vector<unsigned int> m_weather_effects;
    c_game_particle_manager* m_weather_owner = nullptr;
    int m_weather_type = -1;
    uint64_t m_weather_retry = 0;
	std::vector<beam_object_t> m_circle_beams;
	unsigned int m_autopeek_effect_index = 0;
};
inline auto g_particle_mgr = std::make_unique< c_particle_mgr >();