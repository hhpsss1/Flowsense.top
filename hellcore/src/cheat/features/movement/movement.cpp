#include "movement.h"
#include "../engine_prediction/engine_prediction.h"
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/trace.h>
#include <sdk/constants.h>
#include "cheat/features/visuals/overlay_features.h"
#include <cheat/features/ragebot/ragebot.h>
#include <sdk/interfaces/particle_system.h>
#include <cheat/features/visuals/particle_manager.h>

void c_movement::bunny_hop() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_bunny_hop)))
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	static auto sv_autobunnyhopping = g_interfaces->m_engine_convar->find_by_name(xx("sv_autobunnyhopping"));
	if (sv_autobunnyhopping->get_bool())
		return;

	if (!(g_ctx->m_cmd->m_buttons.m_value & IN_JUMP))
		return;

	if (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND)
		g_ctx->m_cmd->m_buttons.set_button_state(IN_JUMP, in_button_state_t::e_button_state::in_button_up_down);
	else
		g_ctx->m_cmd->m_buttons.set_button_state(IN_JUMP, in_button_state_t::e_button_state::in_button_up);
}

void fix_cmd_buttons()
{
	if (g_prediction->is_in_air())
		return;

	if (g_ctx->m_cmd->m_pb.m_base_cmd->m_forward_move > 0.0f)
		g_ctx->m_cmd->m_buttons.set_button_state(IN_FORWARD, in_button_state_t::e_button_state::in_button_down);
	else if (g_ctx->m_cmd->m_pb.m_base_cmd->m_forward_move < 0.0f)
		g_ctx->m_cmd->m_buttons.set_button_state(IN_BACK, in_button_state_t::e_button_state::in_button_down);

	if (g_ctx->m_cmd->m_pb.m_base_cmd->m_side_move > 0.0f)
		g_ctx->m_cmd->m_buttons.set_button_state(IN_MOVERIGHT, in_button_state_t::e_button_state::in_button_down);
	else if (g_ctx->m_cmd->m_pb.m_base_cmd->m_side_move < 0.0f)
		g_ctx->m_cmd->m_buttons.set_button_state(IN_MOVELEFT, in_button_state_t::e_button_state::in_button_down);
}

void c_movement::edge_jump() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_edge_jump)))
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	if (g_ctx->m_cmd->m_buttons.m_value & IN_JUMP)
		return;

	bool on_ground = (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND) != 0;

	if (!on_ground)
		return;

	vec3_t origin = g_ctx->m_local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	vec3_t velocity = g_ctx->m_local_pawn->m_vecAbsVelocity();

	vec3_t predicted = origin + velocity * interval_per_tick;

	vec3_t start = predicted; start.z += 4.f;
	vec3_t end = predicted;   end.z -= 64.f;

	c_ray ray{};
	c_game_trace trace{};
	c_trace_filter filter{ MASK_PLAYERSOLID, g_ctx->m_local_pawn, nullptr, 4 };

	g_interfaces->m_phys2world->trace_shape(&ray, &start, &end, &filter, &trace);


	if (trace.m_fraction >= 1.f) {
		g_ctx->m_cmd->m_buttons.set_button_state(IN_JUMP, in_button_state_t::e_button_state::in_button_up_down);
	}
}



void c_movement::jump_bug() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_jump_bug)))
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	if (g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_LADDER || g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_NOCLIP)
		return;

	static bool was_air = false;
	static bool jb_ducked = false;

	const bool on_ground = (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND) != 0;

	if (!on_ground) {
		was_air = true;

		// falling fast and about to land this tick: duck the landing tick (botox jumpbug).
		const vec3_t velocity = g_ctx->m_local_pawn->m_vecAbsVelocity();
		if (velocity.z < -500.f) {
			vec3_t origin = g_ctx->m_local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
			vec3_t predicted = origin + velocity * interval_per_tick;

			vec3_t start = predicted; start.z += 4.f;
			vec3_t end = predicted;   end.z -= 16.f;

			c_ray ray{};
			c_game_trace trace{};
			c_trace_filter filter{ MASK_PLAYERSOLID, g_ctx->m_local_pawn, nullptr, 4 };

			g_interfaces->m_phys2world->trace_shape(&ray, &start, &end, &filter, &trace);

			if (trace.m_fraction < 1.f) {
				g_ctx->m_cmd->m_buttons.set_button_state(IN_DUCK, in_button_state_t::e_button_state::in_button_down);
				jb_ducked = true;
				return;
			}
		}

		jb_ducked = false;
		return;
	}

	// just landed on a jumpbugged tick: release duck and tap jump, no hard landing.
	if (was_air && jb_ducked) {
		g_ctx->m_cmd->m_buttons.set_button_state(IN_DUCK, in_button_state_t::e_button_state::in_button_up);
		g_ctx->m_cmd->m_buttons.set_button_state(IN_JUMP, in_button_state_t::e_button_state::in_button_up_down);
	}

	was_air = false;
	jb_ducked = false;
}

