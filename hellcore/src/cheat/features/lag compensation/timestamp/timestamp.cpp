#include "timestamp.h"
#include <context.h>

timestamp_t::timestamp_t( const std::int32_t tick, const float frac ) : m_tick( tick ), m_frac( frac ) {
    if ( m_frac < 1.f ) {
        if ( m_frac <= 0.f )
            m_frac = 0.f;
    }
    else {
        m_tick++;
        m_frac = 0.f;
    }
}

timestamp_t::timestamp_t( float time ) {
    auto temp = std::fmodf( time, interval_per_tick );
    m_frac = temp * 64;
    m_tick = time_to_ticks( time - temp );
    normalize( );
}

void timestamp_t::normalize( ) {
    if ( m_frac < 1.f ) {
        if ( m_frac <= 0.f )
            m_frac = 0.f;
    }
    else {
        m_tick++;
        m_frac = 0.f;
    }
}

timestamp_t timestamp_t::operator-( const timestamp_t& other ) const {
    const auto frac_temp = m_frac - other.m_frac;
    auto frac = frac_temp;
    if ( frac_temp < 0.f ) {
        frac += 1.f;
    }
    auto tick = m_tick - other.m_tick - 1;
    if ( frac_temp >= 0.f )
        tick = m_tick - other.m_tick;
    return { tick, frac };
}

timestamp_t timestamp_t::operator+( const timestamp_t& other ) const {
    const auto frac_temp = m_frac + other.m_frac;
    auto frac = frac_temp;
    if ( frac_temp > 1.f ) {
        frac -= 1.f;
    }
    auto tick = m_tick + other.m_tick + 1;
    if ( frac_temp <= 1.f )
        tick = m_tick + other.m_tick;
    return { tick, frac };
}

bool timestamp_t::operator<( const timestamp_t& other ) const {
    if ( m_tick > other.m_tick )
        return false;
    if ( m_tick >= other.m_tick )
        return other.m_frac > m_frac;
    return false;
}

float timestamp_t::to_time( ) const {
    return static_cast<float>(m_tick) * interval_per_tick + m_frac * interval_per_tick;
}