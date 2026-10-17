#include "client_hooks.h"

#include <sdk/interfaces/csgo_input.h>
#include <cheat/menu/menu.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <context.h>
#include <cheat/features/ragebot/ragebot.h>
#include <cheat/config/vars.h>
#include <sdk/constants.h>
#include <cheat/features/engine_prediction/engine_prediction.h>
#include <cheat/features/movement/movement.h>
#include <cheat/features/skins/skins.h>
#include <cheat/features/skins/custom_models.h>
#include <cheat/features/antiaim/antiaim.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/features/visuals/visual_events.h>
#include <cheat/features/visuals/visuals.h>

#include <sdk/interfaces/global_variables.h>
#include <sdk/interfaces/engine_pvs_manager.h>
#include <cheat/features/visuals/skybox_changer.h>
#include <cheat/features/visuals/fog_handler.h>
#include <sdk/interfaces/particle_system.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/features/visuals/chams.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/visuals/particle_manager.h>
#include <cheat/features/exploits/exploits.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/visuals/chams.h>
#include <chrono>
#include <cheat/features/misc/preserve_kill_feed.h>
#include <cheat/features/misc/reveal_equipment.h>

class stopwatch {
public:
	stopwatch() noexcept {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		s_frequency = static_cast<double>(freq.QuadPart);
	}

	void start_timer() noexcept {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		start_count = now.QuadPart;
	}

	uint64_t stop_timer() noexcept {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		auto delta = now.QuadPart - start_count;

		return static_cast<uint64_t>((delta * 1'000'000.0) / s_frequency);
	}

private:
	double  s_frequency;
	int64_t start_count{};
};
inline stopwatch g_stopwatch;
namespace {
	constexpr std::uintptr_t k_glow_offset_in_entity = 0xDE8;

	c_cs_player_pawn* get_glow_owner_pawn(c_glow_property* glow_property) {
		if (!glow_property) {
			return nullptr;
		}

		auto* owner = reinterpret_cast<c_entity_instance*>(reinterpret_cast<std::uintptr_t>(glow_property) - k_glow_offset_in_entity);
		if (!owner) {
			return nullptr;
		}

		auto* identity = owner->m_pEntity();
		if (!identity || !identity->is_valid()) {
			return nullptr;
		}

		const char* class_name = owner->get_class_name();
		if (!class_name || std::strstr(class_name, "C_CSPlayerPawn") == nullptr) {
			return nullptr;
		}

		return reinterpret_cast<c_cs_player_pawn*>(owner);
	}

	// resolve the player whose glow color a glow_property should use.
	// only the player pawn itself glows here — attached entities are handled
	// separately (see get_held_weapon_pawn).
	c_cs_player_pawn* resolve_glow_pawn(c_entity_instance* owner) {
		if (!owner)
			return nullptr;

		if (owner->is_player_pawn())
			return reinterpret_cast<c_cs_player_pawn*>(owner);

		return nullptr;
	}

	// returns the owning player if this glow_property belongs to a weapon/knife
	// held by a player. used to hide the native glow that the game propagates onto
	// held weapons (it flickered when we tried to recolor it, so instead we make it
	// fully transparent — invisible regardless of the game toggling it each frame).
	// dropped weapons have no owner and are left untouched.
	c_cs_player_pawn* get_held_weapon_pawn(c_entity_instance* owner) {
		if (!owner || owner->is_player_pawn())
			return nullptr;

		auto base = reinterpret_cast<c_base_entity*>(owner);
		if (!base->is_weapon())
			return nullptr;

		auto owner_ent = base->m_hOwnerEntity().get<c_entity_instance>();
		if (owner_ent && owner_ent->is_player_pawn())
			return reinterpret_cast<c_cs_player_pawn*>(owner_ent);

		return nullptr;
	}
}

bool c_client_hooks::mouse_input_enabled(void* a1) {
	static const auto original = g_hooks->m_client.m_mouse_input_enabled.get<decltype(&mouse_input_enabled)>();

	return g_menu->m_menu_open ? false : original(a1);
}

void c_client_hooks::is_relative_mouse_mode(void* input_system, bool active) {
	static const auto original = g_hooks->m_client.m_is_relative_mouse_mode.get<decltype(&is_relative_mouse_mode)>();

	g_menu->m_input_active = active;

	return original(input_system, g_menu->m_menu_open ? false : active);
}