void c_movement::long_jump() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_long_jump)))
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	if (g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_LADDER || g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_NOCLIP)
		return;

	static int air_ticks = 0;

	const bool on_ground = (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND) != 0;
	if (on_ground) {
		air_ticks = 0;
		return;
	}

	// botox longjump: hold duck for the first ticks after leaving the ground.
	if (air_ticks < 3) {
		g_ctx->m_cmd->m_buttons.set_button_state(IN_DUCK, in_button_state_t::e_button_state::in_button_down);
	}

	air_ticks++;
}

void c_movement::ramp_boost() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_ramp_boost)))
		return;

	//auto pawn = g_ctx->m_local_pawn;
	//if (!pawn || pawn->m_iHealth() <= 0)
	//	return;

	//if (pawn->m_nActualMoveType() == MOVETYPE_LADDER || pawn->m_nActualMoveType() == MOVETYPE_NOCLIP)
	//	return;

	//vec3_t velocity = pawn->m_vecVelocity();
	//float speed = velocity.length_2d();

	//if (speed < 100.0f)
	//	return;

	//vec3_t origin = pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	//vec3_t mins = pawn->m_pCollision()->m_vecMins();
	//vec3_t maxs = pawn->m_pCollision()->m_vecMaxs();

	//vec3_t start = origin;
	//vec3_t end = origin - vec3_t(0, 0, 16.0f);

	//c_trace_filter filter;
	//g_interfaces->m_phys2world->init_player_movement_trace_filter(&filter, pawn, pawn->m_pCollision()->collision_mask(), 8);

	//c_game_trace trace;
	//g_interfaces->m_phys2world->init_game_trace(&trace);

	//bbox_collision_t bounds = { mins, maxs };

	//if (!g_interfaces->m_phys2world->trace_player_bbox(&start, &end, &bounds, &filter, &trace))
	//	return;

	//if (trace.m_fraction >= 1.0f || !trace.m_ent)
	//	return;

	//vec3_t normal = trace.m_hit_normal;
	//bool is_ramp = normal.z < 0.7f && normal.z > 0.1f;

	//if (!is_ramp)
	//	return;

	//float dot = normal.dot(vec3_t(0, 0, 1));
	//float angle = acosf(dot) * (180.0f / A_PI);

	//if (angle < 20.0f || angle > 70.0f)
	//	return;

	//g_ctx->m_cmd->m_buttons.m_value |= IN_DUCK;

	//vec3_t vel_2d = vec3_t(velocity.x, velocity.y, 0);
	//vel_2d.normalize();

	//float yaw_rad = m_camera_angle.y * (A_PI / 180.0f);
	//vec3_t forward = vec3_t(cosf(yaw_rad), sinf(yaw_rad), 0);
	//vec3_t right = vec3_t(-sinf(yaw_rad), cosf(yaw_rad), 0);

	//float forward_move = vel_2d.dot(forward);
	//float side_move = vel_2d.dot(right);

	//float boost_factor = 1.0f + (angle / 90.0f) * 0.5f;

	//g_ctx->m_base->set_forward_move(forward_move * boost_factor);
	//g_ctx->m_base->set_side_move(side_move * boost_factor);
}

void c_movement::quick_stop() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_quick_stop)) || !(g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND) || (g_ctx->m_cmd->m_buttons.m_value & IN_JUMP))
		return;

	bool holding_movement = g_ctx->m_cmd->m_buttons.m_value & IN_FORWARD || g_ctx->m_cmd->m_buttons.m_value & IN_BACK || g_ctx->m_cmd->m_buttons.m_value & IN_MOVELEFT || g_ctx->m_cmd->m_buttons.m_value & IN_MOVERIGHT;

	if (holding_movement)
		return;

	auto velocity = g_ctx->m_local_pawn->m_vecVelocity();
	auto speed = velocity.length_2d();

	if (speed < 15.f)
		return;

	float forwardmove = 0.f;
	float sidemove = 0.f;

	float velocity_angle = atan2f(velocity.y, velocity.x) * 180 / A_PI;

	float relative_angle = velocity_angle - m_camera_angle.y;

	forwardmove = cosf(relative_angle * A_PI / 180);
	sidemove = sinf(relative_angle * A_PI / 180);

	g_ctx->m_base->set_forward_move(-forwardmove);
	g_ctx->m_base->set_side_move(-sidemove);
}

auto rotate_movement = [](float target_yaw) {
	auto base_cmd = g_ctx->m_base;
	float rad_rotation = DEG2RAD(base_cmd->get_view_angles()->m_ang_value.y - target_yaw);

	float new_forward_move = cosf(rad_rotation) * base_cmd->m_forward_move - sinf(rad_rotation) * base_cmd->m_side_move;
	float new_side_move = sinf(rad_rotation) * base_cmd->m_forward_move + cosf(rad_rotation) * base_cmd->m_side_move;

	base_cmd->set_forward_move(std::clamp(new_forward_move, -1.f, 1.f));
	base_cmd->set_side_move(std::clamp(new_side_move * -1.f, -1.f, 1.f));
	};

