#include <sdk/constants.h>
#include <cheat/config/vars.h>
#include <future>
#include <algorithm>
#include <cheat/features/engine_prediction/engine_prediction.h>
#include <cheat/features/movement/movement.h>
#include <cheat/features/aimbot shared/general_shared.h>
#include <latch>
#include <cheat/menu/tabs/aimbot_tab.h>
#include <cheat/menu/hell_gui/drawlist.h>
#include <sdk/interfaces/engine_cvar.h>
#include <imgui.h>
#include <random>
#include <cmath>

#include "../visuals/overlay_features.h"
#include "shotlogger.h"

#include "ragebot.h"

void c_ragebot::set_config() {

	int weapon_rage_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
	if (weapon_rage_type == 10)
		m_bad_weapon = true;

	m_config.m_mindamage = GET_VAR(int, tabs::aimbot::get_min_damage_holder_id(weapon_rage_type));

	if (!g_ctx->m_rage_config_needs_update)
		return;

	m_config.m_enabled = GET_VAR(bool, RAGEBOT_PATH(m_enabled_ragebot));
	m_config.m_autofire = GET_VAR(bool, RAGEBOT_PATH(m_autofire));

	m_bad_weapon = false;

	m_config.m_hitchance = GET_VAR(int, tabs::aimbot::get_hitchance_holder_id(weapon_rage_type));
	m_config.m_pointscale = GET_VAR(int, tabs::aimbot::get_pointscale_holder_id(weapon_rage_type));

	m_config.m_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_selected_hitboxes)).at(weapon_rage_type);
	m_config.m_multipointed_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_multipoint_hitboxes)).at(weapon_rage_type);
	m_config.m_targetting_type = GET_VAR(std::vector< e_ragebot_target_types >, RAGEBOT_PATH(m_target_type)).at(weapon_rage_type);
	m_config.m_hitbox_preference_type = GET_VAR(std::vector< e_ragebot_hitbox_preference_types >, RAGEBOT_PATH(m_hitbox_preference)).at(weapon_rage_type);

	g_ctx->m_rage_config_needs_update = false;
}

bool c_ragebot::scan_result_t::valid_target() {
	return !m_point.is_zero() && m_record && m_damage > 0 && m_record->is_valid() && m_hitbox;
}

c_ragebot::scan_result_t c_ragebot::scan_result_t::compare_player_internal(c_ragebot::scan_result_t& other) {
	if (!this->valid_target())
		return other;
	if (!other.valid_target())
		return *this;

	bool this_meets_threshold = this->m_damage >= this->m_damage_threshold;
	bool other_meets_threshold = other.m_damage >= other.m_damage_threshold;

	if (other_meets_threshold && !this_meets_threshold)
		return other;
	if (this_meets_threshold && !other_meets_threshold)
		return *this;

	// same player: fresh records are far more hittable on the server,
	// keep an older one only if it hits meaningfully harder.
	const int damage_diff = this->m_damage - other.m_damage;
	if (damage_diff > 10)
		return *this;
	if (-damage_diff > 10)
		return other;

	if (this->m_history_delta != other.m_history_delta)
		return (this->m_history_delta < other.m_history_delta) ? *this : other;

	// furryware: between records of the same player, prefer the point closest to the eye
	const vec3_t& eye = g_ctx->m_extrapolated_shoot_position;
	const float this_dist = eye.dist_to_sq(this->m_point);
	const float other_dist = eye.dist_to_sq(other.m_point);
	return (this_dist <= other_dist) ? *this : other;
}

c_ragebot::scan_result_t c_ragebot::scan_result_t::compare_players(c_ragebot::scan_result_t& other) {
	if (!this->valid_target())
		return other;
	if (!other.valid_target())
		return *this;

	bool this_meets_threshold = this->m_damage >= this->m_damage_threshold;
	bool other_meets_threshold = other.m_damage >= other.m_damage_threshold;

	if (other_meets_threshold && !this_meets_threshold)
		return other;
	if (this_meets_threshold && !other_meets_threshold)
		return *this;

	switch (g_ragebot->m_config.m_targetting_type) {
	case e_ragebot_target_types::rage_target_damage:
		return (this->m_damage > other.m_damage) ? *this : other;
		break;
	case e_ragebot_target_types::rage_target_hitchance:
		return (this->m_world_dist < other.m_world_dist) ? *this : other;
		break;

	case e_ragebot_target_types::rage_target_low_health:
		if (this->m_record && this->m_record->m_pawn &&
			other.m_record && other.m_record->m_pawn) {
			int this_health = this->m_record->m_pawn->m_iHealth();
			int other_health = other.m_record->m_pawn->m_iHealth();
			return (this_health < other_health) ? *this : other;
		}
		break;

	default:
		break;
	}
	return *this;
}

force_inline bool c_ragebot::should_stop() {
	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	if (weapon_accuracy_nospread->get_bool())
		return false;

	if (!GET_VAR(bool, RAGEBOT_PATH(m_autostop)))
		return false;

	if (m_fired)
		return false;

	auto& modes = GET_VAR(std::vector<bool>, RAGEBOT_PATH(m_autostop_modes));

	if (g_prediction->is_definitely_on_ground()) {
		if (!modes.at(e_ragebot_autostop_types::rage_autostop_early))
			return false;

		if (!m_data.m_best_target.valid_target())
			return false;

		if (!g_ctx->m_active_weapon->can_shoot(g_ctx->m_local_controller))
			return false;

		float velocity = g_ctx->m_local_pawn->m_vecAbsVelocity().length_2d();
		float max_speed = g_ctx->m_active_weapon->get_max_speed();

		if (velocity < max_speed * 0.34f)
			return false;

		// stopping only matters if motion actually inflates spread past
		// standing. otherwise halting just makes us a static target.
		if (g_ctx->m_active_weapon_data) {
			const float move_inacc = g_ctx->m_active_weapon_data->m_flInaccuracyMove().m_types[0];
			const float stand_inacc = g_ctx->m_active_weapon_data->m_flInaccuracyStand().m_types[0];
			if (std::isfinite(move_inacc) && std::isfinite(stand_inacc) && velocity * move_inacc <= stand_inacc)
				return false;
		}

		return true;
	}
	else {
		if (modes.at(e_ragebot_autostop_types::rage_autostop_in_air))
			return true;
	}

	return false;
}

