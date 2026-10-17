#include "shotlogger.h"
#include <cheat/features/penetration/autowall.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/config/vars.h>
#include <context.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/engine_cvar.h>
#include <cheat/features/ragebot/ragebot.h>
#include <math/math.h>

static bool can_hit(const vec3_t& start, const vec3_t& end, const vec3_t& v_min, const vec3_t& v_max, float fl_shape_radius, const matrix3x4_t& matrix) {
	vec3_t direction = end - start;
	float direction_length = direction.length();
	
	if (direction_length <= 0.0f || !std::isfinite(direction_length))
		return false;
	
	vec3_t v_direction = direction / direction_length;
	vec3_t v_center = (v_min + v_max) * 0.5f;
	vec3_t v_extent = (v_max - v_min) * 0.5f;

	vec3_t v_to_center = v_center - start;
	float fl_projection = v_to_center.dot(v_direction);

	if (fl_projection < 0.0f || fl_projection > start.distance_to(end))
		return false;

	vec3_t v_closest_point = start + v_direction * fl_projection;
	vec3_t v_offset = v_closest_point - v_center;

	float fl_distance_sqr = 0.0f;
	for (int i = 0; i < 3; i++) {
		float fl_component = std::max(0.0f, std::abs(v_offset[i]) - v_extent[i]);
		fl_distance_sqr += fl_component * fl_component;
	}

	return std::sqrt(fl_distance_sqr) <= fl_shape_radius;
}

void shotlogger::process_shots() {
	if (!g_ctx->m_active_weapon || m_vec_registered_shots.empty())
		return;

	if (!m_b_got_events) {
		bool has_ready_shot = false;
		for (auto& shot : m_vec_registered_shots) {
			if (!shot.m_vec_impacts.empty()) {
				has_ready_shot = true;
				break;
			}
			if (g_interfaces->m_global_vars->m_curtime - shot.m_shot_data.m_fl_time > 0.5f) {
				has_ready_shot = true;
				break;
			}
		}
		if (!has_ready_shot)
			return;
	}

	m_b_got_events = false;

	std::string str_notification = "";
	for (auto it = m_vec_registered_shots.begin(); it != m_vec_registered_shots.end(); ) {
		if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive()) {
			it = m_vec_registered_shots.erase(it);
			continue;
		}

		const bool b_dealt_damage = !it->m_vec_damage.empty();
		
		std::vector<bool> event_list = GET_VAR(std::vector<bool>, MISC_PATH(m_hitlog_modes));
		
		if (!b_dealt_damage && event_list.size() > 1 && event_list[1]) {
			if (!it->m_shot_data.m_p_record || !it->m_shot_data.m_p_record->m_pawn) {
				it = m_vec_registered_shots.erase(it);
				continue;
			}
			
			if (!it->m_shot_data.m_p_record->m_pawn->is_alive()) {
				it = m_vec_registered_shots.erase(it);
				continue;
			}
			
			if (!it->m_shot_data.m_p_hitbox) {
				it = m_vec_registered_shots.erase(it);
				continue;
			}
			
			if (!it->m_shot_data.m_p_record->m_bones) {
				it = m_vec_registered_shots.erase(it);
				continue;
			}
			
			vec3_t v_start = it->m_shot_data.m_v_start;
			vec3_t v_end = it->m_shot_data.m_v_end;
			
			const char* sz_reason{};
			static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
			
			bool b_hit = false;
			float fl_impact_distance = 0.0f;
			
			if (it->m_vec_impacts.empty()) {
				sz_reason = "no impact";
			} else {
				const vec3_t v_min = it->m_shot_data.m_p_hitbox->m_mins;
				const vec3_t v_max = it->m_shot_data.m_p_hitbox->m_maxs;
				matrix3x4_t identity_matrix{};
				identity_matrix[0][0] = 1.0f; identity_matrix[1][1] = 1.0f; identity_matrix[2][2] = 1.0f;

				vec3_t v_impact = it->m_vec_impacts.back();
				b_hit = can_hit(v_start, v_impact, v_min, v_max, it->m_shot_data.m_p_hitbox->m_radius, identity_matrix);
				fl_impact_distance = v_start.dist_to(v_impact);
				
				const float fl_point_distance = v_start.dist_to(v_end);
				
				if (it->m_shot_data.m_b_had_pred_error) 
					sz_reason = "prediction error";
				else if (it->m_shot_data.m_n_backtrack_tick > 0 && b_hit)
					sz_reason = "lagcompensation";
				else if (it->m_shot_data.m_n_backtrack_tick < 0 && b_hit)
					sz_reason = "extrapolation";
				else if (fl_impact_distance > fl_point_distance)
					sz_reason = "spread";
				else if (!b_hit) {
					//float time_since_shot = g_interfaces->m_global_vars->m_curtime - it->m_shot_data.m_fl_time;
					//if (time_since_shot < 0.3f) {
					//	for (int slot = 0; slot < g_interfaces->m_prediction->m_splits.size && slot < 2; slot++) {
					//		auto& split = g_interfaces->m_prediction->m_splits.elements[slot];
					//		if (split.m_last_error_report_time > 0.f &&
					//			std::abs(split.m_last_error_report_time - it->m_shot_data.m_fl_time) < 0.1f) {
					//			sz_reason = "prediction error";
					//			break;
					//		}
					//	}
					//}

					if (!sz_reason) {
						sz_reason = "occlusion";
					}
				}
				else
					sz_reason = "unknown";
			}
		
			const char* sz_hitgroup_name{};
			switch (it->m_shot_data.m_p_hitbox->m_hit_group) {
				case HITGROUP_HEAD: sz_hitgroup_name = "Head"; break;
				case HITGROUP_CHEST: sz_hitgroup_name = "Chest"; break;
				case HITGROUP_STOMACH: sz_hitgroup_name = "Stomach"; break;
				case HITGROUP_LEFTARM: sz_hitgroup_name = "Left arm"; break;
				case HITGROUP_RIGHTARM: sz_hitgroup_name = "Right arm"; break;
				case HITGROUP_LEFTLEG: sz_hitgroup_name = "Left leg"; break;
				case HITGROUP_RIGHTLEG: sz_hitgroup_name = "Right leg"; break;
				case HITGROUP_GENERIC: sz_hitgroup_name = "Generic"; break;
				default: sz_hitgroup_name = "Unknown";
			}

			str_notification =
				std::string("Missed ") +
				it->m_shot_data.m_str_player_name + " in the " +
				sz_hitgroup_name +
				" due to ";
			
			std::string reason_str = sz_reason;
			if (!reason_str.empty()) {
				reason_str[0] = std::toupper(reason_str[0]);
			}
			str_notification += reason_str;
			
			g_overlay->add_notification(str_notification, true);
		}
			
		it = m_vec_registered_shots.erase(it);
	}
}

