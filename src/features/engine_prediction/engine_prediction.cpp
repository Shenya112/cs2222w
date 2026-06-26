#include "engine_prediction.h"

#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/global_variables.h>

void c_engine_prediction::calculate_shoot_position() {
    using fn = void(__fastcall*)(vec3_t*, c_cs_player_pawn*, timestamp_t*, void*, void*);
    static auto calculate_shoot_pos = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 44 8B 92 ? ? ? ?")).as<fn>();
    vec3_t result = {};
    result = g_ctx->m_local_pawn->get_shoot_pos();
    timestamp_t timing = { (int)g_ctx->m_local_controller->m_nTickBase(), 0.f };
    calculate_shoot_pos(&result, g_ctx->m_local_pawn, &timing, nullptr, nullptr);
    g_ctx->m_shoot_position = result;
}

void c_engine_prediction::begin() {
    if (g_ctx->m_cmd->m_sequence_number == this->m_last_sequence_processed)
        return;

    c_player_movement_services* movement_services = g_ctx->m_local_pawn->m_pMovementServices();
    if (!movement_services)
        return;

    c_network_game_client* network_game_client = g_interfaces->m_network_client_services->get_network_game_client();
    if (!network_game_client)
        return;

    c_user_cmd_manager* user_cmd_manager = g_ctx->m_local_controller->get_cmd_manager();
    if (!user_cmd_manager)
        return;

    m_prediction_data.m_pre_prediction_flags = g_ctx->m_local_pawn->m_fFlags();
    m_prediction_data.m_abs_velocity = g_ctx->m_local_pawn->m_vecAbsVelocity();
    m_prediction_data.m_velocity = g_ctx->m_local_pawn->m_vecVelocity();
    g_ctx->m_shoot_position = g_ctx->m_local_pawn->get_eye_pos();
    m_prediction_data.m_real_time = g_interfaces->m_global_vars->m_real_time;
    m_prediction_data.m_frame_count = g_interfaces->m_global_vars->m_frame_count;
    m_prediction_data.m_frame_time = g_interfaces->m_global_vars->m_frame_time;
    m_prediction_data.m_frame_time2 = g_interfaces->m_global_vars->m_frame_advance;
    m_prediction_data.m_curtime = g_interfaces->m_global_vars->m_curtime;
    m_prediction_data.m_render_time = g_interfaces->m_global_vars->m_render_time;
    m_prediction_data.m_client_tick_fraction = g_interfaces->m_global_vars->m_client_tick_fraction;
    m_prediction_data.m_next_tick_fraction = g_interfaces->m_global_vars->m_next_tick_fraction;
    m_prediction_data.m_tick_count = g_interfaces->m_global_vars->m_tick_count;

    m_prediction_data.m_in_prediction = g_interfaces->m_prediction->m_in_prediction;
    m_prediction_data.m_first_prediction = g_interfaces->m_prediction->m_first_prediction;
    m_prediction_data.m_has_been_predicted = g_ctx->m_cmd->m_has_been_predicted;
    m_prediction_data.m_should_predict = network_game_client->m_should_predict;

    memcpy(m_prediction_data.m_backup_movement_services, g_ctx->m_local_pawn->m_pMovementServices(), 0x5E0);

    g_ctx->m_cmd->m_has_been_predicted = false;
    network_game_client->m_should_predict = true;
    g_interfaces->m_prediction->m_first_prediction = false;
    g_interfaces->m_prediction->m_in_prediction = true;

    movement_services->set_prediction_command(g_ctx->m_cmd);
    movement_services->run_command(g_ctx->m_cmd);

    //run prediction itself
    {
        //network_game_client->run_prediction( 0 );
        //run_simulation( g_ctx->m_cmd, g_ctx->m_cmd->m_sequence_number );
    }

    m_prediction_data.m_post_prediction_flags = g_ctx->m_local_pawn->m_fFlags();

    movement_services->reset_prediction_command();

    memcpy(g_ctx->m_local_pawn->m_pMovementServices(), m_prediction_data.m_backup_movement_services, 0x5E0);

    g_ctx->m_base->set_client_tick(g_ctx->m_local_controller->m_nTickBase());
}

void c_engine_prediction::end() {

    if (g_ctx->m_cmd->m_sequence_number == this->m_last_sequence_processed)
        return;

    c_player_movement_services* movement_services = g_ctx->m_local_pawn->m_pMovementServices();
    if (!movement_services)
        return;

    c_network_game_client* network_game_client = g_interfaces->m_network_client_services->get_network_game_client();
    if (!network_game_client)
        return;

    g_interfaces->m_global_vars->m_real_time = m_prediction_data.m_real_time;
    g_interfaces->m_global_vars->m_frame_count = m_prediction_data.m_frame_count;
    g_interfaces->m_global_vars->m_frame_time = m_prediction_data.m_frame_time;
    g_interfaces->m_global_vars->m_frame_advance = m_prediction_data.m_frame_time2;
    g_interfaces->m_global_vars->m_curtime = m_prediction_data.m_curtime;
    g_interfaces->m_global_vars->m_render_time = m_prediction_data.m_render_time;
    g_interfaces->m_global_vars->m_client_tick_fraction = m_prediction_data.m_client_tick_fraction;
    g_interfaces->m_global_vars->m_next_tick_fraction = m_prediction_data.m_next_tick_fraction;
    g_interfaces->m_global_vars->m_tick_count = m_prediction_data.m_tick_count;

    g_interfaces->m_prediction->m_in_prediction = m_prediction_data.m_in_prediction;
    g_interfaces->m_prediction->m_first_prediction = m_prediction_data.m_first_prediction;
    g_ctx->m_cmd->m_has_been_predicted = m_prediction_data.m_has_been_predicted;
    network_game_client->m_should_predict = m_prediction_data.m_should_predict;

    this->m_last_sequence_processed = g_ctx->m_cmd->m_sequence_number;
}