bool c_ragebot::planned_shot_viable() {
	if (!g_ctx->m_local_pawn || !g_ctx->m_active_weapon || !g_ctx->m_active_weapon_data)
		return false;

	if (!g_prediction->is_definitely_on_ground())
		return false;

	vec3_t velocity = g_ctx->m_local_pawn->m_vecAbsVelocity();
	velocity.z = 0.f;
	const float speed = velocity.length_2d();
	const float max_speed = g_ctx->m_active_weapon->get_max_speed();
	const bool scoped = g_ctx->m_local_pawn->m_bIsScoped();

	if (!(speed > max_speed * 0.34f || (scoped && speed > 1.f)))
		return false;

	static auto sv_friction = g_interfaces->m_engine_convar->find_by_name("sv_friction");
	static auto sv_stopspeed = g_interfaces->m_engine_convar->find_by_name("sv_stopspeed");
	static auto sv_accelerate = g_interfaces->m_engine_convar->find_by_name("sv_accelerate");

	const float friction = sv_friction ? sv_friction->get_float() : 4.f;
	const float stop_speed = sv_stopspeed ? sv_stopspeed->get_float() : 80.f;
	float accel = sv_accelerate ? sv_accelerate->get_float() : 5.5f;
	if (!std::isfinite(friction + stop_speed + accel) || friction <= 0.f)
		return false;

	float surface_friction = 1.f;
	if (auto* move_service = g_ctx->m_local_pawn->m_pMovementServices())
		surface_friction = move_service->m_flSurfaceFriction();
	if (!std::isfinite(surface_friction) || surface_friction <= 0.f)
		surface_friction = 1.f;

	// friction sim: how many ticks until halted, and where the eye ends up.
	vec3_t sim_vel = velocity;
	int stop_ticks = 0;
	for (int i = 0; i < 15; ++i) {
		const float sim_speed = sim_vel.length_2d();
		if (sim_speed < 1.f)
			break;
		const float control = std::fmaxf(sim_speed, stop_speed);
		const float drop = friction * surface_friction * control * interval_per_tick;
		float new_speed = std::fmaxf(sim_speed - drop, 0.f);
		const float accel_speed = std::fminf(accel * max_speed * surface_friction * interval_per_tick, new_speed);
		new_speed = std::fmaxf(new_speed - accel_speed, 0.f);
		if (new_speed <= 0.f) {
			sim_vel = vec3_t(0.f, 0.f, 0.f);
			stop_ticks = i + 1;
			break;
		}
		sim_vel = sim_vel * (new_speed / sim_speed);
		stop_ticks = i + 1;
	}
	if (stop_ticks <= 0)
		return false;

	const vec3_t avg_vel = (velocity + sim_vel) * 0.5f;
	const vec3_t future_eye = g_ctx->m_shoot_position + avg_vel * (stop_ticks * interval_per_tick);

	const float live_inaccuracy = g_ctx->m_active_weapon->get_inaccuracy();
	if (!std::isfinite(live_inaccuracy) || live_inaccuracy <= 1e-6f)
		return false;

	const float stand_inaccuracy = g_ctx->m_active_weapon_data->m_flInaccuracyStand().m_types[scoped ? 1 : 0];
	if (!std::isfinite(stand_inaccuracy))
		return false;

	const float scale = std::clamp(stand_inaccuracy / live_inaccuracy, 0.f, 1.f);

	// what-if check from the future eye: never commits anything, the real
	// shot next tick rescans from scratch.
	const vec3_t saved_eye = g_ctx->m_shoot_position;
	const vec3_t saved_point = m_data.m_best_target.m_point;
	g_ctx->m_shoot_position = future_eye;
	const bool viable = hit_chance_at_scale(m_config.m_hitchance, scale);
	g_ctx->m_shoot_position = saved_eye;
	m_data.m_best_target.m_point = saved_point;
	return viable;
}

bool c_ragebot::has_max_accuracy() {

	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	if (weapon_accuracy_nospread->get_bool())
		return true;

	if (!GET_VAR(bool, RAGEBOT_PATH(m_force_shoot)))
		return false;

	if (!g_prediction->is_definitely_on_ground())
		return false;

	const bool should_scope = g_ctx->m_active_weapon_data->m_WeaponType() == WEAPONTYPE_SNIPER_RIFLE;
	const bool is_scoped = g_ctx->m_local_pawn->m_bIsScoped();

	float min_accuracy = 0.f;

	if (auto movement = g_ctx->m_local_pawn->m_pMovementServices(); movement && (movement->m_bDucking() || movement->m_bDucked())) {
		if (is_scoped)
			min_accuracy = g_ctx->m_active_weapon_data->m_flInaccuracyCrouch().m_types[1];
		else
			min_accuracy = g_ctx->m_active_weapon_data->m_flInaccuracyCrouch().m_types[should_scope ? 1 : 0];
	}
	else {
		if (is_scoped)
			min_accuracy = g_ctx->m_active_weapon_data->m_flInaccuracyStand().m_types[1];
		else
			min_accuracy = g_ctx->m_active_weapon_data->m_flInaccuracyStand().m_types[should_scope ? 1 : 0];
	}

	float current_inaccuracy = g_ctx->m_active_weapon->get_inaccuracy();
	float tolerance = min_accuracy * 1.05f + 0.0001f;

	return current_inaccuracy <= tolerance;
}

bool c_ragebot::hit_chance(int hitchance_value) {
	return hit_chance_at_scale(hitchance_value, 1.0f);
}

bool c_ragebot::force_shot(int hitchance_value, int iteration_count, float minimum_spread_scale) {
	iteration_count = std::clamp(iteration_count, 1, 3);

	if (iteration_count == 1)
		return hit_chance_at_scale(hitchance_value, 1.0f);

	const float divisor = static_cast<float>(iteration_count - 1);
	const float spread_scale_range = 1.0f - minimum_spread_scale;

	for (int iteration = 0; iteration < iteration_count; ++iteration) {
		const float spread_scale = 1.0f - spread_scale_range * static_cast<float>(iteration) / divisor;
		if (hit_chance_at_scale(hitchance_value, spread_scale))
			return true;
	}

	return false;
}

