#include "scene_system.h"

#include <context.h>

void* c_scene_system::create_scene_object( void* desc, __int64 zero_or_eight, __int64 mesh_flag, void* scene_world ) {
	return g_memory->call_virtual<void*>( this, 11, desc, zero_or_eight, mesh_flag, scene_world );
}

void* c_scene_system::get_scene_object_desc( const char* desc_name ) {
	return g_memory->call_virtual<void*>( this, 17, desc_name );
}

void c_scene_system::delete_scene_object( void* object ) {
	if ( !object )
		return;

	const auto flags = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const std::byte*>(object) + 0x80);
    if (!(flags & 0x4000000000000000ull))
        g_memory->call_virtual<void>(this, 16, object);
}