void c_client_hooks::create_move(c_csgo_input* input, int slot, bool active) {
	static const auto original = g_hooks->m_client.m_create_move.get<decltype(&create_move)>();

	original(input, slot, active);
    g_interfaces->refresh_level_state();

	static bool logged_cm = false;
	if (!logged_cm) { LOG("create_move: fired"); logged_cm = true; }

    g_ctx->m_cmd_manager = nullptr;
    g_ctx->m_cmd = nullptr;
    g_ctx->m_base = nullptr;
    g_ctx->m_active_weapon = nullptr;
    g_ctx->m_active_weapon_data = nullptr;
    if (slot != 0 || !active || !g_interfaces->refresh_global_vars() || !g_interfaces->m_entity_system ||
        !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game()) {
        g_ctx->m_local_controller = nullptr;
        g_ctx->m_local_pawn = nullptr;
        return;
    }

	g_ctx->m_local_controller = g_interfaces->m_entity_system->get_player_controller(0);
	g_ctx->m_local_pawn = g_interfaces->m_entity_system->get_player_pawn(0);

    if (!g_ctx->m_local_controller || !g_ctx->m_local_pawn || !g_ctx->m_local_controller->m_bPawnIsAlive() || g_ctx->m_local_pawn->m_iHealth() <= 0 || (g_ctx->m_local_pawn->m_iTeamNum() != 2 && g_ctx->m_local_pawn->m_iTeamNum() != 3))
        return;
	if (!g_ctx->update())
		return;

	g_ctx->m_active_weapon->try_update_accuracy();

    if (!g_ctx->m_base->m_subtick_moves_field.valid() || !g_ctx->m_cmd->m_pb.m_input_history_field.valid() || !g_ctx->m_base->get_view_angles() || !g_ctx->m_base->m_in_button_state)
        return;

	g_prediction->begin();
	g_movement->m_camera_angle = input->get_view_angle();
	//g_movement->m_base_angle = g_ctx->m_base->m_view_angles->m_ang_value;

	g_antiaim->run();

	// run the ragebot scan FIRST so m_wants_stop / target are computed for THIS tick.
	// previously auto_stop used last tick's m_wants_stop, so the stop lagged one tick
	// behind the shot.
	g_ragebot->run();

	if (g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND)
		g_movement->auto_stop(g_ragebot->m_data.m_wants_stop);
	else
		g_movement->wants_stop = g_ragebot->m_data.m_wants_stop;

	g_movement->jumpscout(g_ragebot->m_data.m_wants_stop);

	g_movement->quick_stop();

	g_movement->correct_movement();
	g_movement->straight_throw();

	//g_ragebot->prepare_revolver();

	g_GrenadePrediction->grenade_release();

	g_ragebot->handle_attacking();

	{
		g_lagcomp->force_input_history();
		g_movement->auto_peek();
		g_movement->bunny_hop();
		g_movement->ramp_boost();
		g_movement->edge_jump();
		g_movement->jump_bug();
		g_movement->long_jump();
		g_movement->subtick_air_strafer(g_ragebot->m_data.m_wants_stop);
		g_movement->slow_walk();
	}
	if (!(g_ctx->m_local_pawn->m_fFlags() & FL_ONGROUND))
		g_movement->auto_stop(g_ragebot->m_data.m_wants_stop);
	g_prediction->end();
    auto* buttons = g_ctx->m_base->m_in_button_state;
    buttons->set_value(g_ctx->m_cmd->m_buttons.m_value);
    buttons->set_value_changed(g_ctx->m_cmd->m_buttons.m_value_changed);
    buttons->set_value_scroll(g_ctx->m_cmd->m_buttons.m_value_scroll);
    g_ctx->m_base->set_bits(e_protobuf_bits_t::protoslot_2);
    g_ctx->m_cmd->m_pb.set_bits(e_protobuf_bits_t::protoslot_1);
}

//void* c_client_hooks::update_global_variables(void* a1, void* new_global_vars) {
//	const auto og = g_hooks->m_client.m_update_global_vars.get<decltype(&update_global_variables)>();
//
//	g_interfaces->m_global_vars = (c_global_variables*)new_global_vars;
//
//	return og(a1, new_global_vars);
//}

std::uintptr_t __fastcall c_client_hooks::setup_map_info(std::uintptr_t map_info, void* unk)
{
	static const auto original = g_hooks->m_client.m_setup_map_info.get<decltype(&setup_map_info)>();

    const auto result = original(map_info, unk);
    if (map_info && GET_VAR(bool, VISUALS_PATH(m_enable_world_weather)) &&
        (GET_VAR(int, VISUALS_PATH(m_weather_index)) == 0 || GET_VAR(bool, VISUALS_PATH(m_enable_world_wetness)))) {
        auto* info = reinterpret_cast<c_map_info*>(map_info);
        info->m_bRainTraceToSkyEnabled() = true;
        info->m_flEnvRainStrength() = 1.f;
    }
    return result;
}

