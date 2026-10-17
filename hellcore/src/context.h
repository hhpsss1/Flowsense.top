#pragma once

#include <includes.h>
#include "sdk/entity/controller.h"
#include "sdk/entity/pawn.h"
#include "core/interfaces/interfaces.h"
#include <cheat/config/vars.h>

// lol making this shit constexpr not really making a difference in most cases, unless we doing time_to_ticks ( 1 ) somewhere i guess we gonna save 2 picoseconds but its all worth it my mind bugs me if i know something can be precomputated and it isnt lowkey remind me to never have 3 coffees in 1 day and not eat anything else then hit a shit ton of weed and stuff
constexpr float interval_per_tick = 0.015625f;

constexpr int time_to_ticks( float time ) {
	return static_cast<int>(0.5f + time / interval_per_tick);
}

constexpr float ticks_to_time( int ticks ) {
	return interval_per_tick * static_cast<float>(ticks);
}

#if defined(_MSC_VER) && !defined(__clang__)
#define force_inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define force_inline inline __attribute__((always_inline))
#else
#define force_inline inline
#endif

class c_context {
public:

	std::string m_running_path;
	void* m_this_module;
	HWND m_window;
	WNDPROC m_old_wnd_proc;

	// false when client.dll build doesn't match: cheat still injects and
	// shows the menu, but all client.dll-dependent init is skipped.
	bool m_client_valid = false;

	c_user_cmd_manager* m_cmd_manager;
	c_user_cmd* m_cmd;
	c_base_user_cmd_pb* m_base;

	c_cs_player_pawn* m_local_pawn;
	c_cs_player_controller* m_local_controller;
	c_weapon_base* m_active_weapon;
	c_weapon_base_v_data* m_active_weapon_data;
	vec3_t m_shoot_position;
	vec3_t m_extrapolated_shoot_position;

	int m_weapon_def_index;
	bool m_rage_config_needs_update = false;

	bool m_dumped_skins = false;

	bool update( ) {
		if ( !m_local_controller || !m_local_pawn )
			return false;

		m_cmd_manager = m_local_controller->get_cmd_manager( );
		if ( !m_cmd_manager )
			return false;

		m_cmd = m_cmd_manager->get_cmd( );
		if ( !m_cmd )
			return false;

		m_base = m_cmd->m_pb.m_base_cmd;
		if ( !m_base )
			return false;

		m_active_weapon = m_local_pawn->get_active_weapon( );
		if ( !m_active_weapon )
			return false;

		m_active_weapon_data = m_active_weapon->weapon_data( );
		if ( !m_active_weapon_data )
			return false;

		static int last_wep_def = m_active_weapon->m_AttributeManager( )->m_Item( )->m_iItemDefinitionIndex( );
		m_weapon_def_index = m_active_weapon->m_AttributeManager( )->m_Item( )->m_iItemDefinitionIndex( );

		if ( last_wep_def != m_weapon_def_index )
			m_rage_config_needs_update = true; // our weapon changed, we need to upd config we use
		last_wep_def = m_weapon_def_index;
		return true;
	}

	void play_v_sound( const char* sound, bool called_from_imgui_internal = false ) {
		if ( called_from_imgui_internal && !GET_VAR( bool, MISC_PATH( m_menu_click_sounds ) ) )
			return;

		using fn_play_v_sound = void( __fastcall* )( const char* );
		static fn_play_v_sound play_v_sound = g_modules->m_soundsystem.find(xx("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 33 F6 C7 45 ? ? ? ? ? 4C 8B C1")).as<fn_play_v_sound>();
		if (play_v_sound && sound && *sound) play_v_sound( sound );
	}

	bool is_move_valid() noexcept {
		if (!m_local_pawn)
			return false;
		const auto moveType = m_local_pawn->m_nActualMoveType();
		return moveType != MOVETYPE_NOCLIP && moveType != MOVETYPE_LADDER;
	}
};
inline auto g_ctx = std::make_unique<c_context>( );
