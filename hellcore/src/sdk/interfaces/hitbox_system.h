#pragma once

#include <includes.h>

#include <sdk/interfaces/trace.h>

class c_skeleton_instance_body;

class c_hitbox_system
{
public:
	bool is_hitbox_near_to_trace( c_game_trace* game_trace, int some_count, c_skeleton_instance_body* skeleton_instance_body, c_ray* ray, vec3_t start, vec3_t end );
};