void c_movement::stop_movement() {

	if (!g_prediction->is_definitely_on_ground())
		return;

	auto local_pawn = g_ctx->m_local_pawn;
	c_player_movement_services* move_service = local_pawn->m_pMovementServices();

	c_base_user_cmd_pb* base = g_ctx->m_base;

	c_user_cmd* last_command = g_ctx->m_cmd_manager->get_cmd_by_sequence(g_ctx->m_cmd->m_sequence_number - 1);
    if (!last_command || !last_command->m_pb.m_base_cmd)
        return;
	c_base_user_cmd_pb* last_base = last_command->m_pb.m_base_cmd;

	vec3_t last_move{ last_base->m_forward_move, last_base->m_side_move, last_base->m_up_move };

	const int ticks_to_simulate = std::clamp(12 - base->m_subtick_moves_field.size(), 0, 12);
    if (ticks_to_simulate == 0)
        return;
	const float frame_time = interval_per_tick / static_cast<float>(ticks_to_simulate);

	for (int i = 0; i < ticks_to_simulate; ++i) {
		const auto subtick = base->create_subtick_move();
		if (!subtick)
			continue;

		subtick->set_button(0);
		subtick->set_pressed(false);
		subtick->set_when(static_cast<float>(i) / static_cast<float>(ticks_to_simulate));
		subtick->set_analog_forward_delta(base->m_forward_move - last_move.x);
		subtick->set_analog_left_delta(base->m_side_move - last_move.y);

		last_move.x += subtick->m_analog_forward_delta;
		last_move.y += subtick->m_analog_left_delta;
	}
}

void c_movement::slow_walk() {
	auto* cmd = g_ctx->m_cmd;
	auto* base_cmd = g_ctx->m_base;

	if (!cmd || !base_cmd)
		return;

	if (!GET_VAR(bool, MISC_PATH(m_enabled_slow_walk)))
		return;

	int percent = GET_VAR(int, MISC_PATH(m_slow_walk_percent));
	percent = std::clamp(percent, 1, 100);

	float factor = percent / 100.0f;

	float original_forward = base_cmd->m_forward_move;
	float original_side = base_cmd->m_side_move;

	base_cmd->m_forward_move *= factor;
	base_cmd->m_side_move *= factor;

	auto& buttons = cmd->m_buttons.m_value;
	buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

	if (base_cmd->m_forward_move > 0.0f) buttons |= IN_FORWARD;
	if (base_cmd->m_forward_move < 0.0f) buttons |= IN_BACK;
	if (base_cmd->m_side_move > 0.0f) buttons |= IN_MOVERIGHT;
	if (base_cmd->m_side_move < 0.0f) buttons |= IN_MOVELEFT;

	base_cmd->set_bits(0x20u | 0x40u | 0x80u);
}

void c_movement::correct_movement() {
	vec3_t vecForward, vecRight, vecUp;
	g_math->angle_vectors(g_ctx->m_base->m_view_angles->m_ang_value, &vecForward, &vecRight, &vecUp);

	vec3_t forward_2d = vec3_t(vecForward.x, vecForward.y, 0.0f);
	vec3_t right_2d = vec3_t(vecRight.x, vecRight.y, 0.0f);

	vecUp.z = 0.0f;
	vecUp.normalize_place();

	if (forward_2d.length_2d() < 0.001f) {
		float yaw = g_ctx->m_base->m_view_angles->m_ang_value.y;
		float rad = DEG2RAD(yaw);
		forward_2d.x = cos(rad);
		forward_2d.y = sin(rad);
		forward_2d.z = 0.0f;

		right_2d.x = -sin(rad);
		right_2d.y = cos(rad);
		right_2d.z = 0.0f;
	}

	forward_2d.normalize_place();
	right_2d.normalize_place();

	vec3_t vecOldForward, vecOldRight, vecOldUp;
	g_math->angle_vectors(m_camera_angle, &vecOldForward, &vecOldRight, &vecOldUp);

	vec3_t old_forward_2d = vec3_t(vecOldForward.x, vecOldForward.y, 0.0f);
	vec3_t old_right_2d = vec3_t(vecOldRight.x, vecOldRight.y, 0.0f);

	vecOldUp.z = 0.0f;
	vecOldUp.normalize_place();

	if (old_forward_2d.length_2d() < 0.001f) {
		float yaw = m_camera_angle.y;
		float rad = DEG2RAD(yaw);
		old_forward_2d.x = cos(rad);
		old_forward_2d.y = sin(rad);
		old_forward_2d.z = 0.0f;

		old_right_2d.x = -sin(rad);
		old_right_2d.y = cos(rad);
		old_right_2d.z = 0.0f;
	}

	old_forward_2d.normalize_place();
	old_right_2d.normalize_place();

	const vec3_t vecScaledForward = forward_2d * g_ctx->m_base->m_forward_move;
	const vec3_t vecScaledRight = right_2d * g_ctx->m_base->m_side_move;
	const vec3_t vecScaledUp = vecUp * g_ctx->m_base->m_up_move;

	float flFixedForwardMove = old_forward_2d.dot(vecScaledRight) + old_forward_2d.dot(vecScaledForward) + vecOldUp.dot(vecScaledUp);
	float flFixedSideMove = old_right_2d.dot(vecScaledRight) + old_right_2d.dot(vecScaledForward) + vecOldUp.dot(vecScaledUp);
	float flFixedUpMove = g_ctx->m_base->m_up_move;

	g_ctx->m_base->set_forward_move(std::clamp(flFixedForwardMove, -1.0f, 1.0f));
	g_ctx->m_base->set_side_move(std::clamp(flFixedSideMove, -1.0f, 1.0f));
	g_ctx->m_base->set_up_move(std::clamp(flFixedUpMove, -1.0f, 1.0f));

	fix_cmd_buttons();
}