void c_client_hooks::run_command(c_player_movement_services* movement_services, c_user_cmd* cmd) {
	static const auto original = g_hooks->m_client.m_run_command.get<decltype(&run_command)>();

	return original(movement_services, cmd);
}

float c_client_hooks::get_world_fov(void* rcx) {
	static const auto original = g_hooks->m_client.m_get_world_fov.get<decltype(&get_world_fov)>();

	static float smooth_fov = 90.0f;
	static float dynamic_fov_offset = 0.0f;
	static auto last_time = std::chrono::steady_clock::now();

	float return_value = original(rcx);

	if (!g_ctx || !g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return return_value;

	bool is_scoped = false;
	try {
		is_scoped = g_ctx->m_local_pawn->m_bIsScoped();
	}
	catch (...) {
		return return_value;
	}

	bool override_fov = false;
	bool dynamic_fov_enabled = false;
	int world_fov_value = 90;
	int world_fov_first_scope = 30;
	int world_fov_second_scope = 30;

	try {
		override_fov = GET_VAR(bool, VISUALS_PATH(m_override_world_fov));
		dynamic_fov_enabled = GET_VAR(bool, VISUALS_PATH(m_dynamic_fov));
		if (override_fov)
			world_fov_value = GET_VAR(int, VISUALS_PATH(m_override_world_fov_value));

		world_fov_first_scope = GET_VAR(int, VISUALS_PATH(m_override_world_fov_value_first_scope));
		world_fov_second_scope = GET_VAR(int, VISUALS_PATH(m_override_world_fov_value_second_scope));
	}
	catch (...) {
		return return_value;
	}

	if (!override_fov && !dynamic_fov_enabled) {
		smooth_fov = return_value;
		dynamic_fov_offset = 0.0f;
		last_time = std::chrono::steady_clock::now();
		return return_value;
	}

	float target_fov = return_value;

	if (!is_scoped && override_fov) {
		target_fov = (float)(world_fov_value);
	}
	else if (is_scoped) {
		bool is_second_scope = false;

		try {
			if (g_ctx->m_active_weapon) {
				float zoom_level = g_ctx->m_active_weapon->m_zoomLevel();
				is_second_scope = (zoom_level > 1.0f);
			}
		}
		catch (...) {
			is_second_scope = false;
		}

		if (is_second_scope && world_fov_second_scope >= 30) {
			target_fov = (float)(world_fov_second_scope);
		}
		else if (!is_second_scope && world_fov_first_scope >= 30) {
			target_fov = (float)(world_fov_first_scope);
		}
	}

	if (dynamic_fov_enabled && g_ctx->m_local_pawn) {
		try {
			vec3_t velocity = g_ctx->m_local_pawn->m_vecVelocity();
			float speed = velocity.length_2d();
			
			float target_dynamic_offset = std::min(speed * 0.04f, 10.0f);

			auto now = std::chrono::steady_clock::now();
			float delta = std::chrono::duration<float>(now - last_time).count();
			
			float dynamic_lerp_speed = 12.0f * std::min(delta, 0.033f);
			dynamic_fov_offset += (target_dynamic_offset - dynamic_fov_offset) * dynamic_lerp_speed;
			
			if (override_fov) {
				target_fov += dynamic_fov_offset;
			}
		}
		catch (...) {
		}
	}

	target_fov = std::clamp(target_fov, 1.0f, 179.0f);

	auto now = std::chrono::steady_clock::now();
	float delta = std::chrono::duration<float>(now - last_time).count();
	last_time = now;

	float t = 8.0f * std::min(delta, 0.033f);
	smooth_fov += (target_fov - smooth_fov) * t;

	return smooth_fov;
}

void c_client_hooks::calc_viewmodel(c_cs_player_pawn* pawn, vec3_t* offsets, float* fov)
{
	static const auto original = g_hooks->m_client.m_calc_viewmodel.get<decltype(&calc_viewmodel)>();

	static float smooth_x = 0.0f, smooth_y = 0.0f, smooth_z = 0.0f, smooth_fov = 68.0f;
	static auto last_time = std::chrono::steady_clock::now();

	original(pawn, offsets, fov);

	if (!offsets || !fov || pawn != g_ctx->m_local_pawn || !GET_VAR(bool, VISUALS_PATH(m_override_viewmodel_fov)))
		return;

	float target_x = GET_VAR(int, VISUALS_PATH(m_override_viewmodel_value_fov_x));
	float target_y = GET_VAR(int, VISUALS_PATH(m_override_viewmodel_value_fov_y));
	float target_z = GET_VAR(int, VISUALS_PATH(m_override_viewmodel_value_fov_z));
	float target_fov = GET_VAR(int, VISUALS_PATH(m_override_viewmodel_value));

	auto now = std::chrono::steady_clock::now();
	float delta = std::chrono::duration<float>(now - last_time).count();
	last_time = now;

	float t = 8.0f * std::min(delta, 0.033f);

	smooth_x += (target_x - smooth_x) * t;
	smooth_y += (target_y - smooth_y) * t;
	smooth_z += (target_z - smooth_z) * t;
	smooth_fov += (target_fov - smooth_fov) * t;

	offsets->x += smooth_x;
	offsets->y += smooth_y;
	offsets->z += smooth_z;

	*fov = smooth_fov;

}

void c_client_hooks::level_init(void* client_mode_shared, const char* new_map) {
	static const auto original = g_hooks->m_client.m_level_init.get<decltype(&level_init)>();

    g_interfaces->refresh_global_vars();


	//if (!g_interfaces->m_game_particle_manager)
	//	g_interfaces->m_game_particle_manager = g_modules->m_client.find(xx("E8 ? ? ? ? 66 0F 6E 86")).relative(1, 5).as<c_game_particle_manager * (__fastcall*)()>()();

	if (!g_interfaces->m_pvs_bool)
	{
		g_interfaces->m_pvs->set(false);
		g_interfaces->m_pvs_bool = true;
	}

	g_skybox_changer->init_on_level_load( );
	g_skybox_changer->m_need_update_material = true;
	//g_scoreboard->level_init();
	g_entity_cache->m_players.clear();
	g_entity_cache->m_entity.clear();
	g_entity_cache->m_grenade_entity.clear();
	g_entity_cache->m_c4_entity.clear();
	g_entity_cache->m_weapon_entity.clear();

	// stale pointers from the previous map must not survive into the new one
	g_ctx->m_cmd_manager = nullptr;
    g_ctx->m_cmd = nullptr;
    g_ctx->m_base = nullptr;
	g_ctx->m_local_pawn = nullptr;
	g_ctx->m_local_controller = nullptr;
	g_ctx->m_active_weapon = nullptr;
	g_ctx->m_active_weapon_data = nullptr;

	s_vecPredictedGrenades.clear();

	s_backtrack_models.clear();
	s_onshot_models.clear();
	s_onshot_skeletons.clear();

	g_overlay->avatar_cache.clear();

    original(client_mode_shared, new_map);
    g_interfaces->refresh_level_state();
    g_interfaces->refresh_global_vars();
}

void c_client_hooks::level_shutdown(void* client_mode_shared) {
	static const auto original = g_hooks->m_client.m_level_shutdown.get<decltype(&level_shutdown)>();
    g_overlay->clear_aimbot_hitboxes();

	for (auto& [pawn, model] : s_backtrack_models) {
		model.remove(REMOVE_ALL);
	}
	s_backtrack_models.clear();

	for (auto& [pawn, models] : s_onshot_models) {
		for (auto& model : models) {
			model.remove(REMOVE_ALL);
		}
	}
	s_onshot_models.clear();
	s_onshot_skeletons.clear();

	for (auto& [hash, icon] : m_icons) {
		if (icon.texture_view) {
			icon.texture_view->Release();
		}
	}
	m_icons.clear();

	g_overlay->avatar_cache.clear();
	//g_scoreboard->level_shutdown();
	g_interfaces->m_pvs_bool = false;
	g_particle_mgr->clear_all_particles();
	g_interfaces->m_game_particle_manager = nullptr;
	g_interfaces->m_global_vars = nullptr;
	g_interfaces->m_game_rules = nullptr;
	g_fog_handler->remove_fog();

	g_entity_cache->m_players.clear();
	g_entity_cache->m_entity.clear();
	g_entity_cache->m_grenade_entity.clear();
	g_entity_cache->m_c4_entity.clear();
	g_entity_cache->m_weapon_entity.clear();

	// invalidate cached context pointers - the pawn/weapon entities are freed on map exit,
	// otherwise FRAME_NET_UPDATE_END paths (g_lagcomp->run -> update_local_ctx) dereference dangling pointers
	g_ctx->m_cmd_manager = nullptr;
    g_ctx->m_cmd = nullptr;
    g_ctx->m_base = nullptr;
	g_ctx->m_local_pawn = nullptr;
	g_ctx->m_local_controller = nullptr;
	g_ctx->m_active_weapon = nullptr;
	g_ctx->m_active_weapon_data = nullptr;



	s_vecPredictedGrenades.clear();

	g_overlay->clear();
	g_overlay->clear_hitmarks();

	original(client_mode_shared);

	g_skybox_changer->cleanup( );

	g_interfaces->destroy_render_target();
}

__int64 c_client_hooks::add_entity(void* thisptr, c_entity_instance* inst, c_base_handle handle) {
	static const auto original = g_hooks->m_client.m_add_entity.get<decltype(&add_entity)>();

	g_entity_cache->on_add(inst, handle);

	return original(thisptr, inst, handle);

}
__int64 c_client_hooks::remove_entity(void* thisptr, c_entity_instance* inst, c_base_handle handle) {
	static const auto original = g_hooks->m_client.m_remove_entity.get<decltype(&remove_entity)>();

	g_entity_cache->on_remove(inst, handle);

	return original(thisptr, inst, handle);
}

void c_client_hooks::get_matrix_for_view(void* ecx, void* setup, void* world_to_view, void* view_to_projection, v_matrix* world_to_projection, void* world_to_pixels) {

	static const auto original = g_hooks->m_client.m_matricies_for_view.get<decltype(&get_matrix_for_view)>();
	original(ecx, setup, world_to_view, view_to_projection, world_to_projection, world_to_pixels);

	g_math->m_viewmatrix = *world_to_projection;
}

void c_client_hooks::override_view(void* client_mode_cs_normal, c_view_setup* view_setup) {
	static const auto original = g_hooks->m_client.m_override_view.get<decltype(&override_view)>();

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return original(client_mode_cs_normal, view_setup);

	// override_view runs on the render path and keeps firing during a map transition,
	// after create_move stopped refreshing g_ctx -> m_local_pawn is left dangling (freed
	// object, garbage vtable) and the virtual get_eye_pos() below crashes. re-fetch the
	// live pawn/controller here so the null guard below bails cleanly when we leave the map.
	g_ctx->m_local_controller = g_interfaces->m_entity_system->get_player_controller(0);
	g_ctx->m_local_pawn = g_interfaces->m_entity_system->get_player_pawn(0);

	if (!g_ctx->m_local_controller || !g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return original(client_mode_cs_normal, view_setup);

	if (int ratio = GET_VAR(int, VISUALS_PATH(m_aspect_ratio))) {
		static float target = 0.f;
		static float current = view_setup->m_aspect_ratio;

		if (target != ratio / 10.f) {
			target = ratio / 10.f;
			current = view_setup->m_aspect_ratio;
		}

		if (auto* gv = g_interfaces->m_global_vars)
			current = std::lerp(current, target, gv->m_frame_time * 12.f);
		else
			current = target;
		view_setup->m_aspect_ratio = current;
		view_setup->m_some_flags |= 2;
	} else view_setup->m_some_flags &= ~2;

	original(client_mode_cs_normal, view_setup);

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller)
		return;

	if (GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_aimpunch))
		view_setup->m_view_angle = g_interfaces->m_csgo_input->get_view_angle();

	static bool fake_stand = false;
	static bool this_tick = false;

	if (GET_VAR(bool, RAGEBOT_PATH(m_duck_peek)) && g_ctx->m_cmd && (g_ctx->m_cmd->m_buttons.m_value & IN_DUCK)) {
		fake_stand = true;
		this_tick = true;
	}

	if (this_tick) {
		auto scene_node = g_ctx->m_local_pawn->m_pGameSceneNode();
		if (!scene_node)
			return;

		view_setup->m_origin = scene_node->m_vecAbsOrigin() + vec3_t(0.f, 0.f, 64.f);

		auto movement = g_ctx->m_local_pawn->m_pMovementServices();
		if (movement && movement->m_flDuckAmount() <= 0.01f && !movement->m_bDucking() && !movement->m_bDucked()) {
			fake_stand = false;
			this_tick = false;
		}
	}

	qangle_t inverse_view_angles = g_interfaces->m_csgo_input->get_view_angle();
	inverse_view_angles.x = -inverse_view_angles.x;
	vec3_t eye_position = g_ctx->m_local_pawn->get_eye_pos();

	if (GET_VAR(bool, VISUALS_PATH(m_third_person_enabled))) {
		float distance = (float)GET_VAR(int, VISUALS_PATH(m_third_person_distance));

		view_setup->m_origin = g_math->calculate_camera_position(eye_position, -distance, inverse_view_angles);

		c_ray ray{};
		c_game_trace game_trace{};
		c_trace_filter filter{ 0x1c3003, g_ctx->m_local_pawn, nullptr, 4 };

		if (g_interfaces->m_phys2world->trace_shape(&ray, &eye_position, &view_setup->m_origin, &filter, &game_trace)) {
			if (game_trace.hit_world()) {
				view_setup->m_origin = game_trace.m_hit_point;
			}
		}
	}
}