bool c_ragebot::hit_chance_at_scale(int hitchance_value, float spread_scale) {
	if (hitchance_value <= 0) return true;
	if (!m_data.m_best_target.valid_target() || !m_data.m_best_target.m_hitbox) return false;

	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	if (weapon_accuracy_nospread && weapon_accuracy_nospread->get_bool()) return true;

	vec3_t start = g_ctx->m_shoot_position;

	float range = g_ctx->m_active_weapon_data->m_flRange();

	static auto inaccuracy_fn = g_modules->m_client.find(xx("48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24")).as<float(__fastcall*)(void*, float*, float*)>();
	static auto spread_fn = g_modules->m_client.find(xx("48 63 91 ?? ?? ?? ?? 48 8B 81 ?? ?? ?? ?? 85 D2 78 ?? 48 83 FA 02 73 ?? F3 0F 10 84 90 50 07 00 00 C3 F3 0F 10 80 50 07 00 00 C3")).as<float(__fastcall*)(void*)>();

	if (!inaccuracy_fn || !spread_fn) return false;
	float inaccuracy_x{}, inaccuracy_y{};
	float _inaccuracy = inaccuracy_fn(g_ctx->m_active_weapon, &inaccuracy_x, &inaccuracy_y) * spread_scale;
	float _spread = spread_fn(g_ctx->m_active_weapon) * spread_scale;

	if (_inaccuracy < 0.0001f && _spread < 0.0001f) return true;

	static auto calculate_spread_angles = g_modules->m_client.find(xx("48 8B C4 48 89 58 18 48 89 68 20 89 50 10 56 57 41 54 41 56 41 57 48 81 EC E0 00 00 00 44 8B 15 ? ? ? ? 41 8B F9 0F 29 78 B8 41 8B E8 48 8B 05 ? ? ? ? 0F B7 F1 41 BC 68 00")).as<void(__fastcall*)(int16_t, int, int, unsigned int, float, float, float, float*, float*)>();
	if (!calculate_spread_angles || !std::isfinite(_inaccuracy) || !std::isfinite(_spread)) return false;

	const int total_seeds = 256;
	const int needed_hits = std::max(1, (total_seeds * hitchance_value + 99) / 100);

	// velocity-style hitchance: pure geometry, no autowall per seed.
	// damage was already proven by the scan autowall on this exact point;
	// re-simulating penetration for all 256 seeds only starves the counter
	// on moving/desynced targets (silent bot without force shoot).
	// optionally accumulates the spread direction for aim refinement.
	auto run_pass = [&](const vec3_t& point, vec3_t* dir_sum_out, float* damage_sum_out) {
		vec3_t to_point = point - start;
		if (to_point.length_sqr() < 0.001f)
			return 0;

		vec3_t angles;
		g_math->vector_angles(to_point, angles);

		vec3_t forward, right, up;
		g_math->angle_vectors(angles, &forward, &right, &up);

		const int item_def = g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
		const int scoped = (int)g_ctx->m_local_pawn->m_bIsScoped();
		const float recoil_index = g_ctx->m_active_weapon->m_flRecoilIndex();

		int hits = 0;
		vec3_t dir_sum{};
		float damage_sum = 0.f;

		for (int seed = 0; seed < total_seeds; seed++) {
			float spread_x, spread_y;

			calculate_spread_angles(
				item_def,
				1,
				scoped,
				seed,
				_inaccuracy,
				_spread,
				recoil_index,
				&spread_x,
				&spread_y
			);

			vec3_t spread_dir = forward + (right * spread_x) + (up * spread_y);
			float spread_len = spread_dir.length();
			if (spread_len < 0.001f) {
				hits++;
				if (hits >= needed_hits) break;
				continue;
			}

			spread_dir /= spread_len;
			vec3_t spread_end = start + (spread_dir * range);

			// test the seed against the whole player, not just the chosen
			// hitbox: under real spread most lethal seeds land on
			// neighboring hitboxes, and requiring the exact capsule
			// starves hitchance (silent bot without force shoot).
			bool touches_player = false;
			auto* hit_record = m_data.m_best_target.m_record;
			if (hit_record && !hit_record->m_all_hitboxes.empty()) {
				for (auto& hb : hit_record->m_all_hitboxes) {
					if (hb.segment_intersects_capsule(start, spread_end)) {
						touches_player = true;
						break;
					}
				}
			}
			else {
				touches_player = m_data.m_best_target.m_hitbox->segment_intersects_capsule(start, spread_end);
			}

			if (touches_player) {
				hits++;
				if (dir_sum_out) {
					dir_sum = dir_sum + spread_dir;
					damage_sum += 1.f;
				}
				if (hits >= needed_hits) break;
			}

			int remaining = total_seeds - seed - 1;
			if (hits + remaining < needed_hits) break;
		}

		if (dir_sum_out) *dir_sum_out = dir_sum;
		if (damage_sum_out) *damage_sum_out = damage_sum;
		return hits;
	};

	vec3_t aim_point = m_data.m_best_target.m_point;
	if (start.dist_to(aim_point) > range * 1.1f) return false;

	// failed, but some seeds were lethal: steer the aim point toward the
	// damage-weighted spread center and verify it with a second pass.
	// the shot itself goes through the refined point in handle_attacking.
	vec3_t dir_sum{};
	float damage_sum = 0.f;
	if (run_pass(aim_point, &dir_sum, &damage_sum) >= needed_hits)
		return true;

	if (damage_sum <= 0.f)
		return false;

	vec3_t average_direction = dir_sum / damage_sum;
	if (average_direction.length_sqr() < 1e-8f || !std::isfinite(average_direction.x + average_direction.y + average_direction.z))
		return false;
	average_direction.normalize_place();

	vec3_t aim_delta = aim_point - start;
	const float distance_along_ray = aim_delta.dot(average_direction);
	if (!(distance_along_ray > 0.f) || !std::isfinite(distance_along_ray))
		return false;

	vec3_t refined = start + average_direction * distance_along_ray;
	if (!std::isfinite(refined.x + refined.y + refined.z))
		return false;

	// verify first, commit only on pass: a failed refinement must not
	// corrupt the scanned point for this tick.
	if (run_pass(refined, nullptr, nullptr) >= needed_hits) {
		m_data.m_best_target.m_point = refined;
		return true;
	}

	return false;
}

bool c_ragebot::is_accurate() {
	if (has_max_accuracy())
		return true;

	// force shoot: if it doesn't pass at full spread, retry under
	// progressively relaxed spread assumptions (menu-tunable).
	if (GET_VAR(bool, RAGEBOT_PATH(m_force_shoot))) {
		const int iterations = std::clamp(GET_VAR(int, RAGEBOT_PATH(m_forceshoot_iterations)), 1, 3);
		const float min_scale = std::clamp(GET_VAR(int, RAGEBOT_PATH(m_forceshot_spread_scale)), 10, 100) / 100.f;
		return force_shot(m_config.m_hitchance, iterations, min_scale);
	}

	return hit_chance(m_config.m_hitchance);
}

// furryware hitbox priority — drives target-point selection like the reference ragebot
static int fw_get_hitbox_priority(int hit_group) {
	switch (hit_group) {
	case HITGROUP_HEAD:    return 400;
	case HITGROUP_NECK:    return 300;
	case HITGROUP_CHEST:   return 200;
	case HITGROUP_STOMACH: return 100;
	default:               return 0;
	}
}

void populate_occluders(tight_array<c_hitbox_data, 19>& all, tight_array<c_hitbox_data*, 19>& out, const vec3_t& start, c_hitbox_data* target) {
	vec3_t center = target->m_center;
	for (int i = 0; i < all.size(); ++i) {
		c_hitbox_data& hb = all[i];
		if (hb.m_hit_group == target->m_hit_group)
			continue;

		if (hb.segment_intersects_capsule(start, center))
			out.emplace_back(&hb);
	}
}

bool is_point_occluded(tight_array<c_hitbox_data*, 19>& occluders, const vec3_t& start, const vec3_t& point, c_hitbox_data* target_hitbox) {
	vec3_t direction = point - start;
	float distance = direction.length();
	if (distance < 0.1f) return false;

	direction /= distance;

	for (int i = 0; i < occluders.size(); ++i) {
		c_hitbox_data* hb = occluders[i];
		if (hb->m_hit_group == target_hitbox->m_hit_group)
			continue;

		vec3_t capsule_dir = (hb->m_maxs - hb->m_mins).normalized();
		vec3_t to_start = start - hb->m_center;
		float proj = to_start.dot(capsule_dir);
		vec3_t closest_on_axis = hb->m_center + capsule_dir * std::clamp(proj, -hb->m_radius, hb->m_radius);

		vec3_t to_ray = closest_on_axis - start;
		float ray_proj = to_ray.dot(direction);

		if (ray_proj < 0.0f || ray_proj > distance)
			continue;

		vec3_t closest_on_ray = start + direction * ray_proj;
		float dist_sq = closest_on_axis.dist_to_sq(closest_on_ray);

		if (dist_sq < (hb->m_radius * hb->m_radius * 0.95f))
			return true;
	}
	return false;
}

