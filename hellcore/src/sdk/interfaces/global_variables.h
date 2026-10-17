#pragma once

#include <includes.h>

class c_global_variables {
public:
	float m_real_time; //0x0000
	int32_t m_frame_count; //0x0004
	float m_frame_time; //0x0008
	float m_frame_advance; //0x000C
	int32_t m_max_clients; //0x0010
	bool m_supress_simulation; //0x0014
	char pad_0015[27]; //0x0015
	float m_curtime; //0x0030
	float m_render_time; // 0x0034
	float m_client_tick_fraction; // 0x0038
	float m_next_tick_fraction; // 0x003C
	float m_client_fraction; //0x0040
	int32_t m_client_tick; //0x0044
	int32_t m_tick_count; //0x0048
	float m_tick_fraction; //0x004C
	char pad_0050[8]; //0x0050
	c_network_channel* m_net_channel; //0x0058
	char pad_0060[280]; //0x0060
	const char* m_map_name; //0x0178
	const char* m_map_name_short; //0x0180
};
static_assert(offsetof(c_global_variables, m_curtime) == 0x30);
static_assert(offsetof(c_global_variables, m_tick_count) == 0x48);
static_assert(offsetof(c_global_variables, m_net_channel) == 0x58);
static_assert(offsetof(c_global_variables, m_map_name) == 0x178);