void c_movement::air_strafer(vec3_t& velocity, float frame_time, vec3_t move) {
	static auto sv_airaccelerate = g_interfaces->m_engine_convar->find_by_name("sv_airaccelerate");
	static auto sv_air_max_wishspeed = g_interfaces->m_engine_convar->find_by_name("sv_air_max_wishspeed");

	auto base_cmd = g_ctx->m_base;
	auto local_pawn = g_ctx->m_local_pawn;
	auto move_service = local_pawn->m_pMovementServices();

	static uint64_t last_pressed = 0;
	static uint64_t last_buttons = 0;

	uint64_t current_buttons = g_ctx->m_cmd->m_buttons.m_value;

	auto check_button = [&](uint64_t button) {
		if (current_buttons & button && (!(last_buttons & button)
			|| (button & IN_MOVELEFT && !(last_pressed & IN_MOVERIGHT))
			|| (button & IN_MOVERIGHT && !(last_pressed & IN_MOVELEFT))
			|| (button & IN_FORWARD && !(last_pressed & IN_BACK))
			|| (button & IN_BACK && !(last_pressed & IN_FORWARD)))) {

			// easy strafe, can be by checkbox
			if (button & IN_MOVELEFT)
				last_pressed &= ~IN_MOVERIGHT;
			else if (button & IN_MOVERIGHT)
				last_pressed &= ~IN_MOVELEFT;
			else if (button & IN_FORWARD)
				last_pressed &= ~IN_BACK;
			else if (button & IN_BACK)
				last_pressed &= ~IN_FORWARD;
			// end easy strafe

			last_pressed |= button;
		}
		else if (!(current_buttons & button))
			last_pressed &= ~button;
		};

	check_button(IN_MOVELEFT);
	check_button(IN_MOVERIGHT);
	check_button(IN_FORWARD);
	check_button(IN_BACK);

	last_buttons = current_buttons;

	auto move_type = local_pawn->m_nActualMoveType();
	if (move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP || g_prediction->is_probably_on_ground() || wants_stop)
		return;

	auto velocity_angle = RAD2DEG(std::atan2f(velocity.y, velocity.x));

	if (velocity_angle < 0.0f)
		velocity_angle += 360.0f;

	if (velocity_angle < 0.0f)
		velocity_angle += 360.0f;

	velocity_angle -= floorf(velocity_angle / 360.0f + 0.5f) * 360.0f;
	float speed = velocity.length_2d();

	float offset = 0.f;

	if (last_pressed & IN_MOVELEFT)
		offset = 90.f;
	if (last_pressed & IN_MOVERIGHT)
		offset = -90.f;

	if (last_pressed & IN_FORWARD)
		offset *= 0.5f;
	else if (last_pressed & IN_BACK)
		offset = -offset * 0.5f + 180.f;

	float yaw = g_math->normalize_float(m_camera_angle.y);
	yaw += offset;

	base_cmd->set_forward_move(0.f);
	base_cmd->set_side_move(0.f);

	float ideal = std::clamp(
		RAD2DEG(
			atanf(
				std::fmax(15.f, sv_air_max_wishspeed->get_float() - (
					speed *
					sv_airaccelerate->get_float() *
					g_ctx->m_local_pawn->m_pMovementServices()->m_flSurfaceFriction() *
					frame_time
					)) / speed
			)
		),
		0.f,
		45.f
	);

	const auto correct = std::clamp(100.f * (1.0f / frame_time), 50.f, 200.f);

	float velocity_delta = g_math->normalize_float(yaw - velocity_angle);
	static bool side_switch = false;
	side_switch = !side_switch;

	if (-correct <= velocity_delta || speed <= 80.f) {
		if (side_switch) {
			yaw -= ideal;
			g_ctx->m_base->set_side_move(-1.f);
		}
		else {
			yaw += ideal;
			g_ctx->m_base->set_side_move(1.f);
		}
		rotate_movement(g_math->normalize_float(yaw));
	}
	else {
		yaw = velocity_angle - correct;
		g_ctx->m_base->set_side_move(1.f);

		rotate_movement(g_math->normalize_float(yaw));
	}

	//if ((fabsf(velocity_delta) > 170.f || velocity_delta > ideal) && speed > 80.f) {
	//	yaw = ideal + velocity_angle;
	//	base_cmd->set_side_move(-1.f);
	//}
	//else {
	//	if (-ideal <= velocity_delta || speed <= 80.f) {
	//		if (side_switch) {
	//			yaw -= ideal;
	//			base_cmd->set_side_move(-1.f);
	//		}
	//		else {
	//			yaw += ideal;
	//			base_cmd->set_side_move(1.f);
	//		}
	//	}
	//	else {
	//		yaw = velocity_angle - ideal;
	//		base_cmd->set_side_move(1.f);
	//	}
	//}

	//rotate_movement(g_math->normalize_float(yaw));
}

