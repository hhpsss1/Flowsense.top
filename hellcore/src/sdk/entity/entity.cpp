#include "entity.h"
#include "pawn.h"

#include <cstring>

#include <sdk/interfaces/schema_system.h>

matrix3x4_t bone_data_t::to_matrix( ) const {
	matrix3x4_t matrix;

	const vec4_t& q = m_rot;
	const vec3_t& pos = m_pos;

	const float xx = q.x * q.x;
	const float yy = q.y * q.y;
	const float zz = q.z * q.z;
	const float xy = q.x * q.y;
	const float xz = q.x * q.z;
	const float yz = q.y * q.z;
	const float wx = q.w * q.x;
	const float wy = q.w * q.y;
	const float wz = q.w * q.z;

	matrix[ 0 ][ 0 ] = 1.0f - 2.0f * ( yy + zz );
	matrix[ 0 ][ 1 ] = 2.0f * ( xy - wz );
	matrix[ 0 ][ 2 ] = 2.0f * ( xz + wy );
	matrix[ 0 ][ 3 ] = pos.x;

	matrix[ 1 ][ 0 ] = 2.0f * ( xy + wz );
	matrix[ 1 ][ 1 ] = 1.0f - 2.0f * ( xx + zz );
	matrix[ 1 ][ 2 ] = 2.0f * ( yz - wx );
	matrix[ 1 ][ 3 ] = pos.y;

	matrix[ 2 ][ 0 ] = 2.0f * ( xz - wy );
	matrix[ 2 ][ 1 ] = 2.0f * ( yz + wx );
	matrix[ 2 ][ 2 ] = 1.0f - 2.0f * ( xx + yy );
	matrix[ 2 ][ 3 ] = pos.z;

	return matrix;
}

c_schema_class_binding_base* c_entity_instance::get_class_binding_base() {
 auto* identity=m_pEntity();
 auto* binding=identity ? *reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(identity)+8) : nullptr;
 return binding ? *reinterpret_cast<c_schema_class_binding_base**>(binding+0x58) : nullptr;
}

const char* c_entity_instance::get_class_name( )
{
	const auto class_binding_base = get_class_binding_base( );
	return class_binding_base ? class_binding_base->get_name() : nullptr;
}

bool c_entity_instance::is_player_pawn( )
{
	const char* name = get_class_name( );
	return name && std::strstr( name, "C_CSPlayerPawn" ) != nullptr;
}

bool c_entity_instance::is_player_controller( )
{
	const char* name = get_class_name( );
	return name && std::strstr( name, "CCSPlayerController" ) != nullptr;
}

bool c_entity_instance::is_player( )
{
	return is_player_pawn( ) || is_player_controller( );
}

c_base_handle c_entity_instance::get_handle( )
{
	auto identity = m_pEntity( );
	if ( identity )
		return c_base_handle( identity->index( ), identity->get_serial_number( ) - ( identity->flags( ) & 1 ) );

	return {};
}

bool c_entity_identity::is_valid( )
{
	return index( ) != INVALID_EHANDLE_INDEX;
}

int c_entity_identity::get_serial_number( )
{
	return index( ) >> NUM_SERIAL_NUM_SHIFT_BITS;
}

int c_entity_identity::get_entry_index( )
{
	if ( is_valid( ) )
		return index( ) & ENT_ENTRY_MASK;

	return ENT_ENTRY_MASK;
}


c_skeleton_instance* c_game_scene_node::get_skeleton_instace() {
 return reinterpret_cast<c_skeleton_instance*>(this);
}

c_skeleton_instance_body* c_game_scene_node::get_skeleton_instace_body( )
{
	return g_memory->call_virtual<c_skeleton_instance_body*>( this, 11 );
}

void c_game_scene_node::set_mesh_group_mask( uint64_t mask )
{
	using fn_set_mesh_group_mask = void( __fastcall* )( void*, uint64_t );
	static auto fn = g_modules->m_client.find( xx( "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71" ) ).as<fn_set_mesh_group_mask>( );
	fn( this, mask );
}



bool c_base_entity::is_weapon() {
 const auto base=reinterpret_cast<std::uintptr_t>(g_modules->m_client.get());
 auto** table=*reinterpret_cast<void***>(this);
 const auto* locator=reinterpret_cast<const std::uint32_t*>(table[-1]);
 if(!locator || locator[0]!=1 || locator[4]>=0x2994000) return false;
 const auto* hierarchy=reinterpret_cast<const std::uint32_t*>(base+locator[4]);
 if(hierarchy[2]>64 || hierarchy[3]>=0x2994000) return false;
 const auto* classes=reinterpret_cast<const std::uint32_t*>(base+hierarchy[3]);
 for(unsigned i=0;i<hierarchy[2];++i) {
  if(classes[i]>=0x2994000) continue;
  const auto type=*reinterpret_cast<const std::uint32_t*>(base+classes[i]);
  if(type>=0x2994000) continue;
  const auto* name=reinterpret_cast<const char*>(base+type+0x10);
  if(std::strcmp(name,".?AVC_BasePlayerWeapon@@")==0) return true;
 }
 return false;
}

void c_base_entity::set_model( const char* name )
{
	using fn_set_model = void( __fastcall* )( void*, const char* );
	static auto fn = g_modules->m_client.find( xx( "40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24" ) ).as<fn_set_model>( );
	if ( !fn )
		return;
	fn( this, name );
}

c_hitbox_sets* c_model::get_hitbox_set() {
    if (m_rendermesh_count <= 0 || m_rendermesh_count > 1024 ||
        reinterpret_cast<std::uintptr_t>(m_render_meshes) < 0x10000)
        return nullptr;
    auto* mesh = m_render_meshes[0];
    if (reinterpret_cast<std::uintptr_t>(mesh) < 0x10000)
        return nullptr;
    auto* set = mesh->get_hitbox_set();
    if (reinterpret_cast<std::uintptr_t>(set) < 0x10000 || set->m_hitbox_count <= 0 ||
        set->m_hitbox_count > 64 || reinterpret_cast<std::uintptr_t>(set->m_hitbox) < 0x10000)
        return nullptr;
    return set;
}

c_hitbox* c_model::get_hitbox(const std::int32_t index) {
    if (index < 0)
        return nullptr;
    auto* set = get_hitbox_set();
    return set && index < set->m_hitbox_count ? &set->m_hitbox[index] : nullptr;
}

int c_model::get_hitbox_count() {
    auto* set = get_hitbox_set();
    return set ? set->m_hitbox_count : 0;
}
