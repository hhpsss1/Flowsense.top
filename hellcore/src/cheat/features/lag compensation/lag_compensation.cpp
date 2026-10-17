#include "lag_compensation.h"
#include <cheat/features/entity cache/entity_cache.h>
#include <sdk/interfaces/global_variables.h>
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/csgo_input.h>
#include <sdk/datatypes/scene_object.h>

#include <context.h>
#include <cheat/features/ragebot/ragebot.h>
#include "timestamp/timestamp.h"
#include <cheat/features/visuals/chams.h>

bool lag_record_t::should_skip_interpolation()
{
    if (m_pawn->m_nNoInterpolationTick() == g_interfaces->m_global_vars->m_tick_count)
        return true;

    return false;
}

void lag_record_t::try_adjust_velocity()
{
    if (should_skip_interpolation())
    {
        if (m_pawn->m_bSimulationTimeChanged())
        {
            float sim_time_diff = m_pawn->m_flSimulationTime() - m_pawn->m_flOldSimulationTime();
            if (sim_time_diff > 0.0001f)
            {
                float inv_dt = 1.0f / sim_time_diff;

                vec3_t current_origin = m_game_scene_node->m_vecOrigin();
                vec3_t old_origin = m_pawn->m_vOldOrigin();
                vec3_t velocity = (current_origin - old_origin) * inv_dt;

                m_pawn->set_velocity(&velocity);
            }
        }
    }
}

void store_hitboxes(lag_record_t* record, c_penetration::player_context_t& player_ctx) {
    if (!record || !record->m_skeleton_instance)
        return;

    c_model* model = record->m_skeleton_instance->m_modelState().m_model_handle;
    if (!model)
        return;

    auto* hitbox_set = model->get_hitbox_set();
    if (!hitbox_set || hitbox_set->m_hitbox_count < 19)
        return;
    auto* hitbox_arr = hitbox_set->m_hitbox;

    auto& bones = record->m_bones;
    int pointscale = g_ragebot->m_config.m_pointscale;

    static constexpr int bone_indices[19] = { 6, 5, 0, 1, 2, 3, 4, 22, 25, 23, 26, 24, 27, 10, 15, 8, 9, 13, 14 };
    for (int i = 0; i < 19; ++i) 
        record->m_all_hitboxes.emplace_back(construct_hitbox_data(hitbox_arr[i], bones[bone_indices[i]], false, pointscale));

    auto& store_hitboxes = record->m_rage_hitboxes;

    auto& mp_setting = g_ragebot->m_config.m_multipointed_hitboxes;
    int dmg_threshold = g_ragebot->m_config.m_mindamage > 100 ? record->m_pawn->m_iHealth() + g_ragebot->m_config.m_mindamage - 100 : std::min(g_ragebot->m_config.m_mindamage, record->m_pawn->m_iHealth());
    float weapon_damage = g_ctx->m_active_weapon_data ? g_ctx->m_active_weapon_data->m_nDamage() : 100.f;

    // no hitboxes ticked in the menu = scan everything instead of nothing,
    // otherwise a fresh config silently never shoots.
    static const std::vector<int> all_hitbox_groups = { 0, 1, 2, 3, 4, 5, 6, 7 };
    const std::vector<int>* menu_hitboxes = &g_ragebot->m_config.m_hitboxes;
    if (menu_hitboxes->empty())
        menu_hitboxes = &all_hitbox_groups;

    for (int menu_hitbox : *menu_hitboxes) {
        bool is_multipointed = std::find(g_ragebot->m_config.m_multipointed_hitboxes.begin(), g_ragebot->m_config.m_multipointed_hitboxes.end(), menu_hitbox) != g_ragebot->m_config.m_multipointed_hitboxes.end();

        switch (menu_hitbox) {
        case 0: // head
            if (player_ctx.scale_damage(HITGROUP_HEAD, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[0].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[0]);
            }
            break;
        case 1: // upper chest
            if (player_ctx.scale_damage(HITGROUP_CHEST, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[6].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[6]);
            }
            break;
        case 2: // chest
            if (player_ctx.scale_damage(HITGROUP_CHEST, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[4].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[4]);
            }
            break;
        case 3: // stomach
            if (player_ctx.scale_damage(HITGROUP_STOMACH, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[3].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[3]);
            }
            break;
        case 4: // pelvis
            if (player_ctx.scale_damage(HITGROUP_STOMACH, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[2].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[2]);
            }
            break;
        case 5: // arms
            if (player_ctx.scale_damage(HITGROUP_LEFTARM, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[16].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[16]);

                record->m_all_hitboxes[18].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[18]);
            }
            break;
        case 6: // legs
            if (player_ctx.scale_damage(HITGROUP_LEFTLEG, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[9].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[9]);

                record->m_all_hitboxes[10].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[10]);
            }
            break;
        case 7: //feet
            if (player_ctx.scale_damage(HITGROUP_LEFTLEG, weapon_damage) >= dmg_threshold) {
                record->m_all_hitboxes[11].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[11]);

                record->m_all_hitboxes[12].m_multipoint = is_multipointed;
                store_hitboxes.emplace_back(record->m_all_hitboxes[12]);
            }
            break;
        }
    }
}

