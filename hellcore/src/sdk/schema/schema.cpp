#include "schema.h"
#include "current_schema.h"

#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/schema_system.h>

c_schema::schema_table_map_t c_schema::schema_table_map;
std::mutex c_schema::map_mutex;

std::string c_schema::extract_class_name(const std::string& full_name)
{
    size_t arrow_pos = full_name.find("->");
    if (arrow_pos == std::string::npos)
        return "";
    return full_name.substr(0, arrow_pos);
}

std::string c_schema::extract_field_name(const std::string& full_name)
{
    size_t arrow_pos = full_name.find("->");
    if (arrow_pos == std::string::npos)
        return "";
    return full_name.substr(arrow_pos + 2);
}

void c_schema::init_class(const std::string& class_name)
{
    std::lock_guard<std::mutex> lock(map_mutex);

    if (schema_table_map.find(class_name) != schema_table_map.end())
        return;

    std::deque<const char*> schema_modules = {
        "client.dll",
        "engine2.dll",
        "schemasystem.dll",
        "animationsystem.dll"
    };

    for (const char* dll_file : schema_modules)
    {
        auto* type_scope = g_interfaces->m_schema_system->find_type_scope_for_module(dll_file);
        if (!type_scope)
            continue;

        auto* class_info = type_scope->find_declared_class(class_name.c_str());
        if (!class_info)
            continue;

        short fields_size = class_info->get_fields_size();
        auto* fields = class_info->get_fields();

        auto& key_value_map = schema_table_map[class_name];

        for (int i = 0; i < fields_size; ++i)
        {
            auto& field = fields[i];
            key_value_map.emplace(field.m_name, field.m_offset);
        }

        return;
    }

    schema_table_map.emplace(class_name, schema_key_value_map_t{});
    printf_s("[Schema] Warning: Class not found: %s\n", class_name.c_str());
}

std::uint32_t c_schema::get_offset(const std::string& full_name)
{
    // live schema first: hardcoded current_schema values go stale on every
    // game update (stale offsets = broken esp, stuck scope, crashes).
    // the dumped table below is only a fallback for classes the live
    // schema scopes above don't declare.
    const auto class_name=extract_class_name(full_name);
    const auto field_name=extract_field_name(full_name);
    if(class_name.empty() || field_name.empty()) return 0;
    init_class(class_name);
    {
        std::lock_guard<std::mutex> lock(map_mutex);
        const auto cls=schema_table_map.find(class_name);
        if(cls!=schema_table_map.end()) {
            const auto field=cls->second.find(field_name);
            if(field!=cls->second.end() && field->second!=0) return field->second;
        }
    }
    if(auto offset=current_schema::find(full_name)) return *offset;
    return 0;
}
