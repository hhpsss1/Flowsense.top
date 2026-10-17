#pragma once
#include <memory>
#include <vector>
#include <sdk/entity/controller.h>
#include <sdk/entity/pawn.h>
#include <sdk/interfaces/schema_system.h>
#include <core/interfaces/interfaces.h>
#include <utils/tight_array.h>
#include <cheat/features/lag compensation/lag_compensation.h>
#include <context.h>
#include <cheat/features/penetration/autowall.h>

void cleanup_player_models( c_cs_player_pawn* pawn );

struct cached_player_t {
    cached_player_t( ) = default;
    cached_player_t( c_cs_player_controller* ent, int idx )
        : m_controller( ent ), m_idx( idx )
    {
    }

    c_cs_player_controller* m_controller = nullptr;
    c_cs_player_pawn* m_pawn = nullptr;
    int                     m_idx = 0;

    bool check_and_update_pawn( ) {
        if ( !m_controller ) return false;
        auto* pawn = m_controller == g_ctx->m_local_controller ? g_ctx->m_local_pawn :
            m_controller->m_hPawn().get<c_cs_player_pawn>();
        if (m_pawn != pawn) {
            if (m_pawn)
                cleanup_player_models(m_pawn);
            m_lagcomp_data.invalidate();
            m_pawn = pawn;
        }
        return (m_pawn != nullptr);
    }

    struct lag_compensation_data_t {
        tight_array<lag_record_t, 24> m_lag_records;
        force_inline bool is_valid( ) { return m_valid; }
        force_inline void validate( ) { m_valid = true; }
        force_inline void invalidate( ) { m_lag_records.clear( ); m_valid = false; }
        force_inline void add_record( lag_record_t record ) { m_lag_records.push_back( record ); validate( ); }
        force_inline void remove_oldest( ) { m_lag_records.pop_front( ); if (m_lag_records.empty()) m_valid = false; }

    private:
        bool m_valid;
    }; 
    lag_compensation_data_t                        m_lagcomp_data = {};
    c_penetration::player_context_t         m_penetration_context = {};
};  

struct entity_object_t
{
    entity_object_t() = default;
    entity_object_t(c_base_entity* ent, int idx)
        : m_pEntity(ent), m_idx(idx)
    {
    }
    int                     m_idx = 0;

	c_base_entity* m_pEntity = nullptr;
	bool m_bPredictedGrenade = false;

};


class c_entity_cache {
public:

    tight_array<cached_player_t, 64> m_players;
    tight_array<entity_object_t, 50> m_entity;
    tight_array<entity_object_t, 5> m_c4_entity;
    tight_array<entity_object_t, 20> m_grenade_entity;
    tight_array<entity_object_t, 150> m_weapon_entity;

    void ensure_local_player() {
                                                                              
        auto* controller = g_ctx->m_local_controller;
        if (!controller || !controller->get_handle().is_valid())
            return;

        for (auto& player : m_players) {
            if (player.m_controller == controller)
                return;
        }

        if (m_players.size() < m_players.capacity())
            m_players.emplace_back(controller, controller->get_handle().get_entry_index());
    }

    cached_player_t& find( c_base_entity* entity ) {
        for ( auto& player : m_players ) {
            if ( player.m_pawn == entity )
                return player;
        }
        return m_players.front( );
    }

    void on_add( c_entity_instance* inst, c_base_handle handle ) {
        if ( !inst )
            return;
            
        const int idx = handle.get_entry_index( );
        if ( idx < 0 ) return;

        auto* ent = reinterpret_cast<c_base_entity*>(inst);
        if ( !ent || handle.get_entry_index( ) > 0x3FFF )
            return;
            
        if ( !ent->get_handle().is_valid() || ent->get_handle( ) != handle )
            return;

        auto binding = ent->get_class_binding_base( );
        if ( !binding ) return;

        const auto hashed = fnv_hash( binding->get_name( ) );
        if ( hashed == fnv_hash( xx( "CCSPlayerController" ) ) ) {

            for (auto& player : m_players) {
                if (player.m_idx != idx)
                    continue;
                if (player.m_controller != ent) {
                    if (player.m_pawn)
                        cleanup_player_models(player.m_pawn);
                    player = cached_player_t(static_cast<c_cs_player_controller*>(ent), idx);
                }
                return;
            }

            if ( m_players.size( ) < m_players.capacity( ) ) {
                m_players.push_back(
                    cached_player_t( static_cast<c_cs_player_controller*>(ent), idx )
                );
            }
        }
        else if (hashed == fnv_hash(xx("C_Inferno")))
        {
            m_entity.push_back(entity_object_t(static_cast<c_base_entity*>(ent), idx));
        }
        else if (hashed == fnv_hash(xx("C_PlantedC4")))
        {
            m_c4_entity.push_back(entity_object_t(static_cast<c_base_entity*>(ent), idx));
        }
        else if (hashed == fnv_hash(xx("C_HEGrenadeProjectile")) ||
            hashed == fnv_hash(xx("C_FlashbangProjectile")) ||
            hashed == fnv_hash(xx("C_SmokeGrenadeProjectile")) ||
            hashed == fnv_hash(xx("C_DecoyProjectile")) ||
            hashed == fnv_hash(xx("C_MolotovProjectile"))) {
            m_grenade_entity.push_back(entity_object_t(static_cast<c_base_entity*>(ent), idx));
        }
        else if (ent->is_weapon()) {
            m_weapon_entity.push_back(entity_object_t(static_cast<c_base_entity*>(ent), idx));
        }
    }

    void on_remove(c_entity_instance*,c_base_handle handle) {
 const int index=handle.get_entry_index();
 for(size_t i=0;i<m_players.size();++i) if(m_players[i].m_idx==index) {
  if(m_players[i].m_pawn) cleanup_player_models(m_players[i].m_pawn);
  m_players[i].m_lagcomp_data.invalidate();
  m_players.erase_unordered(i);
  break;
 }
 auto erase_index=[index](auto& entries) {
  for(size_t i=0;i<entries.size();) {
   if(entries[i].m_idx==index) entries.erase_unordered(i);
   else ++i;
  }
 };
 erase_index(m_entity);
 erase_index(m_c4_entity);
 erase_index(m_grenade_entity);
 erase_index(m_weapon_entity);
}

};

inline auto g_entity_cache = std::make_unique<c_entity_cache>( );