void c_movement::air_accelerate(vec3_t angles, vec3_t & velocity, vec3_t & move, float frame_time) {
	static auto sv_airaccelerate = g_interfaces->m_engine_convar->find_by_name(xx("sv_airaccelerate"));
	static auto sv_air_max_wishspeed = g_interfaces->m_engine_convar->find_by_name(xx("sv_air_max_wishspeed"));

	vec3_t forward, right, up;
	angles.to_directions(&forward, &right, &up);

	forward.z = 0.0f;
	right.z = 0.0f;
	forward.normalize_place();
	right.normalize_place();

	vec3_t wishDir;
	wishDir.x = (forward.x * move.x) + (right.x * move.y);
	wishDir.y = (forward.y * move.x) + (right.y * move.y);
	wishDir.z = 0.0f;

	float wishSpeed = wishDir.normalize_place();
	wishSpeed = std::fminf(wishSpeed, sv_air_max_wishspeed->get_float());

	float currentSpeed = velocity.dot(wishDir);
	float speedAdd = wishSpeed - currentSpeed;

	if (speedAdd <= 0.0f)
		return;

	float accelLimit = sv_airaccelerate->get_float() * wishSpeed * frame_time;

	if (accelLimit > speedAdd)
		accelLimit = speedAdd;

	velocity.x += wishDir.x * accelLimit;
	velocity.y += wishDir.y * accelLimit;

	if (g_ctx->m_local_pawn->m_pMovementServices()->m_flStamina() > 70.0f)
		velocity.z += 0.3f * frame_time;
}

void c_movement::subtick_air_strafer(bool stop) {
	if (stop) return;
	if (!GET_VAR(bool, MISC_PATH(m_enabled_autostrafe)))
		return;

	c_cs_player_pawn* local_pawn = g_ctx->m_local_pawn;
	auto move_type = local_pawn->m_nActualMoveType();

	if (move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP || g_prediction->is_probably_on_ground())
		return;

	const uint64_t move_keys = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
	if (!(g_ctx->m_cmd->m_buttons.m_value & move_keys))
		return;

	c_player_movement_services* move_service = local_pawn->m_pMovementServices();
	c_base_user_cmd_pb* base = g_ctx->m_base;

	c_user_cmd* last_command = g_ctx->m_cmd_manager->get_cmd_by_sequence(g_ctx->m_cmd->m_sequence_number - 1);
    if (!last_command || !last_command->m_pb.m_base_cmd)
        return;
	c_base_user_cmd_pb* last_base = last_command->m_pb.m_base_cmd;

	vec3_t last_move = { last_base->m_forward_move, last_base->m_side_move, last_base->m_up_move };

	int ticks_to_simulate = std::clamp(32 - base->m_subtick_moves_field.size(), 0, 32);
    if (ticks_to_simulate == 0)
        return;
	float subtick_frame_time = interval_per_tick / (float)ticks_to_simulate;

	vec3_t abs_velocity = local_pawn->m_vecAbsVelocity();

	vec3_t vec_last = base->get_view_angles()->m_ang_value;

	for (int i = 0; i < ticks_to_simulate; ++i) {
		air_accelerate(base->get_view_angles()->m_ang_value, abs_velocity, last_move, subtick_frame_time);
		air_strafer(abs_velocity, subtick_frame_time, last_move);

		subtick_move_step_t* subtick = base->create_subtick_move();
		if (!subtick)
			return;

		subtick->set_when((float)i / (float)ticks_to_simulate);

		subtick->set_analog_forward_delta(base->m_forward_move - last_move.x);
		subtick->set_analog_left_delta(base->m_side_move - last_move.y);

		subtick->set_analog_pitch_delta(base->get_view_angles()->m_ang_value.x - vec_last.x);
		subtick->set_analog_yaw_delta(base->get_view_angles()->m_ang_value.y - vec_last.y);

		vec_last = base->get_view_angles()->m_ang_value;

		last_move.x += subtick->m_analog_forward_delta;
		last_move.y += subtick->m_analog_left_delta;
	}

	auto& buttons = g_ctx->m_cmd->m_buttons.m_value;
	bool player_holds_side = (buttons & (IN_MOVELEFT | IN_MOVERIGHT)) != 0;
	if (!player_holds_side) {
		if (base->m_side_move > 0.0f)
			buttons |= IN_MOVERIGHT;
		else if (base->m_side_move < 0.0f)
			buttons |= IN_MOVELEFT;
	}

	bool player_holds_fwd = (buttons & (IN_FORWARD | IN_BACK)) != 0;
	if (!player_holds_fwd) {
		if (base->m_forward_move > 0.0f)
			buttons |= IN_FORWARD;
		else if (base->m_forward_move < 0.0f)
			buttons |= IN_BACK;
	}
}



