#pragma once
#include <nlohmann/json.hpp>
#include "vars.h"

#include "config_serialization_registry.h"
#include <cheat/input.h>

#define LOAD(json, obj, key) \
    if ((json).contains(#key)) (json).at(#key).get_to((obj).key);

#define SAVE(json, obj, key) \
    (json)[#key] = (obj).key;

inline void to_json( nlohmann::json& j, const c_vars::skins_t::skin_settings_t& s ) {
    SAVE( j, s, m_item_definition_index );
    SAVE( j, s, m_paint_kit );
    SAVE( j, s, m_previous_skin );
}

inline void from_json( const nlohmann::json& j, c_vars::skins_t::skin_settings_t& s ) {
    LOAD( j, s, m_item_definition_index )
    LOAD( j, s, m_paint_kit )
    LOAD( j, s, m_previous_skin )
}

inline void to_json( nlohmann::json& j, const keybind_t& k ) {
    SAVE( j, k, m_holder_id );
    SAVE( j, k, m_mode );
    SAVE( j, k, m_key );
    SAVE( j, k, m_keybind_type );
    SAVE( j, k, m_is_on_mode );

    switch ( k.m_keybind_type ) {
    case e_keybind_type::keybind_type_checkbox:
        if ( std::holds_alternative<bool>( k.m_default_val ) ) {
            j[ xx( "m_default_val" ) ] = std::get<bool>( k.m_default_val );
        }
        break;
    case e_keybind_type::keybind_type_slider_int:
        if ( std::holds_alternative<int>( k.m_default_val ) ) {
            j[ xx( "m_default_val" ) ] = std::get<int>( k.m_default_val );
        }
        break;
    case e_keybind_type::keybind_type_slider_float:
        if ( std::holds_alternative<float>( k.m_default_val ) ) {
            j[ xx( "m_default_val" ) ] = std::get<float>( k.m_default_val );
        }
        break;
    default:
        break;
    }

    switch ( k.m_keybind_type ) {
    case e_keybind_type::keybind_type_checkbox:
        if ( std::holds_alternative<bool>( k.m_override_val ) ) {
            j[ xx( "m_override_val" ) ] = std::get<bool>( k.m_override_val );
        }
        break;
    case e_keybind_type::keybind_type_slider_int:
        if ( std::holds_alternative<int>( k.m_override_val ) ) {
            j[ xx( "m_override_val" ) ] = std::get<int>( k.m_override_val );
        }
        break;
    case e_keybind_type::keybind_type_slider_float:
        if ( std::holds_alternative<float>( k.m_override_val ) ) {
            j[ xx( "m_override_val" ) ] = std::get<float>( k.m_override_val );
        }
        break;
    default:
        break;
    }
}

inline void from_json( const nlohmann::json& j, keybind_t& k ) {
    LOAD( j, k, m_holder_id );
    LOAD( j, k, m_mode );
    LOAD( j, k, m_key );
    LOAD( j, k, m_keybind_type );
    LOAD( j, k, m_is_on_mode );

    if ( j.contains( xx( "m_default_val" ) ) ) {
        switch ( k.m_keybind_type ) {
        case e_keybind_type::keybind_type_checkbox:
            k.m_default_val = j.at( xx( "m_default_val" ) ).get<bool>( );
            break;
        case e_keybind_type::keybind_type_slider_int:
            k.m_default_val = j.at( xx( "m_default_val" ) ).get<int>( );
            break;
        case e_keybind_type::keybind_type_slider_float:
            k.m_default_val = j.at( xx( "m_default_val" ) ).get<bool>( );
            break;
        }
    }

    if ( j.contains( xx( "m_override_val" ) ) ) {
        switch ( k.m_keybind_type ) {
        case e_keybind_type::keybind_type_checkbox:
            k.m_override_val = j.at( xx( "m_override_val" ) ).get<bool>( );
            break;
        case e_keybind_type::keybind_type_slider_int:
            k.m_override_val = j.at( xx( "m_override_val" ) ).get<int>( );
            break;
        case e_keybind_type::keybind_type_slider_float:
            k.m_override_val = j.at( xx( "m_override_val" ) ).get<float>( );
            break;
        }
    }
}

inline void to_json( nlohmann::json& j, const hellcolor& c ) {
    j = {
        static_cast<int>( c.Value.x * 255.0f ),
        static_cast<int>( c.Value.y * 255.0f ),
        static_cast<int>( c.Value.z * 255.0f ),
        static_cast<int>( c.Value.w * 255.0f )
    };
}

inline void from_json( const nlohmann::json& j, hellcolor& c ) {
    std::vector<int> arr = j.get<std::vector<int>>( );
    if ( arr.size( ) >= 4 ) {
        c = hellcolor(
            arr[ 0 ],
            arr[ 1 ],
            arr[ 2 ],
            arr[ 3 ]
        );
    }
}

inline void to_json( json& j, const kb_map_t& map ) {
    j = json::object( );
    for ( const auto& [id, arr] : map )
        j[ std::to_string( id ) ] = arr;
}

inline void from_json( const json& j, kb_map_t& map ) {
    map.clear( );
    for ( auto it = j.begin( ); it != j.end( ); ++it ) {
        ImGuiID id = static_cast<ImGuiID>( std::stoul( it.key( ) ) );
        map[ id ] = it.value( ).get<std::vector<keybind_t>>( );
    }
}

inline void to_json( nlohmann::json& j, const c_visuals::text_object_t& s ) {
    SAVE( j, s, m_text_shadow_type );
    SAVE( j, s, m_text_alignment );
    SAVE( j, s, m_font_type );
    SAVE( j, s, m_element_position );
    SAVE( j, s, m_element_index );
    SAVE( j, s, m_size );
    SAVE( j, s, m_padding );
    SAVE( j, s, m_fg_color );
    SAVE( j, s, m_bg_color );
}

inline void from_json( const nlohmann::json& j, c_visuals::text_object_t& s ) {
    LOAD( j, s, m_text_shadow_type );
    LOAD( j, s, m_text_alignment );
    LOAD( j, s, m_font_type );
    LOAD( j, s, m_element_position );
    LOAD( j, s, m_element_index );
    LOAD( j, s, m_size );
    LOAD( j, s, m_padding );
    LOAD( j, s, m_fg_color );
    LOAD( j, s, m_bg_color );
}

/*
struct text_object_t {
        const char* m_text;
        e_text_shadow_type m_text_shadow_type;
        e_text_alignment m_text_alignment;

        e_font_type m_font_type;

        e_element_bb_position m_element_position;
        int m_element_index;

        int m_size;
        int m_padding;

        hellcolor m_fg_color;
        hellcolor m_bg_color;
    };

    struct bar_object_t {
        int m_value;

        bool m_number_value_enabled;
        text_object_t m_number_value_text;

        e_element_bb_position m_element_position;
        int m_element_index;

        int m_size;
        int m_padding;

        hellcolor m_fg_color;
        hellcolor m_bg_color;
    };
*/

inline void to_json( nlohmann::json& j, const c_visuals::bar_object_t& s ) {
    SAVE( j, s, m_value );
    SAVE( j, s, m_number_value_enabled );
    SAVE( j, s, m_number_value_text );
    SAVE( j, s, m_element_position );
    SAVE( j, s, m_element_index );
    SAVE( j, s, m_size );
    SAVE( j, s, m_padding );
    SAVE( j, s, m_fg_color );
    SAVE( j, s, m_bg_color );
}

inline void from_json( const nlohmann::json& j, c_visuals::bar_object_t& s ) {
    LOAD( j, s, m_value );
    LOAD( j, s, m_number_value_enabled );
    LOAD( j, s, m_number_value_text );
    LOAD( j, s, m_element_position );
    LOAD( j, s, m_element_index );
    LOAD( j, s, m_size );
    LOAD( j, s, m_padding );
    LOAD( j, s, m_fg_color );
    LOAD( j, s, m_bg_color );
}

inline void to_json( nlohmann::json& j, const c_visuals::player_visual_data_t& s ) {
    SAVE( j, s, m_name_text );
    SAVE( j, s, m_weapon_text );
    SAVE( j, s, m_armor_bar );
    SAVE( j, s, m_health_bar );
}

inline void from_json( const nlohmann::json& j, c_visuals::player_visual_data_t& s ) {
    LOAD( j, s, m_name_text );
    LOAD( j, s, m_weapon_text );
    LOAD( j, s, m_armor_bar );
    LOAD( j, s, m_health_bar );
}

class c_config_serializers {
private:

public:
    void register_all_config_types( ) {
        REGISTER_CONFIG_TYPE( int );
        REGISTER_CONFIG_TYPE( bool );
        REGISTER_CONFIG_TYPE( float );
        REGISTER_CONFIG_TYPE( hellcolor );
        REGISTER_CONFIG_TYPE( std::vector<int> );
        REGISTER_CONFIG_TYPE( std::vector<bool> );
        REGISTER_CONFIG_TYPE( std::vector<std::vector<int>> );
        REGISTER_CONFIG_TYPE( std::vector<c_vars::skins_t::skin_settings_t> );
        REGISTER_CONFIG_TYPE( kb_map_t );
        REGISTER_CONFIG_TYPE( c_visuals::player_visual_data_t );
    }

};
static auto g_config_serializers = std::make_unique<c_config_serializers>( );