#include "input.h"
#include <cstring>
#include <context.h>
#include <cheat/hooks/render_system/render_system_hooks.h>
#include <core/interfaces/interfaces.h>

struct window_search_t {
	HWND first_acceptable = nullptr;
	HWND sdl_window = nullptr;
};

static BOOL CALLBACK enum_windows_callback( HWND handle, LPARAM lparam ) {
	DWORD pid = 0;
	GetWindowThreadProcessId( handle, &pid );

	if ( GetCurrentProcessId( ) != pid )
		return TRUE;

	if ( GetWindow( handle, GW_OWNER ) != nullptr || !IsWindowVisible( handle ) || handle == GetConsoleWindow( ) )
		return TRUE;

	// CS2's real window is SDL_app; other visible ownerless windows
	// (CEF overlays, helper windows) must not steal the wndproc hook,
	// otherwise the menu key never reaches us.
	char class_name[ 64 ] = {};
	GetClassNameA( handle, class_name, sizeof( class_name ) - 1 );

	auto* out = reinterpret_cast<window_search_t*>( lparam );
	if ( !out->first_acceptable )
		out->first_acceptable = handle;

	if ( std::strcmp( class_name, "SDL_app" ) == 0 ) {
		out->sdl_window = handle;
		return FALSE;
	}

	return TRUE;
}

void c_input::init( void ) {
	while ( !g_ctx->m_window ) {
		window_search_t search{ };
		EnumWindows( enum_windows_callback, reinterpret_cast<LPARAM>( &search ) );

		if ( search.sdl_window )
			g_ctx->m_window = search.sdl_window;
		else
			g_ctx->m_window = search.first_acceptable;

		if ( !g_ctx->m_window )
			Sleep( 200 );
	}

	g_ctx->m_old_wnd_proc = reinterpret_cast<WNDPROC>( SetWindowLongPtrW( g_ctx->m_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( c_render_system_hooks::wnd_proc ) ) );
}

