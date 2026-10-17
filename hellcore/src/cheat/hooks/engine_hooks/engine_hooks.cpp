#include "engine_hooks.h"
#include <core/hooks/hooks.h>
#include <core/interfaces/interfaces.h>
#include <context.h>

std::uint64_t get_hash_function(const char* str, std::int32_t seed)
{
    static auto hash = g_modules->m_client.find(xx("48 89 5C 24 ? 44 0F B6 09")).as<std::uint64_t(__fastcall*)(const char*, std::int32_t)>()(str, seed);
    return hash;
}

unsigned int __fastcall c_engine_hooks::setup_blur(void* a1, unsigned int dof, vec4_t* range)
{
    const auto original = g_hooks->m_engine.m_setup_blur.get<decltype(&setup_blur)>();

    if (GET_VAR(bool, VISUALS_PATH(m_enable_world_blur)))
    {
        int h20 = get_hash_function(xx("DofRanges"), 0x3141592F);
        unsigned int hash2_ranges = (0x5BD1E995 * (h20 ^ 0x73)) ^ ((0x5BD1E995 * (h20 ^ 0x73u)) >> 13);
        unsigned int hash3_ranges = (1540483477 * hash2_ranges) ^ ((1540483477 * hash2_ranges) >> 15);
        vec4_t vector_range{ GET_VAR(float, VISUALS_PATH(m_world_blur_near_start)), GET_VAR(float, VISUALS_PATH(m_world_blur_near_end)), GET_VAR(float, VISUALS_PATH(m_world_blur_far_start)), GET_VAR(float, VISUALS_PATH(m_world_blur_far_end)) };
        original(a1, hash3_ranges, &vector_range);
    }

    return original(a1, dof, range);
}