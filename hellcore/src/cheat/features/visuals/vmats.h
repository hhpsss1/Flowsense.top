#pragma once
//     g_tTranslucency = resource:"materials/default/default_tga_27a82287.vtex"

static const char latex_vmat[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_character.vfx"
    F_DISABLE_Z_PREPASS = 0
    F_DISABLE_Z_WRITE = 0

	F_TRANSLUCENT = 1
	F_PAINT_VERTEX_COLORS = 1

    g_vColorTint = [1.0, 1.0, 1.0, 0.0]
    g_bFogEnabled = 0
    g_flMetalness = 0.000
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tAmbientOcclusion = resource:"materials/default/default_ao_tga_79a2e0d0.vtex"
    g_tNormal = resource:"materials/default/default_normal_tga_1b833b2a.vtex"
    g_tMetalness = resource:"materials/default/default_metal_tga_8fbc2820.vtex"
} )";

static const char latex_vmat_invis[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_character.vfx"
    F_DISABLE_Z_BUFFERING = 1
    F_DISABLE_Z_PREPASS = 1
    F_DISABLE_Z_WRITE = 1

	F_TRANSLUCENT = 1
	F_PAINT_VERTEX_COLORS = 1

    g_vColorTint = [1.0, 1.0, 1.0, 0.0]
    g_bFogEnabled = 0
    g_flMetalness = 0.000
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tAmbientOcclusion = resource:"materials/default/default_ao_tga_79a2e0d0.vtex"
    g_tNormal = resource:"materials/default/default_normal_tga_1b833b2a.vtex"
    g_tMetalness = resource:"materials/default/default_metal_tga_8fbc2820.vtex"
} )";

static const char flat_vmat[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_unlitgeneric.vfx"
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
    g_tRoughness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    g_tAmbientOcclusion = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    g_tMetalness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    F_RENDER_BACKFACES = 1
    F_DISABLE_Z_BUFFERING = 0
	F_PAINT_VERTEX_COLORS = 1
    F_TRANSLUCENT = 1
    F_BLEND_MODE = 1
    g_vColorTint = [1.0, 1.0, 1.0, 1.0]
} )";

static const char flat_vmat_invis[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_unlitgeneric.vfx"
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
    g_tRoughness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    g_tAmbientOcclusion = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    g_tMetalness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
    F_RENDER_BACKFACES = 1
    F_DISABLE_Z_BUFFERING = 1
	F_PAINT_VERTEX_COLORS = 1
    F_TRANSLUCENT = 1
    F_BLEND_MODE = 1
    g_vColorTint = [1.0, 1.0, 1.0, 1.0]
} )";

static const char glow_vmat[] = R"(
	<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
	{
		"shader" = "csgo_effects.vfx"
 
		 "g_tColor" = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
 
		 "g_tMask1" = resource:"materials/default/default_mask_tga_344101f8.vtex"
		 "g_tMask2" = resource:"materials/default/default_mask_tga_344101f8.vtex"
		 "g_tMask3" = resource:"materials/default/default_mask_tga_344101f8.vtex"
	 
		 "g_flColorBoost" = 20 
		 "g_flOpacityScale" = 0.6999999 
		 "g_flFresnelExponent" = 10 
		 "g_flFresnelFalloff" = 10 
		 "g_flFresnelMax" = 0
		 "g_flFresnelMin" = 1
	 
		 "F_ADDITIVE_BLEND" = 1
		 "F_BLEND_MODE" = 1
		 "F_TRANSLUCENT" = 1
		 "F_IGNOREZ" = 0
 
		 "F_DISABLE_Z_BUFFERING" = 0
	 
		 "F_RENDER_BACKFACES" = 0
	 
		 "g_vColorTint" = [1.00000, 1.00000, 1.00000]
	}
)";

static const char glow_vmat_invis[] = R"(
	<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
	{
		"shader" = "csgo_effects.vfx"
 
		 "g_tColor" = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
 
		 "g_tMask1" = resource:"materials/default/default_mask_tga_344101f8.vtex"
		 "g_tMask2" = resource:"materials/default/default_mask_tga_344101f8.vtex"
		 "g_tMask3" = resource:"materials/default/default_mask_tga_344101f8.vtex"
	 
		 "g_flColorBoost" = 20 
		 "g_flOpacityScale" = 0.6999999 
		 "g_flFresnelExponent" = 10 
		 "g_flFresnelFalloff" = 10 
		 "g_flFresnelMax" = 0
		 "g_flFresnelMin" = 1
	 
		 "F_ADDITIVE_BLEND" = 1
		 "F_BLEND_MODE" = 1
		 "F_TRANSLUCENT" = 1
		 "F_IGNOREZ" = 1
 
		 "F_DISABLE_Z_BUFFERING" = 1
	 
		 "F_RENDER_BACKFACES" = 0
	 
		 "g_vColorTint" = [1.00000, 1.00000, 1.00000]
	}
)";

static const char bloom_vmat[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "solidcolor.vfx"
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
	g_tRoughness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
	g_tMetalness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
	g_tAmbientOcclusion = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"

	F_IGNOREZ = 0
	F_DISABLE_Z_WRITE = 0
	F_RENDER_BACKFACES = 0
	g_vColorTint = [9.0, 9.0, 9.0, 9.0]
} )";

static const char bloom_vmat_invis[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "solidcolor.vfx"
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
	g_tRoughness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
	g_tMetalness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
	g_tAmbientOcclusion = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"

	F_IGNOREZ = 1
	F_DISABLE_Z_WRITE = 1
	F_DISABLE_Z_BUFFERING = 1
	F_RENDER_BACKFACES = 0
	g_vColorTint = [9.0, 9.0, 9.0, 9.0]
} )";

static const char ghost_vmat[ ] =
R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
		{
			shader = "csgo_effects.vfx"
			F_ADDITIVE_BLEND = 1
			F_BLEND_MODE = 1               
			F_TRANSLUCENT = 1

			g_flOpacityScale = 0.45
			g_flFresnelExponent = 0.75
			g_flFresnelFalloff = 1.0
			g_flFresnelMax = 0.0
			g_flFresnelMin = 1.0
			g_flToolsVisCubemapReflectionRoughness = 1.0
			g_flBeginMixingRoughness = 1.0

			g_tColor = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask1 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask2 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask3 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tSceneDepth = resource:"materials/default/default_mask_tga_fde710a5.vtex"

			g_vColorTint = [ 1.000000, 1.000000, 1.000000, 0 ]
		})";

static const char ghost_vmat_invis[ ] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
		{
			shader = "csgo_effects.vfx"
			F_ADDITIVE_BLEND = 1
			F_BLEND_MODE = 1               
			F_TRANSLUCENT = 1
			F_DISABLE_Z_BUFFERING = 1

			g_flOpacityScale = 0.45
			g_flFresnelExponent = 0.75
			g_flFresnelFalloff = 1.0
			g_flFresnelMax = 0.0
			g_flFresnelMin = 1.0
			g_flToolsVisCubemapReflectionRoughness = 1.0
			g_flBeginMixingRoughness = 1.0

			g_tColor = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask1 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask2 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tMask3 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
			g_tSceneDepth = resource:"materials/default/default_mask_tga_fde710a5.vtex"

			g_vColorTint = [ 1.000000, 1.000000, 1.000000, 0 ]
		})";