void c_movement::auto_peek() {
	auto cmd = g_ctx->m_cmd;
	auto base_cmd = g_ctx->m_base;
	auto local = g_ctx->m_local_pawn;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!cmd || !base_cmd || !local || !local->is_alive() || !g_interfaces->m_global_vars) {
		g_auto_peek_returning = false;
		g_auto_peek_render_state.retreating = false;
		return;
	}

	static bool was_enabled = false;
	bool is_enabled = GET_VAR(bool, MISC_PATH(m_enabled_auto_peek));

	if (!is_enabled) {
		if (was_enabled) {
			g_particle_mgr->clean_autopeek_effect();
		}
		g_auto_peek_returning = false;
		g_auto_peek_render_state.retreating = false;
		g_auto_peek_origin.clear();
		was_enabled = false;
		return;
	}

	const uint64_t movement_keys = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
	bool retreat = !(cmd->m_buttons.m_value & movement_keys);

	if (g_ctx->m_local_controller->is_firing(g_ctx->m_active_weapon) && !g_auto_peek_returning) {
		g_auto_peek_returning = true;
		g_auto_peek_render_state.retreating = true;
	}

	if (g_auto_peek_origin.empty()) {
		vec3_t origin = local->m_pGameSceneNode()->m_vecAbsOrigin();
		g_auto_peek_origin.push_back(origin);
		g_auto_peek_render_state.origin = origin;
		g_auto_peek_render_state.active = true;
		g_auto_peek_render_state.retreating = false;
		
		hellcolor color = GET_VAR(hellcolor, MISC_PATH(m_auto_peek_color));
		g_particle_mgr->create_autopeek_effect(origin + vec3_t(0, 0, 2.f), color);
	}

	was_enabled = true;

	if (g_auto_peek_returning) {
		if (g_auto_peek_origin.empty()) {
			g_auto_peek_returning = false;
			g_auto_peek_render_state.retreating = false;
			return;
		}

		vec3_t current_origin = local->m_pGameSceneNode()->m_vecAbsOrigin();
		vec3_t delta = g_auto_peek_origin.back() - current_origin;

		if (delta.length_2d_sqr() > 0.f && delta.length_2d_sqr() < 225.f) {
			g_auto_peek_returning = false;
			g_auto_peek_render_state.retreating = false;
			return;
		}

		vec2_t dir = { delta.x, delta.y };
		dir.normalize();

		auto view_angles = base_cmd->get_view_angles()->m_ang_value;
		float yaw_rad = DEG2RAD(view_angles.y);
		float cos_yaw = cosf(yaw_rad);
		float sin_yaw = sinf(yaw_rad);

		vec2_t forward = { cos_yaw, sin_yaw };
		vec2_t right = { -sin_yaw, cos_yaw };

		float forward_move = forward.dot(dir);
		float side_move = right.dot(dir);

		base_cmd->set_forward_move(std::clamp(forward_move, -1.f, 1.f));
		base_cmd->set_side_move(std::clamp(side_move, -1.f, 1.f));
		fix_cmd_buttons();
	}
}

void c_movement::jumpscout(bool want) {
	wants_stop = want;

	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	if (weapon_accuracy_nospread && weapon_accuracy_nospread->get_bool())
		return;

	if (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND)
		return;

	if (!(g_ctx->m_cmd->m_buttons.m_value & IN_JUMP))
		return;

	if (!wants_stop)
		return;

	vec3_t velocity = g_ctx->m_local_pawn->m_vecVelocity();

	g_ctx->m_base->set_side_move(0.0f);
	g_ctx->m_base->set_forward_move(velocity.length_2d() > 20.0f ? 1.0f : 0.0f);

	vec3_t angle = m_camera_angle;
	float yaw = g_math->calculate_angles(vec3_t(0.0f, 0.0f, 0.0f), velocity).y + 180.f;
	float rot = DEG2RAD(angle.y - yaw);

	float cosrot = cos(rot);
	float sinrot = sin(rot);

	float new_forwardmove = cosrot * g_ctx->m_base->m_forward_move - sinrot * g_ctx->m_base->m_side_move;
	float new_sidemove = sinrot * g_ctx->m_base->m_forward_move + cosrot * g_ctx->m_base->m_side_move;

	g_ctx->m_base->set_forward_move(new_forwardmove);
	g_ctx->m_base->set_side_move(-new_sidemove);

	g_ctx->m_cmd->m_buttons.m_value &= ~(IN_TURNLEFT | IN_FORWARD | IN_BACK | IN_TURNRIGHT | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP);
	g_ctx->m_cmd->m_buttons.m_value_changed &= ~(IN_TURNLEFT | IN_FORWARD | IN_BACK | IN_TURNRIGHT | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP);
	g_ctx->m_cmd->m_buttons.m_value_scroll &= ~(IN_TURNLEFT | IN_FORWARD | IN_BACK | IN_TURNRIGHT | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP);
}

