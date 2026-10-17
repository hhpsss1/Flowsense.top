#include "misc_tab.h"

#include <includes.h>
#include <cheat/menu/hell_gui/hell_gui.h>

inline const char* hitsound_names[ ] = {
	xx( "Coin" ),
	xx( "Click" ),
	xx( "Shot" ),
	xx( "Assembly" ),
	xx( "Star" )
};

void tabs::render_misc( ) {
	hell::set_cursor_pos_relative_y( );

	hellvec2 child_size = hell::calculate_column_size( 3 );

	float spacing = 18.f;
	float start_y = ImGui::GetCursorPosY( );
	float col_spacing = ImGui::GetStyle( ).WindowPadding.x;

	float col1_x = ImGui::GetCursorPosX( );
	float col2_x = col1_x + child_size.x + col_spacing;

	static const char* primary_weapon_names[] = {
		xx("None"),
		xx("SSG 08"),
		xx("Auto Sniper"),
		xx("AWP")
	};
	
	static const char* secondary_weapon_names[] = {
		xx("None"),
		xx("Deagle"),
		xx("P250"),
		xx("Revolver"),
		xx("Dual Berettas")
	};
	
	static const char* additional_item_names[] = {
		xx("HE Grenade"),
		xx("Molotov"),
		xx("Smoke"),
		xx("Zeus"),
		xx("Armor"),
		xx("Kits")
	};

	float other_h = hell::calculate_child_height( [ ] {
		if ( hell::button( xx( "Debug" ), { -1.f, 0.f } ) ) {
			g_interfaces->m_engine->exec_client_cmd_unrestricted( xx( "sv_cheats 1; bot_kick; bot_stop 1; bot_add; sv_quantize_movement_input 0; sv_airaccelerate 875; sv_falldamage_scale 1; mp_limitteams 0; mp_autoteambalance 0; mp_respawn_immunitytime -1;" ) );
		}
		hell::checkbox(xx("Hit sound"), MISC_PATH(m_enabled_hitsound), [] {
			int selection = (int)GET_VAR(int, MISC_PATH(m_hitsound_selection));
			hell::combo(xx("Sound"), selection, hitsound_names, e_hitsounds::hitsounds_max);
			GET_VAR(int, MISC_PATH(m_hitsound_selection)) = (int)selection;
			});
		hell::checkbox( xx( "Preserve killfeed" ), MISC_PATH(m_preserve_kill_feed) );
		hell::checkbox( xx( "Reveal scoreboard equipment" ), MISC_PATH(m_reveal_equipment) );
		hell::checkbox(xx("Auto buy"), MISC_PATH(m_enabled_autobuy), [] {
			int primary_selection = GET_VAR(int, MISC_PATH(m_autobuy_primary));
			hell::combo(xx("Primary"), primary_selection, primary_weapon_names, 4);
			GET_VAR(int, MISC_PATH(m_autobuy_primary)) = primary_selection;

			int secondary_selection = GET_VAR(int, MISC_PATH(m_autobuy_secondary));
			hell::combo(xx("Secondary"), secondary_selection, secondary_weapon_names, 5);
			GET_VAR(int, MISC_PATH(m_autobuy_secondary)) = secondary_selection;

			hell::multi_combo(xx("Additional"), additional_item_names, GET_VAR(std::vector<bool>, MISC_PATH(m_autobuy_additional)), 6);
			});
		//hell::checkbox(xx("Menu Click Sound"), MISC_PATH(m_menu_click_sounds));
		hell::checkbox(xx("Watemark"), MISC_PATH(m_enabled_watermark));
		hell::checkbox(xx("Hotkeys list"), VISUALS_PATH(m_enabled_hotkey_list));
		hell::checkbox(xx("Observer list"), VISUALS_PATH(m_enabled_observer_list));
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		hell::color_picker(xx("AccentClr"), GET_VAR(hellcolor, MISC_PATH(m_accent_color)), 0, true);

		ImGui::Text("Accent color");

		static const char* hitlog_mode_names[] = {
			xx("Hit"),
			xx("Miss")
		};
		hell::multi_combo(xx("Shot Logs"), hitlog_mode_names, GET_VAR(std::vector<bool>, MISC_PATH(m_hitlog_modes)), 2);
		static const char* hitlog_pos_names[] = {
			xx("Bottom"),
			xx("Top left")
		};
		int hitlog_pos_calc = GET_VAR(int, MISC_PATH(m_hitlog_position));
		hell::combo(xx("Hitlog position"), hitlog_pos_calc, hitlog_pos_names, 2);
		GET_VAR(int, MISC_PATH(m_hitlog_position)) = hitlog_pos_calc;
		static const char* indicator_mode_names[] = {
			xx("Rage"),
			xx("Hide shots"),
			xx("Force shoot"),
			xx("Ping"),
			xx("Hit chance"),
			xx("Bomb"),
			xx("Min damage"),
			xx("Slow"),
			xx("Left"),
			xx("Right")
		};
		hell::checkbox(xx("Indicators"), MISC_PATH(m_enabled_indicators));
		hell::multi_combo(xx("Indicator modes"), indicator_mode_names, GET_VAR(std::vector<bool>, MISC_PATH(m_indicator_modes)), 10);
		} );

	float movement_h = hell::calculate_child_height( [ ] {
		hell::checkbox( xx( "Bunnyhop" ), MISC_PATH( m_enabled_bunny_hop ) );
		hell::checkbox(xx("Standalone quick stop"), MISC_PATH(m_enabled_quick_stop));
		hell::checkbox(xx("Edge jump"), MISC_PATH(m_enabled_edge_jump));
		hell::checkbox(xx("Jump bug"), MISC_PATH(m_enabled_jump_bug));
		hell::checkbox(xx("Long jump"), MISC_PATH(m_enabled_long_jump));
		hell::checkbox( xx( "Auto strafe" ), MISC_PATH( m_enabled_autostrafe ), [ ] {
			hell::checkbox(xx("Ramp boost"), MISC_PATH(m_enabled_ramp_boost));
			//hell::slider_int( xx( "Boost" ), MISC_PATH( m_autostrafe_boost ), 1, 100, 0, false, xx( "%d%%" ) );
			} );
		hell::checkbox( xx( "Override slow walk" ), MISC_PATH( m_enabled_slow_walk ), [ ] {
			hell::slider_int( xx( "Speed" ), MISC_PATH( m_slow_walk_percent ), 1, 100, 0, false, xx( "%d%%" ) );
			} );
		hell::checkbox(xx("Straight throw"), MISC_PATH(m_enabled_straight_throw));
		hell::checkbox(xx("Auto grenade release"), MISC_PATH(m_enabled_grenade_release), [] {
			hell::slider_int(xx("Damage"), MISC_PATH(m_grenade_release_damage), 1, 99, 0, false, xx("%dHP"));
			});
		} );

	ImGui::SetCursorPos( { col1_x, start_y } );
	hell::child( xx( "Other" ), { child_size.x, other_h }, [ ] {
		if ( hell::button( xx( "Debug" ), { -1.f, 0.f } ) ) {
			g_interfaces->m_engine->exec_client_cmd_unrestricted( xx( "sv_cheats 1; bot_stop 1; bot_add; sv_quantize_movement_input 0; sv_airaccelerate 875; sv_falldamage_scale 0; mp_limitteams 0; mp_autoteambalance 0; mp_respawn_immunitytime -1;" ) );
		}

		hell::checkbox(xx("Hit sound"), MISC_PATH(m_enabled_hitsound), [] {
			int selection = (int)GET_VAR(int, MISC_PATH(m_hitsound_selection));
			hell::combo(xx("Sound"), selection, hitsound_names, e_hitsounds::hitsounds_max);
			GET_VAR(int, MISC_PATH(m_hitsound_selection)) = (int)selection;
			});
		hell::checkbox(xx("Preserse killfeed"), MISC_PATH(m_preserve_kill_feed));
		hell::checkbox(xx("Reveal scoreboard equipment"), MISC_PATH(m_reveal_equipment));
		hell::checkbox(xx("Auto buy"), MISC_PATH(m_enabled_autobuy), [] {
			int primary_selection = GET_VAR(int, MISC_PATH(m_autobuy_primary));
			hell::combo(xx("Primary"), primary_selection, primary_weapon_names, 4);
			GET_VAR(int, MISC_PATH(m_autobuy_primary)) = primary_selection;

			int secondary_selection = GET_VAR(int, MISC_PATH(m_autobuy_secondary));
			hell::combo(xx("Secondary"), secondary_selection, secondary_weapon_names, 5);
			GET_VAR(int, MISC_PATH(m_autobuy_secondary)) = secondary_selection;

			hell::multi_combo(xx("Additional"), additional_item_names, GET_VAR(std::vector<bool>, MISC_PATH(m_autobuy_additional)), 6);
			});
		//hell::checkbox(xx("Menu Click Sound"), MISC_PATH(m_menu_click_sounds));
		hell::checkbox(xx("Watermark"), MISC_PATH(m_enabled_watermark));
		hell::checkbox(xx("Hotkey list"), VISUALS_PATH(m_enabled_hotkey_list));
		hell::checkbox(xx("Observer list"), VISUALS_PATH(m_enabled_observer_list));
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		hell::color_picker(xx("AccentClr"), GET_VAR(hellcolor, MISC_PATH(m_accent_color)), 0, true);

		ImGui::Text("Accent color");

		static const char* hitlog_mode_names[] = {
			xx("Hit"),
			xx("Miss")
		};
		hell::multi_combo(xx("Shot logs"), hitlog_mode_names, GET_VAR(std::vector<bool>, MISC_PATH(m_hitlog_modes)), 2);
		static const char* hitlog_pos_names[] = {
			xx("Bottom"),
			xx("Top left")
		};
		int hitlog_pos = GET_VAR(int, MISC_PATH(m_hitlog_position));
		hell::combo(xx("Hitlog position"), hitlog_pos, hitlog_pos_names, 2);
		GET_VAR(int, MISC_PATH(m_hitlog_position)) = hitlog_pos;
		static const char* indicator_mode_names[] = {
			xx("Rage"),
			xx("Hide shots"),
			xx("Force shoot"),
			xx("Ping"),
			xx("Hit chance"),
			xx("Bomb"),
			xx("Min damage"),
			xx("Slow"),
			xx("Left"),
			xx("Right")
		};
		hell::checkbox(xx("Indicators"), MISC_PATH(m_enabled_indicators));
		hell::multi_combo(xx("Indicator modes"), indicator_mode_names, GET_VAR(std::vector<bool>, MISC_PATH(m_indicator_modes)), 10);
		
		} );

	ImGui::SetCursorPos( { col2_x, start_y } );
	hell::child( xx( "Movement" ), { child_size.x, movement_h }, [ ] {
		hell::checkbox( xx( "Bunnyhop" ), MISC_PATH( m_enabled_bunny_hop ) );
		hell::checkbox(xx("Standalone quick stop"), MISC_PATH(m_enabled_quick_stop));
		hell::checkbox(xx("Edge jump"), MISC_PATH(m_enabled_edge_jump));
		hell::checkbox(xx("Jump bug"), MISC_PATH(m_enabled_jump_bug));
		hell::checkbox(xx("Long jump"), MISC_PATH(m_enabled_long_jump));
		hell::checkbox( xx( "Auto strafe" ), MISC_PATH( m_enabled_autostrafe ), [ ] {
			hell::checkbox(xx("Ramp boost"), MISC_PATH(m_enabled_ramp_boost));
			//hell::slider_int( xx( "Boost" ), MISC_PATH( m_autostrafe_boost ), 1, 100, 0, false, xx( "%d%%" ) );
			} );

		hell::checkbox( xx( "Override slow walk" ), MISC_PATH( m_enabled_slow_walk ), [ ] {
			hell::slider_int( xx( "Speed" ), MISC_PATH( m_slow_walk_percent ), 1, 100, 0, false, xx( "%d%%" ) );
			} );
		hell::checkbox( xx( "Straight throw" ), MISC_PATH( m_enabled_straight_throw ) );
		hell::checkbox( xx( "Auto grenade release" ), MISC_PATH( m_enabled_grenade_release ), [ ] {
			hell::slider_int( xx( "Damage" ), MISC_PATH( m_grenade_release_damage ), 1, 99, 0, false, xx( "%dHP" ) );
			} );

		} );
}