void c_ragebot::generate_head_points(c_hitbox_data* hitbox, tight_array<vec3_t, 64>& points, const vec3_t& start) {
	if (!hitbox || hitbox->m_radius <= 0.0f) return;

	float pointscale = m_config.m_pointscale;
	if (pointscale <= 1)
		pointscale = calculate_dynamic_point_scale(hitbox->m_center, hitbox);

	float scale = pointscale / 100.0f;
	float final_radius = hitbox->m_radius * scale;

	if (final_radius < 0.2f) {
		points.emplace_back(hitbox->m_center);
		return;
	}

	vec3_t length = hitbox->m_maxs - hitbox->m_mins;
	vec3_t capsule_dir = length.normalized();
	float length_mag = length.length();

	vec3_t view_dir = (start - hitbox->m_center).normalized();
	vec3_t side_dir;
	if (fabsf(capsule_dir.z) < 0.9f)
		side_dir = vec3_t(-capsule_dir.y, capsule_dir.x, 0.0f).normalized();
	else
		side_dir = vec3_t(1.0f, 0.0f, 0.0f);

	vec3_t up_dir = capsule_dir.cross(side_dir).normalized();

	vec3_t top_point = hitbox->m_center + capsule_dir * (length_mag * 0.45f);
	points.emplace_back(top_point);

	points.emplace_back(hitbox->m_center);

	float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f, 45.0f, 135.0f, 225.0f, 315.0f };
	for (float angle : angles) {
		float rad = angle * (3.14159265f / 180.0f);
		vec3_t offset = (side_dir * cosf(rad) + up_dir * sinf(rad)) * final_radius * 0.9f;
		points.emplace_back(hitbox->m_center + offset);
	}

	vec3_t forward_point = hitbox->m_center + view_dir * final_radius * 0.7f;
	forward_point.z += final_radius * 0.3f;
	points.emplace_back(forward_point);

	vec3_t back_point = hitbox->m_center - view_dir * final_radius * 0.7f;
	back_point.z += final_radius * 0.3f;
	points.emplace_back(back_point);
}

void c_ragebot::generate_horizontal_capsule_points(c_hitbox_data* hitbox, tight_array<vec3_t, 12>& points, const vec3_t& start) {
	if (!hitbox || hitbox->m_radius <= 0.0f)
		return;

	float pointscale = m_config.m_pointscale;
	if (pointscale <= 1)
		pointscale = calculate_dynamic_point_scale(hitbox->m_center, hitbox);

	float scale = pointscale / 100.0f;
	float final_radius = hitbox->m_radius * scale;

	if (final_radius < 0.2f) {
		points.emplace_back(hitbox->m_center);
		return;
	}

	vec3_t length = hitbox->m_maxs - hitbox->m_mins;
	vec3_t capsule_dir = length.normalized();

	vec3_t side_dir;
	if (fabs(capsule_dir.z) < 0.9f)
		side_dir = vec3_t(-capsule_dir.y, capsule_dir.x, 0.0f).normalized();
	else
		side_dir = vec3_t(1.0f, 0.0f, 0.0f);

	vec3_t up_dir = vec3_t(0.0f, 0.0f, 1.0f);

	points.emplace_back(hitbox->m_center);

	points.emplace_back(hitbox->m_mins + side_dir * final_radius * 0.9f);
	points.emplace_back(hitbox->m_mins - side_dir * final_radius * 0.9f);
	points.emplace_back(hitbox->m_maxs + side_dir * final_radius * 0.9f);
	points.emplace_back(hitbox->m_maxs - side_dir * final_radius * 0.9f);

	points.emplace_back(hitbox->m_center + up_dir * final_radius * 0.7f);
	points.emplace_back(hitbox->m_center - up_dir * final_radius * 0.7f);

	vec3_t quarter = hitbox->m_mins + length * 0.33f;
	vec3_t three_quarter = hitbox->m_mins + length * 0.66f;

	points.emplace_back(quarter);
	points.emplace_back(three_quarter);
	points.emplace_back(quarter + side_dir * final_radius * 0.7f);
	points.emplace_back(three_quarter - side_dir * final_radius * 0.7f);
}

void c_ragebot::generate_vertical_capsule_points(c_hitbox_data* hitbox, tight_array<vec3_t, 4>& points) {
	if (!hitbox || hitbox->m_radius <= 0.0f)
		return;

	float pointscale = m_config.m_pointscale;
	if (pointscale <= 1)
		pointscale = calculate_dynamic_point_scale(hitbox->m_center, hitbox);

	float scale = pointscale / 100.0f;
	float final_radius = hitbox->m_radius * scale;

	if (final_radius < 0.2f) {
		points.emplace_back(hitbox->m_center);
		return;
	}

	vec3_t length = hitbox->m_maxs - hitbox->m_mins;

	points.emplace_back(hitbox->m_center);
	points.emplace_back(hitbox->m_mins + vec3_t(0, 0, final_radius * 0.9f));
	points.emplace_back(hitbox->m_maxs - vec3_t(0, 0, final_radius * 0.9f));
	points.emplace_back(hitbox->m_mins + length * 0.5f);
}

