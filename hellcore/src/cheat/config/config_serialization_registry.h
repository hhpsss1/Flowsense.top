#pragma once
#include <context.h>
#include "../external/json/json.hpp"

using json = nlohmann::json;

struct config_serializer_t {
	std::function<void( json&, const std::any& )> to_json_fn;
	std::function<void( const json&, std::any& )> from_json_fn;
};

class c_config_serializer_registry {
public:
	void register_serializer( fnv1a_t type_hash, config_serializer_t serializer );
	bool has_serializer( fnv1a_t type_hash ) const;
	const config_serializer_t& get_serializer( fnv1a_t type_hash ) const;

private:
	std::unordered_map<fnv1a_t, config_serializer_t> m_serializers;
};
static auto g_config_serializer_registry = std::make_unique< c_config_serializer_registry >( );

#define REGISTER_CONFIG_TYPE(type) \
	g_config_serializer_registry->register_serializer( \
		fnv_hash(#type), \
		config_serializer_t{ \
			[](json& j, const std::any& val) { j = std::any_cast<type>(val); }, \
			[](const json& j, std::any& val) { val = j.get<type>(); } \
		} \
	)
