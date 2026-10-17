#include "physics.h"

#include <utils/memory.h>

void* c_surface_property_manager::get_surface( int surface_count )
{
    return g_memory->call_virtual<void*>( this, 3, surface_count );
}

c_surface_property_manager* c_vhpysics::get_surface_property_manager( )
{
    return g_memory->call_virtual<c_surface_property_manager*>( this, 17 );
}

int c_shape::get_shape_type( )
{
    return g_memory->call_virtual<int>( this, 4 );
}

rn_collision_attr* c_shape::get_collision_attribute( )
{
    return g_memory->call_virtual<rn_collision_attr*>( this, 18 );
}

int c_body::get_entity_id( )
{
    return g_memory->call_virtual<int>( this, 149 );
}

void* c_physics_aggregate_instance::setup_trace_game_physics_on_ray( c_game_trace* game_trace, c_ray* ray, vec3_t* start, vec3_t* end, void* trace_data )
{
    return g_memory->call_virtual<void*>( this, 95, game_trace, ray, start, end, trace_data );
}

c_transform* c_body::get_body_transform( bool* a1 )
{
    return g_memory->call_virtual<c_transform*>( this, 61, a1 );
}