int c_ragebot::calculate_dynamic_point_scale(vec3_t& point, c_hitbox_data* hitbox) {
	if (!hitbox || !g_ctx->m_active_weapon)
		return m_config.m_pointscale;

	const float spread = g_ctx->m_active_weapon->get_spread();
	const float inaccuracy = g_ctx->m_active_weapon->get_inaccuracy();
	const float total_spread = spread + inaccuracy;
	const float distance = point.dist_to(g_ctx->m_extrapolated_shoot_position);
	const float effective_spread = total_spread * distance;

	const float min_radius = hitbox->m_radius * 0.3f;
	const float adjusted_radius = std::max(hitbox->m_radius - effective_spread * 0.5f, min_radius);
	const float scale_factor = std::clamp(adjusted_radius / hitbox->m_radius, 0.3f, 1.0f);

	return static_cast<int>(scale_factor * 100.0f);
}

	void c_ragebot::scan_record(scan_result_t& result, lag_record_t* record, c_penetration::player_context_t* penetration_ctx, const bool& allow_multipoint, vec3_t& start) {
		thread_local tight_array<vec3_t, 64> tls_head_points;
		thread_local tight_array<vec3_t, 12> tls_horizontal_capsule_points;
		thread_local tight_array<vec3_t, 4> tls_vertical_capsule_points;

		const int target_health = record->m_pawn->m_iHealth();
		int dmg_threshold = m_config.m_mindamage;
		if (dmg_threshold > 100)
			dmg_threshold = target_health + dmg_threshold - 100;
		else
			dmg_threshold = std::min(dmg_threshold, target_health);

		auto hitbox_preference = m_config.m_hitbox_preference_type;

		result.m_record = record;
		result.m_penetration_context = penetration_ctx;
		result.m_damage = -1;
		result.m_hitbox = nullptr;
		result.m_damage_threshold = dmg_threshold;
		result.m_start = start;
		result.m_world_dist = record->m_origin.dist_to(start);
		result.m_point_projected_radius = -1.0f;

		auto scan_point = [&](vec3_t& point, c_hitbox_data* hitbox) -> bool {
			if (!hitbox || !penetration_ctx)
				return false;

			c_handle_bullet_penetration_data pen_data;
			pen_data.m_damage = m_local_context.m_damage;
			pen_data.m_penetration = m_local_context.m_penetration;
			pen_data.m_range_modifier = m_local_context.m_range_mod;
			pen_data.m_pen_count = GET_VAR(bool, RAGEBOT_PATH(m_autowall)) ? 4 : 0;

			int current_damage = 0;
			if (penetration_ctx->fire_bullet(start, point, record->m_pawn, pen_data))
				current_damage = static_cast<int>(pen_data.m_damage);

			if (current_damage < dmg_threshold)
				return false;

			const float current_dist = point.dist_to_sq(start);
			const float result_dist = result.m_point.is_zero() ? FLT_MAX : result.m_point.dist_to_sq(start);

			const int current_priority = fw_get_hitbox_priority(hitbox->m_hit_group);
			const int result_priority = result.m_hitbox ? fw_get_hitbox_priority(result.m_hitbox->m_hit_group) : -1;

			bool is_better = false;
			if (!result.m_hitbox) {
				is_better = true;
			}
			else if (current_priority > result_priority) {
				is_better = true;
			}
			else if (current_priority == result_priority) {
				if (current_damage > result.m_damage)
					is_better = true;
				else if (current_damage == result.m_damage && current_dist < result_dist)
					is_better = true;
			}

			if (is_better) {
				result.m_point = point;
				result.m_hitbox = hitbox;
				result.m_damage = current_damage;
				result.m_point_projected_radius = hitbox->m_radius;
				return (current_damage >= target_health && hitbox->m_hit_group == HITGROUP_HEAD);
			}
			return false;
			};

		tight_array<c_hitbox_data*, 19> sorted_hitboxes;
		for (auto& hb : record->m_rage_hitboxes) {
			sorted_hitboxes.emplace_back(&hb);
		}

		std::sort(sorted_hitboxes.begin(), sorted_hitboxes.end(), [&](c_hitbox_data* a, c_hitbox_data* b) {
			int a_priority = fw_get_hitbox_priority(a->m_hit_group);
			int b_priority = fw_get_hitbox_priority(b->m_hit_group);
			if (a_priority != b_priority)
				return a_priority > b_priority;
			return a->m_radius > b->m_radius;
			});

		bool found_lethal = false;

		float total_spread = 0.f;
		if (g_ctx->m_active_weapon) {
			const float sp = g_ctx->m_active_weapon->get_spread() + g_ctx->m_active_weapon->get_inaccuracy();
			if (std::isfinite(sp) && sp > 0.f)
				total_spread = sp;
		}

		for (c_hitbox_data* hitbox_ptr : sorted_hitboxes) {
			auto& current_hitbox = *hitbox_ptr;


			if (scan_point(current_hitbox.m_center, &current_hitbox)) {
				found_lethal = true;
				if (current_hitbox.m_hit_group == HITGROUP_HEAD && result.m_damage >= target_health)
					return;
			}

			bool use_multipoint = allow_multipoint && current_hitbox.m_multipoint;
			if (use_multipoint && total_spread > 0.f && current_hitbox.m_radius > 0.f) {
				// safepoint: edge points are lottery when the spread cone
				// at the target is wider than the hitbox itself.
				// fall back to centers instead of missing wide.
				const float cone = total_spread * start.dist_to(current_hitbox.m_center);
				if (cone > current_hitbox.m_radius * 2.f)
					use_multipoint = false;
			}

			if (use_multipoint) {
				switch (current_hitbox.m_orientation) {
				case hitbox_orientation::hitbox_head_identity:
					tls_head_points.clear();
					generate_head_points(&current_hitbox, tls_head_points, start);
					std::sort(tls_head_points.begin(), tls_head_points.end(), [&](const vec3_t& a, const vec3_t& b) {
						return a.dist_to_sq(start) < b.dist_to_sq(start);
						});
					for (vec3_t& mp : tls_head_points) {
						if (scan_point(mp, &current_hitbox)) {
							found_lethal = true;
							if (current_hitbox.m_hit_group == HITGROUP_HEAD && result.m_damage >= target_health)
								return;
						}
					}
					break;

				case hitbox_orientation::hitbox_horizontal:
					tls_horizontal_capsule_points.clear();
					generate_horizontal_capsule_points(&current_hitbox, tls_horizontal_capsule_points, start);
					std::sort(tls_horizontal_capsule_points.begin(), tls_horizontal_capsule_points.end(), [&](const vec3_t& a, const vec3_t& b) {
						return a.dist_to_sq(start) < b.dist_to_sq(start);
						});
					for (vec3_t& mp : tls_horizontal_capsule_points) {
						if (scan_point(mp, &current_hitbox)) {
							found_lethal = true;
							if (result.m_damage >= target_health)
								return;
						}
					}
					break;

				case hitbox_orientation::hitbox_vertical:
					tls_vertical_capsule_points.clear();
					generate_vertical_capsule_points(&current_hitbox, tls_vertical_capsule_points);
					std::sort(tls_vertical_capsule_points.begin(), tls_vertical_capsule_points.end(), [&](const vec3_t& a, const vec3_t& b) {
						return a.dist_to_sq(start) < b.dist_to_sq(start);
						});
					for (vec3_t& mp : tls_vertical_capsule_points) {
						if (scan_point(mp, &current_hitbox)) {
							found_lethal = true;
							if (result.m_damage >= target_health)
								return;
						}
					}
					break;
				}
			}

			if (found_lethal && result.m_damage >= target_health && current_hitbox.m_hit_group != HITGROUP_HEAD && current_hitbox.m_hit_group != HITGROUP_CHEST)
				break;
		}
	}

void c_ragebot::scan_player(scan_result_t& result, cached_player_t::lag_compensation_data_t* lagcomp_ctx, c_penetration::player_context_t* penetration_ctx) {

	if (!g_ctx || !lagcomp_ctx)
		return;

	const float newest_sim = lagcomp_ctx->m_lag_records.newest().m_simulation_time;

	if (!newest_sim)
		return;

	int records_scanned = 0;
	const int max_records = 4;

	// newest records first: stale history aims at positions the server
	// has already discarded, so never prefer it over fresh data.
	const size_t record_count = lagcomp_ctx->m_lag_records.size();
	for (size_t idx = record_count; idx-- > 0; ) {
		if (records_scanned >= max_records)
			break;

		auto& record = lagcomp_ctx->m_lag_records[idx];

		scan_result_t current = {};
		const bool is_front = (record.m_simulation_time == newest_sim);

		// animfix: rebuild hitboxes from bones re-posed at the record
		// transform. falls back to stored data if re-posing fails.
		lag_record_t* scan_target = &record;
		lag_record_t reposed = record;
		if (repose_record_for_scan(&record, &reposed, *penetration_ctx))
			scan_target = &reposed;

		scan_record(current, scan_target, penetration_ctx, is_front, g_ctx->m_shoot_position);
		if (!current.valid_target() && scan_target != &record) {
			// re-posed bones found nothing: rescan the stored record data
			// so the animfix can never shoot worse than the old path.
			scan_record(current, &record, penetration_ctx, is_front, g_ctx->m_shoot_position);
		}
		// the result must reference the persistent record: the reposed copy
		// lives on this stack frame, lagcomp bypass reads it much later.
		current.m_record = &record;
		current.m_history_delta = newest_sim - record.m_simulation_time;

		result = result.compare_player_internal(current);

		if (result.valid_target() && result.m_record && result.m_record->m_pawn) {
			int target_health = result.m_record->m_pawn->m_iHealth();
			if (result.m_damage >= target_health && result.m_hitbox && result.m_hitbox->m_hit_group == HITGROUP_HEAD)
				break;
		}

		records_scanned++;
	}
}