bool lag_record_t::setup(c_cs_player_pawn* pawn) {

    if (!pawn)
        return false;

    m_pawn = pawn;

    m_game_scene_node = pawn->m_pGameSceneNode();
    if (!m_game_scene_node)
        return false;

    m_skeleton_instance = m_game_scene_node->get_skeleton_instace();
    if (!m_skeleton_instance)
        return false;

    if (!m_skeleton_instance->get_bone_matrix())
        return false;

    // Store the actual world-space origin/rotation at record time
    m_origin = m_game_scene_node->m_vecAbsOrigin();
    m_rotation = m_game_scene_node->m_angAbsRotation();
    m_velocity = m_pawn->m_vecAbsVelocity();
    m_flags = static_cast<int>(m_pawn->m_fFlags());

    if (auto* move_services = m_pawn->m_pMovementServices())
        m_duck_amount = move_services->m_flDuckAmount();
    else
        m_duck_amount = 0.f;

    m_simulation_time = m_pawn->m_flSimulationTime();
    m_capture_time = g_interfaces->m_global_vars->m_real_time;

    // Calculate bones in world space (they're already at the correct abs_origin)
    m_skeleton_instance->calc_world_space_bones(0xFFFFF);

    const int bone_count=std::clamp(m_skeleton_instance->get_bone_count(),0,128);
    if(bone_count<28) return false;
    memset(m_bones,0,sizeof(m_bones));
    memcpy(m_bones,m_skeleton_instance->get_bone_matrix(),sizeof(matrix2x4_t)*bone_count);

    return true;
}

bool repose_record_for_scan(lag_record_t* record, lag_record_t* out, c_penetration::player_context_t& player_ctx) {
    if (!record || !out || !record->m_pawn)
        return false;

    c_cs_player_pawn* pawn = record->m_pawn;
    c_game_scene_node* node = pawn->m_pGameSceneNode();
    if (!node)
        return false;

    c_skeleton_instance* skeleton = node->get_skeleton_instace();
    if (!skeleton || !skeleton->get_bone_matrix())
        return false;

    *out = *record;

    const vec3_t saved_origin = node->m_vecAbsOrigin();
    const vec3_t saved_rotation = node->m_angAbsRotation();

    // temporarily pose the skeleton at the record transform so the rebuilt
    // hitboxes sit where the server simulates them, not where the client
    // interpolated them to at capture time.
    node->m_vecAbsOrigin() = record->m_origin;
    node->m_angAbsRotation() = record->m_rotation;
    skeleton->calc_world_space_bones(0xFFFFF);

    const int bone_count = std::clamp(skeleton->get_bone_count(), 0, 128);
    bool ok = bone_count >= 28;
    if (ok) {
        memset(out->m_bones, 0, sizeof(out->m_bones));
        memcpy(out->m_bones, skeleton->get_bone_matrix(), sizeof(matrix2x4_t) * bone_count);
        out->m_all_hitboxes.clear();
        out->m_rage_hitboxes.clear();
        store_hitboxes(out, player_ctx);
        ok = !out->m_rage_hitboxes.empty();
    }

    node->m_vecAbsOrigin() = saved_origin;
    node->m_angAbsRotation() = saved_rotation;
    return ok;
}