void parse_grenades()
{
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!g_ctx->m_local_controller)
		return;

	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	for (auto& object : g_entity_cache->m_grenade_entity)
	{
		if (object.m_bPredictedGrenade)
			continue;

		if (!object.m_pEntity || !object.m_pEntity->get_handle().is_valid())
			continue;

		auto pBaseGrenadeProj = reinterpret_cast<c_base_cs_grenade_projectile*>(object.m_pEntity);
		if (!pBaseGrenadeProj)
			continue;

		object.m_bPredictedGrenade = true;

		GrenadePredictionObject_t objGrenadeObject = {  };
		objGrenadeObject.m_pGrenadeEntity = object.m_pEntity;
		objGrenadeObject.m_vInitialPosition = pBaseGrenadeProj->m_vInitialPosition();
		objGrenadeObject.m_vInitialVelocity = pBaseGrenadeProj->m_vInitialVelocity();

		std::vector<GrenadePathPoint_t> vecGrenadePathPoint = g_GrenadePrediction->GetGrenadePathFromEntity(
			objGrenadeObject.m_vInitialPosition,
			objGrenadeObject.m_vInitialVelocity,
			(c_base_cs_grenade*)pBaseGrenadeProj,
			g_interfaces->m_global_vars->m_curtime
		);

		objGrenadeObject.m_GrenadePathPoint = vecGrenadePathPoint;

		if (!objGrenadeObject.m_GrenadePathPoint.empty())
		{
			objGrenadeObject.m_nTickDetonation = objGrenadeObject.m_GrenadePathPoint.back().m_nTick;
		}

		objGrenadeObject.m_iNadeType = g_GrenadePrediction->get_grenade_type(fnv1a::hash_32(pBaseGrenadeProj->get_class_name()));
		objGrenadeObject.m_flNadeDamage = pBaseGrenadeProj->m_flDamage();
		objGrenadeObject.m_flNadeRadius = pBaseGrenadeProj->m_DmgRadius();

		objGrenadeObject.m_iTotalPoints = objGrenadeObject.m_GrenadePathPoint.size();

		for (auto& CAL : objGrenadeObject.m_GrenadePathPoint)
		{
			objGrenadeObject.m_GrenadePathPoint_1.push_back(CAL.m_vPos);
		}

		s_vecPredictedGrenades.emplace_back(objGrenadeObject);
	}
}


