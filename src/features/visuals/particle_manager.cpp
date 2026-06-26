#include "particle_manager.h"

#include <context.h>
#include <math/math.h>
#include <cheat/config/config_system.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/csgo_input.h>
#include <sdk/interfaces/resource_system.h>
#include <sdk/datatypes/c_buffer_string.h>

const char* get_particle_name( int type ) {
	if ( type < 0 || type >= particle_max )
		return nullptr;
		
	switch ( type ) {
	case particle_firework_1:
		return "particles/entity/env_explosion/explosion_hegrenade_h.vpcf";
	case particle_firework_2:
		return "particles/inferno_fx/explosion_incend_air_falling.vpcf";
	case particle_feathers:
		return "particles/critters/chicken/chicken_gone_feathers_fire.vpcf";
	case particle_explosion:
		return "particles/explosions_fx/explosion_basic.vpcf";
	default:
		return nullptr;
	}
}

void c_particle_mgr::create_particle_at_pos( const vec3_t& position, int type ) {
	if ( !g_interfaces->m_global_vars || !g_interfaces->m_game_particle_manager || !g_interfaces->m_particle_system_mgr )
		return;

	const char* particle_name = get_particle_name( type );
	if ( !particle_name )
		return;

	particle_info_t info {};

	if ( !g_interfaces->m_game_particle_manager->create_particle_effect( &info.m_effect_index, particle_name ) )
		return;

	particle_information_t particle_info = {};
	g_interfaces->m_game_particle_manager->set_particle_data( info.m_effect_index, e_particle_setting::particle_setting_info, &particle_info );

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data( info.m_effect_index, e_particle_setting::particle_setting_position, &effect_position );

	info.m_positions = new vec3_t[1];
	info.m_times = new float[1];

	info.m_positions[0] = position;

	info.m_data.m_positions = info.m_positions;
	info.m_data.m_times = info.m_times;

	g_interfaces->m_particle_system_mgr->create_snapshot( &info.m_snapshot_handle );
	if ( !g_interfaces->m_game_particle_manager->init_effect( info.m_effect_index, 0, &info.m_snapshot_handle ) ) {
		delete[] info.m_positions;
		delete[] info.m_times;
		return;
	}

	info.m_snapshot_handle->draw( 1, &info.m_data );

	g_interfaces->m_game_particle_manager->release_particle_index( info.m_effect_index );

	delete[] info.m_positions;
	delete[] info.m_times;

	beam_object_t obj {};
	obj.m_time_added = g_interfaces->m_global_vars->m_curtime;
	obj.m_effect_index = info.m_effect_index;

	m_active_beams.emplace_back( obj );
}

void c_particle_mgr::create_particle_beam( const vec3_t& start, const vec3_t& end, hellcolor color, float time, float width ) {
	if ( !g_interfaces->m_global_vars )
		return;

	particle_info_t tracer_info{};
	particle_color_t beam_color = { color.Value.x * 255.f, color.Value.y * 255.f, color.Value.z * 255.f };

	g_interfaces->m_game_particle_manager->create_particle_effect( &tracer_info.m_effect_index, xx( "particles/entity/spectator_utility_trail.vpcf" ) );

	g_interfaces->m_game_particle_manager->set_particle_data( tracer_info.m_effect_index, e_particle_setting::particle_setting_color, &beam_color );

	particle_information_t beam_info{};
	beam_info.m_time = time;
	beam_info.m_width = width;
	beam_info.m_alpha = color.Value.w;
	g_interfaces->m_game_particle_manager->set_particle_data( tracer_info.m_effect_index, e_particle_setting::particle_setting_info, &beam_info );

	constexpr int total_points = 5;
	std::vector<vec3_t> positions;
	positions.reserve( total_points );

	vec3_t direction = end - start;
	vec3_t middle = start + direction * 0.5f;

	positions.push_back( start );                                 /* start*/
	positions.push_back( start.lerp( middle, 0.05f ) );             /* near start*/
	positions.push_back( middle );                                /* middle */
	positions.push_back( middle.lerp( end, 0.95f ) );               /* near end*/
	positions.push_back( end );                                   /*  end*/

	tracer_info.m_data.m_positions = positions.data( );

	g_interfaces->m_particle_system_mgr->create_snapshot( &tracer_info.m_snapshot_handle );
	g_interfaces->m_game_particle_manager->init_effect( tracer_info.m_effect_index, 0, &tracer_info.m_snapshot_handle );

	tracer_info.m_snapshot_handle->draw( total_points, &tracer_info.m_data );

	beam_object_t obj{};
	obj.m_time_added = g_interfaces->m_global_vars->m_curtime;
	obj.m_duration = time;
	obj.m_effect_index = tracer_info.m_effect_index;
	m_active_beams.emplace_back( obj );
}

