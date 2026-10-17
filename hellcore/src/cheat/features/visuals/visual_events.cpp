#include "visual_events.h"

#include <cheat/menu/hell_gui/drawlist.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <cheat/features/visuals/chams.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/features/ragebot/ragebot.h>
#include <cheat/features/ragebot/shotlogger.h>

#include <cheat/features/visuals/particle_manager.h>
#include <cheat/config/vars.h>

#include <cheat/features/visuals/overlay_features.h>
#include <cheat/features/skins/skins.h>
#include <cheat/features/misc/autobuy.h>
#include <sstream>
#include "../../features/misc/preserve_kill_feed.h"


inline const char* hitsound_paths[] = {
	xx("sounds/ui/coin_pickup_01.vsnd_c"),
	xx("sounds/ui/panorama/mainmenu_press_shop_01.vsnd_c"),
	xx("sounds/weapons/ssg08/ssg08_distant.vsnd"),
	xx("sounds/ui/Assembly.vsnd"),
	xx("sounds/ui/xp_star_full_03.vsnd")
};

static void on_player_death(c_game_event* event_ptr) {
	auto attacker = reinterpret_cast<c_cs_player_controller*>(event_ptr->get_player_controller("attacker"));
	auto controller = reinterpret_cast<c_cs_player_controller*>(event_ptr->get_player_controller("userid"));
	if (!attacker || !controller || !attacker->m_bIsLocalPlayerController())
		return;

	c_cs_player_pawn* pawn = reinterpret_cast<c_cs_player_pawn*>(controller->m_hPawn().get());

	const int hitgroup = event_ptr->get_int("hitgroup", false);
	int damage = event_ptr->get_int("dmg_health");
	if (damage <= 0)
		return;

	vec3_t vec_body = pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	vec_body.z += 40;

	if (GET_VAR(bool, VISUALS_PATH(m_enabled_kill_effects))) {
		int kill_effect_type = GET_VAR(int, VISUALS_PATH(m_kill_effects_type));

		g_particle_mgr->play_particle_effect(pawn);
		g_particle_mgr->create_particle_at_pos(vec_body, kill_effect_type);
	}
	
	if (controller->m_bIsLocalPlayerController() && GET_VAR(bool, MISC_PATH(m_enabled_auto_peek))) {
		GET_VAR(bool, MISC_PATH(m_enabled_auto_peek)) = false;
		auto& widget_keybinds = GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds));
		for (auto& [widget_id, binds] : widget_keybinds) {
			for (auto& keybind : binds) {
				if (keybind.m_holder_id == MISC_PATH(m_enabled_auto_peek)) {
					keybind.m_keybind_active = false;
				}
			}
		}
	}
}


