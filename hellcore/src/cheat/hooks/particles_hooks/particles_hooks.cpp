#include "particles_hooks.h"
#include <core/hooks/hooks.h>
#include <core/interfaces/interfaces.h>
#include <context.h>

namespace {
bool read_particle_name(void* data, char* output, size_t capacity) {
    __try {
        if (!data || capacity < 2)
            return false;
        auto* definition = *reinterpret_cast<char**>(static_cast<char*>(data) + 0x48);
        if (!definition)
            return false;
        auto* resource = *reinterpret_cast<char**>(definition + 0x18);
        if (!resource)
            return false;
        auto* name_slot = *reinterpret_cast<const char***>(resource + 0x8);
        if (!name_slot || !*name_slot)
            return false;
        const char* name = *name_slot;
        for (size_t i = 0; i < capacity; ++i) {
            output[i] = name[i];
            if (!output[i])
                return true;
        }
        return false;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

void write_particle_color(void* data, const float* color) {
    __try {
        auto* output = reinterpret_cast<float*>(static_cast<char*>(data) + 0x50);
        for (int i = 0; i < 4; ++i)
            output[i] = color[i];
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    }
}
}

void* __fastcall c_particles_hooks::particles_draw_array(void* renderer, void* particle_data, void* draw_data, void* render_state, void* context)
{
    const auto original = g_hooks->m_particles.m_particles_draw_array.get<decltype(&particles_draw_array)>();
    if (GET_VAR(bool, VISUALS_PATH(m_particle_modulation)) && particle_data &&
        g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game()) {
        char name[512]{};
        if (read_particle_name(particle_data, name, sizeof(name))) {
            hellcolor color;
            bool apply = false;
            if (strstr(name, "molotov") || strstr(name, "incendiary") || strstr(name, "inferno_fx") || strstr(name, "explosion_incend_air")) {
                color = GET_VAR(hellcolor, VISUALS_PATH(m_molotov_color));
                apply = true;
            } else if (strstr(name, "taser")) {
                color = GET_VAR(hellcolor, VISUALS_PATH(m_taser_stim_color));
                apply = true;
            } else if (strstr(name, "explosion_basic") || strstr(name, "hegrenade") || strstr(name, "he_grenade")) {
                color = GET_VAR(hellcolor, VISUALS_PATH(m_explosion_color));
                apply = true;
            }
            if (apply) {
                const float rgba[]{std::clamp(color.Value.x, 0.f, 1.f), std::clamp(color.Value.y, 0.f, 1.f), std::clamp(color.Value.z, 0.f, 1.f), std::clamp(color.Value.w, 0.f, 1.f)};
                write_particle_color(particle_data, rgba);
            }
        }
    }
    return original(renderer, particle_data, draw_data, render_state, context);
}
