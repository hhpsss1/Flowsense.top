#pragma once

#include <context.h>

class c_material_color {
public:
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	c_material_color( hellcolor col ) {
		r = (int)(col.Value.x * 255.f);
		g = (int)(col.Value.y * 255.f);
		b = (int)(col.Value.z * 255.f);
		a = (int)(col.Value.w * 255.f);
	}

	c_material_color( ) = default;
};

class c_material_color_no_alpha {
public:
	uint8_t r;
	uint8_t g;
	uint8_t b;

	c_material_color_no_alpha( hellcolor col ) {
		r = ( int )( col.Value.x * 255.f );
		g = ( int )( col.Value.y * 255.f );
		b = ( int )( col.Value.z * 255.f );
	}

	c_material_color_no_alpha( ) = default;
};

struct material_parameter_t {
	vec4_t m_vec;
	void* m_texture_val;
	PAD( 0x10 );
	const char* m_param_name;
	const char* m_value_text;
	std::int64_t m_value;
};

class c_material_2 {
public:
	virtual const char* get_name( ) = 0;
	virtual const char* get_name_with_mod( ) = 0;
	virtual void func2( ) = 0;
	virtual void func3( ) = 0;
	virtual bool is_loaded( ) = 0;

	

	void update_parameter() {
		using fn = void* (__fastcall*)(c_material_2*);
		static auto update_param_fn = g_modules->m_materialsystem2.find(xx("48 89 7C 24 ? 41 56 48 83 EC ? 8B 81")).as<fn>();
		update_param_fn(this);
	}
};

class c_material_system_2 {
public:
	c_material_2*** find_or_create_from_resource( c_material_2*** out_mat, const char* material_name ) {
		return g_memory->call_virtual<c_material_2***>( this, 14, out_mat, material_name );
	}

	c_material_2** create_material( c_material_2*** out_mat, const char* material_name, void* data ) {
		return g_memory->call_virtual<c_material_2**>( this, 29, out_mat, material_name, data, 0, 0, 0, 0, 0, 1 );
	}

	void set_create_data_by_material( const void* data, c_material_2*** const in_mat ) {
		return g_memory->call_virtual<void>( this, 37, in_mat, data );
	}
};