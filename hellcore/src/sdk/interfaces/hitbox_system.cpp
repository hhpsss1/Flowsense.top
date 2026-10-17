#include "hitbox_system.h"

bool c_hitbox_system::is_hitbox_near_to_trace( c_game_trace* game_trace, int some_count, c_skeleton_instance_body* skeleton_instance_body, c_ray* ray, vec3_t start, vec3_t end )
{
	using fn_is_hitbox_near_to_trace = bool( __fastcall* )( void*, c_game_trace*, int, c_skeleton_instance_body*, c_ray*, vec3_t*, vec3_t*, __int64, __int64, __int64, __int64 );
	static auto fn = g_modules->m_client.find( xx( "48 8B C4 48 89 58 ? 48 89 70 ? 55 57 41 56 48 8D 6C 24" ) ).as<fn_is_hitbox_near_to_trace>( );
	return fn( this, game_trace, some_count, skeleton_instance_body, ray, &start, &end, 0, 0, 0, 0 );
}