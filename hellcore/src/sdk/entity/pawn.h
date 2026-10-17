#pragma once

#include "networked_utl_vector.h"

#include "weapon.h"

#include <sdk/datatypes/utl_vector.h>
#include <sdk/constants.h>


enum e_life_state : uint8_t {
	LIFE_ALIVE,
	LIFE_DYING,
	LIFE_DEAD,
	LIFE_RESPAWNABLE,
	LIFE_RESPAWNING
};

class c_player_item_services
{
public:
	SCHEMA( bool, m_bHasDefuser, ( "CCSPlayer_ItemServices->m_bHasDefuser" ) );
	SCHEMA( bool, m_bHasHelmet, ( "CCSPlayer_ItemServices->m_bHasHelmet" ) );

    bool m_bHasHeavyArmor() const { return false; }
};

class c_player_weapon_services : public c_base_entity
{
public:
	SCHEMA( c_base_handle, m_hActiveWeapon, ( "CPlayer_WeaponServices->m_hActiveWeapon" ) );
	SCHEMA( c_base_handle, m_hLastWeapon, ( "CPlayer_WeaponServices->m_hLastWeapon" ) );
	SCHEMA( c_network_utl_vector<c_base_handle>&, m_hMyWeapons, ( "CPlayer_WeaponServices->m_hMyWeapons" ) );
	SCHEMA( float, m_flNextAttack, ( "CCSPlayer_WeaponServices->m_flNextAttack" ) );

	vec3_t weapon_shoot_position( );

    OFFSET(c_cs_player_pawn*, m_owner_pawn, 0x38);
    bool has_shoot_position_history(int tick, float fraction) const {
        const auto* object = reinterpret_cast<const std::byte*>(this);
        const int head = *reinterpret_cast<const int*>(object + 0x368);
        const int count = *reinterpret_cast<const int*>(object + 0x36C);
        if (head < 0 || head >= 32 || count < 2 || count > 32 || !std::isfinite(fraction))
            return false;
        struct history_entry {int tick; float fraction; vec3_t position;};
        static_assert(sizeof(history_entry) == 20);
        const auto* entries = reinterpret_cast<const history_entry*>(object + 0xE8);
        const auto& oldest = entries[head];
        const auto& newest = entries[(head + count - 1) % 32];
        if (!std::isfinite(oldest.fraction) || !std::isfinite(newest.fraction))
            return false;
        const bool after_oldest = tick > oldest.tick || (tick == oldest.tick && fraction >= oldest.fraction);
        const bool before_newest = tick < newest.tick || (tick == newest.tick && fraction <= newest.fraction);
        return after_oldest && before_newest;
    }


};

class c_player_view_model_services
{
public:

};

class c_subtick_move
{
public:
	float m_when;
	uint64_t m_button;

	union
	{
		bool m_pressed;
		struct
		{
			float m_analog_forward_delta;
			float m_analog_left_delta;
			float m_analog_pitch_delta;
			float m_analog_yaw_delta;
		} m_analog_move;
	};
};

struct c_move_data_base
{
	bool has_zero_frametime;
	bool is_late_command;
	int player_handle;
	vec3_t abs_view_angles;
	vec3_t view_angles;
	vec3_t last_movement_impulses;
	float forward_move;
	float side_move;
	float up_move;
	vec3_t velocity;
	vec3_t angles;
	vec3_t unknown;
	c_utl_vector<c_subtick_move> subtick_moves;
	c_utl_vector<c_subtick_move> attack_subtick_moves;
	bool has_subtick_inputs;
	float weapon_time_switch_subticked;
	c_utl_vector<void> touch_list;
	vec3_t collision_normal;
	vec3_t ground_normal;
	vec3_t abs_origin;
	int32_t tick_count;
	int32_t target_tick;
	float subtick_start_fraction;
	float subtick_end_fraction;
};

struct c_move_data : public c_move_data_base
{
	vec3_t out_wish_vel;
	vec3_t old_angles;
	vec3_t input_rotated;
	vec3_t continous_acceleration;
	vec3_t frame_velocity_delta;
	float max_speed;
	float client_max_speed;
	float friction_decel;
	bool in_air;
	bool game_code_moved_player;
};

class c_cs_player_pawn;
class c_game_trace;

struct bbox_collision_t
{
	vec3_t m_min, m_max;
};

class c_player_movement_services
{
public:
	OFFSET( c_cs_player_pawn*, m_pawn, 0x30 );
	OFFSET(in_button_state_t&, m_button_state, 0x50);
	OFFSET(uint64_t&, m_button_toggled_state, 0x190);
	OFFSET(uint64_t, m_button_toggled_state_predicted, 0x218);
	SCHEMA( float, m_flMaxSpeed, ( "CPlayer_MovementServices->m_flMaxspeed" ) );
	SCHEMA(float, m_flDuckAmount, ("CCSPlayer_MovementServices->m_flDuckAmount"));
	SCHEMA( float, m_flSurfaceFriction, ( "CPlayer_MovementServices_Humanoid->m_flSurfaceFriction" ) );
	SCHEMA( int, m_nStepside, ( "CPlayer_MovementServices_Humanoid->m_nStepside" ) );
	SCHEMA( vec3_t &, m_vecLastMovementImpulses, ( "CPlayer_MovementServices->m_vecLastMovementImpulses" ) );

