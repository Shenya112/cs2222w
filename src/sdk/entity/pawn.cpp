#include "pawn.h"
#include <utils/memory.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/global_variables.h>
#include <sdk/constants.h>
#include <context.h>
#include <cheat/features/engine_prediction/engine_prediction.h>

constexpr int COLLISION_GROUP_PLAYER_MOVEMENT = 8;

vec3_t c_player_weapon_services::weapon_shoot_position() {
	vec3_t ret = {};
	g_memory->call_virtual<void>(this, 29, &ret);
	return ret;
}

c_contents* c_cs_player_pawn::get_contents() {
	return g_memory->call_virtual<c_contents*>(this, 63);
}

vec3_t c_cs_player_pawn::get_eye_pos()
{
	vec3_t ret = {};
	g_memory->call_virtual<void>(this, 168, &ret);
	return ret;
}

vec3_t c_cs_player_pawn::get_shoot_pos() {
	return m_pGameSceneNode()->m_vecAbsOrigin() + m_vecViewOffset();
}

bool c_cs_player_pawn::is_alive() {
	return m_iHealth() > 0 && m_lifeState() == LIFE_ALIVE;
}

bool c_cs_player_pawn::is_enemy() {
	c_cs_player_pawn* local_pawn = g_ctx->m_local_pawn;

	if (!local_pawn)
		return true;

	if (this == local_pawn)
		return false;

	return this->m_iTeamNum() != local_pawn->m_iTeamNum();
}

c_weapon_base* c_cs_player_pawn::get_active_weapon()
{
	const auto player_weapon_services = m_pWeaponServices();
	if (!player_weapon_services)
		return nullptr;

	auto entity = player_weapon_services->m_hActiveWeapon().get<c_weapon_base >();
	if (!entity || !entity->is_weapon())
		return nullptr;

	return entity;
}

uint32_t c_cs_player_pawn::get_collision_owner_index()
{
	const auto player_collision = m_pCollision();
	if (player_collision && !(player_collision->m_usSolidFlags() & 4))
		return m_hOwnerEntity().get_entry_index();

	return -1;
}
bool c_cs_player_pawn::has_armor(int hitgroup)
{
	if (hitgroup == HITGROUP_HEAD)
		return m_pItemServices()->m_bHasHelmet();

	return m_ArmorValue();
}

bool c_cs_player_pawn::IsArmored(int hitgroup) {
	if (hitgroup == HITGROUP_HEAD) {
		if (!m_pItemServices())
			return false;

		return m_pItemServices()->m_bHasHelmet();
	}
	else
		return m_ArmorValue() > 0;
}

void c_cs_player_pawn::set_body_group(int first, int second)
{
	using fn_set_body_group = void(__fastcall*)(void*, int, unsigned int);
	static fn_set_body_group set_body_group_ = g_modules->m_client.find(xx("85 D2 0F 88 ? ? ? ? 53 55")).as<fn_set_body_group>();
	set_body_group_(this, first, second);
}

void c_cs_player_pawn::run_pre_think()
{
	g_memory->call_virtual<void>(this, 322);
}

void c_cs_player_pawn::physics_run_think()
{
	using fn_physics_run_think = void(__fastcall*)(void*);
	static fn_physics_run_think fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B 81 ? ? ? ? 48 8B F9")).as<fn_physics_run_think>();
	fn(this);
}

void c_cs_player_pawn::run_post_think()
{
	g_memory->call_virtual<void>(this, 333);
}

void c_player_movement_services::set_prediction_command(c_user_cmd* cmd)
{
	g_memory->call_virtual<void>(this, 42, cmd);
}

bool c_player_movement_services::check_local_server()
{
	return g_memory->call_virtual<bool>(this, 38);
}

bool c_player_movement_services::should_use_viewangles_for_subtick(c_user_cmd* cmd, c_move_data* data)
{
	return g_memory->call_virtual<bool>(this, 33, cmd, data);
}

