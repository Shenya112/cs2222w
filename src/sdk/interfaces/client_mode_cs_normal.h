#pragma once

#include <context.h>

class c_view_setup {
public:
    char pad1[(0x450)];
    float flOrthoLeft; // 0x0494
    float flOrthoTop; // 0x0498
    float flOrthoRight; // 0x049C
    float flOrthoBottom; // 0x04A0
    char pad2[(0x38)];
    float m_world_fov; // 0x04D8
    float m_viewmodel_fov; // 0x04DC
    vec3_t m_origin; // 0x04E0
    char pad3[(0xC)]; // 0x04EC
    qangle_t m_view_angle; // 0x04F8
    char pad4[(0x14)]; // 0x0504
    float m_aspect_ratio; // 0x0518
    char pad5[(0x79)];
    unsigned char m_some_flags;
};

struct c_view_render {
	PAD( 8 );
	class c_view_setup m_view_setup;
	PAD( 8 );
};