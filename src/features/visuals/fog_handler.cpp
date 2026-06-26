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

    const float start_distance = GET_VAR(float, VISUALS_PATH(m_custom_fog_start_distance));
    const float end_distance = GET_VAR(float, VISUALS_PATH(m_custom_fog_end_distance));
    const float falloff = GET_VAR(float, VISUALS_PATH(m_custom_fog_falloff));
    const float brightness = GET_VAR(float, VISUALS_PATH(m_custom_fog_brightness));
    const hellcolor color = GET_VAR(hellcolor, VISUALS_PATH(m_custom_fog_color));

    // the engine only re-evaluates the fog when the entity is (re)created, so any change
    // to the settings has to force a fresh entity - otherwise it only updates on toggle
    const bool changed =
        m_last_start_distance != start_distance ||
        m_last_end_distance != end_distance ||
        m_last_falloff != falloff ||
        m_last_brightness != brightness ||
        m_last_color.Value.x != color.Value.x ||
        m_last_color.Value.y != color.Value.y ||
        m_last_color.Value.z != color.Value.z ||
        m_last_color.Value.w != color.Value.w;

    if (changed)
        remove_fog();

    if (!m_gradient_fog)
        m_gradient_fog = g_interfaces->m_entity_system->create_entity_by_class_name<c_gradient_fog>("env_gradient_fog");

    if (m_gradient_fog)
    {
        auto fog = reinterpret_cast<c_gradient_fog*>(m_gradient_fog);
        fog->m_is_enabled() = true;
        fog->m_fog_start_distance() = start_distance;
        fog->m_fog_end_distance() = end_distance;
        fog->m_fog_strength() = 1.f;
        fog->m_fog_falloff_exponent() = falloff;
        fog->m_fog_max_opacity() = brightness;
        fog->m_fog_color() = c_material_color(color);

        g_memory->call_virtual<void>(m_gradient_fog, 10, 0x1);

        m_last_start_distance = start_distance;
        m_last_end_distance = end_distance;
        m_last_falloff = falloff;
        m_last_brightness = brightness;
        m_last_color = color;
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
