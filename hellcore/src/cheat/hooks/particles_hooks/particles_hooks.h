#pragma once

#include <includes.h>

class c_particles_hooks {
public:
	static void* __fastcall particles_draw_array(void* renderer, void* particle_data, void* draw_data, void* render_state, void* context);
};