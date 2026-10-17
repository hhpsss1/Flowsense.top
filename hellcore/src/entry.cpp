#include "entry.h"

#include <core/hooks/modules.h>
#include <core/hooks/hooks.h>
#include <core/interfaces/interfaces.h>
#include <sdk/schema/schema.h>
#include <cheat/input.h>
#include <cheat/config/config_system.h>
#include <cheat/features/visuals/chams.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/features/skins/skins.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/features/visuals/particle_manager.h>

void c_entry::entry( void* moduleptr ) {
#ifdef _DEBUG
	g_logging->init( xx( "hellcore-console" ) );
#endif
	LOG( xx( "hellcore developer console" ) );

	g_modules->init( );
    const auto* client_base=static_cast<const std::byte*>(g_modules->m_client.get());
    if(!client_base) return;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(client_base);
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(client_base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=0x6AD01314 || nt->OptionalHeader.SizeOfImage!=0x299D000) {
        LOG_ERROR("Unsupported client.dll build, running in menu-only mode");
        MessageBoxA(nullptr, "Unsupported client.dll build: menu only, features disabled", "hellcore", MB_OK | MB_ICONWARNING);
        g_ctx->m_client_valid = false;
    }
    else {
        g_ctx->m_client_valid = true;
    }
	LOG( xx( "modules init" ) );

	g_input->init( );
	LOG( xx( "input init" ) )

	g_interfaces->init( );
	LOG( xx( "interfaces init" ) );

	const bool render_ok = g_interfaces->m_device && g_interfaces->m_swap_chain &&
		g_interfaces->m_swap_chain->m_dxgi_swap_chain;

	g_config_system->init( );
	LOG( xx( "config system init" ) );

	g_chams->init( );
	LOG( xx( "chams init" ) );

	if ( !render_ok || !g_ctx->m_client_valid ) {
		LOG( xx( "skipping client-dependent init (events, skins dump)" ) );
		if ( render_ok ) {
			g_hooks->init( );
			LOG( xx( "hooks init" ) );
		}
		return;
	}

	g_event_listener->setup( {
		xx( "round_start" ), xx( "add_bullet_hit_marker" ), xx( "bullet_impact" ),
		xx( "player_hurt" ), xx( "player_death" ), xx( "weapon_fire" ), xx( "vote_cast" ),
		xx( "vote_started" ), xx( "item_purchase" ), xx( "bomb_defused" ),
		xx( "bomb_begindefuse" ), xx( "bomb_planted" ), xx( "bomb_beginplant" )
	} );
	LOG( xx( "events init" ) );

	g_skins->dump_items( );
	LOG( xx( "skins dumped" ) );

	g_hooks->init( );
	LOG( xx( "hooks init" ) );
}

void c_entry::cleanup( void ) {
	//if (g_interfaces) {
	//	g_interfaces->destroy_render_target();
	//}
	//
	//for (auto& [pawn, model] : s_backtrack_models) {
	//	model.remove(REMOVE_ALL);
	//}
	//s_backtrack_models.clear();
	//
	//for (auto& [pawn, models] : s_onshot_models) {
	//	for (auto& model : models) {
	//		model.remove(REMOVE_ALL);
	//	}
	//}
	//s_onshot_models.clear();

	//s_onshot_skeletons.clear();
	//s_backtrack_skeletons.clear();
	//
	//for (auto& [hash, icon] : m_icons) {
	//	if (icon.texture_view) {
	//		icon.texture_view->Release();
	//	}
	//}
	//m_icons.clear();
	//
	//s_vecPredictedGrenades.clear();
	//
	//if (g_particle_mgr) {
	//	g_particle_mgr->clear_all_particles();
	//}
	//
	//if (g_overlay) {
	//	g_overlay->clear();
	//	g_overlay->clear_hitmarks();
	//}
}