bool lag_record_t::is_valid() {
                                                                                  
    if (m_visual_only) {
        const float age = g_interfaces->m_global_vars->m_real_time - m_capture_time;
        return m_pawn && m_pawn == g_ctx->m_local_pawn && m_pawn->is_alive() &&
            age >= 0.0f && age <= 0.2f;
    }

    if (!g_ctx->m_local_controller)
        return false;

    auto network_client = g_interfaces->m_network_client_services->get_network_game_client();
    auto chan_info = g_interfaces->m_engine->get_net_channel_info();
    if (!chan_info)
        return false;
    if (!network_client)
        return false;

    auto fixed_sim_time = timestamp_t{ m_simulation_time };

    auto next_tick = g_ctx->m_local_controller->m_nTickBase() + 1;
    auto tick_count = timestamp_t{ next_tick, 0.f };

    auto net_latency = timestamp_t{ chan_info->get_net_latency() };

    static auto sv_maxunlag = g_interfaces->m_engine_convar->find_by_name("sv_maxunlag");

    auto max_unlag = timestamp_t{ sv_maxunlag->get_float() };

    auto sim_time = tick_count - net_latency;

    auto delta = fixed_sim_time - sim_time;
    delta.normalize();

    if (fixed_sim_time < tick_count - max_unlag)
        return false;

    if (tick_count < fixed_sim_time)
        return false;

    // hard 0.2s age kills high-ping players: their newest records are
    // already ~latency old through no fault of their own, so the history
    // thrashes down to 1-2 records and the bot goes silent. scale the
    // bound with real latency (capped, server unlag window still applies).
    float max_age = 0.2f + net_latency.to_time();
    if (max_age < 0.2f)
        max_age = 0.2f;
    if (max_age > 0.5f)
        max_age = 0.5f;

    return fabsf(g_interfaces->m_global_vars->m_curtime - m_simulation_time) <= max_age;
}

void c_lag_compensation::run() {
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!g_ctx->m_local_pawn)
        return;

    g_entity_cache->ensure_local_player();
    g_penetration->update_local_ctx();

    for (auto& ref_player : g_entity_cache->m_players) {
        if (!ref_player.check_and_update_pawn()) {
            ref_player.m_lagcomp_data.invalidate();
            continue;
        }

        g_chams->prune_old_onshot(ref_player.m_pawn);

        const bool is_local = (ref_player.m_pawn == g_ctx->m_local_pawn);

        if (is_local) {
            auto& history = ref_player.m_lagcomp_data;
            if (!ref_player.m_pawn->is_alive() ||
                !GET_VAR(bool, VISUALS_PATH(m_chams_local_enabled_bt))) {
                history.invalidate();
                cleanup_player_models(ref_player.m_pawn);
                continue;
            }

            while (!history.m_lag_records.empty() && !history.m_lag_records.oldest().is_valid())
                history.remove_oldest();

                                                                                    
            const bool should_update = history.m_lag_records.empty() ||
                g_interfaces->m_global_vars->m_real_time > history.m_lag_records.newest().m_capture_time;

            if (should_update) {
                lag_record_t new_record = {};
                if (new_record.setup(ref_player.m_pawn)) {
                    new_record.m_visual_only = true;
                    if (history.m_lag_records.size() == history.m_lag_records.capacity())
                        history.remove_oldest();
                    history.add_record(new_record);
                } else {
                    history.invalidate();
                }
            }

            s_backtrack_models[ref_player.m_pawn].handle(ref_player.m_pawn);

            continue;
        }

        if (!ref_player.m_pawn->is_enemy()) {
            ref_player.m_lagcomp_data.invalidate();
            continue;
        }

        // dormant entities carry stale bones/origins - shooting their
        // records is guaranteed misses, drop the history.
        c_game_scene_node* pawn_scene_node = ref_player.m_pawn->m_pGameSceneNode();
        if (!pawn_scene_node || pawn_scene_node->m_dormant()) {
            ref_player.m_lagcomp_data.invalidate();
            continue;
        }

        if (ref_player.m_pawn->is_alive() && !ref_player.m_pawn->m_bGunGameImmunity()) {
            ref_player.m_penetration_context.fill(ref_player.m_pawn);

            bool should_update = false;
            if (ref_player.m_lagcomp_data.m_lag_records.empty())
                should_update = true;
            else
                should_update = ref_player.m_pawn->m_flSimulationTime() >
                ref_player.m_lagcomp_data.m_lag_records.newest().m_simulation_time;

            if (should_update) {
                lag_record_t new_record = {};
                if (!new_record.setup(ref_player.m_pawn)) {
                    ref_player.m_lagcomp_data.invalidate();
                    continue;
                }

                auto& records = ref_player.m_lagcomp_data.m_lag_records;

                // breaking lag compensation / teleport fix: if the origin jumped
                // too far within a couple of ticks the player teleported (or breaks
                // lagcomp) and the server can no longer hit the old records,
                // so drop the stale history and keep only the fresh record.
                if (!records.empty()) {
                    constexpr float max_step_sqr = 64.f * 64.f;
                    const float sim_dt = new_record.m_simulation_time - records.newest().m_simulation_time;
                    if (sim_dt >= 0.f && sim_dt <= 0.0625f &&
                        new_record.m_origin.dist_to_sq(records.newest().m_origin) > max_step_sqr) {
                        ref_player.m_lagcomp_data.invalidate();
                    }
                }

                // tight_array::push_back throws on overflow, make room first.
                if (records.size() == records.capacity())
                    ref_player.m_lagcomp_data.remove_oldest();

                store_hitboxes(&new_record, ref_player.m_penetration_context);
                ref_player.m_lagcomp_data.add_record(new_record);
            }

            while (!ref_player.m_lagcomp_data.m_lag_records.empty() &&
                !ref_player.m_lagcomp_data.m_lag_records.oldest().is_valid()) {
                ref_player.m_lagcomp_data.remove_oldest();
            }
        }
        else 
            ref_player.m_lagcomp_data.invalidate();

        s_backtrack_models[ref_player.m_pawn].handle(ref_player.m_pawn);
    }
}

