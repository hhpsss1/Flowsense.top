#pragma once

#include <includes.h>
#include <sdk/entity/base_handle.h>

#include "physics.h"

enum rn_query_object_set_t
{
	RNQUERY_OBJECTS_STATIC = (1 << 0),
	RNQUERY_OBJECTS_DYNAMIC = (1 << 1),
	RNQUERY_OBJECTS_NON_COLLIDEABLE = (1 << 2),
	RNQUERY_OBJECTS_KEYFRAMED_ONLY = (1 << 3) | (1 << 8),
	RNQUERY_OBJECTS_DYNAMIC_ONLY = (1 << 4) | (1 << 8),

	RNQUERY_OBJECTS_ALL_GAME_ENTITIES = RNQUERY_OBJECTS_DYNAMIC | RNQUERY_OBJECTS_NON_COLLIDEABLE,
	RNQUERY_OBJECTS_ALL = RNQUERY_OBJECTS_STATIC | RNQUERY_OBJECTS_ALL_GAME_ENTITIES,
};

enum built_in_collision_group_t
{
	COLLISION_GROUP_ALWAYS = 0,
	COLLISION_GROUP_NONPHYSICAL = 1,
	COLLISION_GROUP_TRIGGER = 2,
	COLLISION_GROUP_CONDITIONALLY_SOLID = 3,
	COLLISION_GROUP_FIRST_USER = 4,
	COLLISION_GROUPS_MAX_ALLOWED = 64,
};

enum standard_collision_groups_t
{
	COLLISION_GROUP_DEFAULT = COLLISION_GROUP_FIRST_USER,
};

class c_surface_data
{
public:
	char pad_001[0x8];
	float m_penetration_data_modifier;
	float m_damage_modifier;
	char pad_002[0x4];
	int m_material;
};

class c_trace_info {
public:
	float m_unk;
	float m_distance;
	float m_damage;
	std::uint32_t m_pen_count;
	c_base_handle m_handle;
	std::uint32_t m_penetration_flags;
};

class c_array_element
{
public:
	/*char PAD[ 0x0C ];
	c_base_handle m_handle;
	char pad2[ 0x08 ];
	uint16_t m_surface_id;
	uint8_t m_flags;
	char pad3[ 0x01 ];
	int32_t m_unk;
	char pad4[ 0x13 ];
	bool m_hit_type;
	char pad5[ 0x01 ];
	int32_t m_entity_index;*/
	char pad0[48];
};
class trace_segment_t
{
public:
	vec3_t m_normals;            // 0x0000
	float m_fraction;            // 0x000C
	void* m_physics_body;        // 0x0010
	void* m_physics_shape;       // 0x0018
	int32_t m_hit_entity_index;  // 0x0020
	int32_t m_hit_entity_handle; // 0x0024    
	int64_t m_surface;           // 0x0028
}; // Size: 0x0030

struct trace_arr_element_t
{
	char pad_0000[0x38];
};

struct c_segment_holder {
	vec3_t m_hit_normal;
	float m_fraction;
	void* m_body;
	void* m_shape;
    char m_segment_pad[8];
	uint16_t m_bone_index;
	c_base_handle m_entity_handle;
	__int16 m_surface_count;
	bool m_start_in_solid;
	bool m_hit_point;
    char m_segment_tail[4];
};

struct c_trace_data {
 std::int32_t m_filled_segment_count{};
 float m_unk2{52.f};
 c_segment_holder* m_trace_segments_ptr{};
 std::int32_t m_trace_segments_count{128};
 std::int32_t m_trace_segments_count2{static_cast<std::int32_t>(0x80000000)};
 trace_arr_element_t m_trace_segments[128]{};
 void* m_unk_zeroed{};
 int m_surfaces_count{}, m_surface_reserved{};
 c_trace_info* m_trace_info{};
 int m_unk_i{8};
 unsigned m_hit_flags{0x80000000};
 char m_hit_elements[0xC0]{};
 vec3_t m_start{}, m_end{};
 float m_fraction{1.f};
 bool m_finished{};
 char m_pad[0x4B]{};
};

class c_update_value
{
public:
	float m_previous_length_modifier;
	float m_current_length_modifier;
	char pad_001[0x8];
	short m_handle_index;
	char pad_002[0x6];
};

struct c_pmplane
{
	vec3_t m_start_pos;
	vec3_t m_end_pos;
	vec3_t m_hit_normal;
	vec3_t m_hit_point;
};

class c_game_trace_data
{
public:
	c_body* m_body;
	c_shape* m_shape;
	vec3_t m_hit_point;
	vec3_t m_hit_normal;
	c_surface_property_manager* m_surface_property;
	float m_hit_offset;
	float m_fraction;
	int m_triangle;
	bool m_start_in_solid;
};

class c_trace_holder
{
public:
	vec3_t m_hit_normal;
	float m_fraction;
	c_trace_hitbox_data* m_hitbox_data;
	c_shape* m_shape;
	uint16_t m_bone_index;
	c_base_handle m_entity_handle;
	__int16 m_surfaces_count;
	bool m_start_in_solid;
	bool m_hit;
};