void c_ragebot::select_best_target(std::vector<scan_result_t>& results, bool found_kill) {
	vec2_t screen_center(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f);
	const scan_result_t* best_result = nullptr;
	float best_screen_distance = FLT_MAX;
	float best_world_distance = FLT_MAX;

	for (size_t i = 0; i < results.size(); ++i) {
		if (!results[i].valid_target() || !results[i].m_record)
			continue;

		if (found_kill && results[i].m_record && results[i].m_record->m_pawn) {
			int target_health = results[i].m_record->m_pawn->m_iHealth();
			if (results[i].m_damage >= target_health) {
				m_data.m_best_target = results[i];
				return;
			}
		}

		vec3_t target_pos = results[i].m_record->m_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
		vec2_t screen_pos;
		float screen_distance = FLT_MAX;
		float world_distance = results[i].m_world_dist;

		if (g_math->world_to_screen(target_pos, screen_pos))
			screen_distance = screen_pos.dist_to(screen_center);

		if (screen_distance < best_screen_distance ||
			(screen_distance == FLT_MAX && world_distance < best_world_distance)) {
			best_screen_distance = screen_distance;
			best_world_distance = world_distance;
			best_result = &results[i];
		}
	}

	if (best_result)
		m_data.m_best_target = *best_result;
}

void c_ragebot::player_scan() {

	m_data.m_best_target = { };

	auto& players = g_entity_cache->m_players;
	if (players.empty())
		return;

	tight_array<scan_result_t, 64> results;

	tight_array<cached_player_t*, 64> valid_players;
	for (auto& player : players) {
		if (player.m_lagcomp_data.is_valid() && player.m_pawn != g_ctx->m_local_pawn) {
			valid_players.emplace_back(&player);
		}
	}

	if (valid_players.empty())
		return;

	results.resize(valid_players.size());

	const size_t total_players = valid_players.size();
	bool found_lethal_headshot = false;

	for (size_t i = 0; i < total_players; ++i) {
		if (found_lethal_headshot)
			break;

		scan_player(results[i], &valid_players[i]->m_lagcomp_data, &valid_players[i]->m_penetration_context);

		if (results[i].valid_target() && results[i].m_record && results[i].m_record->m_pawn && results[i].m_hitbox) {
			int target_health = results[i].m_record->m_pawn->m_iHealth();
			if (results[i].m_damage >= target_health && results[i].m_hitbox->m_hit_group == HITGROUP_HEAD) {
				found_lethal_headshot = true;
				m_data.m_best_target = results[i];
				return;
			}
		}
	}

	scan_result_t* best_result = nullptr;
	int best_priority = -1;
	float best_damage = -1.0f;
	float best_distance = FLT_MAX;
	float best_delta = FLT_MAX;
	bool best_lethal = false;

	for (size_t i = 0; i < total_players; ++i) {
		if (!results[i].valid_target())
			continue;

		int current_priority = results[i].m_hitbox ? fw_get_hitbox_priority(results[i].m_hitbox->m_hit_group) : -1;
		float current_distance = results[i].m_world_dist;
		int current_damage = results[i].m_damage;
		float current_delta = results[i].m_history_delta;

		bool current_lethal = false;
		if (results[i].m_record && results[i].m_record->m_pawn)
			current_lethal = current_damage >= results[i].m_record->m_pawn->m_iHealth();

		bool is_better = false;

		if (!best_result) {
			is_better = true;
		}
		else if (current_lethal != best_lethal) {
			// a guaranteed kill always beats farming damage.
			is_better = current_lethal;
		}
		else if (current_priority != best_priority) {
			is_better = current_priority > best_priority;
		}
		else if (current_damage != best_damage) {
			// freshness-aware damage: an older record only wins
			// if it hits meaningfully harder.
			const int damage_diff = current_damage - (int)best_damage;
			if (damage_diff > 10 || -damage_diff > 10)
				is_better = damage_diff > 0;
			else
				is_better = current_delta < best_delta;
		}
		else if (current_delta != best_delta) {
			is_better = current_delta < best_delta;
		}
		else if (current_distance < best_distance) {
			is_better = true;
		}

		if (is_better) {
			best_result = &results[i];
			best_priority = current_priority;
			best_damage = current_damage;
			best_distance = current_distance;
			best_delta = current_delta;
			best_lethal = current_lethal;
		}
	}

	if (best_result) {
		m_data.m_best_target = *best_result;
	}
}

bool c_ragebot::zeusbot_can_attack() {
	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller || !g_ctx->m_cmd)
		return false;

	if (!g_ctx->m_active_weapon || !g_ctx->m_active_weapon_data)
		return false;

	if (g_ctx->m_active_weapon_data->m_WeaponType() != WEAPONTYPE_TASER)
		return false;

	if (g_ctx->m_local_pawn->m_iHealth() <= 0)
		return false;

	if (!g_ctx->m_active_weapon->can_shoot(g_ctx->m_local_controller))
		return false;

	return true;
}

bool c_ragebot::zeusbot_is_enemy_in_range(c_cs_player_pawn* p_enemy, float fl_range) {
	if (!p_enemy || !g_ctx->m_local_pawn)
		return false;

	vec3_t vec_local_pos = g_ctx->m_local_pawn->get_eye_pos();
	vec3_t vec_enemy_pos = p_enemy->get_eye_pos();

	return (vec_enemy_pos - vec_local_pos).length() <= fl_range;
}

c_cs_player_pawn* c_ragebot::zeusbot_get_best_target() {
	if (!g_ctx->m_local_pawn || !g_interfaces->m_entity_system)
		return nullptr;

	c_cs_player_pawn* p_best_target = nullptr;
	float fl_best_distance = FLT_MAX;
	const int i_local_team = g_ctx->m_local_pawn->m_iTeamNum();
	vec3_t vec_local_pos = g_ctx->m_local_pawn->get_eye_pos();

	for (auto& player : g_entity_cache->m_players) {
		if (!player.check_and_update_pawn())
			continue;

		c_cs_player_pawn* p_pawn = player.m_pawn;
		if (!p_pawn)
			continue;

		if (p_pawn == g_ctx->m_local_pawn)
			continue;

		if (p_pawn->m_iTeamNum() == i_local_team)
			continue;

		if (!p_pawn->is_enemy())
			continue;

		auto p_scene_node = p_pawn->m_pGameSceneNode();
		if (!p_scene_node || p_scene_node->m_dormant())
			continue;

		if (p_pawn->m_iHealth() <= 0)
			continue;

		vec3_t vec_enemy_pos = p_pawn->get_eye_pos();
		float fl_distance = (vec_enemy_pos - vec_local_pos).length();

		if (fl_distance > 120.0f)
			continue;

		if (fl_distance < fl_best_distance) {
			fl_best_distance = fl_distance;
			p_best_target = p_pawn;
		}
	}

	return p_best_target;
}

