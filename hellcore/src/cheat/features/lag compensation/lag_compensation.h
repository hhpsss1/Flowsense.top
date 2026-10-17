#pragma once
#include <sdk/entity/pawn.h>
#include <sdk/constants.h>
#include <cheat/features/aimbot shared/hitbox.h>
#include <cheat/features/penetration/autowall.h>
#include <utils/tight_array.h>

constexpr auto BONE_MATRIX_MEMORY_SIZE = sizeof( bone_data_t ) * 128;
constexpr float VALID_LAGCOMP_CONE_COSINE = 0.707107f;    // 45 degree angle

struct lag_record_t {

	bool m_visual_only = false;
	float m_capture_time = 0.0f;

	c_cs_player_pawn* m_pawn = nullptr;
	c_game_scene_node* m_game_scene_node = nullptr;
	c_skeleton_instance* m_skeleton_instance = nullptr;

	bone_data_t m_bones[128];

	float m_simulation_time = 0.0f;

	vec3_t m_rotation, m_origin = {};
	vec3_t m_velocity = {};
	float m_duck_amount = 0.0f;
	int m_flags = 0;

	tight_array< c_hitbox_data, 19 > m_all_hitboxes;
	tight_array< c_hitbox_data, 11 > m_rage_hitboxes;	

	bool setup( c_cs_player_pawn* pawn );
	bool should_skip_interpolation( );
	void try_adjust_velocity( );	
	bool is_valid( );
};

void store_hitboxes( lag_record_t* record, c_penetration::player_context_t& player_ctx );

// animfix: re-poses the pawn skeleton at the record transform and rebuilds
// hitboxes from fresh bones into `out`. returns false on any failure, in
// which case the caller must fall back to the stored record data.
bool repose_record_for_scan( lag_record_t* record, lag_record_t* out, c_penetration::player_context_t& player_ctx );

class c_lag_compensation {
public:
	void run( );
	void force_input_history( );
	bool wants_lag_compensation_on_entity( lag_record_t* victim );
};

inline auto g_lagcomp = std::make_unique<c_lag_compensation>( );
