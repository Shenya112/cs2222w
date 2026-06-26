#include "mesh_system.h"

#include <context.h>
#include <sdk/datatypes/scene_object.h>
#include <sdk/interfaces/scene_system.h>

/*

	I know it's a mess right now, this is all for testing, once it works perfectly I'll clean it up.

*/

__int64 sub_180133930( void* physics_prop_multiplayer, void* skeleton_instance, void* created_object, int v12 ) {
	using fn_ = __int64( __fastcall* )( void*, void*, void*, int );
	static fn_ x = g_modules->m_client.find( xx( "48 89 5C 24 ? 48 89 6C 24 ? 41 56 48 83 EC ? 48 8B D9 41 0F B7 C1" ) ).as< fn_ >( );
	return x( physics_prop_multiplayer, skeleton_instance, created_object, v12 );
}
void sub_180248400( void* render_game_system, std::uintptr_t object, int arg ) {
	using fn_ = void( __fastcall* )( void*, std::uintptr_t, int );
	static fn_ x = g_modules->m_client.find( xx( "48 85 D2 74 ? 0F BE 4A" ) ).as< fn_ >( );
	return x( render_game_system, object, arg );
}
void sub_1802484B0( void* render_game_system, std::uintptr_t model_data, std::uintptr_t unknown_data ) {
	using fn_ = void( __fastcall* )( void*, __int64, __int64 );
	static fn_ x = g_modules->m_client.find( xx( "0F BE 4A ? 85 C9 74 ? 83 E9 ? 74 ? 83 F9 ? 75 ? 4C 89 42" ) ).as< fn_ >( );
	return x( render_game_system, model_data, unknown_data );
}
bool model_has_force_lod_flag(void* model_handle) {
	using fn_ = bool(__fastcall*)(void*);
	static fn_ x = g_modules->m_client.find(xx("8B 41 ? 8B 49 ? C1 E8 ? C1 E9 ? 0A C1 24 ? C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 40 57")).as< fn_ >();
	return x(model_handle);
}

c_scene_animatable_object* c_mesh_system::create_scene_animatable_object( c_cs_player_pawn* pawn ) {
	using fn_get_scene_world = void* ( __fastcall* )( void*, unsigned int );
	static fn_get_scene_world get_scene_world = g_modules->m_client.find( xx( "89 54 24 ? 48 83 EC ? 85 D2" ) ).as<fn_get_scene_world>( );

	if ( !pawn )
		return nullptr;

	auto* game_scene_node = pawn->m_pGameSceneNode( );
	if ( !game_scene_node )
		return nullptr;

	c_skeleton_instance* skeleton_instance = game_scene_node->get_skeleton_instace( );
	if ( !skeleton_instance )
		return nullptr;

	int world_group_id = skeleton_instance->get_world_group_id(  );
	auto model_state = skeleton_instance->m_modelState( );
	auto model_handle = model_state.m_model_handle;
	if ( !model_handle.binding || !model_handle.binding->data )
		return nullptr;

	c_scene_animatable_object* object = g_memory->call_virtual<c_scene_animatable_object*>(
		this,
		20,
		&model_handle,
		game_scene_node->m_nodeToWorld( ),
		xx( "AnimatableSceneObjectDesc" ),
		8LL,
		0x220100000001LL,
		get_scene_world( nullptr, world_group_id )
	);

	if ( !object )
		return nullptr;

	object->m_owner = pawn->get_handle( );

	object->m_skeleton_instance = skeleton_instance;

	bool has_force_lod_flag = model_has_force_lod_flag( model_handle.binding->data );
	char lod_flag = object->m_lod_flag( );
	char new_lod_flag = lod_flag;
	if ( has_force_lod_flag )
		new_lod_flag |= 0x10;
	else
		new_lod_flag &= 0xEF;

	object->m_lod_flag( ) = new_lod_flag;

	object->m_owner = pawn->get_handle( );

	/*if ( auto physics_prop = object->m_skeleton_instance->get_physics_prop_multiplayer( ) ) {
		__int64 v46 = sub_180133930( physics_prop, object->m_skeleton_instance, object, -1 );
		model_state.m_unk( ) = v46;

		sub_180248400( nullptr, v46, ( __int64 )( ( __int64 )skeleton_instance + 0x3A4 ) );
		sub_1802484B0( nullptr, v46, ( __int64 )( ( __int64 )skeleton_instance + 0x308 ) );
	}*/


	
	return object;
}