void c_player_movement_services::trace_player_bbox(c_move_data* move_data, vec3_t* progression, c_game_trace* trace)
{
	using fn_trace_player_bbox = void(__fastcall*)(void*, c_move_data*, vec3_t*, c_game_trace*);
	static fn_trace_player_bbox fn = g_modules->m_client.find(xx("48 8B C4 4C 89 48 ? 4C 89 40 ? 48 89 50 ? 48 89 48 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 45 33 E4")).as<fn_trace_player_bbox>();
	fn(this, move_data, progression, trace);
}

void c_player_movement_services::get_player_collision_bound(bbox_collision_t* bounds)
{
	using fn_get_player_collision_bound = void* (__fastcall*)();
	static fn_get_player_collision_bound fn = g_modules->m_client.find(xx("48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 48 FF A0 ? ? ? ? 48 8D 05")).as<fn_get_player_collision_bound>();

	char* bbox = { };

	if (this->m_bDucked())
	{
		bbox = ((char*)fn() + 36);
	}
	else
	{
		bbox = (char*)fn() + 12;
	}

	bounds = (bbox_collision_t*)bbox;
}

void c_player_movement_services::resolve_step_up_trace(c_move_data* move_data)
{
	static auto sv_stepsize = g_interfaces->m_engine_convar->find_by_name("sv_stepsize");
	static auto sv_walkable_normal = g_interfaces->m_engine_convar->find_by_name("sv_walkable_normal");

	if (!sv_stepsize)
		return;

	c_trace_filter trace_filter;
	g_interfaces->m_phys2world->init_player_movement_trace_filter(&trace_filter, g_ctx->m_local_pawn, g_ctx->m_local_pawn->m_pCollision()->m_collisionAttribute().m_interacts_with, 8);

	bbox_collision_t collision;
	get_player_collision_bound(&collision);

	vec3_t adjust_abs_origin = { move_data->abs_origin.x, move_data->abs_origin.y, move_data->abs_origin.z - sv_stepsize->get_float() };

	c_game_trace trace_up;
	g_interfaces->m_phys2world->init_game_trace(&trace_up);
	this->m_nTraceCount()++;
	g_interfaces->m_phys2world->trace_with_spatial(&move_data->abs_origin, &move_data->abs_origin, &collision.m_min, &trace_filter, &trace_up);

	float end_x = trace_up.m_end_pos.x;
	float end_y = trace_up.m_end_pos.y;
	float end_z = trace_up.m_end_pos.z;

	this->m_nTraceCount()++;

	vec3_t second_trace_start = { end_x, end_y, end_z };
	g_interfaces->m_phys2world->trace_with_spatial(&move_data->abs_origin, &adjust_abs_origin, &collision.m_max, &trace_filter, &trace_up);

	if (trace_up.m_fraction > 0.0f && trace_up.m_fraction < 1.0f && !trace_up.m_start_in_solid)
	{
		if (trace_up.m_hit_normal.z >= sv_walkable_normal->get_float() &&
			fabsf(move_data->abs_origin.z - trace_up.m_end_pos.z) > 0.015625f)
		{
			move_data->abs_origin.x = trace_up.m_end_pos.x;
			move_data->abs_origin.z = trace_up.m_end_pos.z;
		}
	}
}

