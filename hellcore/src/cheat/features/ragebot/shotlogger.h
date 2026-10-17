#pragma once

#include <includes.h>
#include <cheat/features/lag compensation/lag_compensation.h>
#include <cheat/features/aimbot shared/hitbox.h>
#include <math/math types/vector.h>

namespace shotlogger {
	struct shot_information_t {
		int m_i_hit_chance = 0;
		int m_i_predicted_damage = 0;
		int m_n_backtrack_tick = 0;
		float m_fl_time = 0.0f;
		lag_record_t* m_p_record = nullptr;
		bool m_b_had_pred_error = false;
		c_hitbox_data* m_p_hitbox = nullptr;
		int m_i_bone_index = 0;
		int m_i_command_number = 0;

		std::string m_str_player_name{};

		vec3_t m_v_start{};
		vec3_t m_v_end{};

		bool m_b_matched = false;
	};

	struct shot_events_t {
		struct player_hurt_t {
			int m_i_damage = 0;
			int m_i_hitgroup = 0;
			int m_i_target_index = 0;
		};

		std::vector<vec3_t> m_vec_impacts{};
		std::vector<player_hurt_t> m_vec_damage{};

		shot_information_t m_shot_data{};
	};

	void check_prediction_errors();
	void process_shots();
	void register_shot(shot_information_t& r_shot_data);
	void game_event(c_game_event* p_event);

	inline std::vector<shot_events_t> m_vec_shots{};
	inline std::vector<shot_events_t> m_vec_registered_shots{};
	inline bool m_b_got_events = false;
	inline int m_last_prediction_errors[2] = { 0, 0 };
}

