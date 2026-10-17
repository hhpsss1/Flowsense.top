#pragma once

#include <includes.h>

class matrix3x4_t;
class vec3_t {
public:
    vec3_t( ) = default;
    vec3_t( float x, float y, float z ) : x( x ), y( y ), z( z ) { }

    void init( )
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    bool is_zero( )
    {
        return x == 0.0f && y == 0.0f && z == 0.0f;
    }

    void init( float x, float y, float z )
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    vec3_t( const float* v )
    {
        this->x = v[ 0 ];
        this->y = v[ 1 ];
        this->z = v[ 2 ];
    }

    float& operator[]( int i )
    {
        return ( ( float* )this )[ i ];
    }

    float operator[]( int i ) const
    {
        return ( ( float* )this )[ i ];
    }

    void zero( )
    {
        this->x = this->y = this->z = 0.0f;
    }

    bool operator==( const vec3_t& src ) const
    {
        return ( src.x == this->x ) && ( src.y == y ) && ( src.z == z );
    }

    bool operator!=( const vec3_t& src ) const
    {
        return ( src.x != this->x ) || ( src.y != y ) || ( src.z != z );
    }

    vec3_t& operator+=( const vec3_t& v )
    {
        this->x += v.x;
        this->y += v.y;
        this->z += v.z;

        return *this;
    }

    vec3_t& operator-=( const vec3_t& v )
    {
        this->x -= v.x;
        this->y -= v.y;
        this->z -= v.z;

        return *this;
    }

    vec3_t& operator*=( float fl )
    {
        this->x *= fl;
        this->y *= fl;
        this->z *= fl;

        return *this;
    }

    vec3_t& operator*=( const vec3_t& v )
    {
        this->x *= v.x;
        this->y *= v.y;
        this->z *= v.z;

        return *this;
    }

    vec3_t& operator/=( const vec3_t& v )
    {
        this->x /= v.x;
        this->y /= v.y;
        this->z /= v.z;

        return *this;
    }

    vec3_t& operator+=( float fl )
    {
        this->x += fl;
        this->y += fl;
        this->z += fl;

        return *this;
    }

    vec3_t& operator/=( float fl )
    {
        this->x /= fl;
        this->y /= fl;
        this->z /= fl;

        return *this;
    }

    vec3_t& operator-=( float fl )
    {
        this->x -= fl;
        this->y -= fl;
        this->z -= fl;

        return *this;
    }

    vec3_t lerp(const vec3_t& other, float t) const {
        return vec3_t(x + t * (other.x - x),
            y + t * (other.y - y),
            z + t * (other.z - z));
    }

    float length_2d( ) const
    {
        return std::sqrtf( this->x * this->x + this->y * this->y );
    }

    float length( ) const
    {
        return std::sqrtf( this->x * this->x + this->y * this->y + this->z * this->z );
    }

    void to_directions( vec3_t* vec_forward, vec3_t* vec_right, vec3_t* vec_up ) const {
        float pitch_rad = this->x * 0.0174532925f;
        float yaw_rad =  this->y * 0.0174532925f;
        float roll_rad =  this->z * 0.0174532925f;

        float sp = std::sin( pitch_rad ), cp = std::cos( pitch_rad );
        float sy = std::sin( yaw_rad ), cy = std::cos( yaw_rad );
        float sr = std::sin( roll_rad ), cr = std::cos( roll_rad );

        if ( vec_forward != nullptr ) {
            vec_forward->x = cp * cy;
            vec_forward->y = cp * sy;
            vec_forward->z = -sp;
        }

        if ( vec_right != nullptr ) {
            vec_right->x = (-sr * sp * cy) + (-cr * -sy);
            vec_right->y = (-sr * sp * sy) + (-cr * cy);
            vec_right->z = -sr * cp;
        }

        if ( vec_up != nullptr ) {
            vec_up->x = (cr * sp * cy) + (-sr * -sy);
            vec_up->y = (cr * sp * sy) + (-sr * cy);
            vec_up->z = cr * cp;
        }
    }


    vec3_t transform( const matrix3x4_t& transform ) const;

    //vec3_t transform( matrix3x4_t& matTransform );

    float dist_to( const vec3_t& other )
    {
        vec3_t delta{ };

        delta.x = this->x - other.x;
        delta.y = this->y - other.y;
        delta.z = this->z - other.z;

        return delta.length( );
    }

     float distance_to(const vec3_t& other) const
    {
        vec3_t delta{ };

        delta.x = this->x - other.x;
        delta.y = this->y - other.y;
        delta.z = this->z - other.z;

        return delta.length();
    }
    float dist_to_sq( const vec3_t& other ) const
    {
        vec3_t delta;
        delta.x = this->x - other.x;
        delta.y = this->y - other.y;
        delta.z = this->z - other.z;

        return delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    }

