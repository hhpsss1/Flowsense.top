#pragma once

#include <memory>
#include <cstdint>
#include <unordered_map>

#include <sdk/datatypes/fnv1a.h>
#include <mutex>

class c_schema {
public:
    using schema_key_value_map_t = std::unordered_map<std::string, std::uint32_t>;
    using schema_table_map_t = std::unordered_map<std::string, schema_key_value_map_t>;

private:
    static schema_table_map_t schema_table_map;
    static std::mutex map_mutex;

    static std::string extract_class_name(const std::string& full_name);
    static std::string extract_field_name(const std::string& full_name);

public:
    void init_class(const std::string& class_name);
    std::uint32_t get_offset(const std::string& full_name);
};

inline auto g_schema = std::make_unique<c_schema>();

#define SCHEMA_ARRAY(type, name, size, full_name_str) \
    type* name##_array() \
    { \
        static auto init_offset = []() -> uintptr_t { \
            static std::string full_name = full_name_str; \
            size_t arrow_pos = full_name.find("->"); \
            if (arrow_pos != std::string::npos) { \
                std::string class_name = full_name.substr(0, arrow_pos); \
                 \
            } \
            return static_cast<uintptr_t>(g_schema->get_offset(full_name)); \
        }; \
        static uintptr_t schema_offset = init_offset(); \
        return reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + schema_offset); \
    } \
    type& name##_at(int index) \
    { \
        if (index < 0 || index >= size) { \
            static type dummy{}; \
            return dummy; \
        } \
        return name##_array()[index]; \
    }

#define SCHEMA(type, name, full_name_str) \
    type name() const \
    { \
        static const auto init_offset = []() -> uintptr_t { \
            static const std::string full_name = full_name_str; \
            static const size_t arrow_pos = full_name.find("->"); \
            static const std::string class_name = (arrow_pos != std::string::npos) ? full_name.substr(0, arrow_pos) : ""; \
            if (!class_name.empty()) { \
                 \
            } \
            return static_cast<uintptr_t>(g_schema->get_offset(full_name)); \
        }; \
        static const uintptr_t schema_offset = init_offset(); \
        return *reinterpret_cast<std::remove_reference_t<type>*>(reinterpret_cast<uintptr_t>(this) + schema_offset); \
    }

#define OFFSET(type, name, offset) \
	type name() \
	{ \
		return *(std::remove_reference_t<type>*)(reinterpret_cast<uintptr_t>(this) + offset);\
	} \

#define SCHEMA_PTR(type, name, full_name_str) \
    type* name() const \
    { \
        static const auto init_offset = []() -> uintptr_t { \
            static const std::string full_name = full_name_str; \
            static const size_t arrow_pos = full_name.find("->"); \
            static const std::string class_name = (arrow_pos != std::string::npos) ? full_name.substr(0, arrow_pos) : ""; \
            if (!class_name.empty()) { \
                 \
            } \
            return static_cast<uintptr_t>(g_schema->get_offset(full_name)); \
        }; \
        static const uintptr_t schema_offset = init_offset(); \
        return reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + schema_offset); \
    }


#define OFFSET_PTR(type, name, offset) \
	type name() \
	{ \
		return (std::remove_reference_t<type>*)(reinterpret_cast<uintptr_t>(this) + offset);\
	} \