bool c_player_movement_services::step_move(c_move_data* move_data, vec3_t& in_position, vec3_t& in_velocity)
{
	static auto sv_stepsize = g_interfaces->m_engine_convar->find_by_name("sv_stepsize");
	static auto sv_walkable_normal = g_interfaces->m_engine_convar->find_by_name("sv_walkable_normal");

	if (!sv_stepsize)
		return false;

	vec3_t original_pos = move_data->abs_origin;
	vec3_t original_vel = move_data->velocity;

	move_data->abs_origin.x = in_position.x;
	move_data->abs_origin.z = in_position.z;
	move_data->velocity.x = in_velocity.x;
	move_data->velocity.z = in_velocity.z;

	float step_height = sv_stepsize->get_float() + 0.03125f;
	vec3_t step_up_pos = move_data->abs_origin;
	step_up_pos.z += step_height;

	c_trace_filter trace_filter;
	g_interfaces->m_phys2world->init_player_movement_trace_filter(&trace_filter, g_ctx->m_local_pawn, g_ctx->m_local_pawn->m_pCollision()->m_collisionAttribute().m_interacts_with, 8);

	bbox_collision_t collision;
	get_player_collision_bound(&collision);

	c_game_trace trace_up;
	g_interfaces->m_phys2world->init_game_trace(&trace_up);
	this->m_nTraceCount()++;
	g_interfaces->m_phys2world->trace_with_spatial(&move_data->abs_origin, &step_up_pos, &collision.m_min, &trace_filter, &trace_up);

	if (trace_up.m_fraction > 0.0f)
		move_data->abs_origin = trace_up.m_end_pos;

	vec3_t start_pos = trace_up.m_end_pos;

	c_game_trace trace_down;
	g_interfaces->m_phys2world->init_game_trace(&trace_down);
	this->m_nTraceCount()++;
	g_interfaces->m_phys2world->trace_with_spatial(&start_pos, &move_data->abs_origin, &collision.m_max, &trace_filter, &trace_down);

	if ((trace_down.m_fraction < 1.0f || trace_down.m_start_in_solid) && trace_down.m_hit_normal.z >= sv_walkable_normal->get_float() && trace_down.m_fraction > 0.0f)
	{
		float old_dx = original_pos.x - in_position.x;
		float old_dy = original_pos.y - in_position.y;
		float new_dx = trace_down.m_end_pos.x - in_position.x;
		float new_dy = trace_down.m_end_pos.y - in_position.y;

		float old_dist_sq = old_dx * old_dx + old_dy * old_dy;
		float new_dist_sq = new_dx * new_dx + new_dy * new_dy;

		if (old_dist_sq <= new_dist_sq)
		{
			move_data->velocity.z = original_vel.z;
			return true;
		}
		else
		{
			move_data->abs_origin = original_pos;
			move_data->velocity = original_vel;
			return true;
		}
	}
	else
	{
		move_data->abs_origin = original_pos;
		move_data->velocity = original_vel;
		return false;
	}
}

void c_player_movement_services::accelerate(c_move_data* move_data, float frame_time, vec3_t& wish_dir, float wish_speed, float acceleration)
{
	static auto sv_accelerate_use_weapon_speed = g_interfaces->m_engine_convar->find_by_name("sv_accelerate_use_weapon_speed");
	if (!sv_accelerate_use_weapon_speed)
		return;

	float stored_accel = acceleration;

	auto pawn = m_pawn();
	float current_speed = move_data->velocity.dot(wish_dir);
	float speed_delta = wish_speed - current_speed;

	if (speed_delta <= 0.0f)
		return;

	if (current_speed < 0)
		current_speed = 0;

	const auto is_ducking = m_button_state().m_value & IN_DUCK || m_bDucking() || pawn->m_fFlags() & FL_DUCKING;
	const auto is_walking = (m_button_state().m_value & IN_SPEED) != 0 && !is_ducking;

	auto acceleration_scale = std::max(250.f, wish_speed);
	auto goal_speed = acceleration_scale;

	auto is_slow_sniper_scoped = false;

	auto wpn = g_ctx->m_active_weapon;
	if (sv_accelerate_use_weapon_speed->get_bool() && wpn)
	{
		is_slow_sniper_scoped = (wpn->m_zoomLevel() > 0 && wpn->weapon_data()->m_nZoomLevels() > 1 && (wpn->weapon_data()->m_flMaxSpeed() * 0.52f) < 110.0);

		goal_speed *= std::min(1.0f, (wpn->weapon_data()->m_flMaxSpeed() / 250.0f));

		if ((!is_ducking && !is_walking) || ((is_walking || is_ducking) && is_slow_sniper_scoped))
			acceleration_scale *= std::min(1.0f, (wpn->weapon_data()->m_flMaxSpeed() / 250.0f));
	}

	if (is_ducking)
	{
		if (!is_slow_sniper_scoped)
			acceleration_scale *= 0.34f;

		goal_speed *= 0.34f;
	}

	if (is_walking)
	{
		if (!is_slow_sniper_scoped)
			acceleration_scale *= 0.52f;

		goal_speed *= 0.52f;
	}

	if (is_walking && current_speed > (goal_speed - 5))
		stored_accel *= std::clamp(1.0f - (std::max(0.0f, current_speed - (goal_speed - 5)) / std::max(0.0f, goal_speed - (goal_speed - 5))), 0.0f, 1.0f);

	float accel_amount = std::fmax(0.0f, std::fmin((acceleration * frame_time * goal_speed * this->m_flSurfaceFriction()), speed_delta));

	move_data->velocity += wish_dir * accel_amount;
}

