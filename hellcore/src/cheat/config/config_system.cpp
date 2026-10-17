#include "config_system.h"
#include "vars.h"
#include "config_serializers.h"
#include "config_serialization_registry.h"
#include <cheat/features/visuals/overlay_features.h>

void c_config_system::init( void ) const {
	if ( !exists( m_config_path ) )
		std::filesystem::create_directories( m_config_path );

    g_config_serializers->register_all_config_types( );
}

void c_config_system::save_config( const std::string_view& config_name ) {
    path config_file = m_config_path / config_name;
    config_file += m_config_file_type;

    json j{};

    for ( const auto& [name_hash, var] : m_config_variables ) {
        const auto& type_hash = var.m_data_type_hash;

        if ( g_config_serializer_registry->has_serializer( type_hash ) ) {
            json value;
            g_config_serializer_registry->get_serializer( type_hash ).to_json_fn( value, var.m_val );
            j[ std::to_string( name_hash ) ] = value;
        }
    }

    std::ofstream out( config_file );
    if ( out.is_open( ) ) {
        out << j.dump( 4 );
        out.close( );
    }
}

void c_config_system::load_config( const std::string_view& config_name ) {
    path config_file = m_config_path / config_name;
    config_file += m_config_file_type;

    std::ifstream in( config_file );
    if ( !in.is_open( ) )
        return;

    json j;
    in >> j;
    in.close( );

    for ( const auto& [name_hash_str, val] : j.items( ) ) {
        if ( !std::all_of( name_hash_str.begin( ), name_hash_str.end( ), ::isdigit ) )
            continue;

        fnv1a_t name_hash = std::stoull( name_hash_str );

        auto it = m_config_variables.find( name_hash );
        if ( it == m_config_variables.end( ) )
            continue;

        auto& var = it->second;
        auto type_hash = var.m_data_type_hash;

        if ( g_config_serializer_registry->has_serializer( type_hash ) ) {
            g_config_serializer_registry->get_serializer( type_hash ).from_json_fn( val, var.m_val );
        }
    }

    g_overlay->add_notification("Loaded the " + std::string(config_name) + " config", false, true);

    /*for ( auto& i : GET_VAR( kb_map_t, CONFIG_PATH( m_widget_keybinds ) ) ) {
        for ( auto& j : i.second ) {
            bool nig = GET_VAR( bool, j.m_holder_id );
            j.m_val = &nig;
            int big_cocks = 69;
        }
    }*/
}

bool c_config_system::delete_config( const std::string_view& config_name ) const {
    std::filesystem::path config_file = m_config_path / config_name;
    config_file += m_config_file_type;

    if ( !std::filesystem::exists( config_file ) )
        return false;

    std::error_code ec;
    std::filesystem::remove( config_file, ec );

    if ( ec ) {
        LOG( xx( "failed to delete config: " ) + std::string( config_name ) + " - " + ec.message( ) );
        return false;
    }

    LOG( xx( "successfully deleted config: " ) + std::string( config_name ) );
    g_overlay->add_notification("Deleted the " + std::string(config_name) + " config", false, true);
    return true;
}

std::vector<std::string> c_config_system::get_all_configs( ) {
    std::vector<std::string> configs;

    for ( const auto& entry : std::filesystem::directory_iterator( m_config_path ) ) {
        if ( entry.path( ).extension( ) == m_config_file_type ) {
            configs.emplace_back( entry.path( ).stem( ).string( ) ); 
        }
    }

    return configs;
}
