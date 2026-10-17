#include "vector.h"

matrix3x4_t vec4_t::to_matrix( const vec3_t& vecOrigin ) const {
    matrix3x4_t matOut;

#ifdef _DEBUG // precalculate common multiplications
    const float x2 = this->x + this->x, y2 = this->y + this->y, z2 = this->z + this->z;
    const float xx = this->x * x2, xy = this->x * y2, xz = this->x * z2;
    const float yy = this->y * y2, yz = this->y * z2;
    const float zz = this->z * z2;
    const float wx = this->w * x2, wy = this->w * y2, wz = this->w * z2;

    matOut[ 0 ][ 0 ] = 1.0f - ( yy + zz );
    matOut[ 1 ][ 0 ] = xy + wz;
    matOut[ 2 ][ 0 ] = xz - wy;

    matOut[ 0 ][ 1 ] = xy - wz;
    matOut[ 1 ][ 1 ] = 1.0f - ( xx + zz );
    matOut[ 2 ][ 1 ] = yz + wx;

    matOut[ 0 ][ 2 ] = xz + wy;
    matOut[ 1 ][ 2 ] = yz - wx;
    matOut[ 2 ][ 2 ] = 1.0f - ( xx + yy );
#else // let the compiler optimize calculations itself
    matOut[ 0 ][ 0 ] = 1.0f - 2.0f * this->y * this->y - 2.0f * this->z * this->z;
    matOut[ 1 ][ 0 ] = 2.0f * this->x * this->y + 2.0f * this->w * this->z;
    matOut[ 2 ][ 0 ] = 2.0f * this->x * this->z - 2.0f * this->w * this->y;

    matOut[ 0 ][ 1 ] = 2.0f * this->x * this->y - 2.0f * this->w * this->z;
    matOut[ 1 ][ 1 ] = 1.0f - 2.0f * this->x * this->x - 2.0f * this->z * this->z;
    matOut[ 2 ][ 1 ] = 2.0f * this->y * this->z + 2.0f * this->w * this->x;

    matOut[ 0 ][ 2 ] = 2.0f * this->x * this->z + 2.0f * this->w * this->y;
    matOut[ 1 ][ 2 ] = 2.0f * this->y * this->z - 2.0f * this->w * this->x;
    matOut[ 2 ][ 2 ] = 1.0f - 2.0f * this->x * this->x - 2.0f * this->y * this->y;
#endif

    matOut[ 0 ][ 3 ] = vecOrigin.x;
    matOut[ 1 ][ 3 ] = vecOrigin.y;
    matOut[ 2 ][ 3 ] = vecOrigin.z;
    return matOut;
}

vec3_t vec4_t::rotate_vector( const vec3_t & v ) const {

    vec4_t qv( v.x, v.y, v.z, 0.0f );

    vec4_t q_conj( -x, -y, -z, w );

    vec4_t temp;
    temp.w = w * qv.w - x * qv.x - y * qv.y - z * qv.z;
    temp.x = w * qv.x + x * qv.w + y * qv.z - z * qv.y;
    temp.y = w * qv.y - x * qv.z + y * qv.w + z * qv.x;
    temp.z = w * qv.z + x * qv.y - y * qv.x + z * qv.w;

    vec4_t result;
    result.w = temp.w * q_conj.w - temp.x * q_conj.x - temp.y * q_conj.y - temp.z * q_conj.z;
    result.x = temp.w * q_conj.x + temp.x * q_conj.w + temp.y * q_conj.z - temp.z * q_conj.y;
    result.y = temp.w * q_conj.y - temp.x * q_conj.z + temp.y * q_conj.w + temp.z * q_conj.x;
    result.z = temp.w * q_conj.z + temp.x * q_conj.y - temp.y * q_conj.x + temp.z * q_conj.w;

    return vec3_t( result.x, result.y, result.z );
}



vec3_t vec3_t::transform( const matrix3x4_t& transform ) const {
    return
    {
        this->dot( transform[ 0 ] ) + transform[ 0 ][ 3 ],
        this->dot( transform[ 1 ] ) + transform[ 1 ][ 3 ],
        this->dot( transform[ 2 ] ) + transform[ 2 ][ 3 ]
    };
}
