#include "econ_item_system.h"
#include <cstring>

#include <sdk/datatypes/fnv1a.h>
#include <core/hooks/modules.h>

bool c_econ_item_definition::is_weapon( )
{
	return get_stickers_supported_count( ) >= 4;
}

bool c_econ_item_definition::is_agent( )
{
    if (!m_item_type_name || !*m_item_type_name)
        return false;
	constexpr auto type_custom_player = fnv_hash( "#Type_CustomPlayer" );
	if ( fnv_hash( m_item_type_name ) != type_custom_player )
		return false;

	return get_stickers_supported_count( ) >= 1;
}

bool c_econ_item_definition::is_knife( bool exclude_default )
{
    if (!m_item_type_name || !*m_item_type_name)
        return false;
	constexpr auto csgo_type_knife = fnv_hash( "#CSGO_Type_Knife" );
	if ( fnv_hash( m_item_type_name ) != csgo_type_knife )
		return false;

	return exclude_default ? m_definition_index >= 500 : true;
}

bool c_econ_item_definition::is_glove( bool exclude_default )
{
    if (!m_item_type_name || !*m_item_type_name)
        return false;
	constexpr auto type_hands = fnv_hash( "#Type_Hands" );
	if ( fnv_hash( m_item_type_name ) != type_hands )
		return false;

	const bool custom_glove = m_definition_index != 5028 && m_definition_index != 5029;
	return exclude_default ? custom_glove : true;
}

bool c_econ_item_definition::is_weapon_case( )
{
    if (!m_item_type_name || !*m_item_type_name)
        return false;
	constexpr auto type_weapon_case = fnv_hash( "#CSGO_Type_WeaponCase" );
	return fnv_hash( m_item_type_name ) == type_weapon_case;
}

bool c_econ_item_definition::is_key( )
{
    if (!m_item_type_name || !*m_item_type_name)
        return false;
	constexpr auto tool_weapon_case_key_tag = fnv_hash( "#CSGO_Tool_WeaponCase_KeyTag" );
	return fnv_hash( m_item_type_name ) == tool_weapon_case_key_tag;
}

const char* c_econ_item_definition::get_model_name( )
{
	return *address_t{ this }.add( MODEL_NAME ).as<const char**>( );
}

const char* c_econ_item_definition::get_simple_weapon_name( )
{
    const char* name = m_inventory_image;
    if (name && *name) {
        const char* slash = std::strrchr(name, '/');
        return slash ? slash + 1 : name;
    }
    if (m_definition_name && *m_definition_name)
        return m_definition_name;
    if (m_item_classname && *m_item_classname)
        return m_item_classname;
    return "";
}

int c_econ_item_definition::get_stickers_supported_count( )
{
	return *address_t{ this }.add( STICKERS_SUPPORTED_COUNT ).as<int*>( );
}

int c_econ_item_definition::get_loadout_slot( )
{
	return *address_t{ this }.add( LOADOUT_SLOT ).as<int*>( );
}

c_econ_item_schema* c_econ_item_system::get_econ_item_schema( )
{
	return *address_t{ this }.add( ECON_ITEM_SCHEMA ).as<c_econ_item_schema**>( );
}