#pragma once

#include <includes.h>


class c_network_channel
{
public:
	float get_net_latency( );

	float get_engine_latency( );
};

class c_server_local_frame_data
{
public:
	vec3_t m_view_angles;
	vec3_t m_shoot_position;
	int32_t m_target_index;
	vec3_t m_targer_head_position;
	vec3_t m_target_abs_origin;
	vec3_t m_target_angles;
};

class c_server_frame_data
{
public:
	int32_t m_process_tick;
	int32_t m_render_tick;
	float m_render_tick_fraction;
	int32_t m_player_tick;
	float m_player_tick_Fraction;
	float m_simulation_time;
	c_server_local_frame_data* m_local_data;
	char pad_0020[32];
};

class c_network_game_client
{
public:
	PAD(0x90);
	float m_realtime;
	int m_frame_count;
	float m_frame_time;
	float m_frame_time2;
	int m_max_clients;
	PAD(0x8);
	float m_player_interp;
	double m_some_timer_db;
	PAD(0x8);
	float m_interval_per_subtick;
	float m_current_time;
	float m_current_time2;
	PAD(0x8);
	bool m_in_prediction;
	PAD(0x3);
	int m_tick_count;
	int m_tick_count2;
	float m_some_timer;
	PAD(0x4);
	void* m_net_channel_info; //todo: make class of net channel
	PAD(0x8);
	bool m_should_predict;
	PAD(0x14B);
	int m_delta_tick;
	PAD(0x124);
	int m_server_tick;
	PAD(2642556);
	int m_client_tick;

	void run_prediction(int pred_reason);

	c_network_channel* get_net_channel()
	{
		return *(c_network_channel**)(reinterpret_cast<uintptr_t>(this) + 0xE0);
	}

};

class c_network_game_client_services
{
public:
	c_network_game_client* get_network_game_client( );
};