void c_ragebot::zeusbot_run() {
	if (!GET_VAR(bool, RAGEBOT_PATH(m_taser_bot)))
		return;

	if (!zeusbot_can_attack()) {
		m_p_last_target = nullptr;
		return;
	}

	c_cs_player_pawn* p_target = zeusbot_get_best_target();
	if (!p_target) {
		m_p_last_target = nullptr;
		return;
	}

	constexpr float ZEUS_RANGE = 105.0f;

	if (!zeusbot_is_enemy_in_range(p_target, ZEUS_RANGE))
		return;

	vec3_t vec_local_pos = g_ctx->m_local_pawn->get_eye_pos();
	vec3_t vec_target_pos = p_target->get_eye_pos();

	vec3_t ang_to_target = g_math->calculate_angle(vec_local_pos, vec_target_pos);

	auto command = g_ctx->m_cmd;
	if (!command)
		return;

	int idx = command->m_pb.m_input_history_field.size() - 1;
	c_csgo_input_history_entry_pb* p_entry = (idx >= 0) ? command->m_pb.m_input_history_field[idx] : nullptr;

	if (p_entry) {
		c_msg_q_angle* p_angles = p_entry->m_view_angles;
		if (p_angles)
			p_angles->set_q_angle(ang_to_target);

		command->m_pb.set_attack1_history_index(idx);
	}
	else {
		command->m_pb.m_base_cmd->get_view_angles()->set_q_angle(ang_to_target);
		command->m_pb.set_attack1_history_index(-1);
	}

	command->m_buttons.m_value |= IN_ATTACK;
	command->m_buttons.m_value_changed |= IN_ATTACK;

	m_p_last_target = p_target;
}

void c_ragebot::zeusbot_draw_radius() {
	if (!GET_VAR(bool, RAGEBOT_PATH(m_taser_bot)))
		return;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!g_ctx->m_local_controller)
		return;

	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->m_pGameSceneNode())
		return;

	if (g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	if (!g_ctx->m_active_weapon || !g_ctx->m_active_weapon_data)
		return;

	if (g_ctx->m_active_weapon_data->m_WeaponType() != WEAPONTYPE_TASER)
		return;

	if (!g_interfaces->m_global_vars)
		return;

	constexpr float ZEUS_RANGE = 105.0f;
	vec3_t local_origin = g_ctx->m_local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	vec3_t ground_pos = local_origin;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	hellcolor color = GET_VAR(hellcolor, RAGEBOT_PATH(m_taser_bot_radius_color));

	const int num_circles = 64;
	const float circle_size = 4.0f;
	const float rotation_speed = 0.5f;

	static float rotation_angle = 0.0f;
	rotation_angle = g_interfaces->m_global_vars->m_curtime * rotation_speed;
	if (rotation_angle > A_2PI) {
		rotation_angle = std::fmod(rotation_angle, A_2PI);
	}

	for (int i = 0; i < num_circles; ++i) {
		float base_angle = (float(i) / float(num_circles)) * A_2PI;
		float angle = base_angle + rotation_angle;
		vec3_t world_pos = ground_pos;
		world_pos.x += std::cos(angle) * ZEUS_RANGE;
		world_pos.y += std::sin(angle) * ZEUS_RANGE;

		vec2_t screen_pos;
		if (g_math->world_to_screen(world_pos, screen_pos))
			draw->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), circle_size, color, 8);
	}
}

void c_ragebot::prepare_revolver() {
	if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive() || !g_ctx->m_cmd)
		return;

	if (g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() != weapon_r8)
		return;

	auto ticks = g_ctx->m_local_controller->m_nTickBase() + 1;

	if (ticks >= time_to_ticks(g_ctx->m_local_pawn->m_pWeaponServices()->m_flNextAttack())) {
		auto second_tick = ticks >= g_ctx->m_active_weapon->m_nNextPrimaryAttackTick() && ticks > g_ctx->m_active_weapon->m_nPostponeFireReadyTicks();

		if (second_tick && ticks < g_ctx->m_active_weapon->m_nNextSecondaryAttackTick())
			g_ctx->m_cmd->m_buttons.m_value |= IN_ATTACK2;
		else if (!second_tick)
			g_ctx->m_cmd->m_buttons.m_value |= IN_ATTACK;
	}

	if (!g_ctx->m_active_weapon->m_iClip1() || g_ctx->m_active_weapon->m_pReserveAmmo() > 0) {
		g_ctx->m_cmd->m_buttons.m_value &= ~IN_ATTACK;
		g_ctx->m_cmd->m_buttons.m_value |= IN_RELOAD;
	}
}

void c_ragebot::duck_peek_assist() {
	if (!GET_VAR(bool, RAGEBOT_PATH(m_duck_peek)))
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive() || !g_ctx->m_cmd)
		return;

	if (!g_ctx->m_active_weapon || !g_ctx->m_active_weapon_data)
		return;

	if (m_fired || (g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK)) {
		g_ctx->m_cmd->m_buttons.m_value |= IN_DUCK;
		return;
	}

	auto& players = g_entity_cache->m_players;
	if (players.empty()) {
		g_ctx->m_cmd->m_buttons.m_value |= IN_DUCK;
		return;
	}

	vec3_t eye_ducked = g_ctx->m_local_pawn->get_eye_pos();
	vec3_t eye_standing = eye_ducked + vec3_t(0.f, 0.f, 19.f);

	c_trace_filter filter;
	g_interfaces->m_phys2world->initialize_trace_filter(&filter, g_ctx->m_local_pawn, 0x1C300B, 3, 15);
	filter.m_ptr4 |= 2u;
	filter.m_ptr[0] |= 0x4000000000ull;

	auto can_damage = [&](const vec3_t& start, const vec3_t& point, c_cs_player_pawn* pawn) -> bool {
		vec3_t dir = point - start;
		float len = dir.length();
		if (len < 1.f)
			return false;

		dir /= len;
		vec3_t delta = dir * g_ctx->m_active_weapon_data->m_flRange();

		c_trace_data trace_data;
		g_interfaces->m_phys2world->initialize_trace_data(&trace_data);

		trace_data.m_start = start;
		trace_data.m_end = start + delta;

		g_penetration->m_trace_calls.create_trace(&trace_data, start, delta, &filter);
		g_penetration->m_trace_calls.damage_to_point(&trace_data, g_ctx->m_active_weapon_data->m_nDamage(), g_ctx->m_active_weapon_data->m_flPenetration(), g_ctx->m_active_weapon_data->m_flRangeModifier(), g_ctx->m_local_controller->m_iTeamNum());

		for (int i = 0; i < trace_data.m_surfaces_count; ++i) {
			c_trace_info& info = trace_data.m_trace_info[i];

			c_game_trace trace;
			g_interfaces->m_phys2world->setup_game_trace_info(&trace_data, &trace, info.m_distance, &trace_data.m_trace_segments_ptr[(info.m_handle.to_int() >> 16) & 0xFFFF]);

			if (trace.m_ent == pawn && info.m_damage > 0.f)
				return true;
		}

		return false;
	};

	bool should_stand = false;

	for (auto& pl : players) {
		if (should_stand)
			break;

		if (!pl.check_and_update_pawn())
			continue;

		c_cs_player_pawn* pawn = pl.m_pawn;
		if (!pawn || pawn == g_ctx->m_local_pawn || pawn->m_iHealth() <= 0)
			continue;

		auto scene = pawn->m_pGameSceneNode();
		if (!scene || scene->m_dormant())
			continue;

		vec3_t head = pawn->get_eye_pos() + vec3_t(0.f, 0.f, 3.f);
		vec3_t chest = scene->m_vecAbsOrigin() + vec3_t(0.f, 0.f, 45.f);

		vec3_t targets[] = { head, chest };

		for (auto& target : targets) {
			if (should_stand)
				break;

			for (int i = 0; i <= 20; ++i) {
				vec3_t eye = eye_ducked.lerp(eye_standing, i / 20.f);

				if (can_damage(eye, target, pawn)) {
					should_stand = true;
					break;
				}
			}
		}
	}

	if (should_stand) g_ctx->m_cmd->m_buttons.m_value &= ~IN_DUCK;
	else g_ctx->m_cmd->m_buttons.m_value |= IN_DUCK;
}

