#include "animation_system_hooks.h"

void* c_animation_system_hooks::should_update_sequences( void* a1, void* a2, void* a3 )
{
	static const auto original = g_hooks->m_animation_system.m_should_update_sequences.get<decltype( &should_update_sequences )>( );

	if ( !a1 || !a2 || !a3 )
		return original( a1, a2, a3 );

	const char* model_name = *reinterpret_cast< const char** >( reinterpret_cast< std::uintptr_t >( a1 ) + 0x8 );
	if ( model_name == nullptr )
		return original( a1, a2, a3 );

	std::string str_model_name = std::string( model_name );
	if ( str_model_name.starts_with( "weapons/models/knife/" ) )
		*reinterpret_cast< __int64* >( reinterpret_cast< std::uintptr_t >( a3 ) + 0x30 ) = 0;

	return original( a1, a2, a3 );
}