#pragma once

#include <memory> 
#include <math/math types/vector.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <context.h>
#include <sdk/interfaces/csgo_input.h>
#include <cheat/features/penetration/autowall.h>
#include <cheat/config/vars.h>
#include <random>
#include <array>
#include <algorithm>

class c_ragebot {
public:
	bool m_fired = false;
	bool m_bad_weapon = false;
	struct {
		bool m_enabled;
		bool m_autofire;
		int m_mindamage;
		int m_hitchance;
		int m_pointscale;
		std::vector<int> m_hitboxes, m_multipointed_hitboxes;
		e_ragebot_target_types m_targetting_type;
		e_ragebot_hitbox_preference_types m_hitbox_preference_type;
	} m_config;

	struct scan_result_t {
		vec3_t m_start, m_point, m_angle;
		bool m_extrapolated_local;
		lag_record_t* m_record;
		c_hitbox_data* m_hitbox;
		int m_damage, m_damage_threshold;
		float m_world_dist, m_history_delta;
		float m_point_projected_radius;
		c_penetration::player_context_t* m_penetration_context;

		bool valid_target();
		scan_result_t compare_player_internal(scan_result_t& other);
		scan_result_t compare_players(scan_result_t& other);
	};

	struct {
		int  m_last_scan;
		bool m_wants_lagcomp_bypass;
		bool m_wants_stop;
		bool m_accurate;
		scan_result_t m_best_target = {};
		void reset() { *this = {}; }
	}   m_data;

	void select_best_target(std::vector<scan_result_t>& results, bool found_kill);
	void player_scan();

	bool zeusbot_can_attack();

	bool should_stop();
	bool planned_shot_viable();
	bool has_max_accuracy();
	bool is_accurate();
	void handle_attacking();
	void set_config();
	void scan_record(scan_result_t& result, lag_record_t* record, c_penetration::player_context_t* penetration_ctx, const bool& front, vec3_t& start);
	void scan_player(scan_result_t& result, cached_player_t::lag_compensation_data_t* lagcomp_ctx, c_penetration::player_context_t* penetration_ctx);
	bool zeusbot_is_enemy_in_range(c_cs_player_pawn* p_enemy, float fl_range);
	c_cs_player_pawn* zeusbot_get_best_target();
	c_cs_player_pawn* m_p_last_target = nullptr;
	void zeusbot_run();
	void zeusbot_draw_radius();
	void prepare_revolver();

	void duck_peek_assist();

	void run();

	vec2_t get_spread(int seed, float inaccuracy, float spread);
	bool hit_chance(int hitchance_value);
	bool hit_chance_at_scale(int hitchance_value, float spread_scale);
	bool force_shot(int hitchance_value, int iteration_count, float minimum_spread_scale);
	void generate_head_points(c_hitbox_data* hitbox, tight_array<vec3_t, 64>& points, const vec3_t& start);
	void generate_horizontal_capsule_points(c_hitbox_data* hitbox, tight_array<vec3_t, 12>& points, const vec3_t& start);
	void generate_vertical_capsule_points(c_hitbox_data* hitbox, tight_array<vec3_t, 4>& points);
	int calculate_dynamic_point_scale(vec3_t& point, c_hitbox_data* hitbox);
private:
	friend void populate_occluders(tight_array<c_hitbox_data, 19>& all, tight_array<c_hitbox_data*, 19>& out, const vec3_t& start, c_hitbox_data* target);
};
inline auto g_ragebot = std::make_unique<c_ragebot>();