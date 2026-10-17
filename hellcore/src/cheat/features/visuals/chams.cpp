#include "chams.h"
#include <sdk/datatypes/key_values.h>

#include "vmats.h"
#include <cheat/config/vars.h>
#include <cheat/menu/visual_preview.h>
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/scene_system.h>


void c_chams::init( void ) {
	auto create_material = [ ]( const char* material_name, const char vmat[ ] ) -> c_strong_handle<c_material_2> {
		if ( !material_name || !vmat )
			return {};

		c_key_values_3* keyval = c_key_values_3::create_material_resource( );
		if ( !keyval )
			return {};

		keyval->load_from_buffer( vmat );

		c_strong_handle<c_material_2> custom_material{};
		using fn_create_material = __int64( __fastcall* )( void*, void*, const char*, c_key_values_3*, unsigned int, unsigned int );
		static fn_create_material create_material = g_modules->m_materialsystem2.find( xx( "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 8B F2" ) ).as<fn_create_material>( );

		if ( !create_material )
			return {};

		create_material( nullptr, &custom_material, material_name, keyval, 0, 1 );

		return custom_material;
		};

	m_materials[ e_materials::material_latex ].m_occluded = create_material( xx( "flat_vmat_invis.vmat" ), latex_vmat_invis );
	m_materials[ e_materials::material_glow ].m_occluded = create_material( xx( "glow_vmat_invis.vmat" ), glow_vmat_invis );
	m_materials[ e_materials::material_ghost ].m_occluded = create_material( xx( "ghost_vmat_invis.vmat" ), ghost_vmat_invis );
	m_materials[ e_materials::material_flat ].m_occluded = create_material( xx( "flat_vmat_invis.vmat" ), flat_vmat_invis );

	m_materials[ e_materials::material_latex ].m_visible = create_material( xx( "latex_vmat.vmat" ), latex_vmat );
	m_materials[ e_materials::material_glow ].m_visible = create_material( xx( "glow_vmat.vmat" ), glow_vmat );
	m_materials[ e_materials::material_ghost ].m_visible = create_material( xx( "ghost_vmat.vmat" ), ghost_vmat );
	m_materials[ e_materials::material_flat ].m_visible = create_material( xx( "flat_vmat.vmat" ), flat_vmat );
}

void c_chams::set_material_and_color( c_mesh_primitive* mesh_primitive, c_material_color color, c_material_2* material ) {
	if (!mesh_primitive || !material)
		return;
		
	mesh_primitive->m_material = material;
	mesh_primitive->m_material2 = material;
	mesh_primitive->m_color = color;
}

class c_generate_primitives_data {
public:
	void* m_scene_view; //0x0000
	void* m_scene_view_2; //0x0008
	c_scene_layer* m_scene_layer; //0x0010
	char pad_0018[ 88 ]; //0x0018
	void* m_scene_view_3; //0x0070
	char pad_0078[ 16 ]; //0x0078
	void* m_view_drawlist_data; //0x0088
	void* m_view_drawlist; //0x0090
	c_scene_layer* m_scene_layer_2; //0x0098
	char pad_00A0[ 24 ]; //0x00A0
}; //Size: 0x00B8

