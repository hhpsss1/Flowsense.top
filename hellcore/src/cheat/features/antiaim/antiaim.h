#pragma once

#include <context.h>

class c_antiaim {
public:
	float get_pitch( );
	float get_at_target_yaw( );
	float get_yaw( );
	void run( );
};
inline auto g_antiaim = std::make_unique<c_antiaim>( );