static void on_player_hurt(c_game_event* event_ptr) {
	auto attacker = reinterpret_cast<c_cs_player_controller*>(event_ptr->get_player_controller("attacker"));
	auto controller = reinterpret_cast<c_cs_player_controller*>(event_ptr->get_player_controller("userid"));
	if (!attacker || !controller || !attacker->m_bIsLocalPlayerController())
		return;

	c_cs_player_pawn* pawn = reinterpret_cast<c_cs_player_pawn*>(controller->m_hPawn().get());
	if (!pawn)
		return;

	int damage = event_ptr->get_int("dmg_health");
	if (damage <= 0)
		return;

	if (GET_VAR(bool, VISUALS_PATH(m_chams_enabled_os))) {
		model_object_t model = {};
		model.m_draw_begin_time = g_interfaces->m_global_vars->m_curtime;
		
		vec3_t impact_pos = vec3_t(0, 0, 0);
		bool found_impact = false;
		
		for (auto& player : g_entity_cache->m_players) {
			if (player.m_pawn == pawn && player.m_lagcomp_data.is_valid()) {
				auto& newest_record = player.m_lagcomp_data.m_lag_records.newest();
				impact_pos = newest_record.m_origin;
				impact_pos.z += 64.0f;
				found_impact = true;
				break;
			}
		}
		
		if (found_impact) {
			model.m_impact_position = impact_pos;
			
			if (model.create(pawn, SINGLE_OBJECT_INDEX)) {
				model.handle(pawn, true);
				s_onshot_models[pawn].emplace_back(model);
			}
		}
	}

	if (GET_VAR(bool, MISC_PATH(m_enabled_hitsound))) {
		const int selected = GET_VAR(int, MISC_PATH(m_hitsound_selection));
        if (selected >= 0 && selected < static_cast<int>(std::size(hitsound_paths)))
            g_ctx->play_v_sound(hitsound_paths[selected]);
	}

	vec3_t above = pawn->get_world_space_center();
    g_overlay->damage_pos=above;
    if(GET_VAR(bool,VISUALS_PATH(m_enabled_3d_hitmarkers))) g_overlay->add_hitmarker(above);
    if(GET_VAR(bool,VISUALS_PATH(m_local_tracers_only_topoint)) && g_ctx->m_local_pawn)
        g_particle_mgr->create_particle_beam(g_ctx->m_local_pawn->get_eye_pos(),above,
            GET_VAR(hellcolor,VISUALS_PATH(m_local_impact_tracers_color)),
            GET_VAR(int,VISUALS_PATH(m_local_impact_tracers_duration)),
            GET_VAR(int,VISUALS_PATH(m_local_impact_tracers_width))/10.f);

	g_overlay->m_hit_markers2.push_back({ ImGui::GetTime(), 1.0f, 1.0f });

	if (GET_VAR(bool, VISUALS_PATH(m_enabled_3d_damage_markers))) {
		g_overlay->m_world_damage_markers.emplace_back(c_overlay::hit_text_t{
				above,
				damage,
				g_interfaces->m_global_vars->m_curtime,
				1.8f
			});
	}

	if (GET_VAR(bool, VISUALS_PATH(m_skeleton_esp_onshot_enabled))) {
		g_visuals->create_onshot_skeleton_data(pawn);
	}

	std::vector<bool> event_list = GET_VAR(std::vector<bool>, MISC_PATH(m_hitlog_modes));
	if (event_list.size() > 0 && event_list[0]) {
		c_cs_player_controller* p_attacker_controller = event_ptr->get_player_controller("attacker");
		if (p_attacker_controller == g_ctx->m_local_controller) {
			c_cs_player_controller* p_controller = event_ptr->get_player_controller("userid");
			if (p_controller) {
				const char* sz_hitgroup_name{};
				switch (event_ptr->get_int("hitgroup")) {
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

				std::stringstream ss;
				ss << "Hit " << p_controller->m_sSanitizedPlayerName() << " in the " << sz_hitgroup_name << " for " << event_ptr->get_int("dmg_health") << " damage (" << event_ptr->get_int("health") << " remaining)";
				g_overlay->add_notification(ss.str());
			}
		}
	}
	
	shotlogger::game_event(event_ptr);
}

void c_visual_events::on_fire_game_event(c_game_event* event_ptr) {
    if (!event_ptr || !g_interfaces->refresh_global_vars())
        return;
    const auto* name = event_ptr->get_name();
    if (!name || !*name)
        return;
	fnv1a_t hashed_event_name = fnv_hash(name);

	switch (hashed_event_name) {
	case fnv_hash("bullet_impact"):
	{
		shotlogger::game_event(event_ptr);
		
		vec3_t world_position = { event_ptr->get_float(xx("x")), event_ptr->get_float(xx("y")), event_ptr->get_float(xx("z")) };
		c_cs_player_controller* controller = event_ptr->get_player_controller(xx("userid"));
		c_cs_player_pawn* pawn = event_ptr->get_player_pawn(xx("userid"));

		if (!controller || !pawn)
			break;

		bool is_enemy = pawn->is_enemy();

		if (pawn == g_ctx->m_local_pawn) {

			if (GET_VAR(bool, VISUALS_PATH(m_shot_sparks_enabled))) {
				g_particle_mgr->create_shot_sparks(world_position, GET_VAR(hellcolor, VISUALS_PATH(m_shot_sparks_color)));
			}

			if (GET_VAR(bool, VISUALS_PATH(m_local_tracers)) && !GET_VAR(bool, VISUALS_PATH(m_local_tracers_only_topoint))) {
				if (!g_interfaces->m_global_vars)
					return;
				vec3_t shoot_pos = g_ctx->m_local_pawn->get_eye_pos();
				vec3_t direction = (world_position - shoot_pos).normalized();
				vec3_t right = direction.cross(vec3_t(0, 0, 1)).normalized();
				vec3_t up = vec3_t(0, 0, 1);
				
				m_last_impact_pos = world_position;
				m_last_shoot_pos = shoot_pos;
				m_last_impact_time = g_interfaces->m_global_vars->m_curtime;
			}

			if (GET_VAR(bool, VISUALS_PATH(m_local_impact_boxes))) {
				float size = (float)GET_VAR(int, VISUALS_PATH(m_local_impact_boxes_size));
				if (auto* overlay = g_interfaces->m_source2_client->get_debug_overlay())
                    overlay->add_box(
					world_position,
					{ -size, -size, -size },
					{ size, size, size },
					{ 0.f, 0.f, 0.f },
					GET_VAR(hellcolor, VISUALS_PATH(m_local_impact_boxes_color)),
					GET_VAR(int, VISUALS_PATH(m_local_impact_boxes_duration))
				);

			}

			if (GET_VAR(bool, VISUALS_PATH(m_chams_enabled_os))) {
			}
		}
		else if (!is_enemy) {
			//team
		}
		else if (is_enemy) {
			//enemy
		}

		break;
	}

	case fnv_hash("player_hurt"):
	{
		on_player_hurt(event_ptr);
		break;
	}

	case fnv_hash("weapon_fire"):
	{
		shotlogger::game_event(event_ptr);
		break;
	}

	case fnv_hash("player_spawn"):
	{
		break;
	}

	case fnv_hash("player_spawned"):
	{
		break;
	}

	case fnv_hash("player_death"):
	{
		on_player_death(event_ptr);
		break;
	}

	case fnv_hash("round_start"):
	{
		g_particle_mgr->m_b_new_round_callback = true;
		g_kill_feed->b_new_round = true;
		g_autobuy->on_round_start();
		g_skins->on_round_start();
		
		if (GET_VAR(bool, MISC_PATH(m_enabled_auto_peek))) {
			GET_VAR(bool, MISC_PATH(m_enabled_auto_peek)) = false;
			auto& widget_keybinds = GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds));
			for (auto& [widget_id, binds] : widget_keybinds) {
				for (auto& keybind : binds) {
					if (keybind.m_holder_id == MISC_PATH(m_enabled_auto_peek)) {
						keybind.m_keybind_active = false;
					}
				}
			}
		}
		
		break;
	}

	}
}

void c_visual_events::create_delayed_tracer() {
	if (!GET_VAR(bool, VISUALS_PATH(m_local_tracers)) || GET_VAR(bool, VISUALS_PATH(m_local_tracers_only_topoint)))
		return;
		
	if (!g_interfaces->m_global_vars || !g_ctx->m_local_pawn)
		return;
		
	float current_time = g_interfaces->m_global_vars->m_curtime;
	if (m_last_impact_time > 0.f && current_time - m_last_impact_time > 0.05f) {
		g_particle_mgr->create_particle_beam(m_last_shoot_pos,
			m_last_impact_pos,
			GET_VAR(hellcolor, VISUALS_PATH(m_local_impact_tracers_color)),
			GET_VAR(int, VISUALS_PATH(m_local_impact_tracers_duration)),
			(float)GET_VAR(int, VISUALS_PATH(m_local_impact_tracers_width)) / 10.f
		);
		m_last_impact_time = 0.f;
	}
}