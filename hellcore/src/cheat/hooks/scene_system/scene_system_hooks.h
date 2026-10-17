#pragma once

#include <includes.h>
#include <sdk/datatypes/scene_object.h>
#include <sdk/interfaces/scene_system.h>

class c_scene_system_hooks {
public:
	static void* generate_primitives( c_animatable_scene_object_desc* desc, c_scene_animatable_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf );
	//static int64_t update_scene_object(void* a1, void* a2, c_aggregate_object_arr* data);
	static void draw_aggregate_sceneobject_array(void* a1, void* a2, c_aggregate_object_arr* data);
	static void* light_scene_object( void* ptr, c_light_scene* object, void* unk );
	static void __fastcall base_draw_array(scene_object_desc* __this, render_context* pCtx, c_mesh_primitive* pRenderList, int nNumRenderablesToDraw, c_scene_view const* pView, c_scene_layer* pLayer, void* a6);
	static void draw_skybox_array( void* a1, void* a2, c_draw_primitive_sky* draw_primitive, int count, void* a5, void* a6, void* a7 );
	static void tonemap_render_debug( void* a1, void* a2, void* a3 );
};