void c_ragebot::run() {

	zeusbot_run();

	if (!m_config.m_enabled || m_bad_weapon)
		return m_data.reset();

	bool just_fired = m_fired;
	m_fired = false;

	if (!just_fired)
		player_scan();

	duck_peek_assist();

	if (g_ctx->m_cmd->m_sequence_number == m_data.m_last_scan)
		return;

	m_data.m_last_scan = g_ctx->m_cmd->m_sequence_number;

	// clear the per-tick decisions so a stale stop/accurate from a previous tick
	// doesn't survive into a tick where the target is gone (e.g. enemy died).
	// create_move feeds m_wants_stop into auto_stop unconditionally, so leaving it
	// true on an early return below keeps stopping us after the target is dead.
	m_data.m_wants_stop = false;
	m_data.m_accurate = false;

	if (!g_ctx->m_active_weapon->can_shoot(g_ctx->m_local_controller))
		return;

	bool should_extrap = g_ctx->m_local_pawn->m_vecAbsVelocity().length_2d() > g_ctx->m_active_weapon->get_max_speed() * 0.34f;
	g_ctx->m_extrapolated_shoot_position = should_extrap ? g_movement->simulate_movement(g_ctx->m_shoot_position) : g_ctx->m_shoot_position;

	if (!m_data.m_best_target.valid_target())
		return;

	m_data.m_accurate = is_accurate();
	m_data.m_wants_stop = should_stop();
	if (m_data.m_wants_stop && !m_data.m_accurate && m_data.m_best_target.valid_target()) {
		// planned stop: only halt movement if the stopped shot next tick
		// is actually viable (velocity-style). otherwise keep moving.
		m_data.m_wants_stop = planned_shot_viable();
	}
	m_data.m_wants_lagcomp_bypass = !g_lagcomp->wants_lag_compensation_on_entity(m_data.m_best_target.m_record);
}

void c_ragebot::handle_attacking() {

	if (!m_data.m_best_target.valid_target())
		return;

	m_data.m_best_target.m_angle = g_math->calculate_angle(g_ctx->m_shoot_position, m_data.m_best_target.m_point) - g_ctx->m_local_pawn->get_weapon_recoil();

	if (m_data.m_wants_stop)
		g_movement->stop_movement();

	const bool is_sniper = g_ctx->m_active_weapon_data && g_ctx->m_active_weapon_data->m_WeaponType() == WEAPONTYPE_SNIPER_RIFLE;
	const bool is_scoped = g_ctx->m_local_pawn->m_bIsScoped();

	if (GET_VAR(bool, RAGEBOT_PATH(m_auto_scope)) && is_sniper && !is_scoped) {
		g_ctx->m_cmd->m_buttons.m_value |= IN_ATTACK2;
		g_ctx->m_cmd->m_buttons.m_value_changed |= IN_ATTACK2;
		return;
	}

	if (!m_config.m_autofire || !m_data.m_accurate)
		return;

	auto command = g_ctx->m_cmd;

	command->m_buttons.m_value |= IN_ATTACK;

	int idx = command->m_pb.m_input_history_field.size() - 1;
	command->m_pb.set_attack1_history_index(idx);

	bool use_silent = GET_VAR(bool, RAGEBOT_PATH(m_silent_aim));

	if (m_data.m_wants_lagcomp_bypass) {
		if (GET_VAR(bool, ANTIAIM_PATH(m_hide_shots))) {
			vec3_t invalid_angle(179.9f, std::remainderf(m_data.m_best_target.m_angle.y + 180.f, 360.f), 0.f);
			command->m_pb.m_base_cmd->get_view_angles()->set_q_angle(invalid_angle);
		}
		else
			command->m_pb.m_base_cmd->get_view_angles()->set_q_angle(m_data.m_best_target.m_angle);
	}
	else {
		if (!use_silent)
			command->m_pb.m_base_cmd->get_view_angles()->set_q_angle(m_data.m_best_target.m_angle);
	}
	if (!use_silent)
		g_interfaces->m_csgo_input->set_view_angle(m_data.m_best_target.m_angle);

	for (int i = 0; i < command->m_pb.m_input_history_field.size(); ++i) {
		c_csgo_input_history_entry_pb* entry = command->m_pb.m_input_history_field[i];
		if (!entry)
			continue;

		c_msg_q_angle* angles = entry->m_view_angles;
		if (angles)
			angles->set_q_angle(m_data.m_best_target.m_angle);
	}

	if (m_data.m_best_target.m_record && m_data.m_best_target.m_record->m_pawn && m_data.m_best_target.m_hitbox) {
		c_cs_player_controller* target_controller = g_interfaces->m_entity_system->get_base_entity(m_data.m_best_target.m_record->m_pawn->m_hController().get_entry_index()).as<c_cs_player_controller*>();
		if (target_controller) {
			shotlogger::shot_information_t shot_info{};
			shot_info.m_p_record = m_data.m_best_target.m_record;
			shot_info.m_p_hitbox = m_data.m_best_target.m_hitbox;
			shot_info.m_i_predicted_damage = m_data.m_best_target.m_damage;
			shot_info.m_i_hit_chance = m_config.m_hitchance;
			shot_info.m_v_start = g_ctx->m_shoot_position;
			shot_info.m_v_end = m_data.m_best_target.m_point;
			shot_info.m_fl_time = g_interfaces->m_global_vars->m_curtime;
			shot_info.m_str_player_name = target_controller->m_sSanitizedPlayerName();

			if (g_ctx->m_local_controller) {
				float current_time = g_interfaces->m_global_vars->m_curtime;
				float record_time = m_data.m_best_target.m_record->m_simulation_time;
				float time_diff = current_time - record_time;
				static auto sv_maxunlag = g_interfaces->m_engine_convar->find_by_name("sv_maxunlag");
				float max_unlag = sv_maxunlag ? sv_maxunlag->get_float() : 0.2f;

				if (time_diff > 0.0f && time_diff <= max_unlag) {
					int tick_rate = static_cast<int>(1.0f / interval_per_tick);
					shot_info.m_n_backtrack_tick = static_cast<int>(time_diff * tick_rate);
				}
				else if (time_diff < 0.0f) {
					int tick_rate = static_cast<int>(1.0f / interval_per_tick);
					shot_info.m_n_backtrack_tick = static_cast<int>(time_diff * tick_rate);
				}
				else
					shot_info.m_n_backtrack_tick = 0;
			}

			shotlogger::register_shot(shot_info);
		}
	}

	m_fired = true;
}