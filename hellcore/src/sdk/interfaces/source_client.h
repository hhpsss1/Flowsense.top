#pragma once

#include <utils/memory.h>

#include "econ_item_system.h"

class c_debug_overlay_game_system {
public:
    void render_without_dots( bool should ) {
      //  return g_memory->call_virtual<void>( this, 10, should );
    }

    void add_line( const vec3_t& start, const vec3_t& end, const hellcolor& col, bool no_deth_test, float duration ) {
        return g_memory->call_virtual<void>( this, 12, start, end, col, no_deth_test, static_cast<double>( duration ) );
    }

    void add_box( const vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, const vec3_t& rot, const hellcolor& col, float duration ) {
        using fn_t = void(__fastcall*)(void*, const vec3_t&, const vec3_t&, const vec3_t&, const vec3_t&, uint8_t, uint8_t, uint8_t, uint8_t, bool, double, float);
        auto fn = g_memory->get_method<fn_t>(this, 49);
        fn(this, origin, mins, maxs, rot, static_cast<uint8_t>(col.Value.x * 255), static_cast<uint8_t>(col.Value.y * 255), static_cast<uint8_t>(col.Value.z * 255), static_cast<uint8_t>(col.Value.w * 255), false, static_cast<double>(duration), 1.f);
    }

    void add_text( const vec3_t& origin, float duration, int line_offset, const char* text, ... ) {
        return g_memory->call_virtual<void>( this, 59 + 4U, origin, line_offset, static_cast<double>( duration ), text );
    }

    void add_text( const vec3_t& origin, float duration, const char* text, ... ) {
        return g_memory->call_virtual<void>( this, 60 + 4U, origin, static_cast<double>( duration ), text );
    }

    void add_text( const vec3_t& origin, float duration, int line_offset, const hellcolor& col, const char* text... ) {
        return g_memory->call_virtual<void>( this, 61 + 4U, origin, line_offset, static_cast<double>( duration ), col.Value.x * 255, col.Value.y * 255, col.Value.z * 255, col.Value.w * 255, text );
    }
};

class c_source2_client {
public:
    c_econ_item_system* get_econ_item_system() {
 using fn_t=c_econ_item_system*(__fastcall*)();
 static auto fn=g_modules->m_client.find(xx("48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 81")).as<fn_t>();
 return fn ? fn() : nullptr;
}

    c_debug_overlay_game_system* get_debug_overlay( ) {
        static auto** overlay = g_modules->m_client.find(xx("48 89 2D ? ? ? ? 48 89 2D ? ? ? ? 48 89 2D ? ? ? ? 48 89 2D ? ? ? ? 48 85 FF 74 ? 48 8B")).relative(3, 7).as<c_debug_overlay_game_system**>();
        return overlay ? *overlay : nullptr;
    }
};