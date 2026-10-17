#pragma once

#include <includes.h>
#include <utils/tight_array.h>
#include "config/vars.h"

class c_input {
private:
	std::array<e_key_state, 256 + 5> m_key_states = {};
public:
	void init( void );

	void wnd_proc( UINT msg, WPARAM wparam, LPARAM lparam );

	bool key_down( int btn ) {
		return m_key_states[ btn ] == e_key_state::down;
	}

	bool key_released( int btn ) {
		if ( m_key_states[ btn ] == e_key_state::released ) {
			m_key_states[ btn ] = e_key_state::up;
			return true;
		}

		return false;
	}
};
inline auto g_input = std::make_unique<c_input>( );