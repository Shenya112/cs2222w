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
        g_memory->call_virtual<void>( this, 1, count, data );
    }
};

class c_particle_system_mgr {
public:
    void create_snapshot( c_strong_handle<c_particle_snapshot>* snapshot ) {
        int64_t unknown = 0;
        g_memory->call_virtual<void>( this, 41, snapshot, &unknown );
    }
};

class c_create_particle {
public:
    void create_particle( c_strong_handle<c_particle_snapshot>* snapshot, uint64_t unknown ) {
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
    c_strong_handle<c_particle_snapshot> m_snapshot_handle;
    particle_data_t m_data;
};

class c_game_particle_manager {
public:
    bool init_effect(unsigned int effect_index, unsigned int unknown, c_strong_handle<c_particle_snapshot>* particle_snapshot) {
        using fn_init_effect = bool(__fastcall*)(c_game_particle_manager*, unsigned int, unsigned int, c_strong_handle<c_particle_snapshot>*);
        static fn_init_effect init_effect = g_modules->m_client.find(xx("48 89 74 24 10 57 48 83 EC 30 4C 8B D9 49 8B F9 33 C9 41 8B F0 83 FA FF 0F")).as<fn_init_effect>();
        return init_effect(this, effect_index, unknown, particle_snapshot);
    }

    int* create_particle_effect(unsigned int* effect_index, const char* name) {
        using fn_create_particle_effect = int* (__fastcall*)(c_game_particle_manager*, unsigned int*, const char*, int, __int64, __int64, __int64, int);
        static fn_create_particle_effect create_particle_effect = g_modules->m_client.find(xx("E8 ? ? ? ? 8B 08 89 8B ? ? ? ? E8 ? ? ? ? 40 80 FF")).relative(1, 5).as<fn_create_particle_effect>();
        return create_particle_effect(this, effect_index, name, 2, 0ll, 0ll, 0ll, 0);
    }

    bool set_particle_data(unsigned int effect_index, int unknown, void* data) {
        using fn_set_particle_data = bool(__fastcall*)(c_game_particle_manager*, unsigned int, int, void*, int);
        static fn_set_particle_data set_particle_data = g_modules->m_client.find(xx("E8 ? ? ? ? E8 ? ? ? ? 48 8B F8 45 84 FF")).relative(1, 5).as<fn_set_particle_data>();
        return set_particle_data(this, effect_index, unknown, data, 0);
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
