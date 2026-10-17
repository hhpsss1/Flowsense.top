#pragma once
#include <cstdint>
#include <utils/memory.h>
#include <sdk/datatypes/c_buffer_string.h>
#include "material_system.h"

struct material_array_t {
	uint64_t m_count;
	c_material_2*** m_material_resource;
	uint64_t pad_0010[3]; 
};

class c_resource_system {
public:
    resource_binding_t* precache_resource(const char* path, uint64_t type) {
        using preload_t = void(__fastcall*)(void*, c_buffer_string*, const char*);
        static auto preload = g_modules->m_resourcesystem.find("40 53 55 57 48 81 EC 80 00 00 00").as<preload_t>();
        if (!path || !*path || !preload) return nullptr;
        c_buffer_string buffer;
        buffer.load_path(path);
        buffer.set_resource_type(type);
        preload(this, &buffer, "");
        buffer.load_path(path);
        return g_memory->call_virtual<resource_binding_t*>(this, 79, &buffer, false);
    }
	void enumerate_materials( uint64_t type_hashed, material_array_t* result, uint8_t flag ) {
		g_memory->call_virtual<void>( this, 32, type_hashed, result, flag );
	}

	void* blocking_load_by_name( const char* path ) {
		c_buffer_string path_buffer = c_buffer_string( path );
		return g_memory->call_virtual<void*>( this, 40, &path_buffer, "" );
	}

	resource_binding_t* blocking_load_resource_by_name(c_buffer_string* path, const char* szName)
	{
		return g_memory->call_virtual<resource_binding_t*>(this, 40, path, szName);
	}

	void* load_resource( const char* path ) {
		c_buffer_string path_buffer = c_buffer_string( path );
		return g_memory->call_virtual<void*>( this, 41, &path_buffer, "" );
	}
};