//call that before walk move
void c_player_movement_services::friction(c_move_data* move_data)
{
	static auto sv_friction = g_interfaces->m_engine_convar->find_by_name("sv_friction");
	static auto sv_stopspeed = g_interfaces->m_engine_convar->find_by_name("sv_stopspeed");
	if (!sv_stopspeed || !sv_friction)
		return;

	if (g_ctx->m_local_pawn->m_pWaterServices())
		return;

	float length_2d = move_data->velocity.length_2d();
	float friction_factor = 0.f;

	if (this->m_flOffsetTickStashedSpeed() >= 0.1f)
	{
		float surface_friction = this->m_flSurfaceFriction() * sv_friction->get_float();
		float friction_scale = surface_friction * g_ctx->m_local_pawn->m_flFriction();

		friction_factor = (fmaxf(this->m_flOffsetTickStashedSpeed(), sv_stopspeed->get_float()) * friction_scale) * g_interfaces->m_global_vars->m_frame_time;
	}

	float new_speed = fmaxf(this->m_flOffsetTickStashedSpeed() - friction_factor, 0.0f);
	float speed_ratio = new_speed / this->m_flOffsetTickStashedSpeed();

	//move_data->jump = fmaxf(-(this->m_flOffsetTickStashedSpeed() - friction_factor), 0.0f);

	move_data->velocity *= speed_ratio;
	move_data->out_wish_vel = move_data->velocity;
}

void c_player_movement_services::accelerate(vec3_t& velocity, float friction_decel, float frametime, vec3_t& wish_dir, float wish_speed, float acceleration) {
	static auto sv_accelerate_use_weapon_speed = g_interfaces->m_engine_convar->find_by_name("sv_accelerate_use_weapon_speed");
	static auto sv_water_slow_amount = g_interfaces->m_engine_convar->find_by_name("sv_water_slow_amount");

	float flStoredAccel = acceleration;
	auto pPawn = m_pawn();

	if (!pPawn)
		return;

	float flCurrentSpeed = velocity.dot(wish_dir);
	float flSpeedDelta = wish_speed - flCurrentSpeed;

	if (flSpeedDelta <= 0.0f)
		return;

	if (flCurrentSpeed < 0.0f)
		flCurrentSpeed = 0.0f;

	const bool bIsDucking =
		(m_button_state().m_value & IN_DUCK) || m_bDucking() || (pPawn->m_fFlags() & FL_DUCKING);
	const bool bIsWalking =
		((m_button_state().m_value & IN_SPEED) != 0) && !bIsDucking;

	float flAccelScale = std::max(250.0f, wish_speed);
	float flGoalSpeed = flAccelScale;

	bool bIsSlowSniperScoped = false;

	auto pWeapon = g_ctx->m_active_weapon;
	if (sv_accelerate_use_weapon_speed->get_bool() && pWeapon)
	{
		const auto pWpnData = pWeapon->weapon_data();
		if (pWpnData)
		{
			bIsSlowSniperScoped =
				(pWeapon->m_zoomLevel() > 0 &&
					pWpnData->m_nZoomLevels() > 1 &&
					(pWpnData->m_flMaxSpeed() * 0.52f) < 110.0f);

			float flSpeedFactor = std::clamp(pWpnData->m_flMaxSpeed() / 250.0f, 0.0f, 1.0f);

			if ((!bIsDucking && !bIsWalking) || bIsSlowSniperScoped)
				flAccelScale *= flSpeedFactor;

			flGoalSpeed *= flSpeedFactor;
		}
	}

	if (bIsDucking)
	{
		flAccelScale *= 0.34f;
		flGoalSpeed *= 0.34f;
	}

	if (bIsWalking)
	{
		flAccelScale *= 0.52f;
		flGoalSpeed *= 0.52f;
	}

	if (bIsWalking && flCurrentSpeed > (flGoalSpeed - 5.0f))
	{
		float flSpeedFrac =
			std::max(0.0f, flCurrentSpeed - (flGoalSpeed - 5.0f)) /
			std::max(0.01f, flGoalSpeed - (flGoalSpeed - 5.0f));

		flStoredAccel *= std::clamp(1.0f - flSpeedFrac, 0.0f, 1.0f);
	}

	if (frametime > 0.0f)
	{
		float flWishAccel = (flGoalSpeed * flStoredAccel * m_flSurfaceFriction()) - (friction_decel / frametime);

		if (flWishAccel > 0.0f)
		{
			float flAccelSpeed = flWishAccel * frametime;
			if (flAccelSpeed > flSpeedDelta)
				flAccelSpeed = flSpeedDelta;

			velocity += wish_dir * flAccelSpeed;
		}
	}
}

