#include "network_game_client_services.h"

#include <utils/memory.h>

float c_network_channel::get_net_latency( )
{
	return g_memory->call_virtual<float>( this, 12 );
}

float c_network_channel::get_engine_latency( )
{
	return g_memory->call_virtual<float>( this, 13 );
}

c_network_game_client* c_network_game_client_services::get_network_game_client( )
{
    return g_memory->call_virtual<c_network_game_client*>( this, 23 );
}

void c_network_game_client::run_prediction( int pred_reason )
{
	using fn_client_side_prediction = void( __fastcall* )(c_network_game_client*, unsigned int);
	static auto fn = g_modules->m_engine2.find( xx( "40 55 41 56 48 83 EC ? 80 B9" ) ).as<fn_client_side_prediction>( );
	fn( this, pred_reason );
}