void c_client_hooks::frame_stage_notify(void* source2_client, int stage) {
	static const auto original = g_hooks->m_client.m_frame_stage_notify.get<decltype(&frame_stage_notify)>();

    g_interfaces->refresh_level_state();

    if (!g_interfaces->refresh_global_vars() || !g_interfaces->m_entity_system)
        return original(source2_client, stage);
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game()) {
        g_overlay->clear_aimbot_hitboxes();
        g_ctx->m_local_controller = nullptr;
        g_ctx->m_local_pawn = nullptr;
        return original(source2_client, stage);
    }
    g_ctx->m_local_controller = g_interfaces->m_entity_system->get_player_controller(0);
    g_ctx->m_local_pawn = g_interfaces->m_entity_system->get_player_pawn(0);
    const bool alive = g_ctx->m_local_controller && g_ctx->m_local_controller->m_bPawnIsAlive() &&
        g_ctx->m_local_pawn && g_ctx->m_local_pawn->m_iHealth() > 0;
    g_ctx->m_active_weapon = alive ? g_ctx->m_local_pawn->get_active_weapon() : nullptr;
    g_ctx->m_active_weapon_data = g_ctx->m_active_weapon ? g_ctx->m_active_weapon->weapon_data() : nullptr;

    if (stage == FRAME_SKIN_UPDATE && alive) {
        g_skins->agent_changer();
        g_custom_models->update();
        g_skins->glove_changer();
        g_skins->knifes(stage);
        g_skins->weapons();
    }
    original(source2_client, stage);

	switch (stage) {
	case FRAME_RENDER_START:
		g_kill_feed->preserve_kill_feed();
		g_scoreboard->on_frame();

		if (g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game()) {
			parse_grenades();
		}

		//g_skins->weapons();
		break;
	case FRAME_RENDER_END:
		g_skybox_changer->run();

		g_particle_mgr->run_world_weather(GET_VAR(hellcolor, VISUALS_PATH(m_ash_color)));
		break;
	case FRAME_NET_UPDATE_END:
		g_ragebot->set_config();
		g_lagcomp->run();
		g_overlay->update_aimbot_hitboxes();
		g_particle_mgr->update();
		//shotlogger::check_prediction_errors();
		break;
	case FRAME_SIMULATE_END:
		g_fog_handler->fog_controller();
		break;
	}
}