    void normalize_angle( )
    {
        this->x = std::isfinite( this->x ) ? std::remainder( this->x, 360.f ) : 0.f;
        this->y = std::isfinite( this->y ) ? std::remainder( this->y, 360.f ) : 0.f;
        this->z = 0.f;
    }


    float normalize( )
    {
        float length = this->length( );

        if ( length != 0.0f ) {
            this->x /= length + std::numeric_limits< float >::epsilon( );
            this->y /= length + std::numeric_limits< float >::epsilon( );
            this->z /= length + std::numeric_limits< float >::epsilon( );
        }

        return length;
    }

    float normalize_lenght( ) {
        const float len = length( );
        if ( len > 0.f ) {
            x /= len;
            y /= len;
            z /= len;
        }
        return len;
    }

    float normalize_( ) {
        float length = normalize_lenght( );
        if ( length != 0.0f ) {
            x /= length;
            y /= length;
            z /= length;
        }
        return length;
    }

    float normalize_place( )
    {
        auto radius = std::sqrtf( x * x + y * y + z * z );
        auto iradius = 1.0f / ( radius + std::numeric_limits< float >::epsilon( ) );

        x *= iradius;
        y *= iradius;
        z *= iradius;

        return radius;
    }

    vec3_t& clamp( )
    {
        x = std::clamp( x, -89.f, 89.f );
        y = std::clamp( std::remainder( y, 360.0f ), -180.f, 180.f );
        z = 0.f;

        return *this;
    }

    vec3_t normalized( ) const
    {
        auto res = *this;
        auto l = res.length( );

        if ( l != 0.0f )
            res /= l;
        else
            res.x = res.y = res.z = 0.0f;

        return res;
    }

    vec3_t cross( const vec3_t& other ) const
    {
        return { this->y * other.z - this->z * other.y, this->z * other.x - this->x * other.z,
                 this->x * other.y - this->y * other.x };
    }

    float dot( const vec3_t& vec, const bool additional = false ) const
    {
        if ( additional )
            return this->x * vec.y + this->y * vec.x + this->z * vec.z;

        return this->x * vec.x + this->y * vec.y + this->z * vec.z;
    }


    float length_2d_sqr( ) const
    {
        return this->x * this->x + this->y * this->y;
    }

    float length_sqr( ) const
    {
        return this->x * this->x + this->y * this->y + this->z * this->z;
    }

    vec3_t to_vectors( ) const {
        float pitch_rad = x * 0.0174532925f;
        float yaw_rad = y * 0.0174532925f;

        float cp = std::cos( pitch_rad );
        float sp = std::sin( pitch_rad );
        float cy = std::cos( yaw_rad );
        float sy = std::sin( yaw_rad );

        return vec3_t( cp * cy, cp * sy, -sp );
    }


    float x, y, z;

    vec3_t& operator=( const vec3_t& vec )
    {
        this->x = vec.x;
        this->y = vec.y;
        this->z = vec.z;

        return *this;
    }

    vec3_t operator-( ) const
    {
        return vec3_t( -this->x, -this->y, -this->z );
    }

    vec3_t operator+( const vec3_t& v ) const
    {
        return vec3_t( this->x + v.x, this->y + v.y, this->z + v.z );
    }

    vec3_t operator-( const vec3_t& v ) const
    {
        return vec3_t( this->x - v.x, this->y - v.y, this->z - v.z );
    }

    vec3_t operator*( float fl ) const
    {
        return vec3_t( this->x * fl, this->y * fl, this->z * fl );
    }

    vec3_t operator*( const vec3_t& v ) const
    {
        return vec3_t( this->x * v.x, this->y * v.y, this->z * v.z );
    }

    vec3_t operator/( float fl ) const
    {
        return vec3_t( this->x / fl, this->y / fl, this->z / fl );
    }

    vec3_t operator/( const vec3_t& v ) const
    {
        return vec3_t( this->x / v.x, this->y / v.y, this->z / v.z );
    }
};

class __declspec( align( 16 ) ) vec3_aligned_t : public vec3_t {
public:
    vec3_aligned_t( ) = default;

    explicit vec3_aligned_t( const vec3_t& vecBase ) {
        this->x = vecBase.x; this->y = vecBase.y; this->z = vecBase.z; this->w = 0.f;
    }

