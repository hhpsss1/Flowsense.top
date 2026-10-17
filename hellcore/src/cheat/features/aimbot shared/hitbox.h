#pragma once
#include <math/math types/vector.h>
#include <context.h>
#include <sdk/constants.h>
#include <utils/tight_array.h>

enum hitbox_orientation {
    hitbox_vertical = 0,
    hitbox_horizontal,
    hitbox_head_identity // kinda annoying but whatever
};

class c_hitbox_data {
public:
    hitbox_orientation m_orientation;
	vec3_t m_mins, m_maxs, m_center;
	float m_radius;
	int m_hitbox_index, m_hit_group;
	float m_pointscale; // 0.0 - 1.0
	bool m_multipoint; // for ragenz

	vec3_t   m_axis;
	vec3_t   m_dir;
	float    m_axis_len;
	float    m_axis_len_sqr;
	float    m_half_height;
	float    m_capsule_profile_length;

	bool segment_intersects_capsule( const vec3_t& start, const vec3_t& end );
	float projected_valid_radius( const vec3_t& view_point, const vec3_t& point_on_capsule );
};

static c_hitbox_data construct_hitbox_data( const c_hitbox& hitbox, const bone_data_t& bone_data , const bool& multipoint, const int& pointscale ) {
	c_hitbox_data data = {};
	data.m_radius = hitbox.m_shape_radius * std::abs(bone_data.m_scale);
	data.m_hitbox_index = hitbox.m_hitbox_index;
	data.m_mins = bone_data.m_rot.rotate_vector( hitbox.m_min_bounds * bone_data.m_scale ) + bone_data.m_pos;
	data.m_maxs = bone_data.m_rot.rotate_vector( hitbox.m_max_bounds * bone_data.m_scale ) + bone_data.m_pos;
	data.m_center = ( data.m_mins + data.m_maxs ) / 2.f;
	data.m_multipoint = multipoint;
	data.m_pointscale = static_cast<float>(pointscale) / 100.f;
	vec3_t axis = data.m_maxs - data.m_mins;
	data.m_axis = axis;
	data.m_axis_len = axis.length( );
	data.m_axis_len_sqr = axis.length_sqr( );
	data.m_dir = axis.is_zero( ) ? vec3_t( 0, 0, 1 ) : axis.normalized( );
	data.m_half_height = data.m_axis_len * 0.5f;
	data.m_capsule_profile_length = data.m_axis_len + A_PI * (data.m_pointscale * data.m_radius);
    data.m_hit_group = hitbox_to_hitgroup_table[data.m_hitbox_index];

    const float vertical = std::abs( data.m_dir.z );
    const float horizontal = std::sqrt( data.m_dir.x * data.m_dir.x + data.m_dir.y * data.m_dir.y );

    if (data.m_hitbox_index == HITBOX_HEAD) {
        data.m_orientation = hitbox_orientation::hitbox_head_identity;
    }
    else {
        const float vertical = std::abs( data.m_dir.z );
        const float horizontal = std::sqrt( data.m_dir.x * data.m_dir.x + data.m_dir.y * data.m_dir.y );

        data.m_orientation = (vertical > horizontal)
            ? hitbox_orientation::hitbox_vertical
            : hitbox_orientation::hitbox_horizontal;
    }

    return data;
}


struct multipoint_params_t {
    float cos_angle, sin_angle;
    float path_pos;
    bool is_bottom_hemi;
    bool is_cylinder;
};

static const std::array<multipoint_params_t, 64> k_multipoint_params = [] {
    std::array<multipoint_params_t, 64> params{};

    constexpr int num_columns = 7;
    constexpr int num_rows = (64 + num_columns - 1) / num_columns;

    int index = 0;
    for (int i = 0; i < num_columns; ++i) {
        float u_param = (num_columns > 1) ? static_cast<float>(i) / (num_columns - 1) : 0.5f;
        float angle = -A_HALFPI + (u_param * A_PI);
        float cos_angle = std::cosf( angle );
        float sin_angle = std::sinf( angle );

        for (int j = 0; j < num_rows && index < 64; ++j) {
            float v_param = (num_rows > 1) ? static_cast<float>( j ) / (num_rows - 1) : 0.5f;
            float path_pos = v_param;

            multipoint_params_t& p = params[index++];
            p.cos_angle = cos_angle;
            p.sin_angle = sin_angle;
            p.path_pos = path_pos;
        }
    }

    return params;
    }();

