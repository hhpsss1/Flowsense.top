#pragma once

#include <includes.h>

#include <json/json.hpp>
#include <sdk/datatypes/fnv1a.h>

using namespace std::filesystem;
using json = nlohmann::json;

#define ADD_VAR( type, name, default_val ) fnv1a_t name = g_config_system->add_variable<type>( default_val, fnv_hash( #name ), fnv_hash( #type ) )
#define ADD_VAR_VEC( type, size, name, default_val ) fnv1a_t name = g_config_system->add_variable<std::vector<type>>( g_config_system->get_filled_vec<type, size>(default_val), fnv_hash( #name ), fnv_hash( xx( "std::vector<" #type ">" ) ) )
#define GET_VAR( type, path ) g_config_system->get<type>( path )
#define GET_VAR_VEC( type, path ) g_config_system->get<std::vector<type>>( path )

struct config_var_t {
	std::any m_val;
	fnv1a_t m_name_hash;
	fnv1a_t m_data_type_hash;

	template <typename t>
	t& as( ) {
		return *(t*)( std::any_cast<t>( &m_val ) );
	}
};

class c_config_system {
private:
	// this will have to do for now, later on we are gonna do cloud configs using protobufs.
	path m_config_path = "C:\\hellcore-alpha\\configs";
	std::string m_config_file_type = ".hell";

	std::unordered_map<fnv1a_t, config_var_t> m_config_variables;
public:
	void init( void ) const;
	void save_config( const std::string_view& config_name );
	void load_config( const std::string_view& config_name );
	bool delete_config( const std::string_view& config_name ) const;

	std::vector<std::string> get_all_configs( );

	template <typename t>
	t& get( fnv1a_t path ) {
		return m_config_variables[ path ].as< t >( );
	}

	template <typename t>
	fnv1a_t add_variable( t value, fnv1a_t name_hash, fnv1a_t data_type_hash ) {
		return ( m_config_variables[ name_hash ] = { std::make_any< t >( value ), name_hash, data_type_hash }, name_hash );
	}

	template <typename t, int size>
	std::vector<t> get_filled_vec( const t& fill ) {
		std::vector<t> temp_vec( size );
		std::fill( temp_vec.begin( ), temp_vec.begin( ) + size, fill );
		return temp_vec;
	}
};
inline auto g_config_system = std::make_unique<c_config_system>( );