bool c_chams::on_generate_primitives( c_animatable_scene_object_desc* desc, c_scene_animatable_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf ) {
	static const auto original = g_hooks->m_scene_system.m_generate_primitives.get<decltype( &c_scene_system_hooks::generate_primitives )>( );

	if ( !object )
		return false;

	c_base_handle owner = object->m_owner;
	if ( !owner.is_valid( ) )
		return false;

	c_base_entity* entity = owner.get<c_base_entity>( );
	if ( !entity || !entity->get_handle().is_valid() )
		return false;

	c_cs_player_pawn* pawn = ( c_cs_player_pawn* )entity;
	if ( !pawn )
		return false;

	auto set_chams = [ & ]( c_material_color color, int cham_material_index, bool occluded, bool apply_glow = false ) {
			if (cham_material_index < 0 || cham_material_index >= e_materials::material_max)
				return;

			int prev_count = render_buf->get_count();
			original( desc, object, a3, render_buf );

			for ( auto i = prev_count; i < render_buf->get_count(); i++ ) {
				c_mesh_primitive* mesh_primitive = render_buf->get_primitive( i );
                if(!mesh_primitive) continue;

				if ( !mesh_primitive )
					continue;

				c_material_2* material = occluded ? this->m_materials[ cham_material_index ].m_occluded : this->m_materials[ cham_material_index ].m_visible;
				if (!material)
					continue;

				set_material_and_color( mesh_primitive, color, material );
			}

			// Apply overlay - always use glow material for overlay effect
			if (apply_glow && cham_material_index != e_materials::material_glow) {
				int overlay_prev_count = render_buf->get_count();
				original( desc, object, a3, render_buf );

				for ( auto i = overlay_prev_count; i < render_buf->get_count(); i++ ) {
					c_mesh_primitive* mesh_primitive = render_buf->get_primitive( i );
                if(!mesh_primitive) continue;

					if ( !mesh_primitive )
						continue;

					// Always use glow material for overlay
					c_material_2* overlay_material = occluded ? this->m_materials[ e_materials::material_glow ].m_occluded : this->m_materials[ e_materials::material_glow ].m_visible;
					if (!overlay_material)
						continue;

					// Use the same color as base chams for overlay
					set_material_and_color( mesh_primitive, color, overlay_material);
				}
			}
		};

	auto class_name = entity->get_class_name( );
	fnv1a_t class_name_hashed = fnv_hash( entity->get_class_name( ) );

	



	if ( GET_VAR( bool, VISUALS_PATH( m_chams_local_attachments_enabled ) ) && !g_ctx->m_local_pawn->m_bIsScoped()) {
		if ( pawn->is_weapon( )
			&& GET_VAR( bool, VISUALS_PATH( m_third_person_enabled ) )
			&& pawn->m_hOwnerEntity( ).get( ) == g_ctx->m_local_pawn 
			) {
			
			set_chams(GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_attachments_color)), GET_VAR(int, VISUALS_PATH(m_chams_local_attachments_material)), false);
			return true;
		}
	}
	else if (GET_VAR(bool, VISUALS_PATH(m_transparency_in_scope)) && g_ctx->m_local_pawn->m_bIsScoped()) {
		if (pawn->is_weapon()
			&& GET_VAR(bool, VISUALS_PATH(m_third_person_enabled))
			&& pawn->m_hOwnerEntity().get() == g_ctx->m_local_pawn
			) {
			hellcolor transparency = { GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.x, GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.y, GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.z, 0.1 };
			set_chams(transparency, e_materials::material_flat, false);
			return true;
		}
	}

	if ( class_name_hashed == fnv_hash( "C_CSPlayerPawn" ) ) {
		if ( pawn == g_ctx->m_local_pawn ) {
			{
				if ( s_backtrack_models.contains( pawn ) && s_backtrack_models[ pawn ].exists( object ) ) {
					int invis_material = GET_VAR( int, VISUALS_PATH( m_chams_local_material_invis_bt ) );
					int vis_material = GET_VAR( int, VISUALS_PATH( m_chams_local_material_bt ) );
					bool glow_invis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_local_bt_invisible ) );
					bool glow_vis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_local_bt_visible ) );

					set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_local_color_invis_bt ) ), invis_material, true, glow_invis );
					set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_local_color_bt ) ), vis_material, false, glow_vis );

					return true;
				}
			}

			if ( GET_VAR( bool, VISUALS_PATH( m_chams_local_enabled ) ) ) {
				int invis_material = GET_VAR( int, VISUALS_PATH( m_chams_local_material_invis ) );
				int vis_material = GET_VAR( int, VISUALS_PATH( m_chams_local_material ) );
				bool glow_invis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_local_invisible ) );
				bool glow_vis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_local_visible ) );

				set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_local_color_invis ) ), invis_material, true, glow_invis );
				set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_local_color ) ), vis_material, false, glow_vis );

				return true;
			}

			if ( GET_VAR( bool, VISUALS_PATH( m_transparency_in_scope ) ) && pawn->m_bIsScoped() ) {
				hellcolor transparency = { GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.x, GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.y, GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)).Value.z, 0.1 };
				set_chams( transparency, e_materials::material_flat, false );
				return true;
			}
		}

		else if ( !pawn->is_enemy( ) ) {
			/* handle team. */
		} else {
			{
				if ( s_backtrack_models.contains( pawn ) && s_backtrack_models[ pawn ].exists( object ) ) {
					int invis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material_invis_bt ) );
					int vis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material_bt ) );
					bool glow_invis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_bt_invisible ) );
					bool glow_vis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_bt_visible ) );

					set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_invis_bt ) ), invis_material, true, glow_invis );
					set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_bt ) ), vis_material, false, glow_vis );

					return true;
				}
			}

			{
				if ( s_onshot_models.contains( pawn ) ) {
					for ( model_object_t& os_object : s_onshot_models[ pawn ] ) {
						if ( os_object.exists( object ) ) {
							int vis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material_os ) );
							int invis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material_invis_os ) );
							bool glow_vis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_os_visible ) );
							bool glow_invis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_os_invisible ) );
							
							bool should_animate = ( vis_material == e_materials::material_ghost || vis_material == e_materials::material_flat ) &&
							                     ( invis_material == e_materials::material_ghost || invis_material == e_materials::material_flat );
							
							if ( should_animate ) {
								int expiration = GET_VAR( int, VISUALS_PATH( m_chams_os_expiration ) );
								float curtime = g_interfaces->m_global_vars->m_curtime;
								float elapsed = curtime - os_object.m_draw_begin_time;

								float alpha_mult = 1.0f;
								if ( expiration > 1 ) {
									if ( elapsed > 1.0f ) {
										float fade_start = 1.0f;
										float fade_duration = (float)expiration - fade_start;
										if ( fade_duration > 0.0f ) {
											float fade_progress = ( elapsed - fade_start ) / fade_duration;
											alpha_mult = std::clamp( 1.0f - fade_progress, 0.0f, 1.0f );
										}
									}
								} else {
									float fade_duration = (float)expiration;
									if ( fade_duration > 0.0f ) {
										float fade_progress = elapsed / fade_duration;
										alpha_mult = std::clamp( 1.0f - fade_progress, 0.0f, 1.0f );
									}
								}

								hellcolor vis_color = GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_os ) );
								hellcolor invis_color = GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_invis_os ) );

								vis_color.Value.w *= alpha_mult;
								invis_color.Value.w *= alpha_mult;

								set_chams( invis_color, invis_material, true, glow_invis );
								set_chams( vis_color, vis_material, false, glow_vis );
							} else {
								set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_invis_os ) ), invis_material, true, glow_invis );
								set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_os ) ), vis_material, false, glow_vis );
							}

							return true;
						}
					}
				}
			}

			if ( GET_VAR( bool, VISUALS_PATH( m_chams_enabled ) ) && ( pawn->is_alive( ) || GET_VAR( bool, VISUALS_PATH( m_chams_enemy_ragdoll ) ) ) ) {
				int invis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material_invis ) );
				int vis_material = GET_VAR( int, VISUALS_PATH( m_chams_enemy_material ) );
				bool glow_invis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_invisible ) );
				bool glow_vis = GET_VAR( bool, VISUALS_PATH( m_glow_chams_enemy_visible ) );

				set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color_invis ) ), invis_material, true, glow_invis );
				set_chams( GET_VAR( hellcolor, VISUALS_PATH( m_chams_enemy_color ) ), vis_material, false, glow_vis );

				return true;
			}
		}
	}

	return false;
}

