#pragma once

#include <includes.h>

struct hellcore_hook_t {
	void* m_detour;
	void* m_base;
	void* m_original;
	const char* m_hook_name;

	hellcore_hook_t( const char* name ) : m_hook_name( name ) { }

	template <typename t>
	t get( ) {
		return (t)( m_original );
	}

    void hook( void* our_func, void* func ) {
        if ( !func ) {
            LOG_ERROR( xx( std::string( "null function address for " + std::string( m_hook_name ) ).c_str( ) ) );
            return;
        }

        m_base = func;

        if ( MH_STATUS status = MH_CreateHook( m_base, our_func, &m_original ); status != MH_OK ) {
            LOG_ERROR( xx( std::string( "MH_CreateHook failed for " + std::string( m_hook_name ) + " with status: " + std::to_string( status ) ).c_str( ) ) );
            return;
        }

        if ( MH_STATUS status = MH_EnableHook( m_base ); status != MH_OK ) {
            LOG_ERROR( xx( std::string( "MH_EnableHook failed for " + std::string( m_hook_name ) + " with status: " + std::to_string( status ) ).c_str( ) ) );
            return;
        }

        LOG( xx( std::string( "Successfully hooked " + std::string( m_hook_name ) + " at 0x" + std::to_string( (uintptr_t)m_base ) ).c_str( ) ) );
    }

	void hook( module_t* module, void* vtbl, int32_t index ) {
		m_original = module->find( vtbl, index );
		
	}
};