// why the fuck not
class c_viewangle_mgr {
public:
	c_viewangle_mgr(c_csgo_input* input) : m_input(input), m_old(input->get_view_angle()) {}
	~c_viewangle_mgr() {
		m_input->set_view_angle(m_old);
	}
private:
	c_csgo_input* m_input;
	vec3_t m_old;
};

void c_client_hooks::handle_camera_angles(c_csgo_input* input, int slot) {
    static const auto original = g_hooks->m_client.m_handle_camera_angles.get<decltype(&handle_camera_angles)>();
    if (slot != 0 || !GET_VAR(bool, ANTIAIM_PATH(m_enabled_antiaim)))
        return original(input, slot);

    c_viewangle_mgr viewangle_mgr(input);
    original(input, slot);
}

void c_client_hooks::draw_scope(__int64 a1, __int64 a2) {
	static const auto original = g_hooks->m_client.m_draw_scope.get<decltype(&draw_scope)>();

	if (GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_scope)) {
		return;
	}

	original(a1, a2);
}

bool c_client_hooks::draw_overhead(c_cs_player_pawn* pawn, int slot) {
	static const auto original = g_hooks->m_client.m_draw_overhead.get<decltype(&draw_overhead)>();
	if (!pawn) return false;
	if (pawn == g_ctx->m_local_pawn) return !GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_local_name);
	if (!pawn->is_enemy() && GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_team_names)) return false;
	return true;
}


