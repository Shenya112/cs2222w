#include "skybox_changer.h"
#include <sdk/interfaces/resource_system.h>
#include <core/interfaces/interfaces.h>
#include "chams.h"
#include <sdk/datatypes/key_values.h>
#include <sdk/datatypes/c_buffer_string.h>

inline const char* skybox_template = R"(
        <!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
        {
            Shader = "sky.vfx"
            g_flBrightnessExposureBias = 0.000000
            g_flRenderOnlyExposureBias = 0.000000
            SkyTexture = resource:"%s"
            g_tSkyTexture = resource:"%s"
        }
)";

auto create_material = []( const char* material_name, const char vmat[] ) -> c_strong_handle<c_material_2> {
    if ( !material_name || !vmat )
        return {};

    c_key_values_3* keyval = c_key_values_3::create_material_resource( );
    if ( !keyval )
        return {};

    keyval->load_from_buffer( vmat );

    c_strong_handle<c_material_2> custom_material{};
    using fn_create_material = __int64( __fastcall* )(void*, void*, const char*, c_key_values_3*, unsigned int, unsigned int);
    static fn_create_material create_material = g_modules->m_materialsystem2.find( xx( "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05" ) ).as<fn_create_material>( );

    if ( !create_material )
        return {};

    create_material( nullptr, &custom_material, material_name, keyval, 0, 1 );

    return custom_material;
};

void c_skybox_changer::refresh_custom_skyboxes( ) {
    m_custom_skyboxes.clear( );

    std::filesystem::path base_path = g_ctx->m_running_path;
    base_path = base_path.parent_path( ).parent_path( ).parent_path( ) / "csgo" / "materials" / "skybox";

    for ( const auto& entry : std::filesystem::directory_iterator( base_path ) ) {
        if ( entry.is_regular_file( ) ) {
            auto path = entry.path( );
            if ( path.extension( ) == ".vtex_c" ) {
                m_custom_skyboxes.push_back( path.stem( ).string( ) );
            }
        }
    }
}

std::vector<std::string> c_skybox_changer::get_all_skyboxes( ) {
    std::vector<std::string> result;

    for ( int i = 0; i < 14; ++i )
        result.emplace_back( arr_skybox_names[i] );

    result.insert( result.end( ), m_custom_skyboxes.begin( ), m_custom_skyboxes.end( ) );
    return result;
}

void c_skybox_changer::run( ) {
    
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_enable_custom_sky)))
        return;

    int current_idx = GET_VAR( int, VISUALS_PATH( m_selected_skybox ) );

    if ( m_last_selected_skybox == current_idx && !m_need_update_material )
        return;
    
    m_last_selected_skybox = current_idx;
    m_need_update_material = false;

    if ( current_idx == 0 ) {
        m_has_custom_skybox = false;
        return;
    }

    const bool is_custom = current_idx >= 14;
    std::string selected_vtex;

    if ( is_custom ) {
        const int custom_idx = current_idx - 14;
        if ( custom_idx < 0 || custom_idx >= m_custom_skyboxes.size( ) )
            return;

        selected_vtex = "materials/skybox/" + m_custom_skyboxes[custom_idx] + ".vtex";
    }
    else {
        selected_vtex = std::string( arr_skybox_paths[current_idx - 1] );
    }

    c_buffer_string buffer_vtex( selected_vtex.c_str( ), 'xetv' );
    if ( g_interfaces->m_resource_system )
        g_interfaces->m_resource_system->blocking_load_resource_by_name( &buffer_vtex, "" );

    char formatted_material[2048];
    sprintf_s( formatted_material, skybox_template, selected_vtex.c_str( ), selected_vtex.c_str( ) );

    if ( !g_interfaces->m_resource_system )
        return;

    m_custom_skybox_material = create_material( "our_own_skybox", formatted_material );
    m_has_custom_skybox = true;
}

void c_skybox_changer::init_on_level_load( ) {
    if ( !g_interfaces->m_resource_system )
        return;

    m_has_custom_skybox = false;
    m_default_skybox_material = nullptr;

    material_array_t mat_arr = {};
    g_interfaces->m_resource_system->enumerate_materials( 0x74616D76, &mat_arr, 2 );

    if ( !mat_arr.m_material_resource )
        return;

    for ( int i = 0; i < mat_arr.m_count; ++i ) {
        if ( !mat_arr.m_material_resource[i] )
            continue;

        auto material_resource = mat_arr.m_material_resource[i];
        if ( !material_resource )
            continue;

        auto material = *material_resource;
        if ( !material )
            continue;

        std::string material_name = material->get_name( );

        if ( material_name.find( "materials/skybox" ) != std::string::npos && material_name.find( "our_own_skybox" ) == std::string::npos ) {
            m_default_skybox_material = material;
            break;
        }
    }

    for ( int i = 0; i < 13; ++i ) {
        c_buffer_string buffer_vtex( arr_skybox_paths[i].data( ), 'xetv' );
        g_interfaces->m_resource_system->blocking_load_resource_by_name( &buffer_vtex, "" );
    }
}

void c_skybox_changer::restore_default_skybox( ) {
    if ( !m_default_skybox_material || !g_interfaces->m_resource_system )
        return;

    material_array_t mat_arr = {};
    g_interfaces->m_resource_system->enumerate_materials( 0x74616D76, &mat_arr, 2 );

    if ( !mat_arr.m_material_resource )
        return;

    for ( int i = 0; i < mat_arr.m_count; ++i ) {
        if ( !mat_arr.m_material_resource[i] )
            continue;

        auto material_resource = mat_arr.m_material_resource[i];
        if ( !material_resource )
            continue;

        auto material = *material_resource;
        if ( !material )
            continue;

        std::string material_name = material->get_name( );

        if ( material_name.find( "materials/skybox" ) != std::string::npos || material_name.find( "our_own_skybox" ) != std::string::npos ) {
            *material_resource = m_default_skybox_material;
        }
    }
    m_has_custom_skybox = false;
}

void c_skybox_changer::cleanup( ) {
    m_default_skybox_material = nullptr;
    m_custom_skybox_material = nullptr;
    m_has_custom_skybox = false;
}