    constexpr vec3_aligned_t& operator=( const vec3_t& vecBase ) {
        this->x = vecBase.x; this->y = vecBase.y; this->z = vecBase.z; this->w = 0.f;
        return *this;
    }

public:
    float w;
};

class vec4_t {
public:
    matrix3x4_t to_matrix( const vec3_t& vecOrigin = {} ) const;
    vec3_t rotate_vector( const vec3_t& v ) const;
    float x, y, z, w;
};

class vec2_t {
public:
    vec2_t( ) = default;
    vec2_t( float x, float y ) : x( x ), y( y ) { }

    void init( )
    {
        x = 0.0f;
        y = 0.0f;
    }

    void init( float x, float y )
    {
        this->x = x;
        this->y = y;
    }

    bool is_zero( ) const
    {
        return x == 0.0f && y == 0.0f;
    }

    float& operator[]( int i )
    {
        return ( ( float* )this )[ i ];
    }

    float operator[]( int i ) const
    {
        return ( ( float* )this )[ i ];
    }

    void zero( )
    {
        this->x = this->y = 0.0f;
    }

    bool operator==( const vec2_t& src ) const
    {
        return ( src.x == this->x ) && ( src.y == y );
    }

    bool operator!=( const vec2_t& src ) const
    {
        return ( src.x != this->x ) || ( src.y != y );
    }

    vec2_t& operator+=( const vec2_t& v )
    {
        this->x += v.x;
        this->y += v.y;
        return *this;
    }

    vec2_t& operator-=( const vec2_t& v )
    {
        this->x -= v.x;
        this->y -= v.y;
        return *this;
    }

    vec2_t& operator*=( float fl )
    {
        this->x *= fl;
        this->y *= fl;
        return *this;
    }

    vec2_t& operator*=( const vec2_t& v )
    {
        this->x *= v.x;
        this->y *= v.y;
        return *this;
    }

    vec2_t& operator/=( const vec2_t& v )
    {
        this->x /= v.x;
        this->y /= v.y;
        return *this;
    }

    vec2_t& operator+=( float fl )
    {
        this->x += fl;
        this->y += fl;
        return *this;
    }

    vec2_t& operator/=( float fl )
    {
        this->x /= fl;
        this->y /= fl;
        return *this;
    }

    vec2_t& operator-=( float fl )
    {
        this->x -= fl;
        this->y -= fl;
        return *this;
    }

    float length( ) const
    {
        return std::sqrtf( this->x * this->x + this->y * this->y );
    }

    float length_sqr( ) const
    {
        return this->x * this->x + this->y * this->y;
    }

    float dist_to( const vec2_t& other ) const
    {
        vec2_t delta;
        delta.x = this->x - other.x;
        delta.y = this->y - other.y;
        return delta.length( );
    }

    void normalize( )
    {
        float length = this->length( );
        if ( length != 0.0f )
        {
            this->x /= length + std::numeric_limits<float>::epsilon( );
            this->y /= length + std::numeric_limits<float>::epsilon( );
        }
    }

    float normalize_place( )
    {
        float radius = std::sqrtf( x * x + y * y );
        float iradius = 1.0f / ( radius + std::numeric_limits<float>::epsilon( ) );
        x *= iradius;
        y *= iradius;
        return radius;
    }

    vec2_t normalized( ) const
    {
        auto res = *this;
        auto l = res.length( );
        if ( l != 0.0f )
            res /= l;
        else
            res.x = res.y = 0.0f;
        return res;
    }

    bool invalid( ) const {
        return x == -1 && y == -1;
    }

    float dot( const vec2_t& vec ) const
    {
        return this->x * vec.x + this->y * vec.y;
    }

    vec2_t operator-( ) const
    {
        return vec2_t( -this->x, -this->y );
    }

    vec2_t operator+( const vec2_t& v ) const
    {
        return vec2_t( this->x + v.x, this->y + v.y );
    }

    vec2_t operator-( const vec2_t& v ) const
    {
        return vec2_t( this->x - v.x, this->y - v.y );
    }

    vec2_t operator*( float fl ) const
    {
        return vec2_t( this->x * fl, this->y * fl );
    }

    vec2_t operator*( const vec2_t& v ) const
    {
        return vec2_t( this->x * v.x, this->y * v.y );
    }

    vec2_t operator/( float fl ) const
    {
        return vec2_t( this->x / fl, this->y / fl );
    }

    vec2_t operator/( const vec2_t& v ) const
    {
        return vec2_t( this->x / v.x, this->y / v.y );
    }

    vec2_t& operator=( const vec2_t& vec )
    {
        this->x = vec.x;
        this->y = vec.y;
        return *this;
    }

    float x, y;
};

typedef vec3_t qangle_t;