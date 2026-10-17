#include "antiaim.h"
#include <cheat/config/vars.h>

#include <sdk/constants.h>
#include <cheat/features/movement/movement.h>

float c_antiaim::get_pitch( ) {
	int base = GET_VAR( int, ANTIAIM_PATH( m_pitch ) );

	// jitter
	if ( GET_VAR( bool, ANTIAIM_PATH( m_pitch_jitter ) ) ) {
		float amount = GET_VAR( int, ANTIAIM_PATH( m_pitch_jitter_amount ) );
		static bool last = false;
		base += last ? -amount : amount;
		last = !last;
	}

	return ( float )base;
}

float c_antiaim::get_at_target_yaw( ) {
	vec3_t local_eye_pos = g_ctx->m_local_pawn->get_eye_pos( );
	qangle_t local_angles = g_movement->m_camera_angle;
	vec3_t forward, right, up;
	g_math->angle_vectors( local_angles, &forward, &right, &up );

	float nearest_fov = FLT_MAX;
	qangle_t best_ang = g_ctx->m_base->get_view_angles( )->m_ang_value;

	for ( auto& player : g_entity_cache->m_players ) {

		c_cs_player_controller* controller = player.m_controller;
		if ( !controller || controller == g_ctx->m_local_controller )
			continue;

		c_cs_player_pawn* pawn = player.m_pawn;
		if ( !pawn )
			continue;

		if ( pawn->m_iHealth( ) <= 0 || !pawn->is_enemy( ) )
			continue;

		vec3_t target = pawn->get_eye_pos( );

		vec3_t delta = target - local_eye_pos;
		delta.normalize_place( );

		float dot = std::clamp( forward.dot( delta ), -1.0f, 1.0f );
		float fov = RAD2DEG( acosf( dot ) );

		if ( fov < nearest_fov ) {
			nearest_fov = fov;
			best_ang = g_math->calculate_angle( local_eye_pos, target );
		}
	}

	return best_ang.y;
}


float c_antiaim::get_yaw( ) {
	float current_yaw = g_ctx->m_base->get_view_angles( )->m_ang_value.y;
	float base = (float)GET_VAR( int, ANTIAIM_PATH( m_yaw ) );
	
	// jitter
	if ( GET_VAR( bool, ANTIAIM_PATH( m_yaw_jitter ) ) ) {
		float amount = GET_VAR( int, ANTIAIM_PATH( m_yaw_jitter_amount ) );
		static bool last = false;
		base += last ? -amount : amount;
		last = !last;
	}

	// at target
	if ( GET_VAR( bool, ANTIAIM_PATH( m_at_target ) ) ) {
		float target_yaw = this->get_at_target_yaw( );
		float delta = g_math->angle_diff( target_yaw, current_yaw ); 
		base += delta;
	}


	bool& override_left = GET_VAR(bool, ANTIAIM_PATH(m_override_left));
	bool& override_right = GET_VAR(bool, ANTIAIM_PATH(m_override_right));

	
	static bool last_override_left = override_left;
	static bool last_override_right = override_right;

	if (override_left && !last_override_left) {
		override_right = false;
	}
	if (override_right && !last_override_right) {
		override_left = false;
	}

	last_override_left = override_left;
	last_override_right = override_right;

	/* apply final base offset*/
	if (override_left)
		base += -90.f;
	else if (override_right)
		base += 90.f;


	/* both off, do nothing. */

	float output = current_yaw + base;
	return g_math->normalize_float(output);
}

void c_antiaim::run() {
	if (!GET_VAR(bool, ANTIAIM_PATH(m_enabled_antiaim)))
		return;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;

	if (g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_NOCLIP || g_ctx->m_local_pawn->m_nActualMoveType() == MOVETYPE_LADDER || g_ctx->m_cmd->m_buttons.m_value & IN_USE)
		return;

	// don't override when knife attacking
	if (g_ctx->m_active_weapon_data->m_WeaponType() == WEAPONTYPE_KNIFE) {
		if ((g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK && g_ctx->m_active_weapon->m_nNextPrimaryAttackTick() <= g_ctx->m_local_controller->m_nTickBase()) ||
			(g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK2 && g_ctx->m_active_weapon->m_nNextSecondaryAttackTick() <= g_ctx->m_local_controller->m_nTickBase()))
			return;
	}

	// don't override during grenade throws
	if ( c_base_cs_grenade* grenade = reinterpret_cast< c_base_cs_grenade* >( g_ctx->m_active_weapon );
		grenade && g_ctx->m_active_weapon_data->m_WeaponType( ) == WEAPONTYPE_GRENADE ) {
		if ( !grenade->m_bPinPulled( ) || g_ctx->m_cmd->m_buttons.m_value & ( IN_ATTACK | IN_ATTACK2 ) ) {
			if ( grenade->m_fThrowTime( ) > 0.f || grenade->m_bJumpThrow( ) )
				return;
		}
	}

	g_ctx->m_base->get_view_angles()->set_q_angle({ this->get_pitch(), this->get_yaw(), 0.f });
}
