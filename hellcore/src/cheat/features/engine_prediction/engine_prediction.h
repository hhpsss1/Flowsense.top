#pragma once

#include <cheat/features/entity cache/entity_cache.h>
#include <context.h>
#include <sdk/interfaces/csgo_input.h>
#include <cheat/features/lag compensation/timestamp/timestamp.h>

class c_prediction_data
{
public:
    float m_real_time;
    int32_t m_frame_count;
    float m_frame_time;
    float m_frame_time2;
    float m_curtime;
    float m_render_time;
    float m_client_tick_fraction;
    float m_next_tick_fraction;
    int32_t m_tick_count;

    bool m_in_prediction;
    bool m_first_prediction;
    bool m_has_been_predicted;
    bool m_should_predict;

    int64_t m_pre_prediction_flags;
    int64_t m_post_prediction_flags;

    vec3_t m_pre_abs_origin;
    vec3_t m_abs_velocity;
    vec3_t m_velocity;
};

class c_engine_prediction
{
public:
    c_prediction_data m_prediction_data;

    void begin();
    void end();
    void calculate_shoot_position();

    force_inline bool is_definitely_on_ground() {
        return (m_prediction_data.m_pre_prediction_flags & FL_ONGROUND) && (m_prediction_data.m_post_prediction_flags & FL_ONGROUND);
    }

    force_inline bool is_probably_on_ground() {
        return (m_prediction_data.m_pre_prediction_flags & FL_ONGROUND) || (m_prediction_data.m_post_prediction_flags & FL_ONGROUND);
    }

    force_inline bool is_in_air() {
        return !(m_prediction_data.m_pre_prediction_flags & FL_ONGROUND) && !(m_prediction_data.m_post_prediction_flags & FL_ONGROUND);
    }

    force_inline bool just_hit_ground() {
        return !(m_prediction_data.m_pre_prediction_flags & FL_ONGROUND) && (m_prediction_data.m_post_prediction_flags & FL_ONGROUND);
    }
};

inline auto g_prediction = std::make_unique<c_engine_prediction>();