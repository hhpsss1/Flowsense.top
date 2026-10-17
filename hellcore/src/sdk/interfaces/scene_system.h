#pragma once

#include <context.h>

#define RENDER_VIEWPORT_VERSION 1

struct render_viewport_t {
    int m_version;
    int m_top_left_x;
    int n_top_left_y;
    int m_width;
    int m_height;
    float m_min_z;
    float m_max_z;

    render_viewport_t( ) : m_version( RENDER_VIEWPORT_VERSION ) { }

    void Init( ) {
        memset( this, 0, sizeof( render_viewport_t ) );
        m_version = RENDER_VIEWPORT_VERSION;
    }

    void Init( int x, int y, int nWidth, int nHeight, float flMinZ = 0.0f, float flMaxZ = 1.0f ) {
        m_version = RENDER_VIEWPORT_VERSION;
        m_top_left_x = x; n_top_left_y = y; m_width = nWidth; m_height = nHeight;
        m_min_z = flMinZ;
        m_max_z = flMaxZ;
    }
};

class c_texture_dx11 {
public:
    char pad_0000[ 8 ]; //0x0000
    int32_t m_unk; //0x0008
    char pad_000C[ 4 ]; //0x000C
    ID3D11ShaderResourceView* m_texture_SRV0; //0x0010
    ID3D11ShaderResourceView* m_texture_SRV1; //0x0018
}; //Size: 0x0020

struct texture_dx11_handle_t {
    c_texture_dx11* m_texture; //0x0000
    void* m_scratch_renderer; //0x0008
    char pad_0010[ 24 ]; //0x0010
    void* m_material_list; //0x0028
}; //Size: 0x0030

class c_scene_layer {
public:
    void* vtable; //0x0000
    char pad_0008[ 8 ]; //0x0008
    render_viewport_t m_viewport;
    int32_t m_layer_type; //0x002C
    int32_t m_shader_mode; //0x0030
    int32_t m_shading_mode; //0x0034
    uint32_t m_object_flags_required_mask; //0x0038
    uint32_t m_object_flags_excluded_mask; //0x003C
    uint32_t m_layer_flags; //0x0040
    vec4_t m_clear_color; //0x0044
    int32_t m_clear_flags; //0x0054
    int32_t m_layer_index; //0x0058
    int32_t m_render_target_binding_handle; //0x005C
    char pad_0060[ 160 ]; //0x0060
    float m_width; //0x0100
    float m_height; //0x0104
    char pad_0108[ 760 ]; //0x0108
    void* m_vertex_buffer; //0x0400
    void* m_vertex_buffer2; //0x0408
    void* m_vertex_buffer3; //0x0410
    char pad_0418[ 136 ]; //0x0418
    char m_layer_name[ 64 ]; //0x04A0
    char pad_04E0[ 760 ]; //0x04E0
    texture_dx11_handle_t* m_texture_handle; //0x07D8
    char pad_07E0[ 56 ]; //0x07E0
    texture_dx11_handle_t* m_texture_handle2; //0x0818
    char pad_0820[ 5760 ]; //0x0820
}; //Size: 0x1EA0

class govno {
public:
    PAD(0x18);
    void* lightData;
};

class c_scene_system {
public:

	void* create_scene_object( void* desc, __int64 zero_or_eight, __int64 mesh_flag, void* scene_world );
	void* get_scene_object_desc( const char* desc_name );
	void delete_scene_object( void* object );
};