void c_particle_mgr::update() {
	if (!g_interfaces->m_global_vars)
		return;

	auto it = m_active_beams.begin();
	while (it != m_active_beams.end()) {
		auto& beam = *it;
		float current_time = g_interfaces->m_global_vars->m_curtime;
		float delta = fabsf(current_time - beam.m_time_added);

		if (delta > beam.m_duration) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
			it = m_active_beams.erase(it);
		}
		else {
			++it;
		}
	}
}

void c_particle_mgr::run_world_weather(hellcolor color) {
	particle_color_t ash_color = { color.Value.x, color.Value.y, color.Value.z };

	auto remove_effects = [this]() {
		for (unsigned int& i : m_vec_effect_indexes) {
			if (i != 0) {
				g_interfaces->m_game_particle_manager->destroy_particle(i, true, true);
				g_interfaces->m_game_particle_manager->release_particle_index(i);
			}
		}

		if (!m_vec_effect_indexes.empty())
			m_vec_effect_indexes.clear();
		};

	auto create_effects = [this]() {
		static const char* m_sz_particle_path_rain = xx("bin/rain_edge_sparse.vpcf");
		static const char* m_sz_particle_path_ash = xx("particles/rain_fx/econ_weather_ash.vpcf");

		unsigned int u_effect_index[4] = { 0 };

		if (GET_VAR(int, VISUALS_PATH(m_weather_index)) == 0) {
			for (int i = 0; i < 4; i++) {
				unsigned int effect_index = 0;
				if (g_interfaces->m_game_particle_manager->create_particle_effect(&effect_index, m_sz_particle_path_rain) != nullptr) {
					u_effect_index[i] = effect_index;
				}
			}
		}

		else if (GET_VAR(int, VISUALS_PATH(m_weather_index)) == 1) {
			for (int i = 0; i < 4; i++) {
				unsigned int effect_index = 0;
				if (g_interfaces->m_game_particle_manager->create_particle_effect(&effect_index, m_sz_particle_path_ash) != nullptr) {
					u_effect_index[i] = effect_index;
				}
			}
		}

		for (int i = 0; i < 4; i++) {
			if (u_effect_index[i] != 0)
				m_vec_effect_indexes.push_back(u_effect_index[i]);
		}
		};

	if (!g_interfaces->m_engine->in_game() || !g_interfaces->m_game_particle_manager) {
		if (!m_vec_effect_indexes.empty()) {
			remove_effects();
			m_b_new_round_callback = true;
		}
		return;
	}

	static bool m_b_last_world_particles = false;
	static int m_i_last_particles_type = -1;

	bool m_b_particles_changed = (m_b_last_world_particles != GET_VAR(bool, VISUALS_PATH(m_enable_world_weather)));
	bool m_b_type_changed = (m_i_last_particles_type != GET_VAR(int, VISUALS_PATH(m_weather_index)));

	m_b_last_world_particles = GET_VAR(bool, VISUALS_PATH(m_enable_world_weather));
	m_i_last_particles_type = GET_VAR(int, VISUALS_PATH(m_weather_index));

	if (!GET_VAR(bool, VISUALS_PATH(m_enable_world_weather))) {
		if (!m_vec_effect_indexes.empty()) {
			remove_effects();
			m_b_new_round_callback = true;
		}
		return;
	}

	if (m_b_new_round_callback || m_b_particles_changed || m_b_type_changed) {
		if (!m_vec_effect_indexes.empty())
			remove_effects();

		create_effects();
		m_b_new_round_callback = false;
	}

	auto local_pawn = g_ctx->m_local_pawn;
	auto scene_node = local_pawn ? local_pawn->m_pGameSceneNode() : nullptr;

	if (!m_vec_effect_indexes.empty() && local_pawn && scene_node) {
		vec3_t m_vec_abs = scene_node->m_vecAbsOrigin();

		if (!m_vec_abs.x || !m_vec_abs.y || !m_vec_abs.z)
			return;

		qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
		vec3_t vec_forward, vec_right, vec_up;
		g_math->angle_vectors(view_angles, &vec_forward, &vec_right, &vec_up);

		vec_forward.normalize();
		vec_right.normalize();

		float m_fl_distance = 750.f;

		if (GET_VAR(int, VISUALS_PATH(m_weather_index)) == 1)
			m_fl_distance = 450.f;

		vec3_t m_vec_positions[4] = {
			m_vec_abs + vec_forward * m_fl_distance,
			m_vec_abs - vec_forward * m_fl_distance,
			m_vec_abs + vec_right * m_fl_distance,
			m_vec_abs - vec_right * m_fl_distance
		};

		vec3_t den = vec3_t(500.f, 0.f, 0.f);

		int m_effects_per_direction = 1;
		int m_i_total_directions = 4;

		int m_i_effect_index = 0;

		for (int m_i_dir = 0; m_i_dir < m_i_total_directions; m_i_dir++) {
			for (int i = 0; i < m_effects_per_direction; i++) {
				if (m_i_effect_index < m_vec_effect_indexes.size()) {
					unsigned int effect_idx = m_vec_effect_indexes[m_i_effect_index];

					if (effect_idx != 0) {
						if (GET_VAR(int, VISUALS_PATH(m_weather_index)) == 1)
						{
							g_interfaces->m_game_particle_manager->set_particle_data(effect_idx, 16U, &m_vec_positions[m_i_dir]);
							g_interfaces->m_game_particle_manager->set_particle_data(effect_idx, 1U, &ash_color);
						}
						else
							g_interfaces->m_game_particle_manager->set_particle_data(effect_idx, e_particle_setting::particle_setting_position, &m_vec_positions[m_i_dir]);

						g_interfaces->m_game_particle_manager->set_particle_data(effect_idx, e_particle_setting::particle_setting_density, &den);
					}

					m_i_effect_index++;
				}
			}
		}
	}
}