void c_chams::prune_old_onshot( c_cs_player_pawn* pawn ) {
	if (!pawn || !pawn->get_handle().is_valid()) {
		return;
	}

	int expiration = GET_VAR( int, VISUALS_PATH( m_chams_os_expiration ) );
	float curtime = g_interfaces->m_global_vars->m_curtime;

	auto& os_array = s_onshot_models[ pawn ];
	for ( size_t i = 0; i < os_array.size( ); i++ ) {
		auto& model = os_array[ i ];
		if ( !model.m_objects[ SINGLE_OBJECT_INDEX ] )
			continue;

		if ( fabsf( model.m_draw_begin_time - curtime ) > expiration 
			|| !GET_VAR( bool, VISUALS_PATH( m_chams_enabled_os ) ) 
			) {
			model.remove( REMOVE_ALL );
			os_array.erase_unordered( i );

			continue;
		}
	}
}

bool model_object_t::create( c_cs_player_pawn* pawn, int object_index ) {
	if (!pawn || !pawn->get_handle().is_valid() || object_index < 0 || object_index >= m_objects.size())
		return false;

	if ( m_objects[ object_index ] )
		return true;

	if (!g_interfaces->m_mesh_system)
		return false;

	m_objects[ object_index ] = g_interfaces->m_mesh_system->create_scene_animatable_object( pawn );

	if ( !m_objects[ object_index ] )
		return false;

	return true;
}

void model_object_t::remove( int object_index ) {
	if ( object_index <= REMOVE_ALL ) {
		for ( c_scene_animatable_object*& object : m_objects ) {
			if ( !object )
				continue;

			if ( !g_interfaces->m_scene_system ) {
				object = nullptr;
				continue;
			}

			matrix3x4_t* bones = object->get_render_bones();
			int bone_count = object->get_bone_count();
			if ( bones && bone_count > 0 ) {
				vec3_t offset2( 0.f, 0.f, -5000.f );
				for ( int i = 0; i < bone_count; i++ ) {
					bones[ i ][ 0 ][ 3 ] += offset2.x;
					bones[ i ][ 1 ][ 3 ] += offset2.y;
					bones[ i ][ 2 ][ 3 ] += offset2.z;
				}
			}

			try {
				g_interfaces->m_scene_system->delete_scene_object( object );
			} catch (...) {
			}

			object = nullptr;
		}
		return;
	}

	if ( !m_objects[ object_index ] )
		return;

	if ( !g_interfaces->m_scene_system ) {
		m_objects[ object_index ] = nullptr;
		return;
	}

	matrix3x4_t* bones = m_objects[ object_index ]->get_render_bones();
	int bone_count = m_objects[ object_index ]->get_bone_count();
	if ( bones && bone_count > 0 ) {
		vec3_t offset2( 0.f, 0.f, -5000.f );
		for ( int i = 0; i < bone_count; i++ ) {
			bones[ i ][ 0 ][ 3 ] += offset2.x;
			bones[ i ][ 1 ][ 3 ] += offset2.y;
			bones[ i ][ 2 ][ 3 ] += offset2.z;
		}
	}

	try {
		g_interfaces->m_scene_system->delete_scene_object( m_objects[ object_index ] );
	} catch (...) {
	}

	m_objects[ object_index ] = nullptr;
}