	SCHEMA( float&, m_flForwardMove, ( "CPlayer_MovementServices->m_flForwardMove" ) );
	SCHEMA( float&, m_flLeftMove, ( "CPlayer_MovementServices->m_flLeftMove" ) );
	SCHEMA( float&, m_flUpMove, ( "CPlayer_MovementServices->m_flUpMove" ) );
	SCHEMA( float&, m_flStamina, ( "CCSPlayer_MovementServices->m_flStamina" ) );


	SCHEMA( int&, m_nGameCodeHasMovedPlayerAfterCommand, ( "CCSPlayer_MovementServices->m_nGameCodeHasMovedPlayerAfterCommand" ) );
	SCHEMA( bool&, m_bDucked, ( "CCSPlayer_MovementServices->m_bDucked" ) );
	SCHEMA( bool&, m_bDucking, ( "CCSPlayer_MovementServices->m_bDucking" ) );
	SCHEMA( bool&, m_bHasWalkMovedSinceLastJump, ( "CCSPlayer_MovementServices->m_bHasWalkMovedSinceLastJump" ) );
	SCHEMA( int&, m_nTraceCount, ( "CCSPlayer_MovementServices->m_nTraceCount" ) );

	SCHEMA( uint64_t&, m_nQueuedButtonDownMask, ( "CPlayer_MovementServices->m_nQueuedButtonDownMask" ) );
	SCHEMA( uint64_t&, m_nQueuedButtonChangeMask, ( "CPlayer_MovementServices->m_nQueuedButtonChangeMask" ) );
	SCHEMA( uint64_t&, m_nButtonDoublePressed, ( "CPlayer_MovementServices->m_nButtonDoublePressed" ) );
	SCHEMA( uint64_t&, m_nToggleButtonDownMask, ( "CPlayer_MovementServices->m_nToggleButtonDownMask" ) );

	SCHEMA( float&, m_flMaxJumpHeightThisJump, ( "CCSPlayer_MovementServices->m_flMaxJumpHeightThisJump" ) );
	SCHEMA( float&, m_flHeightAtJumpStart, ( "CCSPlayer_MovementServices->m_flHeightAtJumpStart" ) );



























	bool should_use_viewangles_for_subtick2(c_user_cmd* userCmd, c_move_data* m_pMoveData);






};

class c_cs_player_hostage_services {
public:
	SCHEMA(c_handle<c_base_entity>, m_hCarriedHostage, ("CCSPlayer_HostageServices->m_hCarriedHostage"));
};

class c_player_camera_services {
public:
	SCHEMA(vec3_t, m_vClientScopeInaccuracy, ("CCSPlayer_CameraServices->m_vClientScopeInaccuracy"));
};

class c_hud_model_weapon;
class c_hud_model_arms;

class c_contents;
class c_cs_player_pawn : public c_base_entity
{
public:
	SCHEMA( c_player_item_services*, m_pItemServices, ( "C_BasePlayerPawn->m_pItemServices" ) );
	SCHEMA( float, m_flLastSpawnTimeIndex, ( "C_CSPlayerPawnBase->m_flLastSpawnTimeIndex" ) );
	SCHEMA( c_player_weapon_services*, m_pWeaponServices, ( "C_BasePlayerPawn->m_pWeaponServices" ) );
	SCHEMA( c_player_camera_services*, m_pCameraServices, ( "C_BasePlayerPawn->m_pCameraServices" ) );
	SCHEMA( int, m_ArmorValue, ( "C_CSPlayerPawn->m_ArmorValue" ) );
	SCHEMA( c_player_movement_services*&, m_pMovementServices, ( "C_BasePlayerPawn->m_pMovementServices" ) );

	SCHEMA( c_econ_item_view&, m_EconGloves, ( "C_CSPlayerPawn->m_EconGloves" ) );
	SCHEMA( bool&, m_bNeedToReApplyGloves, ( "C_CSPlayerPawn->m_bNeedToReApplyGloves" ) );

	SCHEMA( bool&, m_bIsScoped, ( "C_CSPlayerPawn->m_bIsScoped" ) );
	SCHEMA( bool, m_bIsDefusing, ( "C_CSPlayerPawn->m_bIsDefusing" ) );
	SCHEMA( bool, m_bInBuyZone, ( "C_CSPlayerPawn->m_bInBuyZone" ) );
	SCHEMA( bool, m_bIsWalking, ( "C_CSPlayerPawn->m_bIsWalking" ) );

