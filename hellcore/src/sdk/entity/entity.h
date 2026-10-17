#pragma once

#include <sdk/schema/schema.h>
#include "base_handle.h"

#include <includes.h>

#include <math/math types/vector.h>
#include <math/math.h>

#include <utils/memory.h>


class c_schema_class_binding_base;
class c_physics_aggregate_instance;
class matrix3x4_t;

struct bone_data_t
{
	vec3_t m_pos;
	float m_scale;
	vec4_t m_rot;

	matrix3x4_t to_matrix( ) const;
};


class c_entity_identity
{
public:
	SCHEMA(uint32_t, flags, ("CEntityIdentity->m_flags"));
	OFFSET( int, index, 0x10 );

	bool is_valid( );
	int get_serial_number( );
	int get_entry_index( );
};


class c_entity_instance
{
public:
	SCHEMA(c_entity_identity*, m_pEntity, ("CEntityInstance->m_pEntity"));
	SCHEMA(__int64, m_CScriptComponent, ("CEntityInstance->m_CScriptComponent"));


	c_schema_class_binding_base* get_class_binding_base( );
	const char* get_class_name( );
	bool is_player_pawn( );
	bool is_player_controller( );
	bool is_player( );
	c_base_handle get_handle( );
};

class c_hitbox {
public:
	const char* m_name;                  // 0x0000
	const char* m_surface_property;      // 0x0008
	const char* m_bone_name;             // 0x0010
	vec3_t m_min_bounds;                 // 0x0018
	vec3_t m_max_bounds;                 // 0x0024
	float m_shape_radius;                // 0x0030
	std::uint32_t m_bone_name_hash;      // 0x0034
	std::int32_t m_group_id;             // 0x0038
	std::uint8_t m_shape_type;           // 0x003C
	bool m_translation_only;             // 0x003D
	std::uint32_t m_crc;                 // 0x0040
	PAD(4);                // 0x0044 color ?
	std::uint16_t m_hitbox_index;        // 0x0048
	PAD( 0x22 );

};

class c_hitbox_sets {
public:
    PAD(0x28);
    std::int32_t m_hitbox_count;
    PAD(4);
    c_hitbox* m_hitbox;
    PAD(0x10);
};
static_assert(offsetof(c_hitbox_sets, m_hitbox_count) == 0x28);
static_assert(offsetof(c_hitbox_sets, m_hitbox) == 0x30);
static_assert(sizeof(c_hitbox_sets) == 0x48);
static_assert(sizeof(c_hitbox) == 0x70);

class c_render_mesh {
public:
 PAD(0x150);
 c_hitbox_sets* m_hitbox_sets;
 PAD(0xC);
 std::uint32_t m_storage_flags;
 char m_storage[0xC];
 int m_set_count;
 c_hitbox_sets* get_hitbox_set() {
  if(m_set_count>0 && m_set_count<=32) return m_storage_flags&0x7FFFFFFF ? *reinterpret_cast<c_hitbox_sets**>(m_storage) : reinterpret_cast<c_hitbox_sets*>(m_storage);
  return m_hitbox_sets;
 }
}; // Size=0x118

class c_render_meshes {
public:
	c_render_mesh* m_meshes;
};

class c_model {
public:
	int get_bone_count() const {return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(this)+0x178);}
	c_hitbox* get_hitbox( const std::int32_t index );
    c_hitbox_sets* get_hitbox_set();
	const char* get_hitbox_name( const std::int32_t index );
	int get_hitbox_count( );

public:
	PAD( 0x70 );                    // 0x0000
	std::int32_t m_rendermesh_count; // 0x0070
	PAD( 0x4 );                     // 0x0074
	c_render_mesh** m_render_meshes;// 0x0078
};

static_assert(offsetof(c_model, m_rendermesh_count) == 0x70);
static_assert(offsetof(c_model, m_render_meshes) == 0x78);
static_assert(offsetof(c_render_mesh, m_hitbox_sets) == 0x150);
static_assert(offsetof(c_render_mesh, m_storage_flags) == 0x164);
static_assert(offsetof(c_render_mesh, m_storage) == 0x168);
static_assert(offsetof(c_render_mesh, m_set_count) == 0x174);

