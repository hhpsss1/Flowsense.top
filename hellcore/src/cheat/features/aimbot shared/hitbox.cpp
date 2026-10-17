#include "hitbox.h"

constexpr float K_EPSILON = 1e-6f;

float segment_to_segment_dist_sq(const vec3_t& p0, const vec3_t& p1, const vec3_t& q0, const vec3_t& q1) {
    const vec3_t u = p1 - p0, v = q1 - q0, w = p0 - q0;
    const float a = u.dot(u), b = u.dot(v), c = v.dot(v);
    const float d = u.dot(w), e = v.dot(w);
    float s = 0.f, t = 0.f;
    if (a <= K_EPSILON && c <= K_EPSILON)
        return w.dot(w);
    if (a <= K_EPSILON) {
        t = std::clamp(e / c, 0.f, 1.f);
    } else if (c <= K_EPSILON) {
        s = std::clamp(-d / a, 0.f, 1.f);
    } else {
        const float denominator = a * c - b * b;
        if (denominator > K_EPSILON * a * c)
            s = std::clamp((b * e - c * d) / denominator, 0.f, 1.f);
        t = (b * s + e) / c;
        if (t < 0.f) {
            t = 0.f;
            s = std::clamp(-d / a, 0.f, 1.f);
        } else if (t > 1.f) {
            t = 1.f;
            s = std::clamp((b - d) / a, 0.f, 1.f);
        }
    }
    const vec3_t separation = w + u * s - v * t;
    return separation.dot(separation);
}

bool c_hitbox_data::segment_intersects_capsule(const vec3_t& start, const vec3_t& end) {
    if (m_radius > 0.f)
        return segment_to_segment_dist_sq(start, end, m_mins, m_maxs) <= m_radius * m_radius;
    const vec3_t delta = end - start;
    const float origins[] = {start.x, start.y, start.z};
    const float directions[] = {delta.x, delta.y, delta.z};
    const float first[] = {m_mins.x, m_mins.y, m_mins.z};
    const float second[] = {m_maxs.x, m_maxs.y, m_maxs.z};
    float entry = 0.f, exit = 1.f;
    for (int axis = 0; axis < 3; ++axis) {
        const float low = std::min(first[axis], second[axis]);
        const float high = std::max(first[axis], second[axis]);
        if (std::abs(directions[axis]) <= K_EPSILON) {
            if (origins[axis] < low || origins[axis] > high) return false;
        } else {
            const float t0 = (low - origins[axis]) / directions[axis];
            const float t1 = (high - origins[axis]) / directions[axis];
            entry = std::max(entry, std::min(t0, t1));
            exit = std::min(exit, std::max(t0, t1));
            if (entry > exit) return false;
        }
    }
    return true;
}

float c_hitbox_data::projected_valid_radius(const vec3_t& view_point, const vec3_t& point_on_capsule) {

    const vec3_t vec_to_point = point_on_capsule - m_mins;
    float t = vec_to_point.dot(m_dir);
    t = std::clamp(t, 0.0f, m_axis_len);

    const vec3_t closest_point_on_axis = m_mins + m_dir * t;
    vec3_t radial = point_on_capsule - closest_point_on_axis;

    float radial_length_sqr = radial.length_sqr();
    if (radial_length_sqr < 1e-6f) {
        return 0.0f;
    }

    vec3_t view_dir = (point_on_capsule - view_point);
    float view_distance_sqr = view_dir.length_sqr();

    if (view_distance_sqr < 1e-6f) {
        return m_radius;
    }

    view_dir = view_dir.normalized();
    vec3_t radial_normalized = radial.normalized();

    vec3_t cross_product = radial_normalized.cross(view_dir);
    float sin_angle = cross_product.length();

    return std::min(m_radius * sin_angle, m_radius);
}