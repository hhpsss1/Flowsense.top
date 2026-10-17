#include "config_serialization_registry.h"

void c_config_serializer_registry::register_serializer( fnv1a_t type_hash, config_serializer_t serializer ) {
	m_serializers[ type_hash ] = std::move( serializer );
}

bool c_config_serializer_registry::has_serializer( fnv1a_t type_hash ) const {
	return m_serializers.find( type_hash ) != m_serializers.end( );
}

const config_serializer_t& c_config_serializer_registry::get_serializer( fnv1a_t type_hash ) const {
	return m_serializers.at( type_hash );
}
