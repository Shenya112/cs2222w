#pragma once

#include "yougey_hook.h"

class c_hooks {
public:
	struct {
		yougey_hook_t m_mouse_input_enabled{ xx( "mouse_input_enabled" ) };
		yougey_hook_t m_is_relative_mouse_mode{ xx( "is_relative_mouse_mode" ) };
		yougey_hook_t m_create_move{ xx( "create_move" ) };
		yougey_hook_t m_get_world_fov{ xx( "get_world_fov" ) };
		yougey_hook_t m_calc_viewmodel{ xx("calc_viewmodel") };
		yougey_hook_t m_level_init{ xx( "level_init" ) };
		yougey_hook_t m_level_shutdown{ xx( "level_shutdown" ) };
		yougey_hook_t m_add_entity{ xx( "add_entity" ) };
		yougey_hook_t m_remove_entity{ xx( "remove_entity" ) };
		yougey_hook_t m_matricies_for_view{ xx( "matricies_for_view" ) };
		yougey_hook_t m_override_view{ xx( "override_view" ) };
		yougey_hook_t m_frame_stage_notify{ xx( "frame_stage_notify" ) };
		yougey_hook_t m_handle_camera_angles{ xx( "handle_camera_angles" ) };
		yougey_hook_t m_run_command{ xx( "run_command" ) };
		yougey_hook_t m_setup_map_info{ xx("setup_map_info") };
		yougey_hook_t m_draw_scope{ xx( "draw_scope" ) };
		yougey_hook_t m_draw_overhead{ xx( "draw_overhead" ) };
		yougey_hook_t m_smoke_volume_drawarray{ xx( "smoke_volume_drawarray" ) };
		yougey_hook_t m_draw_flash_overlay{ xx( "draw_flash_overlay" ) };
		yougey_hook_t m_first_person_legs{ xx( "first_person_legs" ) };
		yougey_hook_t m_handle_glow{ xx("handle_glow") };
		yougey_hook_t m_is_glow{ xx("m_is_glow") };
		yougey_hook_t m_report_hit { xx( "report_hit" ) };
		yougey_hook_t m_setup_move{ xx("setup_move") };
		//yougey_hook_t m_update_global_vars{ xx("update_global_vars") };
	} m_client;

	struct {
		yougey_hook_t m_present{ xx( "present" ) };
		yougey_hook_t m_resize_buffers{ xx( "resize_buffers" ) };
		yougey_hook_t m_create_swap_chain{ xx( "create_swap_chain" ) };
	} m_render_system;

	struct {
		yougey_hook_t m_setup_blur{ xx("setup_blur") };
	} m_engine;

	struct {
		yougey_hook_t m_generate_primitives{ xx( "generate_primitives" ) };
		yougey_hook_t m_draw_aggregate_sceneobject_array{ xx( "draw_aggregate_sceneobject_array" ) };
		yougey_hook_t m_light_scene_object{ xx( "light_scene_object" ) };
		yougey_hook_t m_draw_skybox_array{ xx( "draw_skybox_array" ) };
		yougey_hook_t m_tonemap_debug{ xx( "tonemap_debug" ) };
		yougey_hook_t m_draw_aggregate_sceneobject{ xx( "draw_aggregate_sceneobject" ) };
		yougey_hook_t m_base_draw_array{ xx( "base_draw_array" ) };
	} m_scene_system;

	struct {
		yougey_hook_t m_particles_draw_array{ xx("particles_draw_array") };
	} m_particles;

	struct
	{
		yougey_hook_t m_should_update_sequences{ xx( "should_update_sequences" ) };
	} m_animation_system;

	struct {
		yougey_hook_t m_test_hook_1{ xx( "test_hook_1" ) };
	} m_test;
public:
	void init( void );
};
inline auto g_hooks = std::make_unique<c_hooks>( );
