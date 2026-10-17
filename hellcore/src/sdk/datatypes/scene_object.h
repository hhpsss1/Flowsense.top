#pragma once

#include <context.h>
#include <sdk/entity/base_handle.h>
#include <sdk/interfaces/material_system.h>

class c_mesh_instance {
public:
	PAD( 96 );
};

class c_scene_animatable_object;
class c_animatable_scene_object_desc {
public:
	c_scene_animatable_object* create( ) {
		return g_memory->call_virtual<c_scene_animatable_object*>( this, 21 );
	}
};

class c_scene_animatable_object {
public:
	OFFSET(int, get_bone_count, 0xD0);
	OFFSET(matrix3x4_t*, get_render_bones, 0xD8);
	OFFSET(uint32_t&, flags, 0x78);
	OFFSET(char&, m_lod_flag, 0x8E);

	PAD(0x78);
	std::uint32_t m_flags;
	PAD(0x44);
	c_base_handle m_owner{ };
	PAD(76);
	c_skeleton_instance* m_skeleton_instance{ };
	PAD(24);
};

class c_mesh_primitive {
public:
	PAD( 24 );
	c_scene_animatable_object* m_scene_animatable_object;
	c_material_2* m_material;
	c_material_2* m_material2;
	PAD( 32 );
	c_material_color m_color;
    PAD(4);
    void* m_object_info;
    PAD(0x10);
};


class c_mesh_primitive_output_buffer {
public:
 c_mesh_primitive* m_mesh_primitive_array;
 int m_fixed_capacity;
 int m_arr_size;
 int m_overflow_count;
 int m_reserved;
 c_mesh_primitive* m_overflow;
 int m_overflow_capacity;
 int m_flags;
 int get_count() const {return m_arr_size+m_overflow_count;}
 c_mesh_primitive* get_primitive(int index) {
  if(index<0 || index>=get_count()) return nullptr;
  return index<m_arr_size ? (m_mesh_primitive_array ? &m_mesh_primitive_array[index] : nullptr) : (m_overflow ? &m_overflow[index-m_arr_size] : nullptr);
 }
};

class c_light_scene
{
public:
	PAD( 0xE4 ); // 0x0
	hellcolor m_color; // 0xE4
};

class c_scene_object
{
public:
	PAD( 0xD8 ); //0x0000
	c_material_color m_color; //0x00B8
	PAD( 196 ); //0x00BC
}; //Size: 0x0180

class c_base_scene_object
{
public:
	PAD( 24 ); //0x0000
	c_scene_object* m_scene_object; //0x0018
	c_material_2* m_material; //0x0020 
	c_material_2* m_material2; //0x0028
	PAD( 0x20 ); //0x0030
	c_material_color m_color; //0x0040
	PAD( 0x12 ); //0x0044
};

class c_aggregate_scene_object_data
{
public:
	PAD( 0x38 ); // 0x0
	c_material_color_no_alpha m_color; // 0x38
	PAD( 0x9 );
};


class c_aggregate_scene_object_govno {
public:
	PAD(0x4);
	int nCount;
	PAD(0x30);
	int nIndex;
};

class c_aggregate_scene_object
{
public:
	PAD( 0x120 );
	int m_count; // 0x120
	PAD( 0x4 );
	c_aggregate_scene_object_data* m_data; // 0x128
};

class c_draw_primitive_sky
{
public:
	PAD(0xD0);
    void** m_material;
    PAD(0xE8 - 0xD0 - sizeof(void*));
    vec3_t m_sky_color;
    PAD(0x118 - 0xE8 - sizeof(vec3_t));
	PAD( 0x10 ); //0x118
	hellcolor m_color; //0x0128
};
static_assert(offsetof(c_draw_primitive_sky, m_material) == 0xD0);
static_assert(offsetof(c_draw_primitive_sky, m_sky_color) == 0xE8);

class c_aggregate_object_arr
{
public:
	c_aggregate_scene_object* object;
	c_aggregate_scene_object_govno* data;
};

class c_mesh_system_impl : public c_mesh_system
{
public:
};

class scene_object_desc
{
public:
};

class render_context
{
	void* m_vtable;
public:
};

class scene_view
{
	void* m_vtable;
public:
};

class c_scene_view : public scene_view
{
public:
};

static_assert(sizeof(c_mesh_primitive)==0x70);
static_assert(sizeof(c_mesh_primitive_output_buffer)==0x28);