void* c_client_hooks::smoke_volume_scene_object_drawarray(void* a1, void* a2, void* a3, int a4, void* a5, void* a6, void* a7) {
	static const auto original = g_hooks->m_client.m_smoke_volume_drawarray.get<decltype(&smoke_volume_scene_object_drawarray)>();
	return GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_smoke) ? nullptr : original(a1, a2, a3, a4, a5, a6, a7);
}

void* c_client_hooks::draw_flash_overlay(__int64 a1, int a2, __int64* a3, __int64 a4, float a5[4]) {
	static const auto original = g_hooks->m_client.m_draw_flash_overlay.get<decltype(&draw_flash_overlay)>();
	return GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_flash) ? nullptr : original(a1, a2, a3, a4, a5);
}

void* c_client_hooks::first_person_legs(void* a1, void* a2, void* a3, void* a4, void* a5) {
	static const auto original = g_hooks->m_client.m_first_person_legs.get<decltype(&first_person_legs)>();
	return GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)).at(e_removals::removal_legs) ? nullptr : original(a1, a2, a3, a4, a5);
}

void c_client_hooks::handle_glow(c_glow_property* glow_property, float* color) {
    static const auto original=g_hooks->m_client.m_handle_glow.get<decltype(&handle_glow)>();
    original(glow_property,color);

	if (!glow_property)
		return;

	if (!glow_property->get_owner())
		return;

	auto pawn = resolve_glow_pawn(glow_property->get_owner());
	if (!pawn || !pawn->is_alive()) {
		// hide the native glow on weapons/knives held by a player.
		if (get_held_weapon_pawn(glow_property->get_owner())) {
			color[0] = color[1] = color[2] = 0.f;
			color[3] = 0.f;
		}
		return;
	}

	c_cs_player_pawn* local = g_ctx->m_local_pawn;
	if (!local)
		return;

	bool is_local = pawn == local;
	bool is_teamate = !is_local && pawn->m_iTeamNum() == local->m_iTeamNum();
	bool is_enemy = pawn->m_iTeamNum() != local->m_iTeamNum();

	if (is_local && GET_VAR(bool, VISUALS_PATH(m_local_glow)) && GET_VAR(bool, VISUALS_PATH(m_third_person_enabled))) {

		//glow_property->m_bGlowing( ) = true;
		hellcolor col_over = GET_VAR(hellcolor, VISUALS_PATH(m_local_glow_color));
		color[0] = col_over.Value.x;
		color[1] = col_over.Value.y;
		color[2] = col_over.Value.z;
		color[3] = col_over.Value.w;
	}
	else if (is_teamate && GET_VAR(bool, VISUALS_PATH(m_teamate_glow))) {
		//glow_property->m_bGlowing( ) = true;
		hellcolor col_over = GET_VAR(hellcolor, VISUALS_PATH(m_teamate_glow_color));
		color[0] = col_over.Value.x;
		color[1] = col_over.Value.y;
		color[2] = col_over.Value.z;
		color[3] = col_over.Value.w;
	}
	else if (is_enemy && GET_VAR(bool, VISUALS_PATH(m_enemy_glow))) {
		//glow_property->m_bGlowing( ) = true;
		hellcolor col_over = GET_VAR(hellcolor, VISUALS_PATH(m_enemy_glow_color));
		color[0] = col_over.Value.x;
		color[1] = col_over.Value.y;
		color[2] = col_over.Value.z;
		color[3] = col_over.Value.w;
	}
}

