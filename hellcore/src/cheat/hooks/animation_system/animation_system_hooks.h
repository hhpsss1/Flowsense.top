#pragma once

#include <includes.h>
#include <core/hooks/hooks.h>
#include <core/hooks/hellcore_hook.h>
#include <core/interfaces/interfaces.h>

class c_animation_system_hooks
{
public:
	static void* should_update_sequences( void* a1, void* a2, void* a3 );
};