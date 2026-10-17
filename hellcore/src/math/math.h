#pragma once
#include <utils/encryption/encrypted_ptr.h>

#include <includes.h>
#include "math types/view_matrix.h"

constexpr double A_PI = 3.14159265358979323846;
constexpr double A_2PI = A_PI * 2.f;
constexpr double A_HALFPI = A_PI / 2.f;

constexpr double A_RAD2DEG = 180.0 / A_PI;
constexpr double A_DEG2RAD = A_PI / 180.0;

constexpr float DEG2RAD( float x ) {
    return x * A_DEG2RAD;
}

constexpr float RAD2DEG( float x ) {
    return x * A_RAD2DEG;
}

class c_transform
{
public:
    vec3_aligned_t m_vec_position;
    vec4_t m_quat_orientation;
};

class c_math {
public:
    v_matrix m_viewmatrix;

    bool screen_transform( const vec3_t& source, v_matrix& matrix, vec2_t& output );
    bool world_to_screen( const vec3_t& pos, vec2_t& out );
    vec3_t calculate_angles(vec3_t view_pos, vec3_t pos);
    vec3_t calculate_angle( const vec3_t& origin, const vec3_t& destination );
    vec3_t calculate_camera_position( vec3_t anchor, float distance, qangle_t view_angles );
    void angle_vectors(const vec3_t& angles, vec3_t& forward);
    void angle_vectors( const vec3_t& angles, vec3_t* forward, vec3_t* right = nullptr, vec3_t* up = nullptr );
    void angle_vectors_new(const vec3_t& angles, vec3_t* forward, vec3_t* right = nullptr, vec3_t* up = nullptr);
    float normalize_float( float value );
    void vector_angles( const vec3_t& forward, qangle_t& angles );
    qangle_t vector_to_angle(const vec3_t& vForward);
    float angle_diff( float a, float b );
    float rand_float( float min, float max );
    vec3_t extrapolate_tick(const vec3_t& p0, const vec3_t& v0, const int ticks);
};

inline auto g_math = std::make_unique<c_math>( );