#pragma once
#include <core/interfaces/interfaces.h>

enum e_particle_setting : unsigned int {
    particle_setting_position = 0,
    particle_setting_density = 2,
    particle_setting_info = 3,
    particle_setting_color = 16
};

class c_particle_snapshot {
public:
    void draw( int count, void* data ) {
        if (count <= 0 || !data)
            return;
        auto* snapshot = *reinterpret_cast<void**>(this);
        if (!snapshot)
            return;
        using draw_t = void(__fastcall*)(void*, int, void*);
        auto draw_fn = g_memory->get_method<draw_t>(snapshot, 1);
        if (draw_fn)
            draw_fn(snapshot, count, data);
    }
};

class c_particle_system_mgr {
public:
    void create_snapshot( c_particle_snapshot** snapshot ) {
        int64_t unknown = 0;
        g_memory->call_virtual<void>( this, 41, snapshot, &unknown );
    }
};

class c_create_particle {
public:
    void create_particle( c_particle_snapshot** snapshot, uint64_t unknown ) {
        g_memory->call_virtual<void>( this, 42, snapshot, &unknown );
    }
};

struct particle_color_t {
    float r;
    float g;
    float b;
};

struct particle_information_t {
    float m_time;
    float m_width;
    float m_alpha;
};

class particle_effect_t {
public:
    const char* m_name;
    PAD( 0x30 );
};

struct particle_data_t {
    vec3_t* m_positions;
    PAD( 0x74 );
    float* m_times;
    PAD( 192 );
};

struct particle_info_t {
    uint32_t m_effect_index;
    vec3_t* m_positions;
    float* m_times;
    c_particle_snapshot* m_snapshot_handle{};
    particle_data_t m_data;
};

class c_game_particle_manager {
public:
    bool init_effect(unsigned int effect_index, unsigned int unknown, c_particle_snapshot** particle_snapshot) {
        using fn_init_effect = bool(__fastcall*)(c_game_particle_manager*, unsigned int, unsigned int, c_particle_snapshot*);
        static fn_init_effect init_effect = g_modules->m_client.find(xx("56 48 83 EC 20 41 8B F0 49 8B C1 45 33 C0 4C 8B C9 83 FA FF")).as<fn_init_effect>();
        return init_effect(this, effect_index, unknown, particle_snapshot ? *particle_snapshot : nullptr);
    }

    int* create_particle_effect(unsigned int* effect_index, const char* name) {
        using fn_create_particle_effect = int* (__fastcall*)(c_game_particle_manager*, unsigned int*, const char*, int, __int64, __int64, __int64, int);
        static fn_create_particle_effect create_particle_effect = g_modules->m_client.find(xx("4C 8B DC 53 48 81 EC 90 00 00 00 F2 0F 10 05")).as<fn_create_particle_effect>();
        return create_particle_effect(this, effect_index, name, 2, 0ll, 0ll, 0ll, 0);
    }

    bool set_particle_data(unsigned int effect_index, int unknown, void* data) {
        using fn_set_particle_data = bool(__fastcall*)(c_game_particle_manager*, unsigned int, int, void*, float);
        static fn_set_particle_data set_particle_data = g_modules->m_client.find(xx("48 83 EC 58 F3 41 0F 10 51 04 F3 41 0F 10 09 F3 41 0F 10 59 08 4C 8D 4C 24 30 0F 28 05 ? ? ? ? 0F 14 CA F2 0F 11 4C 24 30 F3 0F 10 8C 24 80 00 00 00 F3 0F 11 4C")).as<fn_set_particle_data>();
        return set_particle_data(this, effect_index, unknown, data, 0.f);
    }

    void destroy_particle(int index, bool a1, bool a2) {
        using destroy_particle_t = void(__fastcall*)(c_game_particle_manager*, int, char, char);
        static const destroy_particle_t fn = g_modules->m_client.find(xx("83 FA ? 0F 84 ? ? ? ? 41 54")).as<destroy_particle_t>();
        return fn(this, index, a1, a2);
    }

    void release_particle_index( int index ) {
        g_memory->call_virtual<void>( this, 3, index );
    }
};
