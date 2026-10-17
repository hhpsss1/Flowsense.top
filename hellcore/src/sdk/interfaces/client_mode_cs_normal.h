#pragma once

#include <context.h>

class c_view_setup {
public:
 char pad0[0x498];
 float m_world_fov;
 float m_viewmodel_fov;
 vec3_t m_origin;
 char pad1[0xC];
 qangle_t m_view_angle;
 char pad2[0x10];
 float m_aspect_ratio;
 char pad3[0x79];
 unsigned char m_some_flags;
};

struct c_view_render {
	PAD( 8 );
	class c_view_setup m_view_setup;
	PAD( 8 );
};
static_assert(offsetof(c_view_setup,m_origin)==0x4A0);
static_assert(offsetof(c_view_setup,m_aspect_ratio)==0x4D4);
