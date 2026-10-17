#pragma once

#include <includes.h>
#include <utils/fonts/font_manager.h>
#include <cheat/features/visuals/grenade.h>
#include <sdk/entity/entity.h>

struct cached_player_t;
class c_cs_player_controller;
class c_cs_player_pawn;

class c_visuals {
public:
	enum class e_anchor_type : unsigned int {
		top_left = 0,
		top_center,
		top_right,
		mid_left,
		mid_center,
		mid_right,
		bottom_left,
		bottom_center,
		bottom_right
	};
	
	enum e_element_bb_position : unsigned int {
		left = 0,
		top,
		right,
		bottom,
		max
	};

	enum e_text_shadow_type : unsigned int {
		text_shadow_none = 0,
		text_shadow_drop,
		text_shadow_full
	};

	enum e_text_alignment : unsigned int {
		text_alignment_left = 0,
		text_alignment_center,
		text_alignment_right,
	};

	enum e_font_type : unsigned int {
		font_type_smallest_pixel = 0,
		font_type_verdana,
		font_type_verdana_bold,
		font_type_calibri,
		font_type_tahoma
	};

	ImFont* get_font( e_font_type font_type ) {
		switch ( font_type ) {
		case e_font_type::font_type_smallest_pixel:
			return g_font_manager->m_smallest_pixel;
		case e_font_type::font_type_verdana:
			return g_font_manager->m_verdana_12;
		case e_font_type::font_type_verdana_bold:
			return g_font_manager->m_verdana_12_bold;
		case e_font_type::font_type_calibri:
			return g_font_manager->m_calibri_14;
		case e_font_type::font_type_tahoma:
			return g_font_manager->m_tahoma_bd_12;
		}
	}

	struct bb_t {
		hellvec2 m_min;
		hellvec2 m_max;
		bb_t( hellvec2 min, hellvec2 max ) : m_min( min ), m_max( max ) { }

		bool m_oob = false;
		hellcolor m_bb_fg_color;
		hellcolor m_bb_bg_color;

		hellvec2 get_center( ) const {
			return ( m_min + m_max ) * 0.5f;
		}

		bb_t normalize( ) {
			if ( m_min.x > m_max.x ) std::swap( m_min.x, m_max.x );
			if ( m_min.y > m_max.y ) std::swap( m_min.y, m_max.y );
			return *this;
		}

		void shrink( float pixels ) {
			m_min.x += pixels;
			m_min.y += pixels;

			m_max.x -= pixels;
			m_max.y -= pixels;
		}

		void expand( float pixels ) {
			m_min.x -= pixels;
			m_min.y -= pixels;

			m_max.x += pixels;
			m_max.y += pixels;
		}

		void shrink( float x, float y ) {
			m_min.x += x;
			m_min.y += y;

			m_max.x -= x;
			m_max.y -= y;
		}

		void expand( float x, float y ) {
			m_min.x -= x;
			m_min.y -= y;

			m_max.x += x;
			m_max.y += y;
		}

		void align( ) {
			m_min.x = floor( m_min.x );
			m_min.y = floor( m_min.y );

			m_max.x = floor( m_max.x );
			m_max.y = floor( m_max.y );
		}

		void set_size( float width, float height ) {
			hellvec2 center = get_center( );

			if ( width > 0.f ) {
				float half_width = width * 0.5f;
				m_min.x = center.x - half_width;
				m_max.x = center.x + half_width;
			}

			if ( height > 0.f ) {
				float half_height = height * 0.5f;
				m_min.y = center.y - half_height;
				m_max.y = center.y + half_height;
			}
		}

		hellvec2 get_anchor( e_anchor_type anchor_type, hellvec2 offset = { 0.f, 0.f } ) const;
	};

	/*
	draw_text(
            g_font_manager->m_verdana_12,
            data.m_player_name,
            this->get_element_bb( data.m_bb, e_element_bb_position::top, 1, true ),
            data.m_name_color.AlphaOverride( data.m_alpha_modifier ),
            data.m_name_color_bg.AlphaOverride( data.m_alpha_modifier ),
            e_text_shadow_type::text_shadow_drop,
            data.m_draw_list
        );
	*/
	struct text_object_t {
		const char* m_text;
		e_text_shadow_type m_text_shadow_type;
		e_text_alignment m_text_alignment = e_text_alignment::text_alignment_center;

		e_font_type m_font_type = e_font_type::font_type_smallest_pixel;

		e_element_bb_position m_element_position = e_element_bb_position::bottom;
		int m_element_index = 1;

		int m_size = 5;
		int m_padding = 8;
		float m_text_offset_x = 0.0f;

