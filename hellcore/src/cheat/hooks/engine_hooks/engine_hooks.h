#pragma once

#include <includes.h>

class c_engine_hooks {
public:
	static unsigned int __fastcall setup_blur(void* a1, unsigned int dof, vec4_t* range);
};