	SCHEMA(c_handle<c_hud_model_arms>, m_hud_model_arms, ("C_CSPlayerPawn->m_hHudModelArms"));
	SCHEMA(e_life_state, m_lifeState, ("C_BaseEntity->m_lifeState"));
	SCHEMA(float, m_flWaterLevel, ("C_BaseEntity->m_flWaterLevel"));

	SCHEMA(vec3_t, m_angEyeAngles, ("C_CSPlayerPawn->m_angEyeAngles"));

	SCHEMA( void*, m_pObserverServices, ( "C_BasePlayerPawn->m_pObserverServices" ) );
	SCHEMA( void*, m_pWaterServices, ( "C_BasePlayerPawn->m_pWaterServices" ) );
	SCHEMA( c_base_handle, m_hController, ( "C_BasePlayerPawn->m_hController" ) );
	SCHEMA( float, m_flFlashDuration, ( "C_CSPlayerPawnBase->m_flFlashDuration" ) );
	SCHEMA( bool, m_bWaitForNoAttack, ( "C_CSPlayerPawn->m_bWaitForNoAttack" ) );
	SCHEMA( bool, m_bGunGameImmunity, ( "C_CSPlayerPawn->m_bGunGameImmunity" ) );
	SCHEMA( vec3_t, v_angle, ( "C_BasePlayerPawn->v_angle" ) );
	SCHEMA( vec3_t, m_vOldOrigin, ( "C_BasePlayerPawn->m_vOldOrigin" ) );
	SCHEMA( float, m_flOldSimulationTime, ( "C_BasePlayerPawn->m_flOldSimulationTime" ) );
	OFFSET( int, some_shit, 0xA68 );
	OFFSET( float, m_max_player_movement_speed, 0x1254 );
	SCHEMA(c_cs_player_hostage_services*, m_pHostageServices, ("C_CSPlayerPawn->m_pHostageServices"));

	vec3_t get_world_space_center() {
		vec3_t vOrigin = this->m_pGameSceneNode()->m_vecAbsOrigin();
		auto vMin = this->m_pCollision()->m_vecMins() + vOrigin;
		auto vMax = this->m_pCollision()->m_vecMaxs() + vOrigin;

		return vMin + (vMax - vMin) * 0.69999999f;
	};
	
	int GetAssociatedTeam()
	{
		const int nTeam = this->m_iTeamNum();

		return nTeam;
	}

	c_contents* get_contents();
	vec3_t get_eye_pos( );
	bool is_enemy( );
	vec3_t get_shoot_pos();
	c_weapon_base* get_active_weapon( );
	uint32_t get_collision_owner_index( );
	bool has_armor( int hitgroup );
	bool IsArmored(int nHitGroup);
	void set_body_group( int first, int second );
	void run_pre_think( );

	void run_post_think( );
	float get_some_timing( int idx0, int idx1 );
	void set_velocity( vec3_t* velocity );
	bool is_alive();
	void set_host_entity();
	void reset_host_entity();
	vec3_t get_weapon_recoil( );
	c_hud_model_arms* get_view_arms();
	std::vector<c_hud_model_weapon*> get_view_models();
	c_hud_model_weapon* get_view_model_brutforce();
};

class player_observer_services
{
public:
	SCHEMA(std::uint8_t, m_iObserverMode, ("CPlayer_ObserverServices->m_iObserverMode"));

	SCHEMA(c_handle<c_base_entity>, m_hObserverTarget, ("CPlayer_ObserverServices->m_hObserverTarget"));
};

class c_cs_observer_services : public player_observer_services
{
public:
};

class c_cs_observer_pawn : public c_cs_player_pawn
{
public:
};


struct c_glow_color {
 uint8_t r,g,b,a;
 c_glow_color& operator=(const hellcolor& color) {
  r=static_cast<uint8_t>(std::clamp(color.Value.x,0.f,1.f)*255.f);
  g=static_cast<uint8_t>(std::clamp(color.Value.y,0.f,1.f)*255.f);
  b=static_cast<uint8_t>(std::clamp(color.Value.z,0.f,1.f)*255.f);
  a=static_cast<uint8_t>(std::clamp(color.Value.w,0.f,1.f)*255.f);
  return *this;
 }
};
static_assert(sizeof(c_glow_color)==4);
class c_glow_property {
public:
	PAD( 0x18 )
	c_entity_instance* m_legacy_owner;
 c_entity_instance* get_owner() const {
  return reinterpret_cast<c_entity_instance*>(reinterpret_cast<std::uintptr_t>(this)-0xDE8);
 }

	SCHEMA( c_glow_color&, m_glowColorOverride, ( "CGlowProperty->m_glowColorOverride" ) );
	SCHEMA( bool&, m_bGlowing, ( "CGlowProperty->m_bGlowing" ) );
	SCHEMA( int&, m_iGlowType, ( "CGlowProperty->m_iGlowType" ) );
	SCHEMA( float&, m_flGlowTime, ( "CGlowProperty->m_flGlowTime" ) );
};