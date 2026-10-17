#pragma once

#include <includes.h>
#include <sdk/datatypes/utl_vector.h>

class c_cs_input_message {
public:
    int32_t m_render_tick_count; //0x0000
    float m_render_tick_fraction; //0x0004
    int32_t m_player_tick_count; //0x0008
    float m_player_tick_fraction; //0x000C
    vec3_t m_view_angles; //0x0010
    vec3_t m_origin; //0x001C
    char pad_0028[ 56 ]; //0x0028
}; //Size: 0x0060

class c_csgo_input {
public:
    void set_view_angle(const vec3_t& angle, int slot = 0) {
        using fn_t = void(__fastcall*)(c_csgo_input*, int, const vec3_t&);
        static auto fn = g_modules->m_client.find(xx("85 D2 75 ? 48 63 81")).as<fn_t>();
        if (fn)
            fn(this, slot, angle);
    }

    vec3_t get_view_angle(int slot = 0) {
        using fn_t = const vec3_t*(__fastcall*)(c_csgo_input*, int);
        static auto fn = g_modules->m_client.find(xx("4C 8B C1 85 D2 74 08 48 8D 05 ? ? ? ? C3")).as<fn_t>();
        const auto* angle = fn ? fn(this, slot) : nullptr;
        return angle ? *angle : vec3_t{};
    }
};