void c_input::wnd_proc( UINT msg, WPARAM wparam, LPARAM lparam ) {
	WPARAM curr_key = 0;
	e_key_state state = e_key_state::none;

	switch ( msg ) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ( wparam < 256U ) {
			curr_key = wparam;
			state = e_key_state::down;
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if ( wparam < 256U ) {
			curr_key = wparam;
			state = e_key_state::up;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		curr_key = VK_LBUTTON;
		state = e_key_state::down;
		break;
	case WM_LBUTTONUP:
		curr_key = VK_LBUTTON;
		state = e_key_state::up;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		curr_key = VK_RBUTTON;
		state = e_key_state::down;
		break;
	case WM_RBUTTONUP:
		curr_key = VK_RBUTTON;
		state = e_key_state::up;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		curr_key = VK_MBUTTON;
		state = e_key_state::down;
		break;
	case WM_MBUTTONUP:
		curr_key = VK_MBUTTON;
		state = e_key_state::up;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
	{
		uint16_t xButton = ( (WORD)( ( ( (DWORD_PTR)( ( wparam ) ) >> 16 ) & 0xffff ) ) );
		curr_key = ( xButton == XBUTTON1 ) ? VK_XBUTTON1 : VK_XBUTTON2;
		state = e_key_state::down;
		break;
	}
	case WM_XBUTTONUP:
	{
		uint16_t xButton = ( (WORD)( ( ( (DWORD_PTR)( ( wparam ) ) >> 16 ) & 0xffff ) ) );
		curr_key = ( xButton == XBUTTON1 ) ? VK_XBUTTON1 : VK_XBUTTON2;
		state = e_key_state::up;
		break;
	}
	default:
		return;
	}

	if ( curr_key != 0 && state != e_key_state::none ) {
		if ( state == e_key_state::up && m_key_states[ curr_key ] == e_key_state::down )
			m_key_states.at( curr_key ) = e_key_state::released;
		else
			m_key_states.at( curr_key ) = state;
	}

	//need to fix player alive crash
	bool is_player_alive = g_ctx->m_local_controller && g_ctx->m_local_pawn && g_ctx->m_local_pawn->m_iHealth() > 0;
	bool is_in_game = g_interfaces->m_engine && g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game();
	
	static bool was_alive = false;
	static bool was_in_game = false;
	
	if (is_in_game)
	{
		if (was_alive && !is_player_alive) {
			for (auto& [widget_id, binds] : GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds))) {
				for (auto& keybind : binds) {
					if (keybind.m_keybind_type == e_keybind_type::keybind_type_checkbox) {
						using T = bool;
						auto& actual = GET_VAR(T, keybind.m_holder_id);
						if (keybind.m_keybind_active || keybind.m_on_mode_activated) {
							actual = false;
							keybind.m_keybind_active = false;
							keybind.m_on_mode_activated = false;
						}
					}
					else if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_int ||
						keybind.m_keybind_type == e_keybind_type::keybind_type_slider_float) {
						if (keybind.m_keybind_active) {
							if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_int) {
								if (std::holds_alternative<int>(keybind.m_default_val)) {
									int def = std::get<int>(keybind.m_default_val);
									auto& actual = GET_VAR(int, keybind.m_holder_id);
									actual = def;
								}
							}
							else {
								if (std::holds_alternative<float>(keybind.m_default_val)) {
									float def = std::get<float>(keybind.m_default_val);
									auto& actual = GET_VAR(float, keybind.m_holder_id);
									actual = def;
								}
							}
							keybind.m_keybind_active = false;
						}
					}
				}
			}
		}
	}
	
	was_alive = is_player_alive;
	was_in_game = is_in_game;

	for ( auto& [widget_id, binds] : GET_VAR( kb_map_t, CONFIG_PATH( m_widget_keybinds ) ) ) {
		for ( auto& keybind : binds ) {
			if ( keybind.m_key != curr_key )
				continue;
			
			if (!is_in_game || !is_player_alive)
				continue;

			using DefaultVal = decltype( keybind.m_default_val );

			auto apply_keybind_value = [ & ]( auto& val, bool active ) {
				using T = std::decay_t<decltype( val )>;
				auto& actual = GET_VAR( T, keybind.m_holder_id );

				if constexpr ( std::is_same_v<T, bool> ) {
					if ( keybind.m_is_on_mode ) {
						if ( active ) {
							actual = true;
							keybind.m_keybind_active = true;
							keybind.m_on_mode_activated = true;
						} else {
							actual = false;
							keybind.m_keybind_active = false;
							keybind.m_on_mode_activated = false;
						}
					} else {
						if ( active ) {
							actual = false;
							keybind.m_keybind_active = true;
						} else {
							actual = true;
							keybind.m_keybind_active = false;
						}
					}
				} else if constexpr ( std::is_same_v<T, int> || std::is_same_v<T, float> ) {
					if ( std::holds_alternative<T>( keybind.m_default_val ) &&
						std::holds_alternative<T>( keybind.m_override_val ) ) {
						T def = std::get<T>( keybind.m_default_val );
						T over = std::get<T>( keybind.m_override_val );
						actual = active ? over : def;
						keybind.m_keybind_active = active;
					}
				}
				};

			auto toggle_keybind_value = [ & ]( auto& val ) {
				using T = std::decay_t<decltype( val )>;
				auto& actual = GET_VAR( T, keybind.m_holder_id );

				if constexpr ( std::is_same_v<T, bool> ) {
					actual = !actual;
					if ( keybind.m_is_on_mode ) {
						keybind.m_on_mode_activated = actual;
						keybind.m_keybind_active = actual;
					} else {
						keybind.m_keybind_active = !actual;
					}
				} else if constexpr ( std::is_same_v<T, int> || std::is_same_v<T, float> ) {
					if ( std::holds_alternative<T>( keybind.m_default_val ) &&
						std::holds_alternative<T>( keybind.m_override_val ) ) {
						T def = std::get<T>( keybind.m_default_val );
						T over = std::get<T>( keybind.m_override_val );
      						actual = ( actual == def ) ? over : def;
						keybind.m_keybind_active = ( actual != def );
					}
				}
				};

			switch ( keybind.m_mode ) {
			case e_key_mode::key_mode_toggle:
				if ( state == e_key_state::down ) {
					std::visit( toggle_keybind_value, keybind.m_default_val );
				}
				break;

			case e_key_mode::key_mode_hold:
				if ( state == e_key_state::down ) {
					std::visit( [ & ]( auto& val ) { apply_keybind_value( val, true ); }, keybind.m_default_val );
				} else if ( state == e_key_state::up ) {
					if ( keybind.m_keybind_type == e_keybind_type::keybind_type_checkbox ) {
						using T = bool;
						auto& actual = GET_VAR( T, keybind.m_holder_id );
						if ( keybind.m_is_on_mode ) {
							actual = false;
							keybind.m_on_mode_activated = false;
						} else {
							actual = true;
						}
						keybind.m_keybind_active = false;
					} else {
						std::visit( [ & ]( auto& val ) { apply_keybind_value( val, false ); }, keybind.m_default_val );
					}
				}
				break;
			}
		}
	}
}