static void generate_head_points( c_hitbox_data* hitbox, tight_array<vec3_t, 64>& points, const vec3_t& view_point ) {

    vec3_t view_dir = view_point - hitbox->m_center;
    vec3_t front_dir, side_dir;
    vec3_t v_proj = view_dir - hitbox->m_dir * view_dir.dot( hitbox->m_dir );

    if (v_proj.length_sqr( ) < 1e-8f) {
        vec3_t tmp = fabsf( hitbox->m_dir.z ) < 0.99f ? vec3_t( 0, 0, 1 ) : vec3_t( 1, 0, 0 );
        side_dir = hitbox->m_dir.cross( tmp ).normalized( );
        front_dir = side_dir.cross( hitbox->m_dir );
    }
    else {
        front_dir = v_proj.normalized( );
        side_dir = hitbox->m_dir.cross( front_dir ).normalized( );
    }

    const float radius = hitbox->m_pointscale * hitbox->m_radius;
    const float bottom_arc = A_HALFPI * radius;
    const float total_len = bottom_arc * 2.0f + hitbox->m_axis_len;

    const vec3_t bottom_center = hitbox->m_center - hitbox->m_dir * hitbox->m_half_height;
    const vec3_t top_center = hitbox->m_center + hitbox->m_dir * hitbox->m_half_height;

    for (int i = 0; i < 64; ++i) {
        const auto& p = k_multipoint_params[i];
        vec3_t horiz_dir = front_dir * p.cos_angle + side_dir * p.sin_angle;
        float scaled_path = p.path_pos * total_len;

        vec3_t candidate;
        if (scaled_path < bottom_arc) {
            float phi = scaled_path / radius;
            vec3_t offset = horiz_dir * (cosf( phi ) * radius) - hitbox->m_dir * (sinf( phi ) * radius);
            candidate = bottom_center + offset;
        }
        else if (scaled_path < bottom_arc + hitbox->m_axis_len) {
            float z = scaled_path - bottom_arc;
            vec3_t point_on_axis = bottom_center + hitbox->m_dir * z;
            candidate = point_on_axis + horiz_dir * radius;
        }
        else {
            float phi = (scaled_path - (bottom_arc + hitbox->m_axis_len)) / radius;
            vec3_t offset = horiz_dir * (cosf( phi ) * radius) + hitbox->m_dir * (sinf( phi ) * radius);
            candidate = top_center + offset;
        }

        points.emplace_back( candidate );
    }

}
static void generate_horizontal_capsule_points( c_hitbox_data* hitbox, tight_array<vec3_t, 12>& points, const vec3_t& view_point ) {

    vec3_t front_dir = (hitbox->m_center - view_point).normalized( );
    vec3_t side_dir = vec3_t( -front_dir.y, front_dir.x, 0 ).normalized( );

    const float radius = hitbox->m_pointscale * hitbox->m_radius;
    const vec3_t bottom_center = hitbox->m_center - hitbox->m_dir * hitbox->m_half_height;
    const vec3_t top_center = hitbox->m_center + hitbox->m_dir * hitbox->m_half_height;

    constexpr int half_circle_segments = 6; 
    const float step = A_PI / float( half_circle_segments - 1 ); 

    for (int i = 0; i < half_circle_segments; ++i) {
        float theta = -A_HALFPI + step * i; 

        vec3_t circle_dir = front_dir * cosf( theta ) + side_dir * sinf( theta );

        points.emplace_back( bottom_center + circle_dir * radius );
        points.emplace_back( top_center - circle_dir * radius );
    }
}

static void generate_vertical_capsule_points( c_hitbox_data* hitbox, tight_array<vec3_t, 2>& points ) {
    const float offset = hitbox->m_pointscale * hitbox->m_radius;
    const vec3_t offset_vec = hitbox->m_dir * offset;

    points.emplace_back( hitbox->m_center + offset_vec );
    points.emplace_back( hitbox->m_center - offset_vec );
}
