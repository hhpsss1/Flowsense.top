#pragma once
#include <cstdint>

struct timestamp_t {
    std::int32_t m_tick{ };
    float m_frac{ };

    timestamp_t( std::int32_t tick, float frac );

    timestamp_t( float time );

    void normalize( );

    timestamp_t operator-( const timestamp_t& other ) const;

    timestamp_t operator+( const timestamp_t& other ) const;

    bool operator<( const timestamp_t& ) const;

    float to_time( ) const;
};