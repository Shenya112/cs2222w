#pragma once

#include <sdk/entity/entity.h>

class c_fog_handler {
public:
	void fog_controller();
	void remove_fog();

private:
	c_base_entity* m_gradient_fog = nullptr;

	// last applied settings, used to detect changes and force a live refresh
	float m_last_start_distance = 0.f;
	float m_last_end_distance = 0.f;
	float m_last_falloff = 0.f;
	float m_last_brightness = 0.f;
	hellcolor m_last_color = {};
};

inline auto g_fog_handler = std::make_unique<c_fog_handler>();