void c_player_movement_services::friction(vec3_t& velocity, float& decel_fric) {
	static auto sv_friction = g_interfaces->m_engine_convar->find_by_name("sv_friction");
	static auto sv_stopspeed = g_interfaces->m_engine_convar->find_by_name("sv_stopspeed");

	float speed = velocity.length_2d();
	float original_speed = speed;

	if (g_interfaces->m_global_vars->m_curtime <= m_flOffsetTickCompleteTime())
		speed = m_flOffsetTickStashedSpeed();

	if (speed < 0.1f)
		return;

	float control = std::max(speed, sv_stopspeed->get_float());
	float decel = control * (m_flSurfaceFriction() * sv_friction->get_float()) * g_ctx->m_local_pawn->m_flFriction();

	float frametime = g_interfaces->m_global_vars->m_frame_time;
	float drop = frametime * decel;

	decel_fric = std::max(-(original_speed - drop), 0.0f);

	if (original_speed > 0.0f) {
		float new_speed = std::max(original_speed - drop, 0.0f);
		float scale = new_speed / original_speed;

		velocity *= scale;
	}
}

void c_player_movement_services::walk_move(vec3_t& vel, vec3_t move, float max_speed, float frametime, vec3_t angles) {
	static auto sv_accelerate = g_interfaces->m_engine_convar->find_by_name("sv_accelerate");
	if (!sv_accelerate)
		return;

	float friction_decel = 0.f;
	friction(vel, friction_decel);

	vec3_t fwd, right, up;
	g_math->angle_vectors(angles, &fwd, &right, &up);

	const float forward_move = move.x;
	const float side_move = move.y;

	fwd.z = 0.0f;
	right.z = 0.0f;

	fwd.normalize_place();
	right.normalize_place();

	vec3_t wish_vel(fwd.x * forward_move + right.x * side_move, fwd.y * forward_move + right.y * side_move, 0.0f);

	vec3_t wish_dir = wish_vel;

	float analog_speed = fwd.y * move.x;

	float wish_speed = analog_speed;
	if (analog_speed != 0.0)
		wish_speed = fminf(max_speed, analog_speed);

	accelerate(vel, friction_decel, frametime, wish_dir, wish_speed, sv_accelerate->get_float());
}

