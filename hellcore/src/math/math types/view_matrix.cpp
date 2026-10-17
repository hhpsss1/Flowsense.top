#include "view_matrix.h"

#include "vector.h"

vec3_t matrix2x4_t::get_origin( int index ) {
    return vec3_t( this[ index ]._11, this[ index ]._12, this[ index ]._13 );
}

inline void matrix2x4_t::set_origin( int index, vec3_t vec ) {
    this[ index ]._11 = vec.x;
    this[ index ]._12 = vec.y;
    this[ index ]._13 = vec.z;
}

void matrix3x4_t::set_forward( const vec3_t& vec_forward ) {
    arr_data[ 0 ][ 0 ] = vec_forward.x;
    arr_data[ 1 ][ 0 ] = vec_forward.y;
    arr_data[ 2 ][ 0 ] = vec_forward.z;
}

void matrix3x4_t::set_left( const vec3_t& vec_left ) {
    arr_data[ 0 ][ 1 ] = vec_left.x;
    arr_data[ 1 ][ 1 ] = vec_left.y;
    arr_data[ 2 ][ 1 ] = vec_left.z;
}
void matrix3x4_t::set_up( const vec3_t& vec_up ) {
    arr_data[ 0 ][ 2 ] = vec_up.x;
    arr_data[ 1 ][ 2 ] = vec_up.y;
    arr_data[ 2 ][ 2 ] = vec_up.z;
}

void matrix3x4_t::set_origin( const vec3_t& vec_origin ) {
    arr_data[ 0 ][ 3 ] = vec_origin.x;
    arr_data[ 1 ][ 3 ] = vec_origin.y;
    arr_data[ 2 ][ 3 ] = vec_origin.z;
}

vec3_t matrix3x4_t::forward( ) const {
    return { arr_data[ 0 ][ 0 ], arr_data[ 1 ][ 0 ], arr_data[ 2 ][ 0 ] };
}

vec3_t matrix3x4_t::left( ) const {
    return { arr_data[ 0 ][ 1 ], arr_data[ 1 ][ 1 ], arr_data[ 2 ][ 1 ] };
}

vec3_t matrix3x4_t::up( ) const {
    return { arr_data[ 0 ][ 2 ], arr_data[ 1 ][ 2 ], arr_data[ 2 ][ 2 ] };
}

vec3_t matrix3x4_t::origin( ) const {
    return { arr_data[ 0 ][ 3 ], arr_data[ 1 ][ 3 ], arr_data[ 2 ][ 3 ] };
}