void c_movement::auto_stop(bool want) {
	wants_stop = want;

	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	if (weapon_accuracy_nospread && weapon_accuracy_nospread->get_bool())
		return;

	if (!wants_stop)
		return;

	auto* local_pawn = g_ctx->m_local_pawn;
	if (!local_pawn)
		return;

	vec3_t velocity = local_pawn->m_vecVelocity();
	if (velocity.length_2d() < 1.0f && (local_pawn->m_fFlags() & FL_ONGROUND))
		return;

	if (!(local_pawn->m_fFlags() & FL_ONGROUND)) {
        auto* base = g_ctx->m_base;
        auto* services = local_pawn->m_pMovementServices();
        if (!base || !services || !base->get_view_angles() || !g_ctx->m_cmd ||
            local_pawn->m_nActualMoveType() == MOVETYPE_LADDER ||
            local_pawn->m_nActualMoveType() == MOVETYPE_NOCLIP ||
            !std::isfinite(velocity.x) || !std::isfinite(velocity.y))
            return;
        const float yaw = DEG2RAD(base->get_view_angles()->m_ang_value.y);
        const float speed = velocity.length_2d();
        const float divisor = std::max(speed, 1.f);
        const float forward = -(velocity.x * cosf(yaw) + velocity.y * sinf(yaw)) / divisor;
        const float side = (velocity.y * cosf(yaw) - velocity.x * sinf(yaw)) / divisor;
        static auto air_accel = g_interfaces->m_engine_convar->find_by_name("sv_airaccelerate");
        const float max_speed = g_ctx->m_active_weapon ? g_ctx->m_active_weapon->get_max_speed() : 250.f;
        const float deceleration = air_accel ? air_accel->get_float() * max_speed *
            interval_per_tick * services->m_flSurfaceFriction() : speed;
        const float strength = deceleration > 0.f ? std::clamp(speed / deceleration, 0.f, 1.f) : 1.f;
        base->set_forward_move(forward * strength);
        base->set_side_move(side * strength);
        for (int i = 0; i < base->m_subtick_moves_field.size(); ++i) {
            auto* step = base->m_subtick_moves_field[i];
            if (!step) continue;
            step->set_analog_forward_delta(0.f);
            step->set_analog_left_delta(0.f);
        }
        const uint64_t keys = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
        auto& buttons = g_ctx->m_cmd->m_buttons;
        buttons.m_value &= ~keys;
        buttons.m_value_changed &= ~keys;
        buttons.m_value_scroll &= ~keys;
        return;
    }

	if (g_ctx->m_cmd->m_buttons.m_value & IN_JUMP)
		return;

	auto base = g_ctx->m_base;
	if (!base)
		return;

	// furryware ground autostop: counter-strafe by rotating a full forward move against
	// the velocity direction. No walk_move — directly drive forward/side move.
	base->set_side_move(0.f);
	base->set_forward_move(velocity.length_2d() > 20.0f ? 1.0f : 0.0f);

	const float yaw = g_math->calculate_angle(vec3_t(0.f, 0.f, 0.f), velocity).y + 180.0f;
	const float rotation = (m_camera_angle.y - yaw) * (A_PI / 180.0f);

	const float cos_rotation = cosf(rotation);
	const float sin_rotation = sinf(rotation);

	const float forwardmove = cos_rotation * base->m_forward_move - sin_rotation * base->m_side_move;
	const float sidemove = sin_rotation * base->m_forward_move + cos_rotation * base->m_side_move;

	base->set_forward_move(forwardmove);
	base->set_side_move(-sidemove);

	fix_cmd_buttons();
}