class c_contents
{
	char gap0[24];
public:
	rn_collision_attr m_collision;
	__unaligned __declspec(align(1)) void* oword28;
	void* qword38;
};
struct rn_query_shape_attr_t
{
public:
	rn_query_shape_attr_t()
	{
		m_interacts_with = 0;
		m_interacts_exclude = 0;
		m_interacts_as = 0;

		m_entity_ids_to_ignore[0] = -1;
		m_entity_ids_to_ignore[1] = -1;

		m_owner_ids_to_ignore[0] = -1;
		m_owner_ids_to_ignore[1] = -1;

		m_hierarchy_ids[0] = 0;
		m_hierarchy_ids[1] = 0;

		m_object_set_mask = RNQUERY_OBJECTS_ALL;
		m_collision_group = COLLISION_GROUP_ALWAYS;

		m_hit_solid = true;
		m_hit_solid_requires_generate_contacts = false;
		m_hit_trigger = false;
		m_should_ignore_disabled_pairs = true;
		m_ignore_if_both_interact_with_hitboxes = false;
		m_force_hit_everything = false;
		m_unknown = true;
	}

	bool has_interacts_as_layer(int layer_index) const { return (m_interacts_as & (1ull << layer_index)) != 0; }
	bool has_interacts_with_layer(int layer_index) const { return (m_interacts_with & (1ull << layer_index)) != 0; }
	bool has_interacts_exclude_layer(int layer_index) const { return (m_interacts_exclude & (1ull << layer_index)) != 0; }
	void enable_interacts_as_layer(int layer) { m_interacts_as |= (1ull << layer); }
	void enable_interacts_with_layer(int layer) { m_interacts_with |= (1ull << layer); }
	void enable_interacts_exclude_layer(int layer) { m_interacts_exclude |= (1ull << layer); }
	void disable_interacts_as_layer(int layer) { m_interacts_as &= ~(1ull << layer); }
	void disable_interacts_with_layer(int layer) { m_interacts_with &= ~(1ull << layer); }
	void disable_interacts_exclude_layer(int layer) { m_interacts_exclude &= ~(1ull << layer); }

public:
	uint64_t m_interacts_with;
	uint64_t m_interacts_exclude;
	uint64_t m_interacts_as;

	uint32_t m_entity_ids_to_ignore[2];
	uint32_t m_owner_ids_to_ignore[2];
	uint16_t m_hierarchy_ids[2];
	uint16_t m_object_set_mask;
	uint8_t m_collision_group;

	bool m_hit_solid : 1;
	bool m_hit_solid_requires_generate_contacts : 1;
	bool m_hit_trigger : 1;
	bool m_should_ignore_disabled_pairs : 1;
	bool m_ignore_if_both_interact_with_hitboxes : 1;
	bool m_force_hit_everything : 1;
	bool m_unknown : 1;
};

struct rn_query_attr_t : public rn_query_shape_attr_t
{
};

struct c_trace_filter {
public:
	char pad_0000[8];
	int64_t m_mask;
	std::array<int64_t, 2> m_ptr{};
	std::array<int32_t, 4> m_skip_handles{};
	std::array<int16_t, 2> m_arr_collisions{};
	char m_pad34[3]{};
	uint8_t m_ptr2{};
	uint8_t m_ptr3{};
	uint8_t m_ptr4{};
	char m_pad3A[6]{};
	uint8_t m_collision{};

	c_trace_filter() = default;

	c_trace_filter(uint64_t mask, c_cs_player_pawn* entity, c_cs_player_pawn* player, int layer) {
		using fn_init_trace_t = void (__fastcall*)(c_trace_filter*, void*, uint64_t, int);
		static fn_init_trace_t fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF 24")).as<fn_init_trace_t>();

		if (fn) fn(this, entity ? entity : player, mask, layer);
        m_ptr2 = 7;
	}
};


class c_vphys2world
{
public:
	void init_game_trace(c_game_trace* game_trace);
	void init_player_movement_trace_filter(c_trace_filter* trace_filter, void* entity_instance, uint64_t interact_with, int collision_group);
	void setup_trace_data(c_game_trace* game_trace, c_ray* ray, vec3_t start, vec3_t end, c_game_trace_data* trace_data);
	bool clip_ray_entity(c_ray* ray, vec3_t start, vec3_t end, c_cs_player_pawn* pawn, c_trace_filter* filter, c_game_trace* game_trace);
	void setup_game_trace_info(c_trace_data* trace_data, c_game_trace* game_trace, float unk, c_segment_holder* trace_holder);
	void trace_to_exit(c_trace_data* trace, vec3_t start, vec3_t end, c_trace_filter& filter, int penetration_count);
	void get_trace_info(c_trace_data* pTraceData, c_game_trace* pTrace, float fl, PVOID unk);
	void get_trace_info(c_trace_data* trace, c_game_trace* hit, c_trace_info* trace_info);
	void init_trace(c_trace_filter& filter, c_cs_player_pawn* pawn, uint64_t mask, uint8_t layer, uint16_t unknown);
	bool trace_shape(c_ray* ray, vec3_t* start, vec3_t* end, c_trace_filter* filter, c_game_trace* game_trace);
	c_trace_filter* initialize_trace_filter(c_trace_filter* filter, c_cs_player_pawn* pawn, int64_t mask, uint8_t layer, uint8_t layer2);
	c_trace_data* initialize_trace_data(c_trace_data*);
	void update_world_collisions();
};
static_assert(sizeof(c_trace_filter)==0x48);
static_assert(sizeof(c_trace_data)==0x1D60);
static_assert(offsetof(c_trace_data,m_surfaces_count)==0x1C20);

static_assert(sizeof(c_segment_holder)==0x38);