void c_lag_compensation::force_input_history() {
    static auto parse_input_message = g_modules->m_client.find(xx("48 89 5C 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 01 48 8B F9")).as<__int64(__fastcall*)(c_cs_input_message*, c_csgo_input_history_entry_pb*, bool, float, float, c_cs_player_pawn*)>();

    auto& target = g_ragebot->m_data.m_best_target;

    if (!target.m_record || !target.m_record->m_pawn)
        return;

    auto command = g_ctx->m_cmd;
    if (!parse_input_message || !command || !command->m_pb.m_base_cmd || !command->m_pb.m_input_history_field.valid())
        return;

    const float interpolation_time0 = target.m_record->m_pawn->get_some_timing(0, 1);
    const float interpolation_time1 = target.m_record->m_pawn->get_some_timing(1, 1);
    if (!std::isfinite(interpolation_time0) || !std::isfinite(interpolation_time1))
        return;
    const timestamp_t interpolation_timing0(interpolation_time0);

    c_cs_input_message input_message = {};

    input_message.m_view_angles = target.m_angle;

    input_message.m_player_tick_count = command->m_pb.m_base_cmd->m_client_tick;
    input_message.m_render_tick_count = time_to_ticks(target.m_record->m_simulation_time) + interpolation_timing0.m_tick;

    input_message.m_player_tick_fraction = interpolation_timing0.m_frac;
    input_message.m_render_tick_fraction = interpolation_timing0.m_frac;

    for (int i = 0; i < command->m_pb.m_input_history_field.size(); i++) {

        c_csgo_input_history_entry_pb* entry = command->m_pb.m_input_history_field[i];
        if (!entry)
            continue;

        parse_input_message(&input_message, entry, true, interpolation_time0, interpolation_time1, target.m_record->m_pawn);
    }
}

bool c_lag_compensation::wants_lag_compensation_on_entity(lag_record_t* victim) {

    vec3_t difference = victim->m_origin - g_ctx->m_local_pawn->m_pGameSceneNode()->m_vecOrigin();
    difference.normalize_place();

    vec3_t forward;
    g_ctx->m_base->get_view_angles()->m_ang_value.to_directions(&forward, nullptr, nullptr);

    if (forward.dot(difference) < VALID_LAGCOMP_CONE_COSINE)
        return false;

    return true;
}