class c_model_state {
public:
	PAD( 160 ); // some bone array here - 8
	c_strong_handle<c_model> m_model_handle;

	OFFSET( __int64&, m_unk, 0xD8 );
};

class c_cs_player_pawn;
class c_skeleton_instance 
{
public:

	void calc_world_space_bones(std::uint32_t flags)
	{
		static auto fn = g_modules->m_client.find(xx("48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8D 6C 24 ? 48 8B 81")).as<__int64(__fastcall*)(c_skeleton_instance*, std::uint32_t)>();
		if (!fn)
			return;
		fn(this, flags);
	}

	

	int get_world_group_id() {
		using fn_get_world_group_id = int* (__fastcall*)(c_skeleton_instance*, int*);
		static fn_get_world_group_id get_world_group_id = g_modules->m_client.find(xx("48 8B 41 ? 48 85 C0 74 ? 48 8B 40 ? 8B 48")).as<fn_get_world_group_id>();

		int world_group_id = 0;
		get_world_group_id(this, &world_group_id);
		return world_group_id;
	}

	

public:
	char pad0[0x1AC];
	int m_legacy_bone_count;
	char pad1[0x18];
	int m_mask;
	char pad2[0x4];
	matrix2x4_t* m_legacy_bone_matrix;
 matrix2x4_t* get_bone_matrix() {return *reinterpret_cast<matrix2x4_t**>(reinterpret_cast<char*>(&m_modelState())+0x80);}
 int get_bone_count() {auto* model=static_cast<c_model*>(m_modelState().m_model_handle.binding ? m_modelState().m_model_handle.binding->data : nullptr);return model ? model->get_bone_count() : 0;}

	OFFSET( c_cs_player_pawn*, m_pawn, 0x30 );
	SCHEMA(c_model_state&, m_modelState, ("CSkeletonInstance->m_modelState"));
	SCHEMA(std::uint8_t, m_nHitboxSet, ("CSkeletonInstance->m_nHitboxSet"));
};


class c_skeleton_instance_body
{
public:
	OFFSET( c_physics_aggregate_instance*, get_physics_aggregate_instance, 0x250 );
};

class c_game_scene_node
{
public:
	SCHEMA(bool, m_dormant, ("CGameSceneNode->m_bDormant"));
	SCHEMA(vec3_t&, m_vecOrigin, ("CGameSceneNode->m_vecOrigin"));
	SCHEMA(vec3_t&, m_angRotation, ("CGameSceneNode->m_angRotation"));
	SCHEMA(float&, m_flScale, ("CGameSceneNode->m_flScale"));
	SCHEMA(vec3_t&, m_vecAbsOrigin, ("CGameSceneNode->m_vecAbsOrigin"));
	SCHEMA(vec3_t&, m_angAbsRotation, ("CGameSceneNode->m_angAbsRotation"));
	SCHEMA(float&, m_flAbsScale, ("CGameSceneNode->m_flAbsScale"));

	SCHEMA(c_transform, m_nodeToWorld, ("CGameSceneNode->m_nodeToWorld"));
	SCHEMA(c_game_scene_node*, m_pParent, ("CGameSceneNode->m_pParent"));
	SCHEMA(c_game_scene_node*, m_child, ("CGameSceneNode->m_pChild"));
	SCHEMA(c_entity_instance*, m_owner, ("CGameSceneNode->m_pOwner"));
	SCHEMA(c_game_scene_node*, m_sibling, ("CGameSceneNode->m_pNextSibling"));

	c_skeleton_instance* get_skeleton_instace( );
	c_skeleton_instance_body* get_skeleton_instace_body( );
	void set_mesh_group_mask( uint64_t mask );
};

class c_body_component
{
public:
	SCHEMA(c_game_scene_node*, m_pSceneNode, ("CBodyComponent->m_pSceneNode"));
};

class v_physics_collision_attribute_t
{
public:
	char pad_001[ 0x10 ];
	uint64 m_interacts_with;