		hellcolor m_fg_color = { 255, 255, 255, 255 };
		hellcolor m_bg_color = { 0  , 0  , 0  , 180 };
	};

	struct bar_object_t {
		int m_value = 50;

		bool m_number_value_enabled = true;
		text_object_t m_number_value_text;

		e_element_bb_position m_element_position = e_element_bb_position::left;
		int m_element_index = 1;

		int m_size = 1.f;
		int m_padding = 7;

		hellcolor m_fg_color = { 255, 255, 255, 255 };
		hellcolor m_bg_color = { 0  , 0  , 0  , 180 };

		bool m_glow_enabled = false;
		float m_glow_intensity = 1.0f;
		bool m_gradient_enabled = false;
		hellcolor m_gradient_color = { 255, 255, 255, 255 };
		bool m_gradient_reverse = false;
	};

	struct icon_object_t {
		e_element_bb_position m_element_position = e_element_bb_position::bottom;
		int m_element_index = 1;
		int m_size = 16;
		int m_padding = 8;
		hellcolor m_color = { 255, 255, 255, 255 };
	};

	struct player_visual_data_t {
		player_visual_data_t( ) = default;

		ImDrawList* m_draw_list;

		bb_t m_bb{ { }, { } };
		cached_player_t* m_cached_entity;
		c_cs_player_controller* m_controller;
		c_cs_player_pawn* m_pawn;

		text_object_t m_name_text;
		text_object_t m_weapon_text;
		icon_object_t m_weapon_icon;
		icon_object_t m_grenade_icons;

		bar_object_t m_armor_bar;
		bar_object_t m_health_bar;

		std::string m_weapon_name_storage;
		std::string m_ping_storage;
		uint64_t m_steam_id;

		float m_alpha_modifier = 1.f;

		bool m_preview_mode = false;
	};
private:
	hellvec2 m_outline_pos_arr[ 8 ] = {
			{ -1, -1 },
			{ -1, 1 },
			{ 1, -1 },
			{ 1, 1 },
			{ 0, 1 },
			{ 1, 0 },
			{ 0, -1 },
			{ -1, 0 },
	};
public:
	bool get_bb( c_cs_player_pawn* entity, bb_t& bb );
	bb_t get_element_bb( bb_t anchor, e_element_bb_position position, int row, float padding, float size );

	void draw_bar( bb_t owner_bb, bar_object_t& obj, float alpha_modifier, ImDrawList* draw_list = nullptr );
	
	void draw_text( bb_t owner_bb, text_object_t& obj, float alpha_modifier, ImDrawList* draw_list = nullptr );

	void draw_icon( bb_t owner_bb, icon_object_t& obj, const icon_data_t& icon_data, float alpha_modifier, ImDrawList* draw_list = nullptr );
	void draw_grenade_icons( bb_t owner_bb, icon_object_t& obj, const std::vector<icon_data_t>& icons, float alpha_modifier, ImDrawList* draw_list = nullptr );

	void draw_skeleton( c_cs_player_pawn* pawn, hellcolor col_color, hellcolor col_outline, ImDrawList* draw_list = nullptr );
	void draw_skeleton_backtrack( c_cs_player_pawn* pawn, ImDrawList* draw_list, float alpha_modifier );
	void draw_skeleton_onshot( c_cs_player_pawn* pawn, ImDrawList* draw_list, float alpha_modifier );

	struct skeleton_onshot_data_t {
		bone_data_t m_bones[128];
		int m_bone_count;
		float m_draw_begin_time;
		float m_target_simtime;
	};

	struct skeleton_backtrack_data_t {
		bone_data_t m_bones[128];
		int m_bone_count;
		float m_simulation_time;
	};

	struct backtrack_skeleton_cache_t {
		std::vector<skeleton_backtrack_data_t> m_skeletons;
		float m_last_update_time = 0.0f;
	};

	void create_onshot_skeleton_data(c_cs_player_pawn* pawn);
	void prune_old_onshot_skeletons(c_cs_player_pawn* pawn);
	void cleanup_onshot_skeletons(c_cs_player_pawn* pawn);

	void draw_player_visual_data( player_visual_data_t& data );
	void draw_custom_scope( );
	void present( );
private:
	void draw_player_flags( player_visual_data_t& data );
	void draw_player_flags_runtime( player_visual_data_t& data );
	void draw_player_flags_preview( player_visual_data_t& data );
};

inline std::unordered_map<c_cs_player_pawn*, std::vector<c_visuals::skeleton_onshot_data_t>> s_onshot_skeletons;
inline std::unordered_map<c_cs_player_pawn*, c_visuals::backtrack_skeleton_cache_t> s_backtrack_skeletons;

inline auto g_visuals = std::make_unique<c_visuals>( );