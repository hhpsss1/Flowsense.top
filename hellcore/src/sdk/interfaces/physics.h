#pragma once

#include <includes.h>

#include <math/math.h>
#include <sdk/entity/pawn.h>

enum ray_type_t : uint8_t
{
	RAY_TYPE_LINE = 0,
	RAY_TYPE_SPHERE,
	RAY_TYPE_HULL,
	RAY_TYPE_CAPSULE,
	RAY_TYPE_MESH,
};

struct rn_collision_attr
{
    uint64_t m_interacts_as;
    uint64_t m_interacts_with;
    uint64_t m_interacts_exclude;
    uint32_t m_entity_id;
    uint32_t m_owner_id;
    uint16_t m_hierarchy_id;
    uint8_t m_collision_group;
    uint8_t m_collision_function_mask;

    OFFSET(int, m_iWorldCollision, 0x5A);
};

class c_surface_property_manager
{
public:
    void* get_surface( int surface_count );
};

class c_vhpysics
{
public:
    c_surface_property_manager* get_surface_property_manager( );
};
class c_shape
{
public:
    int get_shape_type( );
    rn_collision_attr* get_collision_attribute( );
};

class c_body
{
public:
    int get_entity_id( );
    c_transform* get_body_transform( bool* a1 );
};

class c_ray
{
public:
	vec3_t m_start;
	vec3_t m_end;
	vec3_t m_mins;
	vec3_t m_maxs;
	char pad_001[ 0x4 ];
	ray_type_t m_type;
};

class c_trace_hitbox_data
{
public:
	char pad_001[ 0x38 ];
	int m_hit_group;
	char pad_002[ 0x4 ];
	int m_hitbox_id;
};

class c_phys_surface_properties_physics
{
public:
    float m_fraction;
    float m_elasticity;
    float m_density;
    float m_thickness;
    float m_soft_contact_frequency;
    float m_soft_contact_damping_ratio;
};

class c_phys_surface_properties_vehicle
{
public:
    float m_wheel_drag;
    float m_wheel_friction_scale;
};

class c_phys_surface_properties_sound_names
{
public:
    const char* m_impact_soft;
    const char* m_impact_hard;
    const char* m_scrape_smooth;
    const char* m_scrape_rough;
    const char* m_bullet_impact;
    const char* m_rolling;
    const char* m_break_sound;
    const char* m_strain;
    const char* m_melee_impact;
    const char* m_push_off;
    const char* m_skid_stop;
    const char* m_resonant;
};

class c_phys_surface_properties_audio
{
public:
    float m_reflectivity;
    float m_hardness_factor;
    float m_roughness_factor;
    float m_rough_threshold;
    float m_hard_threshold;
    float m_hard_velocity_threshold;
    float m_static_impact_volume;
    float m_occlusion_factor;
};

class c_phys_surface_properties
{
public:
    const char* m_name;
    uint32_t m_name_hash;
    uint32_t m_base_name_hash;
    int32_t m_list_index;
    int32_t m_base_list_index;
    bool m_hidden;
    const char* m_description;

    c_phys_surface_properties_physics m_physics;
    c_phys_surface_properties_vehicle m_vehicle_params;
    c_phys_surface_properties_sound_names m_audio_sounds;
    c_phys_surface_properties_audio m_audio_params;
};

struct c_game_trace
{
public:
    c_phys_surface_properties* m_surface_prop;
	c_cs_player_pawn* m_ent;
	c_trace_hitbox_data* m_hitbox;
	c_body* m_body;
	c_shape* m_shape;
	uint64_t m_contents;
	c_transform m_body_transform;
	rn_collision_attr m_shape_attributes;
	vec3_t m_start_pos;
	vec3_t m_end_pos;
	vec3_t m_hit_normal;
	vec3_t m_hit_point;
	float m_hit_offset;
	float m_fraction;
	int m_triangle;
	int16_t m_hitbox_bone_index;
	ray_type_t m_ray_type;
	bool m_start_in_solid;
	bool m_exact_hit_point;

	c_game_trace( ) = default;
	int get_hitbox_id( ) const;
	int get_hit_group( ) const;
	bool hit_world( ) const;
	bool is_visible( ) const;
};

class c_physics_aggregate_instance
{
public:
	void* setup_trace_game_physics_on_ray( c_game_trace* game_trace, c_ray* ray, vec3_t* start, vec3_t* end, void* trace_data );
};