	SCHEMA(uint64_t, m_nInteractsExclude, ("VPhysicsCollisionAttribute_t->m_nInteractsExclude"));
};

class c_collision
{
public:
	SCHEMA(v_physics_collision_attribute_t&, m_collisionAttribute, ("CCollisionProperty->m_collisionAttribute"));
	SCHEMA(vec3_t, m_vecMins, ("CCollisionProperty->m_vecMins"));
	SCHEMA(vec3_t, m_vecMaxs, ("CCollisionProperty->m_vecMaxs"));
	SCHEMA(uint8_t, m_usSolidFlags, ("CCollisionProperty->m_usSolidFlags"));
	SCHEMA(uint8_t, m_CollisionGroup, ("CCollisionProperty->m_CollisionGroup"));
	SCHEMA(float, m_flBoundingRadius, ("CCollisionProperty->m_flBoundingRadius"));
	OFFSET( uint16_t, collision_mask, 0x38 );

	
};
class c_base_entity : public c_entity_instance
{
public:
	SCHEMA(uint8_t, m_iTeamNum, ("C_BaseEntity->m_iTeamNum"));
	SCHEMA(int, m_iHealth, ("C_BaseEntity->m_iHealth"));
	SCHEMA(int&, m_fFlags, ("C_BaseEntity->m_fFlags"));
	SCHEMA(uint8_t, m_MoveType, ("C_BaseEntity->m_MoveType"));
	SCHEMA(int&, m_iEFlags, ("C_BaseEntity->m_iEFlags"));
	SCHEMA(float&, m_flFriction, ("C_BaseEntity->m_flFriction"));
	SCHEMA(c_base_handle, m_hOwnerEntity, ("C_BaseEntity->m_hOwnerEntity"));
	SCHEMA(uint8_t, m_nActualMoveType, ("C_BaseEntity->m_nActualMoveType"));
	SCHEMA(int&, m_nLastPredictableCommand, ("C_BaseEntity->m_nLastPredictableCommand"));
	SCHEMA(int&, m_nFirstPredictableCommand, ("C_BaseEntity->m_nFirstPredictableCommand"));
	SCHEMA(vec3_t&, m_vecVelocity, ("C_BaseEntity->m_vecVelocity"));
	SCHEMA(vec3_t&, m_vecAbsVelocity, ("C_BaseEntity->m_vecAbsVelocity"));
	SCHEMA(uint32_t&, m_nSubclassID, ("C_BaseEntity->m_nSubclassID"));
	SCHEMA(vec3_t, m_vecViewOffset, ("C_BaseModelEntity->m_vecViewOffset"));
	SCHEMA(vec3_t&, m_vecBaseVelocity, ("C_BaseEntity->m_vecBaseVelocity"));
	SCHEMA(float&, m_flSimulationTime, ("C_BaseEntity->m_flSimulationTime"));
	SCHEMA(int&, m_nSimulationTick, ("C_BaseEntity->m_nSimulationTick"));
	SCHEMA(float&, m_flGravityScale, ("C_BaseEntity->m_flGravityScale"));
	SCHEMA(int, m_nGroundBodyIndex, ("C_BaseEntity->m_nGroundBodyIndex"));
	SCHEMA(c_base_handle, m_hGroundEntity, ("C_BaseEntity->m_hGroundEntity"));//c_base_handle for c_base_entity
	SCHEMA(c_game_scene_node*, m_pGameSceneNode, ("C_BaseEntity->m_pGameSceneNode"));

	SCHEMA(c_body_component*, m_CBodyComponent, ("C_BaseEntity->m_CBodyComponent"));

	SCHEMA(c_collision*, m_pCollision, ("C_BaseEntity->m_pCollision"));
	SCHEMA(int&, m_nNoInterpolationTick, ("C_BaseEntity->m_nNoInterpolationTick"));
	SCHEMA(bool&, m_bSimulationTimeChanged, ("C_BaseEntity->m_bSimulationTimeChanged"));

	OFFSET(c_base_entity*, m_pCollideEntity, 0x148);