vec3_t c_movement::simulate_movement(vec3_t pos) {
	static auto weapon_accuracy_nospread = g_interfaces->m_engine_convar->find_by_name("weapon_accuracy_nospread");
	static auto sv_accelerate = g_interfaces->m_engine_convar->find_by_name("sv_accelerate");
	static auto sv_accelerate_use_weapon_speed = g_interfaces->m_engine_convar->find_by_name("sv_accelerate_use_weapon_speed");
	static auto sv_water_slow_amount = g_interfaces->m_engine_convar->find_by_name("sv_water_slow_amount");

	vec3_t velocity = g_ctx->m_local_pawn->m_vecVelocity();
	velocity.z = 0.f;

	if (weapon_accuracy_nospread && weapon_accuracy_nospread->get_bool())
		return pos;

	if (g_ctx->m_cmd->m_buttons.m_value & IN_JUMP)
		return pos;

	if (!(g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND))
		return pos;

	float surface_friction = g_ctx->m_local_pawn->m_pMovementServices()->m_flSurfaceFriction();

	auto& player = g_ctx->m_local_pawn;
	auto& move_service = player->m_pMovementServices();
	auto& weapon = g_ctx->m_active_weapon;
	auto& cmd = g_ctx->m_cmd;

	vec3_t extrap = pos;
	float max_speed = g_ctx->m_active_weapon->get_max_speed();
	float target_speed = max_speed * 0.34f;

	float initial_velocity = velocity.length_2d();
	if (initial_velocity <= target_speed)
		return pos;

	int max_ticks = static_cast<int>((initial_velocity / (max_speed * 0.5f)) * 10.f);
	max_ticks = std::clamp(max_ticks, 1, 32);

	for (int i = 0; i < max_ticks; i++) {
		float current_velocity = velocity.length_2d();
		if (current_velocity <= target_speed) {
			if (i > 0) {
				extrap.x -= velocity.x * interval_per_tick;
				extrap.y -= velocity.y * interval_per_tick;
			}
			break;
		}

		vec3_t wish_dir = -velocity;
		float wish_speed = wish_dir.normalize_place();
		float current_speed = velocity.dot(wish_dir);

		int button_flags = cmd->m_buttons.m_value;
		bool is_ducking = (button_flags & IN_DUCK) || move_service->m_bDucking() || (player->m_fFlags() & FL_DUCKING);
		bool holding_sprint = (button_flags & IN_SPRINT) && !is_ducking;

		float clamped_wish_speed = std::max(250.f, wish_speed);
		float effective_abs_max_speed = clamped_wish_speed;
		float acceleration_factor = 1.0f;
		bool weapon_causes_slowdown = false;

		if (sv_accelerate_use_weapon_speed->get_bool() && weapon) {
			float weapon_max_speed = weapon->get_max_speed();
			auto data = weapon->weapon_data();

			if (data && weapon->m_zoomLevel() > 0 && data->m_nZoomLevels() > 1 && (weapon_max_speed * 0.51999998f) < 110.f)
				weapon_causes_slowdown = true;

			float weapon_speed_ratio = std::min(1.0f, weapon_max_speed / 250.f);

			if ((!is_ducking && !holding_sprint) || weapon_causes_slowdown)
				acceleration_factor = weapon_speed_ratio;

			effective_abs_max_speed = clamped_wish_speed * weapon_speed_ratio;
		}

		float base_speed_for_accel = clamped_wish_speed;

		unsigned int water_level = (player->m_flWaterLevel() * 4.f) + 1.f;

		if (water_level >= 2) {
			float water_slow_amount = sv_water_slow_amount->get_float();
			effective_abs_max_speed *= water_slow_amount;

			if (is_ducking) {
				effective_abs_max_speed *= 0.34f;
				acceleration_factor = std::min(0.34f, acceleration_factor);
			}
			else {
				if (!holding_sprint)
					base_speed_for_accel = clamped_wish_speed * water_slow_amount;
			}
		}
		else {
			if (is_ducking) {
				effective_abs_max_speed *= 0.34f;
				acceleration_factor = std::min(0.34f, acceleration_factor);
			}
		}

		float final_speed_cap = base_speed_for_accel * acceleration_factor;

		float modified_sv_accelerate = sv_accelerate->get_float();
		float positive_current_speed = std::max(0.0f, current_speed);

		if (holding_sprint) {
			bool is_carrying_hostage = player->m_pHostageServices() ? player->m_pHostageServices()->m_hCarriedHostage().Get() : false;

			if (!is_carrying_hostage && !weapon_causes_slowdown)
				final_speed_cap *= 0.51999998f;

			float reduced_abs_maxspeed = effective_abs_max_speed * 0.51999998f;
			float sprint_speed_threshold = reduced_abs_maxspeed - 5.0f;

			if (positive_current_speed > sprint_speed_threshold) {
				float numerator = std::max(0.0f, positive_current_speed - sprint_speed_threshold);
				float denumerator = std::max(0.001f, reduced_abs_maxspeed - sprint_speed_threshold);
				float speed_factor_ratio = numerator / denumerator;
				float accel_reduction_factor = std::min(1.0f, std::max(0.0f, 1.0f - speed_factor_ratio));
				modified_sv_accelerate *= accel_reduction_factor;
			}
		}

		float potential_accel_gain = modified_sv_accelerate * interval_per_tick * final_speed_cap * surface_friction;
		float add_speed = -current_speed;
		float accel_amount_to_add = std::max(0.0f, std::min(potential_accel_gain, add_speed));

		velocity.x += wish_dir.x * accel_amount_to_add;
		velocity.y += wish_dir.y * accel_amount_to_add;

		extrap.x += velocity.x * interval_per_tick;
		extrap.y += velocity.y * interval_per_tick;
	}

	return extrap;
}

void c_movement::straight_throw() {
	if (!GET_VAR(bool, MISC_PATH(m_enabled_straight_throw)))
		return;

	if (g_interfaces->m_game_rules && g_interfaces->m_game_rules->m_bIsValveDS())
		return;

	if (!g_ctx->m_active_weapon)
		return;

	if (g_ctx->m_active_weapon_data->m_WeaponType() == WEAPONTYPE_KNIFE)
		return;

	if (g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_C4_EXPLOSIVE)
		return;

	if (!g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_HIGH_EXPLOSIVE_GRENADE || !g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_FLASHBANG || !g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_DECOY_GRENADE || !g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_MOLOTOV || !g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_INCENDIARY_GRENADE || !g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == WEAPON_SMOKE_GRENADE)
		return;

	vec3_t vThrowDirection{};
	g_math->angle_vectors(g_ctx->m_base->m_view_angles->m_ang_value, &vThrowDirection);

	const float flThrowStrength = std::clamp(reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon)->m_flThrowStrength(), 0.f, 1.f);
	vec3_t vNewDirection = vThrowDirection * (std::clamp(g_ctx->m_active_weapon_data->m_flThrowVelocity() * 0.9f, 15.f, 750.f) * (flThrowStrength * 0.7f + 0.3f)) + g_ctx->m_local_pawn->m_vecAbsVelocity() * 1.25f;
	vNewDirection.normalize();

	vec3_t aNewViewAngle;
	g_math->vector_angles(vNewDirection, aNewViewAngle);

	vec3_t aCurrentViewAngles = g_ctx->m_base->m_view_angles->m_ang_value;
	aCurrentViewAngles.y += (aCurrentViewAngles.y - aNewViewAngle.y);

	if (!reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon)->m_bPinPulled() || g_ctx->m_cmd->m_buttons.m_value & (IN_ATTACK | IN_ATTACK2)) {
		if (reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon)->m_fThrowTime() > 0.f || reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon)->m_bJumpThrow())
			g_ctx->m_base->m_view_angles->m_ang_value = aCurrentViewAngles;
	}
}