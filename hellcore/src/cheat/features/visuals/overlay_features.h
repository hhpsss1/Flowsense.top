#pragma once
#include "math.h"
#include <cheat/config/vars.h>
#include <utils/fonts/font_manager.h>
#include "cheat/features/aimbot shared/hitbox.h"
#include <sdk/constants.h>
#include <sdk/datatypes/fnv1a.h>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <mutex>
#include <chrono>
#include <cheat/features/ragebot/shotlogger.h>

class c_cs_player_pawn;

class c_overlay {
public:
    struct hit_text_t {
        vec3_t m_world_origin;
        int m_damage;
        float m_spawn_time;
        float m_lifetime;
    };

    struct hit_marker_t {
        vec3_t m_world_origin;
        float m_spawn_time;
        float m_lifetime;
    };

    struct hit_marker_tt {
        double time;
        float alpha;
        float scale;
    };

    struct debug_capsule_t {
        vec3_t m_start;
        vec3_t m_end;
        float m_radius;
        ImU32 m_color;
        int m_segments;
    };

private:
    std::mutex m_aimbot_capsules_mutex;
    std::vector<debug_capsule_t> m_aimbot_capsules;
    std::chrono::steady_clock::time_point m_aimbot_capsules_time{};

public:

    struct notification_t {
        std::string m_str_text{};
        float m_fl_remove_time{};
        float m_fl_alpha = 1.f;
        bool m_b_is_miss = false;
        bool m_b_is_config = false;
        float m_fl_spawn_time = 0.f;
        float m_fl_anim_offset = 0.f;
        float m_fl_target_offset = 0.f;
        float m_fl_bounce_offset = 0.f;
        float m_fl_scale = 1.0f;
        bool m_b_appearing = true;
        bool m_b_disappearing = false;
    };

    std::vector<hit_text_t> m_world_damage_markers;
    std::vector<hit_marker_t> m_hit_markers;
    std::vector<hit_marker_tt> m_hit_markers2;
    std::vector<debug_capsule_t> m_capsules;
    std::deque<notification_t> m_queue_notifications;
    std::deque<notification_t> m_config_notifications;

    std::unordered_map<uint64_t, ImTextureID> avatar_cache;

    struct hotkey_animation_t {
        float base_rect_height = 0.0f;
        float target_base_rect_height = 0.0f;
        bool has_active_keybinds = false;
        ImVec2 widget_pos = ImVec2(-1.0f, -1.0f);
        bool dragging = false;
        ImVec2 drag_offset = ImVec2(0.0f, 0.0f);
        std::map<fnv1a_t, float> text_alphas;
        std::map<fnv1a_t, float> target_text_alphas;
        std::map<fnv1a_t, float> keybind_activation_times;
        float widget_alpha = 0.0f;
        float target_widget_alpha = 0.0f;
        float widget_width = 150.0f;
        float target_widget_width = 150.0f;
    };
    hotkey_animation_t m_hotkey_anim;

    struct observer_animation_t {
        float base_rect_height = 0.0f;
        float target_base_rect_height = 0.0f;
        bool has_active_observers = false;
        ImVec2 widget_pos = ImVec2(-1.0f, -1.0f);
        bool dragging = false;
        ImVec2 drag_offset = ImVec2(0.0f, 0.0f);
        std::map<std::string, float> text_alphas;
        std::map<std::string, float> target_text_alphas;
        std::map<std::string, float> observer_activation_times;
        float widget_alpha = 0.0f;
        float target_widget_alpha = 0.0f;
        float widget_width = 150.0f;
        float target_widget_width = 150.0f;
    };
    observer_animation_t m_observer_anim;

    void draw_world_damage_markers( );
    void add_hitmarker( const vec3_t& world_pos );
    void draw_hitmarkers( );
    void clear_hitmarks( );
    void spread_circle( );

    void push_hitbox( const c_hitbox_data& hitbox, ImU32 color, int segments = 8 );
    void render_all( );
    void clear( );
    void draw_bomb_hud();
    void draw_planted_bomb_world();
    void visualize_aimbot( );
    void update_aimbot_hitboxes();
    void clear_aimbot_hitboxes();
    void radial_gradient( ImDrawList* draw_list, const vec3_t& world_center, float radius, ImU32 center_color, ImU32 edge_color );
    void hitmarker();
    void add_notification(std::string str_text, bool is_miss = false, bool is_config = false);
    void draw_notifications();
    void hotkey_list( );
    void observer_list( );
    ImTextureID get_avatar(uint64_t steam_id);
    void crosshair_indicators( );
    void indicators( );
    void watermark( );
    std::string get_keybind_name(fnv1a_t holder_id);

    vec3_t damage_pos;

    void present( ) {
        static bool logged = false;
        if (!logged) { LOG("overlay: present enter"); logged = true; }
        this->visualize_aimbot( );
        this->render_all( );
        this->clear( );
        this->draw_hitmarkers( );
        this->draw_world_damage_markers( );
        this->spread_circle( );
        this->hitmarker();
        this->draw_bomb_hud();
        this->draw_planted_bomb_world();
        shotlogger::process_shots();
        this->draw_notifications();
        this->hotkey_list( );
        this->observer_list( );
        this->crosshair_indicators( );
        static bool logged_mid = false;
        if (!logged_mid) { LOG("overlay: pre-indicators"); logged_mid = true; }
        this->indicators( );
        this->watermark( );
        static bool logged_out = false;
        if (!logged_out) { LOG("overlay: present done"); logged_out = true; }
    }
};
inline auto g_overlay = std::make_unique<c_overlay>( );

void draw_oof(c_cs_player_pawn* pPawn);

inline std::string get_hitgroup_name(int hitgroup) {
    switch (hitgroup) {
    case HITGROUP_HEAD:
        return "Head";
    case HITGROUP_CHEST:
        return "Chest";
    case HITGROUP_STOMACH:
        return "Stomach";
    case HITGROUP_LEFTARM:
        return "Left Arm";
    case HITGROUP_RIGHTARM:
        return "Right Arm";
    case HITGROUP_LEFTLEG:
        return "Left Leg";
    case HITGROUP_RIGHTLEG:
        return "Right Leg";
    case HITGROUP_NECK:
        return "Neck";
    case HITGROUP_GENERIC:
        return "Generic";
    default:
        return "Unknown";
    }
}