void c_player_movement_services::walk_move(c_move_data* move_data)
{
	static auto sv_accelerate = g_interfaces->m_engine_convar->find_by_name("sv_accelerate");
	if (!sv_accelerate)
		return;

	vec3_t forward, right, up;
	g_math->angle_vectors(move_data->view_angles, &forward, &right, &up);

	auto const fmove = move_data->forward_move;
	auto const smove = move_data->side_move;

	forward.z = right.z = 0.f;

	forward.normalize_place();
	right.normalize_place();

	vec3_t wishvel(forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f);

	auto wishdir = wishvel;
	auto wishspeed = wishdir.normalize_place();

	if (wishspeed != 0 && (wishspeed > move_data->max_speed))
	{
		wishvel = wishvel * (move_data->max_speed / wishspeed);
		wishspeed = move_data->max_speed;
	}

	move_data->velocity.z = 0;
	accelerate(move_data, g_interfaces->m_global_vars->m_frame_time, wishdir, wishspeed, sv_accelerate->get_float());
	move_data->velocity.z = 0;

	if (move_data->velocity.length_sqr() > move_data->max_speed * move_data->max_speed)
	{
		float ratio = move_data->max_speed / move_data->velocity.length();
		move_data->velocity *= ratio;
	}

	move_data->out_wish_vel += wishdir * wishspeed;
}

void c_player_movement_services::stop_to_speed(c_user_cmd* cmd, float speed) {

	c_move_data* move_data = reset_move_data();
	setup_move(cmd, move_data);

	friction(move_data);
	walk_move(move_data);
	const float vel_2d = move_data->velocity.length_2d();
	if (vel_2d < speed) {
		move_data->forward_move = 0.f;
		move_data->side_move = 0.f;
		move_data->up_move = 0.f;

		cmd->m_pb.m_base_cmd->set_forward_move(0.f);
		cmd->m_pb.m_base_cmd->set_side_move(0.f);
		return;
	}

	vec3_t direction;
	g_math->vector_angles(move_data->velocity * -1.f, direction);

	direction.y = g_math->normalize_float(move_data->view_angles.y - direction.y);

	vec3_t fwd{};
	g_math->angle_vectors(direction, &fwd);
	fwd = fwd.normalized();

	float max_speed = move_data->max_speed;

	move_data->forward_move = fwd.x * max_speed;
	move_data->side_move = fwd.y * max_speed;
	move_data->up_move = 0.f;

	cmd->m_pb.m_base_cmd->set_forward_move(move_data->forward_move);
	cmd->m_pb.m_base_cmd->set_side_move(move_data->side_move);
}

vec3_t c_player_movement_services::get_ground_pos(c_move_data* move_data, float* out_fraction) {
	/*vec3_t abs_origin = m_pawn( )->m_pGameSceneNode( )->m_vecAbsOrigin( );;

	c_trace_filter filter;
	g_interfaces->m_phys2world->init_player_movement_trace_filter( &filter, g_ctx->m_local_pawn, g_ctx->m_local_pawn->m_pCollision( )->m_collisionAttribute( ).m_interacts_with, COLLISION_GROUP_PLAYER_MOVEMENT );
	vec3_t ground = abs_origin;
	ground.z -= 2.f;

	bbox_collision_t bounds;
	g_ctx->m_local_pawn->m_pMovementServices( )->get_player_collision_bound( &bounds );

	c_game_trace trace;
	g_interfaces->m_phys2world->init_game_trace( &trace );

	g_interfaces->m_phys2world->trace_player_bbox( &abs_origin, &ground, &bounds, &filter, &trace );

	if ( trace.m_start_in_solid || trace.m_fraction == 1.f ) {
		return abs_origin;
	}

	return trace.m_end_pos;*/

	vec3_t start = m_pawn()->m_pGameSceneNode()->m_vecAbsOrigin();
	vec3_t end = start;
	end.z -= 200.f;

	c_ray ray{};
	c_game_trace trace{};
	c_trace_filter filter{ 0x1c3003, g_ctx->m_local_pawn, nullptr, 4 };

	g_interfaces->m_phys2world->trace_shape(&ray, &start, &end, &filter, &trace);

	if (out_fraction)
		*out_fraction = trace.m_fraction;

	if (trace.hit_world()) {
		return trace.m_end_pos;
	}

	return start;
}