void c_particle_mgr::release_world_weather()
{
	auto remove_effects = [this]() {
		for (unsigned int& i : m_vec_effect_indexes) {
			if (i != 0) {
				g_interfaces->m_game_particle_manager->destroy_particle(i, true, true);
				g_interfaces->m_game_particle_manager->release_particle_index(i);
			}
		}

		if (!m_vec_effect_indexes.empty())
			m_vec_effect_indexes.clear();
		};

	if (!m_vec_effect_indexes.empty()) {
		remove_effects();
	}
	return;
}

void c_particle_mgr::play_particle_effect(c_cs_player_pawn* pawn)
{
	if (!GET_VAR(bool, VISUALS_PATH(m_enabled_kill_effects)))
		return;
	using fn_play_particle_effect = void(__fastcall*)(const char*, int, c_cs_player_pawn*, char, int, char, unsigned int, int, char);
	static auto play_particle_effect = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B D9")).as<fn_play_particle_effect>();
	if (!play_particle_effect)
		return;

	int kill_effect_type = GET_VAR(int, VISUALS_PATH(m_kill_effects_type));

	if (kill_effect_type == particle_glow_stars) {
		c_buffer_string glow_stars("bin/glow_stars.vpcf", 'fcpv');
		g_interfaces->m_resource_system->blocking_load_resource_by_name(&glow_stars, "");
		play_particle_effect(("bin/glow_stars.vpcf"), 3, pawn, 0, 0, 0, 0xFFFFFFFF, 0, 0);
	}

	if (kill_effect_type == particle_sakura2) {
		c_buffer_string sakura1("bin/sakura_red.vpcf", 'fcpv');
		g_interfaces->m_resource_system->blocking_load_resource_by_name(&sakura1, "");
		play_particle_effect(("bin/sakura_red.vpcf"), 3, pawn, 0, 0, 0, 0xFFFFFFFF, 0, 0);
	}

	if (kill_effect_type == particle_sakura3) {
		c_buffer_string sakura1("bin/sakura_purple.vpcf", 'fcpv');
		g_interfaces->m_resource_system->blocking_load_resource_by_name(&sakura1, "");
		play_particle_effect(("bin/sakura_purple.vpcf"), 3, pawn, 0, 0, 0, 0xFFFFFFFF, 0, 0);
	}
}