	void remove() {
		g_memory->call_virtual<void>(this, 151);
	}

	bool is_weapon( );
	void set_model( const char* name );
};

class C_Inferno : public c_base_entity
{
public:
	SCHEMA_ARRAY(bool, m_bFireIsBurning, 64, ("C_Inferno->m_bFireIsBurning"));
	SCHEMA(std::int32_t, m_fireCount, ("C_Inferno->m_fireCount"));
	SCHEMA(std::int32_t, m_nFireEffectTickBegin, ("C_Inferno->m_nFireEffectTickBegin"));
	SCHEMA(std::float_t, m_nFireLifetime, ("C_Inferno->m_nFireLifetime"));
	SCHEMA_ARRAY(vec3_t, m_firePositions, 64, ("C_Inferno->m_firePositions"));
	SCHEMA_ARRAY(vec3_t, m_fireParentPositions, 64, ("C_Inferno->m_fireParentPositions"));
	SCHEMA(vec3_t, m_minBounds, ("C_Inferno->m_minBounds"));
	SCHEMA(vec3_t, m_maxBounds, ("C_Inferno->m_maxBounds"));

	//[[nodiscard]] inline std::float_t GetMaxTime()
	//{
	//	return g_interfaces->m_engine_convar->find_by_name("inferno_flame_lifetime")->get_float();
	//}

	/*const char* GetInfernoType()
	{
		const char* szInfernoType = Memory::CallVFunc<const char*, 0x1F8>(this);
		return szInfernoType;
	}*/
}; 

class C_SmokeGrenadeProjectile : public c_base_entity
{
public:
	SCHEMA(std::int32_t, m_nSmokeEffectTickBegin, ("C_SmokeGrenadeProjectile->m_nSmokeEffectTickBegin"));
	SCHEMA(bool, m_bDidSmokeEffect, ("C_SmokeGrenadeProjectile->m_bDidSmokeEffect"));
	SCHEMA(std::int32_t, m_nRandomSeed, ("C_SmokeGrenadeProjectile->m_nRandomSeed"));
	SCHEMA(vec3_t, m_vSmokeColor, ("C_SmokeGrenadeProjectile->m_vSmokeColor"));
	SCHEMA(vec3_t, m_vSmokeDetonationPos, ("C_SmokeGrenadeProjectile->m_vSmokeDetonationPos"));
	SCHEMA(bool, m_bSmokeVolumeDataReceived, ("C_SmokeGrenadeProjectile->m_bSmokeVolumeDataReceived"));
	SCHEMA(bool, m_bSmokeEffectSpawned, ("C_SmokeGrenadeProjectile->m_bSmokeEffectSpawned"));
};

class c_material_color;

class c_gradient_fog : public c_base_entity {
public:
	SCHEMA(bool&, m_is_enabled, ("C_GradientFog->m_bIsEnabled"));
	SCHEMA(float&, m_fog_start_distance, ("C_GradientFog->m_flFogStartDistance"));
	SCHEMA(float&, m_fog_end_distance, ("C_GradientFog->m_flFogEndDistance"));
	SCHEMA(float&, m_fog_strength, ("C_GradientFog->m_flFogStrength"));
	SCHEMA(float&, m_fog_falloff_exponent, ("C_GradientFog->m_flFogFalloffExponent"));
	SCHEMA(c_material_color&, m_fog_color, ("C_GradientFog->m_fogColor"));
	SCHEMA(float&, m_fog_max_opacity, ("C_GradientFog->m_flFogMaxOpacity"));
};

//class c_base_view_model : public c_base_entity {
//public:
//	SCHEMA( c_base_handle, m_hWeapon, fnv_hash( "C_BaseViewModel->m_hWeapon" ) );
//};
class c_map_info : public c_base_entity {
public:
    SCHEMA(bool&, m_bRainTraceToSkyEnabled, ("CMapInfo->m_bRainTraceToSkyEnabled"));
    SCHEMA(float&, m_flEnvRainStrength, ("CMapInfo->m_flEnvRainStrength"));
};