void c_player_movement_services::reset_prediction_command()
{
	g_memory->call_virtual<void>(this, 43);
}

void c_player_movement_services::run_command(c_user_cmd* cmd)
{
	g_memory->call_virtual<void>(this, 27, cmd);
}

void c_player_movement_services::force_buttons_down(__int64 a2)
{
	using fn_force_buttons_down = void* (__fastcall*)(void*, __int64);
	static fn_force_buttons_down force_buttons_down_ = g_modules->m_client.find(xx("40 53 56 41 56 48 81 EC ? ? ? ? 4C 8B 41")).as<fn_force_buttons_down>();

	force_buttons_down_(this, a2);
}

void c_player_movement_services::setup_movement_moves(c_move_data* move_data)
{
	using fn_setup_movement_moves = void* (__fastcall*)(void*, c_move_data*);
	static fn_setup_movement_moves setup_movement_moves_ = g_modules->m_client.find(xx("48 89 5C 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 E8 ? ? ? ? 44 0F B6 C0")).as<fn_setup_movement_moves>();

	setup_movement_moves_(this, move_data);
}

void c_player_movement_services::finish_move(c_user_cmd* cmd, c_move_data* move_data)
{
	g_memory->call_virtual<void>(this, 34, cmd, move_data);
}

void c_player_movement_services::post_think() {
	g_memory->call_virtual<void>(this, 5);
}

void c_player_movement_services::quantize_movement(c_move_data* move_data)
{
	static auto sv_quantize_movement_input = g_interfaces->m_engine_convar->find_by_name("sv_quantize_movement_input");

	move_data->game_code_moved_player = (this->m_pawn()->m_fFlags() & FL_ONGROUND) == 0;

	if (sv_quantize_movement_input->get_bool())
	{
		move_data->forward_move = std::roundf(move_data->forward_move);
		move_data->side_move = std::roundf(move_data->side_move);
		move_data->up_move = std::roundf(move_data->up_move);
	}
}

void c_player_movement_services::process_movement(c_move_data* move_data)
{
	g_memory->call_virtual<void>(this, 26, move_data);
}

__int64 c_player_movement_services::process_movement_cmd(c_user_cmd* cmd) {
	using fn = __int64(__fastcall*)(c_player_movement_services*, c_user_cmd*);
	static fn x = g_modules->m_client.find(xx("E8 ? ? ? ? 48 8B 06 48 8B CE FF 90 ? ? ? ? 48 85 DB")).absolute(1, 0).as<fn>();
	return x(this, cmd);
}

void c_player_movement_services::calcualte_jump_height(c_move_data* move_data)
{
	if (this->m_pawn()->m_fFlags() & FL_ONGROUND)
		this->m_flMaxJumpHeightThisJump() = std::fmaxf(move_data->abs_origin.z - this->m_flHeightAtJumpStart(), 0.0f);

	if (this->m_nButtonDownMaskPrev() != this->m_button_state().m_value)
		this->m_nButtonDownMaskPrev() = this->m_button_state().m_value;
}

void c_player_movement_services::process_impacts_attack(c_move_data* move_data)
{
	g_memory->call_virtual<void>(this, 35, move_data);
}

void c_player_movement_services::process_impacts(c_user_cmd* cmd, c_move_data* move_data)
{
	using fn_process_impacts = void* (__fastcall*)(void*, c_user_cmd*, c_move_data*);
	static fn_process_impacts process_impacts_ = g_modules->m_client.find(xx("48 8B C4 53 56 41 55")).as<fn_process_impacts>();

	process_impacts_(this, cmd, move_data);
}

void c_player_movement_services::setup_subtick(c_user_cmd* cmd, c_move_data* move_data)
{
	using fn_setup_subtick = void* (__fastcall*)(void*, c_user_cmd*, c_move_data*, bool);
	static fn_setup_subtick setup_subtick_ = g_modules->m_client.find(xx("48 8B C4 48 89 50 ? 48 89 48 ? 53 57")).as<fn_setup_subtick>();

	setup_subtick_(this, cmd, move_data, 0);
}