void c_particle_mgr::clear_all_particles() {
	if (!g_interfaces->m_game_particle_manager)
		return;

	std::vector<beam_object_t> beams_to_clear;
	beams_to_clear.swap(m_active_beams);

	for (auto& beam : beams_to_clear) {
		if (beam.m_effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
		}
	}

	std::vector<beam_object_t> circle_beams_to_clear;
	circle_beams_to_clear.swap(m_circle_beams);

	for (auto& beam : circle_beams_to_clear) {
		if (beam.m_effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(beam.m_effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(beam.m_effect_index);
		}
	}

	std::vector<unsigned int> effects_to_clear;
	effects_to_clear.swap(m_vec_effect_indexes);

	for (unsigned int& effect_index : effects_to_clear) {
		if (effect_index != 0) {
			g_interfaces->m_game_particle_manager->destroy_particle(effect_index, true, true);
			g_interfaces->m_game_particle_manager->release_particle_index(effect_index);
		}
	}

	clean_autopeek_effect();
}

void c_particle_mgr::create_shot_sparks(const vec3_t& position, hellcolor color) {
	if (!g_interfaces->m_game_particle_manager)
		return;

	c_buffer_string shot_sparks("bin/shot_sparks.vpcf", 'fcpv');
	g_interfaces->m_resource_system->blocking_load_resource_by_name(&shot_sparks, "");
	unsigned int effect_index = 0;

	particle_info_t info{};
	if (!g_interfaces->m_game_particle_manager->create_particle_effect(&effect_index, "bin/shot_sparks.vpcf"))
		return;
	particle_color_t spark_color = { color.Value.x, color.Value.y, color.Value.z };
	g_interfaces->m_game_particle_manager->set_particle_data(effect_index, e_particle_setting::particle_setting_color, &spark_color);

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data(effect_index, e_particle_setting::particle_setting_position, &effect_position);
}

void c_particle_mgr::create_autopeek_effect(const vec3_t& position, hellcolor color) {
	if (!g_interfaces->m_game_particle_manager)
		return;

	if (m_autopeek_effect_index != 0)
		return;

	c_buffer_string autopeek_effect("bin/autopeek.vpcf", 'fcpv');
	g_interfaces->m_resource_system->blocking_load_resource_by_name(&autopeek_effect, "");

	particle_info_t info{};
	if (!g_interfaces->m_game_particle_manager->create_particle_effect(&m_autopeek_effect_index, "bin/autopeek.vpcf"))
		return;
	particle_color_t effect_color = { color.Value.x, color.Value.y, color.Value.z };
	g_interfaces->m_game_particle_manager->set_particle_data(m_autopeek_effect_index, 2U, &effect_color);

	vec3_t effect_position = position;
	g_interfaces->m_game_particle_manager->set_particle_data(m_autopeek_effect_index, e_particle_setting::particle_setting_position, &effect_position);
}

void c_particle_mgr::clean_autopeek_effect() {
	if (!g_interfaces->m_game_particle_manager || m_autopeek_effect_index == 0)
		return;

	g_interfaces->m_game_particle_manager->destroy_particle(m_autopeek_effect_index, true, true);
	g_interfaces->m_game_particle_manager->release_particle_index(m_autopeek_effect_index);
	m_autopeek_effect_index = 0;
}