void shotlogger::register_shot(shot_information_t& r_shot_data) {
	shot_events_t& shot_event = m_vec_shots.emplace_back();
	shot_event.m_shot_data = r_shot_data;
}

void shotlogger::game_event(c_game_event* p_event) {
	if (!g_ctx->m_local_pawn || !g_ctx->m_active_weapon)
		return;

	if (m_vec_shots.empty() && m_vec_registered_shots.empty())
		return;

	if (!m_vec_shots.empty()) {
		for (auto it = m_vec_shots.begin(); it != m_vec_shots.end(); )
		{
			if (std::abs(it->m_shot_data.m_fl_time - g_interfaces->m_global_vars->m_curtime) >= 2.5f || it->m_shot_data.m_b_matched) {
				it = m_vec_shots.erase(it);
			}
			else {
				it = std::next(it);
			}
		}
	}

	fnv1a_t u_event_hash = fnv_hash(p_event->get_name());
	switch (u_event_hash) {
		case fnv_hash("weapon_fire"): {
			c_cs_player_controller* p_victim = p_event->get_player_controller("userid");
			if (!p_victim || !p_victim->m_bIsLocalPlayerController())
				return;

			if (!m_vec_shots.empty()) {
				const auto it = std::find_if(m_vec_shots.rbegin(), m_vec_shots.rend(), [&](shot_events_t it) -> bool {return !it.m_shot_data.m_b_matched; });
				if (it != m_vec_shots.rend() && !it->m_shot_data.m_b_matched) {
					it->m_shot_data.m_b_matched = true;
					m_vec_registered_shots.emplace_back(*it);
				}
			}
			break;
		}

		case fnv_hash("player_hurt"): {
			if (m_vec_registered_shots.empty())
				return;

			c_cs_player_controller* p_victim = p_event->get_player_controller("userid");
			if (!p_victim || p_victim->m_bIsLocalPlayerController() || p_victim->m_iTeamNum() == g_ctx->m_local_controller->m_iTeamNum())
				return;

			shot_events_t::player_hurt_t hurt{};
			hurt.m_i_damage = p_event->get_int("dmg_health");
			hurt.m_i_hitgroup = p_event->get_int("hitgroup");
			hurt.m_i_target_index = p_event->get_int("userid");

			m_vec_registered_shots.back().m_vec_damage.push_back(hurt);
			break;
		}

		case fnv_hash("bullet_impact"): {
			c_cs_player_controller* p_victim = p_event->get_player_controller("userid");
			if (m_vec_registered_shots.empty() || !p_victim || !p_victim->m_bIsLocalPlayerController())
				return;

			vec3_t v_impact{};
			v_impact.x = p_event->get_float("x");
			v_impact.y = p_event->get_float("y");
			v_impact.z = p_event->get_float("z");

			m_vec_registered_shots.back().m_vec_impacts.emplace_back(v_impact);
			m_b_got_events = true;
			break;
		}
	}
}

void shotlogger::check_prediction_errors() {
	if (!g_interfaces->m_prediction)
		return;

	//for (int slot = 0; slot < g_interfaces->m_prediction->m_splits.size && slot < 2; slot++) {
	//	auto& split = g_interfaces->m_prediction->m_splits.elements[slot];

	//	if (split.m_total_prediction_errors > m_last_prediction_errors[slot]) {
	//		int error_diff = split.m_total_prediction_errors - m_last_prediction_errors[slot];
	//		int last_acked_cmd = split.m_last_knowledged_command;

	//		for (auto& shot_event : m_vec_registered_shots) {
	//			auto& shot = shot_event.m_shot_data;
	//			if (shot.m_i_command_number <= last_acked_cmd && shot.m_i_command_number > last_acked_cmd - error_diff) 
	//				shot.m_b_had_pred_error = true;
	//		}

	//		for (auto& shot_event : m_vec_shots) {
	//			auto& shot = shot_event.m_shot_data;
	//			if (shot.m_i_command_number <= last_acked_cmd && shot.m_i_command_number > last_acked_cmd - error_diff) 
	//				shot.m_b_had_pred_error = true;
	//		}
	//	}

	//	m_last_prediction_errors[slot] = split.m_total_prediction_errors;
	//}
}