void c_player_movement_services::prepare_movement(c_move_data* move_data) {
	g_memory->call_virtual<void>(this, 31, move_data);
}

void c_player_movement_services::setup_move(c_user_cmd* cmd, c_move_data* move_data)
{
	g_memory->call_virtual<void>(this, 30, cmd, move_data);
}

void c_player_movement_services::update_button_states(c_user_cmd* cmd)
{
	g_memory->call_virtual<void>(this, 24, cmd);
}

void c_player_movement_services::check_moving_ground(float frame_time)
{
	g_memory->call_virtual<void>(this, 39, frame_time);
}

c_move_data* c_player_movement_services::reset_move_data()
{
	return g_memory->call_virtual<c_move_data*>(this, 29);
}

float c_cs_player_pawn::get_some_timing(int idx0, int idx1) {
	static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 63 D8 48 8B F1")).as<float(__fastcall*)(c_cs_player_pawn*, int, int)>();
	return fn(this, idx0, idx1);
}

void c_cs_player_pawn::set_velocity(vec3_t* velocity)
{
	static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 80 B9 ? ? ? ? ? 48 8B EA")).as<void(__fastcall*)(c_cs_player_pawn*, vec3_t*)>();
	fn(this, velocity);
}

vec3_t c_cs_player_pawn::get_weapon_recoil() {
	vec3_t recoil = {};
	//static auto fn = g_modules->m_client.find(xx("40 53 56 57 48 83 EC ? 80 B9 ? ? ? ? ? 41 0F B6 F0")).as<void(__fastcall*)(void*, vec3_t*, bool)>();
	//fn(this, &recoil, true);
	return recoil;
}


c_hud_model_arms* c_cs_player_pawn::get_view_arms()
{

	c_hud_model_arms* m_hud_model = (c_hud_model_arms*)this->m_hud_model_arms().get();
	if (!m_hud_model)
		return nullptr;

	return m_hud_model;
}

std::vector<c_hud_model_weapon*> c_cs_player_pawn::get_view_models()
{
	c_hud_model_arms* pViewArms = get_view_arms(); // m_hHudModelArms
	if (!pViewArms)
		return {};

	c_game_scene_node* pGameSceneNode = pViewArms->m_pGameSceneNode();
	if (!pGameSceneNode) {
		return {};
	}

	std::vector<c_hud_model_weapon*> vecViewModels = {};
	for (c_game_scene_node* pChild = pGameSceneNode->m_child(); pChild; pChild = pChild->m_sibling()) {
		c_base_entity* pOwner = reinterpret_cast<c_base_entity*>(pChild->m_owner());
		if (!pOwner) {
			continue;
		}

		if (pOwner->get_class_name() == (("C_CS2HudModelWeapon")) != std::string::npos) { // pOwner->GetClassName() == "C_CS2HudModelWeapon"
			vecViewModels.push_back(reinterpret_cast<c_hud_model_weapon*>(pOwner));
		}
	}

	return vecViewModels;
}

c_hud_model_weapon* c_cs_player_pawn::get_view_model_brutforce() {

	c_base_player_weapon* pWeapon = this->get_active_weapon(); // CPlayer_WeaponServices -> m_hActiveWeapon
	if (!pWeapon) {
		return nullptr;
	}

	std::vector<c_hud_model_weapon*> vecViewModels = get_view_models();
	if (vecViewModels.empty()) {
		return nullptr;
	}

	for (c_hud_model_weapon* pViewModel : vecViewModels) {
		c_base_entity* pOwner = (c_base_entity*)pViewModel->m_hOwnerEntity().get(); // m_hOwnerEntity
		if (!pOwner) {
			continue;
		}

		if (pOwner == pWeapon) {
			return pViewModel;
		}
	}

	return nullptr;
}
