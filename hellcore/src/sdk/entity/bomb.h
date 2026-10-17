#pragma once

#include <includes.h>

#include <sdk/entity/entity.h>
#include <sdk/entity/pawn.h>
#include <sdk/interfaces/entity_system.h>
#include <sdk/interfaces/trace.h>
#include <core/interfaces/interfaces.h>
#include <cheat/features/entity cache/entity_cache.h>

class C_PlantedC4 : public c_base_entity {
public:
    SCHEMA(bool,  m_bBombTicking,      ("C_PlantedC4->m_bBombTicking"));
    SCHEMA(float, m_flC4Blow,          ("C_PlantedC4->m_flC4Blow"));
    SCHEMA(float, m_flTimerLength,     ("C_PlantedC4->m_flTimerLength"));
    SCHEMA(bool,  m_bBeingDefused,     ("C_PlantedC4->m_bBeingDefused"));
    SCHEMA(float, m_flDefuseLength,    ("C_PlantedC4->m_flDefuseLength"));
    SCHEMA(float, m_flDefuseCountDown, ("C_PlantedC4->m_flDefuseCountDown"));
    SCHEMA(bool,  m_bBombDefused,      ("C_PlantedC4->m_bBombDefused"));
    SCHEMA(int,   m_nBombSite,         ("C_PlantedC4->m_nBombSite"));
};

namespace bomb
{

    inline C_PlantedC4* get_planted_bomb()
    {
        for (auto& object : g_entity_cache->m_c4_entity)
        {
            if (!object.m_pEntity)
                continue;

            auto c4 = reinterpret_cast<C_PlantedC4*>(object.m_pEntity);
            if (!c4)
                continue;

            if (!c4->m_bBombTicking() || c4->m_bBombDefused())
                continue;

            return c4;
        }

        return nullptr;
    }

    inline bool is_bomb_planted()
    {
        C_PlantedC4* bomb_ent = get_planted_bomb();
        return bomb_ent && bomb_ent->m_bBombTicking();
    }

    inline int get_bomb_site()
    {
        C_PlantedC4* bomb_ent = get_planted_bomb();
        if (!bomb_ent || !bomb_ent->m_bBombTicking())
            return -1;

        return bomb_ent->m_nBombSite();
    }

    inline float get_time_to_explode()
    {

        C_PlantedC4* bomb_ent = get_planted_bomb();
        if (!bomb_ent || !bomb_ent->m_bBombTicking() || !g_interfaces || !g_interfaces->m_global_vars)
            return -1.0f;

        float curtime = g_interfaces->m_global_vars->m_curtime;
        return bomb_ent->m_flC4Blow() - curtime;
    }

    inline int get_bomb_damage_to_player(c_cs_player_pawn* pawn)
    {
        if (!pawn || pawn->m_iHealth() <= 0 || !pawn->is_alive())
            return 0;

        C_PlantedC4* bomb_ent = get_planted_bomb();
        if (!bomb_ent || !bomb_ent->m_bBombTicking())
            return 0;

        auto scene_node = bomb_ent->m_pGameSceneNode();
        if (!scene_node)
            return 0;

        vec3_t& bomb_pos = scene_node->m_vecAbsOrigin();
        vec3_t player_pos = pawn->get_world_space_center();

        float dist = player_pos.dist_to(bomb_pos);

        const float bombDamage = 500.0f;
        const float bombRadius = 2410.0f;

        float dmg = bombDamage * std::exp(
            - (dist * dist) /
            (2.0f * std::pow(bombRadius / 3.0f, 2.0f))
        );

        if (pawn->m_ArmorValue() > 0)
        {
            float armor_ratio_part = dmg * 0.5f;
            float armor_damage = (dmg - armor_ratio_part) * 0.5f;

            if (armor_damage > static_cast<float>(pawn->m_ArmorValue()))
            {
                armor_damage = static_cast<float>(pawn->m_ArmorValue()) * (1.0f / 0.5f);
                armor_ratio_part = dmg - armor_damage;
            }

            dmg = armor_ratio_part;
        }

        return static_cast<int>(std::round(dmg));
    }
}


