#pragma once
#include <utils/memory.h>

class c_engine_pvs_manager
{
public:
	void set( bool v )
	{
		g_memory->call_virtual<void*>( this, 6, v );
	}
};