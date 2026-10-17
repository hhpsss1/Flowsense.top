#include "engine_prediction.h"

#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/global_variables.h>

void c_engine_prediction::calculate_shoot_position() {
    auto* pawn = g_ctx->m_local_pawn;
    auto* controller = g_ctx->m_local_controller;
    g_ctx->m_shoot_position = {};
    if (!pawn || !controller || !pawn->m_pGameSceneNode())
        return;

    vec3_t result = pawn->get_shoot_pos();
    g_ctx->m_shoot_position = result;
    auto* services = pawn->m_pWeaponServices();
    if (!services || services->m_owner_pawn() != pawn)
        return;

    const timestamp_t timing(controller->m_nTickBase(), 0.f);
    if (!services->has_shoot_position_history(timing.m_tick, timing.m_frac))
        return;

    using fn = vec3_t*(__fastcall*)(c_player_weapon_services*, vec3_t*, const timestamp_t*);
    static auto calculate_shoot_pos = g_modules->m_client.find(xx("40 55 56 41 56 48 81 EC 20 01 00 00")).as<fn>();
    if (calculate_shoot_pos)
        calculate_shoot_pos(services, &result, &timing);
    if (std::isfinite(result.x) && std::isfinite(result.y) && std::isfinite(result.z))
        g_ctx->m_shoot_position = result;
}

void c_engine_prediction::begin() {
 if(!g_ctx->m_cmd || !g_ctx->m_local_pawn || !g_ctx->m_local_controller || !g_interfaces->m_global_vars) return;
 auto* pawn=g_ctx->m_local_pawn;
 m_prediction_data.m_pre_prediction_flags=pawn->m_fFlags();
 m_prediction_data.m_post_prediction_flags=m_prediction_data.m_pre_prediction_flags;
 m_prediction_data.m_velocity=pawn->m_vecVelocity();
 m_prediction_data.m_abs_velocity=pawn->m_vecAbsVelocity();
 auto* node=pawn->m_pGameSceneNode();
 if(node) m_prediction_data.m_pre_abs_origin=node->m_vecAbsOrigin();
 calculate_shoot_position();
 auto* collision=pawn->m_pCollision();
 if(node && collision && g_interfaces->m_phys2world && !(pawn->m_fFlags()&FL_ONGROUND) && pawn->m_vecVelocity().z<=0.f) {
  const auto start=node->m_vecAbsOrigin();
  auto end=start+pawn->m_vecVelocity()*(1.f/64.f);
  end.z-=800.f*(1.f/64.f)*(1.f/64.f)*0.5f;
  c_ray hull{};
  hull.m_mins=collision->m_vecMins();
  hull.m_maxs=collision->m_vecMaxs();
  hull.m_type=RAY_TYPE_HULL;
  c_trace_filter filter(0x1C300B,pawn,nullptr,8);
  c_game_trace trace{};
  auto mutable_start=start;
  if(g_interfaces->m_phys2world->trace_shape(&hull,&mutable_start,&end,&filter,&trace) && !trace.m_start_in_solid && trace.m_fraction<1.f && trace.m_hit_normal.z>0.7f)
   m_prediction_data.m_post_prediction_flags|=FL_ONGROUND;
 }
}

void c_engine_prediction::end() {
}