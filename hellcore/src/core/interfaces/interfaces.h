#pragma once

#include <includes.h>

#include <sdk/interfaces/entity_system.h>
#include <sdk/interfaces/hitbox_system.h>
#include <sdk/interfaces/trace.h>
#include <sdk/interfaces/network_game_client_services.h>
#include <sdk/interfaces/prediction.h>
#include <sdk/interfaces/global_variables.h>
#include <sdk/interfaces/isteamfriend.h>
#include <sdk/interfaces/isteamutils.h>
#include <utils/memory.h>
#include <sdk/interfaces/source_client.h>
#include <sdk/interfaces/filesystem.h>
#include <sdk/interfaces/engine_client.h>
#include <sdk/interfaces/event_manager.h>
#include <sdk/interfaces/mesh_system.h>
#include <sdk/interfaces/game_rules.h>
//#include <../../src/cheat/features/visuals/particle_system.h>
class c_csgo_input;
class c_schema_system;
class c_input_system;
class c_engine_cvar;
class c_material_system_2;
class c_world;
class c_engine_pvs_manager;
class c_resource_system;
class c_game_particle_manager;
class c_particle_system_mgr;
class c_scene_system;
class c_panorama_ui_engine;
class c_localize {
public:
	const char* find_safe( const char* token ) {
		return g_memory->call_virtual<const char*>( this, 17, token );
	}
};
class c_swap_chain_dx_11
{
public:
	char pad_170[ 0x170 ];
	IDXGISwapChain* m_dxgi_swap_chain;
};


class c_interfaces {
public:
	template <typename T = void*>
	T find_interface( void* module_handle, const char* name );
	void init( void );
    bool refresh_global_vars();
    void refresh_level_state();
	void create_render_target( );
	void destroy_render_target( );
public:
	using tt = uint64_t(__cdecl*)(void);
	using fn_steam_client = ISteamClient * (__cdecl*)(void);

	c_csgo_input* m_csgo_input;
	c_global_variables* m_global_vars;
	c_schema_system* m_schema_system;
	c_entity_system* m_entity_system;
	c_hitbox_system* m_hitbox_system;
	c_vphys2world* m_phys2world;
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_device_context;
	ID3D11RenderTargetView* m_render_target_view;
	c_vhpysics* m_vphysics2;
	c_swap_chain_dx_11* m_swap_chain;
	c_input_system* m_input_system;
	c_localize* m_localize;
	c_engine_cvar* m_engine_convar;
	c_game_rules* m_game_rules;
	c_material_system_2* m_material_system;
	c_network_game_client_services* m_network_client_services;
	c_prediction* m_prediction;
	c_source2_client* m_source2_client;
	c_file_system* m_file_system;
	c_engine_client* m_engine;
	c_world* m_world;
	c_game_event_manager2* m_game_event_manager;
	c_mesh_system* m_mesh_system;
	c_scene_system* m_scene_system;
	c_engine_pvs_manager* m_pvs;
	c_resource_system* m_resource_system;
	c_game_particle_manager* m_game_particle_manager;
	c_particle_system_mgr* m_particle_system_mgr;
	c_panorama_ui_engine* m_panorama_ui_engine;
	ISteamFriends* m_steam_friends;
	ISteamUtils* m_steam_utils;

	bool m_pvs_bool;
};
inline auto g_interfaces = std::make_unique<c_interfaces>( );