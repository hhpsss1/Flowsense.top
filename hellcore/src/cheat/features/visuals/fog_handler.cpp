#include "fog_handler.h"
#include <cheat/config/vars.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/resource_system.h>

void c_fog_handler::fog_controller()
{
    if (!GET_VAR(bool, VISUALS_PATH(m_enable_custom_fog)) || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
    {
        remove_fog();
        return;
    }

    if (!m_gradient_fog)
        m_gradient_fog = g_interfaces->m_entity_system->create_entity_by_class_name<c_gradient_fog>("env_gradient_fog");

    if (m_gradient_fog)
    {
        auto fog = reinterpret_cast<c_gradient_fog*>(m_gradient_fog);
        fog->m_is_enabled() = true;
        fog->m_fog_start_distance() = GET_VAR(float, VISUALS_PATH(m_custom_fog_start_distance));
        fog->m_fog_end_distance() = GET_VAR(float, VISUALS_PATH(m_custom_fog_end_distance));
        fog->m_fog_strength() = 1.f;
        fog->m_fog_falloff_exponent() = GET_VAR(float, VISUALS_PATH(m_custom_fog_falloff));
        fog->m_fog_max_opacity() = GET_VAR(float, VISUALS_PATH(m_custom_fog_brightness));
        fog->m_fog_color() = c_material_color(GET_VAR(hellcolor, VISUALS_PATH(m_custom_fog_color)));

        g_memory->call_virtual<void>(m_gradient_fog, 10, 0x1);
    }
}

void c_fog_handler::remove_fog()
{
    if (m_gradient_fog)
    {
        m_gradient_fog->remove();
        m_gradient_fog = nullptr;
    }
}