bool c_client_hooks::m_is_glow(c_glow_property* glow_property)
{
	static const auto original = g_hooks->m_client.m_is_glow.get<decltype(&m_is_glow)>();

	if (!glow_property || !glow_property->get_owner())
		return original(glow_property);

	auto pawn = resolve_glow_pawn(glow_property->get_owner());
	if (!pawn || !pawn->is_alive()) {
		// hide the native glow on weapons/knives held by a player by forcing a
		// fully transparent override color (invisible even while the game toggles it).
		if (get_held_weapon_pawn(glow_property->get_owner()))
			glow_property->m_glowColorOverride() = hellcolor(0, 0, 0, 0);
		return original(glow_property);
	}

	c_cs_player_pawn* local = g_ctx->m_local_pawn;
	if (!local)
		return original(glow_property);

	bool is_local = pawn == local;
	bool is_teamate = !is_local && pawn->m_iTeamNum() == local->m_iTeamNum();
	bool is_enemy = pawn->m_iTeamNum() != local->m_iTeamNum();
	hellcolor color{};
	bool should_glow = false;

	if (is_local && GET_VAR(bool, VISUALS_PATH(m_local_glow)) && GET_VAR(bool, VISUALS_PATH(m_third_person_enabled))) {
		color = GET_VAR(hellcolor, VISUALS_PATH(m_local_glow_color));
		should_glow = true;
	}
	else if (is_teamate && GET_VAR(bool, VISUALS_PATH(m_teamate_glow))) {
		color = GET_VAR(hellcolor, VISUALS_PATH(m_teamate_glow_color));
		should_glow = true;
	}
	else if (is_enemy && GET_VAR(bool, VISUALS_PATH(m_enemy_glow))) {
		color = GET_VAR(hellcolor, VISUALS_PATH(m_enemy_glow_color));
		should_glow = true;
	}

	if (should_glow) {
		glow_property->m_bGlowing() = true;
		glow_property->m_flGlowTime() = INT_MAX;
		glow_property->m_iGlowType() = 3;
		glow_property->m_glowColorOverride() = color;
	}

	return original(glow_property);
}

void* c_client_hooks::report_hit(report_hit_t* hit) {
	static const auto original = g_hooks->m_client.m_report_hit.get<decltype(&report_hit)>();

	if (GET_VAR(bool, VISUALS_PATH(m_local_tracers_only_topoint))) {
		if (!g_ctx->m_local_pawn)
			return original(hit);
			
		vec3_t shoot_pos = g_ctx->m_local_pawn->get_eye_pos();
		vec3_t direction = (hit->m_position - shoot_pos).normalized();
		vec3_t right = direction.cross(vec3_t(0, 0, 1)).normalized();
		vec3_t up = vec3_t(0, 0, 1);
		vec3_t corrected_shoot_pos = shoot_pos + (right * 3.0f) + (up * 1.0f);
		
		g_particle_mgr->create_particle_beam(corrected_shoot_pos,
			hit->m_position,
			GET_VAR(hellcolor, VISUALS_PATH(m_local_impact_tracers_color)),
			GET_VAR(int, VISUALS_PATH(m_local_impact_tracers_duration)),
			(float)GET_VAR(int, VISUALS_PATH(m_local_impact_tracers_width)) / 10.f
		);
	}

	if (GET_VAR(bool, VISUALS_PATH(m_enabled_3d_hitmarkers))) {
		g_overlay->add_hitmarker(hit->m_position);
	}

	g_overlay->damage_pos = hit->m_position;

	return original(hit);
}

void c_client_hooks::setup_move(c_player_movement_services* mv_services, c_user_cmd* user_cmd, c_move_data* mv_data)
{
	static const auto original = g_hooks->m_client.m_setup_move.get<decltype(&setup_move)>();

	original(mv_services, user_cmd, mv_data);
}