void model_object_t::set_bones( c_scene_animatable_object* object, bone_data_t* bones, int count ) {
	if ( !object || !bones || count <= 0 || count > 128 )
		return;

	auto* render_bones = object->get_render_bones();
	if ( !render_bones )
		return;

	int bone_count = object->get_bone_count();
	if ( bone_count <= 0 || bone_count > 128 )
		return;

	int actual_count = std::min( count, bone_count );
	for ( int i = 0; i < actual_count; i++ ) {
		render_bones[ i ] = bones[ i ].to_matrix();
	}
}

void model_object_t::handle(c_cs_player_pawn* pawn, bool onshot) {
	cached_player_t& player = g_entity_cache->find(pawn);

	bool is_local = pawn == g_ctx->m_local_pawn;

	auto& lagcomp_data = player.m_lagcomp_data;
	if (lagcomp_data.m_lag_records.empty()
		|| (onshot && !GET_VAR(bool, VISUALS_PATH(m_chams_enabled_os)))
		|| (is_local && !GET_VAR(bool, VISUALS_PATH(m_chams_local_enabled_bt)))
		|| (!onshot && !is_local && !GET_VAR(bool, VISUALS_PATH(m_chams_enabled_bt)))
		) {
		this->remove(REMOVE_ALL);
		return;
	}

	auto& lag_records = lagcomp_data.m_lag_records;

	if (!onshot) {
		auto& record_newest = lag_records.newest();

		int cham_type = is_local ? GET_VAR(int, VISUALS_PATH(m_chams_local_type_bt)) : GET_VAR(int, VISUALS_PATH(m_chams_type_bt));

		int max_objects = static_cast<int>(this->m_objects.size());
		int max_records = static_cast<int>(lag_records.size());
		int size = (cham_type == e_backtrack_cham_type::backtrack_cham_all) ? std::min(max_objects, max_records) : SINGLE_OBJECT_INDEX + 1;
		for (int i = size; i < max_objects; i++)
			this->remove(i);

		for (int i = 0; i < size; i++) {
			if (i >= max_records)
				break;

			auto& object = this->m_objects[i];

			auto& record_current = lagcomp_data.m_lag_records[i];
			if (!record_current.is_valid()) {
				this->remove(i);
				continue;
			}

			if (record_current.m_origin.dist_to(record_newest.m_origin) < .25f || pawn->m_iHealth() <= 0) {
				this->remove(i);
				continue;
			}

			if (!create(pawn, i))
				continue;

			if (object && record_current.m_bones) {
				set_bones(object, record_current.m_bones, object->get_bone_count());
			}
		}
	}
	else {
		if (lag_records.empty())
			return;

		if (!create(pawn, SINGLE_OBJECT_INDEX))
			return;

		if (m_objects[SINGLE_OBJECT_INDEX]) {
			lag_record_t* best_record = nullptr;
			float best_distance = FLT_MAX;
			
			for (auto& record : lag_records) {
				if (!record.is_valid())
					continue;
					
				vec3_t record_pos = record.m_origin;
				record_pos.z += 64.0f;
				
				float distance = m_impact_position.dist_to(record_pos);
				if (distance < best_distance) {
					best_distance = distance;
					best_record = &record;
				}
			}
			
			if (best_record) {
				set_bones(m_objects[SINGLE_OBJECT_INDEX], best_record->m_bones, m_objects[SINGLE_OBJECT_INDEX]->get_bone_count());
			}
		}
	}
}
void cleanup_player_models( c_cs_player_pawn* pawn ) {
	if ( !pawn )
		return;

	auto bt_it = s_backtrack_models.find( pawn );
	if ( bt_it != s_backtrack_models.end( ) ) {
		bt_it->second.remove( REMOVE_ALL );
		s_backtrack_models.erase( bt_it );
	}

	auto os_it = s_onshot_models.find( pawn );
	if ( os_it != s_onshot_models.end( ) ) {
		for ( auto& model : os_it->second ) {
			model.remove( REMOVE_ALL );
		}
		s_onshot_models.erase( os_it );
	}
}
