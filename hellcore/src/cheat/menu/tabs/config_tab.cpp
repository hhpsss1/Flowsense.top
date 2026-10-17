#include "config_tab.h"

#include <includes.h>
#include <cheat/menu/hell_gui/hell_gui.h>
#include <cheat/config/config_system.h>
#include <cheat/features/visuals/overlay_features.h>

void tabs::render_config( ) {
	hell::set_cursor_pos_relative_y( );

	hellvec2 child_size = hell::calculate_column_size( 3 );

	static int selected_config = 0;
	static char config_name_buffer[256] = "";
	static std::vector<std::string> file_names = g_config_system->get_all_configs( );

	float spacing = 18.f;
	float start_y = ImGui::GetCursorPosY( );
	float col_spacing = ImGui::GetStyle( ).WindowPadding.x;

	float col1_x = ImGui::GetCursorPosX( );
	float col2_x = col1_x + child_size.x + col_spacing;

	if ( selected_config >= file_names.size( ) )
		selected_config = 0;

	float select_config_h = hell::calculate_child_height( [ & ] {
		for ( int i = 0; i < file_names.size( ); i++ ) {
			if ( hell::selectable( file_names[ i ].c_str( ), i == selected_config ) ) {
				selected_config = i;
			}
		}
		} );

	float create_config_h = hell::calculate_child_height( [ & ] {
		hell::input_text( "##config_name", config_name_buffer, sizeof( config_name_buffer ), "Config name...", ImGuiInputTextFlags_None, -1.f );

		if ( hell::button( xx( "Create" ), { -1.f, 0.f } ) ) {
			if ( strlen( config_name_buffer ) > 0 ) {
				g_config_system->save_config( config_name_buffer );
				g_overlay->add_notification("Created the " + std::string(config_name_buffer) + " config", false, true);
				config_name_buffer[ 0 ] = '\0';
			}
		}
		} );

	static bool should_refresh = false;
	float options_h = hell::calculate_child_height( [ & ] {
		if ( file_names.empty( ) )
			return;

		std::string selected_name = file_names[ selected_config ];

		if ( hell::button( xx( "Load" ), { -1.f, 0.f } ) ) {
			g_config_system->load_config( selected_name );
		}
		if ( hell::button( xx( "Save" ), { -1.f, 0.f } ) ) {
			g_config_system->save_config( selected_name );
			g_overlay->add_notification("Saved the " + selected_name + " config", false, true);
		}
		if ( hell::button( xx( "Delete" ), { -1.f, 0.f } ) ) {
			g_config_system->delete_config( selected_name );
			should_refresh = true;
		}
		} );

	ImGui::SetCursorPos( { col1_x, start_y } );
	hell::child( xx( "Select config" ), { child_size.x, select_config_h }, [ & ] {
		for ( int i = 0; i < file_names.size( ); i++ ) {
			if ( hell::selectable( file_names[ i ].c_str( ), i == selected_config ) ) {
				selected_config = i;
			}
		}
		} );

	ImGui::SetCursorPos( { col2_x, start_y } );
	hell::child( xx( "Create config" ), { child_size.x, create_config_h }, [ & ] {
		hell::input_text( "##config_name", config_name_buffer, sizeof( config_name_buffer ), "Config name...", ImGuiInputTextFlags_None, -1.f );

		if ( hell::button( xx( "Create" ), { -1.f, 0.f } ) ) {
			if ( strlen( config_name_buffer ) > 0 ) {
				g_config_system->save_config( config_name_buffer );
				g_overlay->add_notification("Created the " + std::string(config_name_buffer) + " config", false, true);
				config_name_buffer[ 0 ] = '\0';
				should_refresh = true;
			}
		}
		} );

	ImGui::SetCursorPos( { col2_x, start_y + create_config_h + col_spacing } );
	hell::child( xx( "Options" ), { child_size.x, options_h }, [ & ] {
		if ( file_names.empty( ) )
			return;

		std::string selected_name = file_names[ selected_config ];

		if ( hell::button( xx( "Load" ), { -1.f, 0.f } ) ) {
			g_config_system->load_config( selected_name );
		}
		if ( hell::button( xx( "Save" ), { -1.f, 0.f } ) ) {
			g_config_system->save_config( selected_name );
			g_overlay->add_notification("Saved the " + selected_name + " config", false, true);
		}
		if ( hell::button( xx( "Delete" ), { -1.f, 0.f } ) ) {
			g_config_system->delete_config( selected_name );
			should_refresh = true;
		}
		} );

	if ( should_refresh ) {
		file_names = g_config_system->get_all_configs( );
		if ( selected_config >= file_names.size( ) )
			selected_config = std::max( 0, ( int )file_names.size( ) - 1 );
		should_refresh = false;
	}
}
