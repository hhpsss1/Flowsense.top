#pragma once

#include <memory.h>

class c_network_channel;

struct c_local_data2
{
	vec3_t m_viewangles;
	vec3_t m_eye_pos;
};

class c_networked_client_info
{
public:
	int32_t unk; //0x0000
	int32_t m_render_tick; //0x0004
	float m_render_tick_fraction; //0x0008
	int32_t m_player_tick_count; //0x000C
	float m_player_tick_fraction; //0x0010
	char pad_0014[4]; //0x0014
	c_local_data2* m_local_data; //0x0018
	char pad_0020[32]; //0x0020
}; //Size: 0x0040

class c_engine_client
{
public:
	bool in_game( ) {
		return g_memory->call_virtual<bool>( this, 38U);
	}

	bool is_connected( ) {
		return g_memory->call_virtual<bool>( this, 40U);
	}

	void exec_client_cmd_unrestricted( const char* cmd ) {
		g_memory->call_virtual<void>( this, 49U, 0, cmd, 0x7FFEF001 );
	}

	c_server_frame_data* get_server_frame_data()
	{
		static thread_local c_server_frame_data server_frame_data{};
		g_memory->call_virtual<void>(this, 179, &server_frame_data);
		return &server_frame_data;
	}

	c_network_channel* get_net_channel_info(int split_slot=0) {
 auto* service=GetModuleHandleA("engine2.dll");
 using factory_t=void*(*)(const char*,int*);
 static auto factory=reinterpret_cast<factory_t>(GetProcAddress(service,"CreateInterface"));
 static auto* client=factory ? factory("NetworkClientService_001",nullptr) : nullptr;
 return client ? g_memory->call_virtual<c_network_channel*>(client,42,split_slot) : nullptr;
}
};