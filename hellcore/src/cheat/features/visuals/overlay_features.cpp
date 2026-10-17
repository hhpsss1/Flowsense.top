#include "overlay_features.h"
#include "../src/sdk/interfaces/global_variables.h"
#include "../src/core/interfaces/interfaces.h"
#include <context.h>
#include "cheat/features/entity cache/entity_cache.h"
#include <sdk/entity/bomb.h>
#include <cheat/config/vars.h>
#include <cheat/menu/menu.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/menu/hell_gui/blur.h>
#include <sdk/entity/pawn.h>
#include <algorithm>
#include <d3d11.h>
#include <cheat/menu/hell_gui/hell_gui.h>
#include <sdk/interfaces/csgo_input.h>
#include <math/math.h>
#include <array>
#include <sdk/constants.h>
#include <cheat/menu/hell_gui/colors.h>
#include <cheat/menu/tabs/aimbot_tab.h>
#include <cheat/features/ragebot/shotlogger.h>
#include <cheat/features/ragebot/ragebot.h>
#include <chrono>
#include <ctime>
#include <cheat/input.h>
#include <utils/fonts/compressed_fonts/font_awesome.h>

namespace {
    // gradient background in minory style: solid in the middle, faded on the sides
    void draw_gradient_rect(ImVec2 pos, ImVec2 size, ImU32 color, ImU32 gradient_color, ImDrawList* draw_list = ImGui::GetBackgroundDrawList()) {
        ImVec2 p1 = pos;
        ImVec2 p2 = ImVec2(pos.x + size.x, pos.y + size.y);

        draw_list->AddRectFilled(ImVec2(p1.x + 20, p1.y), ImVec2(p2.x - 20, p2.y), gradient_color);
        draw_list->AddRectFilledMultiColor(ImVec2(p1.x + 20, p2.y), p1, gradient_color, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), gradient_color);
        draw_list->AddRectFilledMultiColor(p2, ImVec2(p2.x - 20, p1.y), IM_COL32(0, 0, 0, 0), gradient_color, gradient_color, IM_COL32(0, 0, 0, 0));
    }

    int get_cpu_usage() {
        static ULARGE_INTEGER last_idle_time = {}, last_kernel_time = {}, last_user_time = {};
        static auto last_update_time = std::chrono::steady_clock::now();
        static float cached_cpu_usage = 0.0f;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);

        if (elapsed.count() < 1000)
            return static_cast<int>(cached_cpu_usage + 0.5f);

        last_update_time = now;

        FILETIME idle_time, kernel_time, user_time;
        if (!GetSystemTimes(&idle_time, &kernel_time, &user_time))
            return static_cast<int>(cached_cpu_usage + 0.5f);

        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idle_time.dwLowDateTime;
        idle.HighPart = idle_time.dwHighDateTime;
        kernel.LowPart = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;

        ULONGLONG sys_idle = idle.QuadPart - last_idle_time.QuadPart;
        ULONGLONG sys_kernel = kernel.QuadPart - last_kernel_time.QuadPart;
        ULONGLONG sys_user = user.QuadPart - last_user_time.QuadPart;
        ULONGLONG sys_total = sys_kernel + sys_user;

        last_idle_time = idle;
        last_kernel_time = kernel;
        last_user_time = user;

        if (sys_total == 0)
            return static_cast<int>(cached_cpu_usage + 0.5f);

        cached_cpu_usage = (float)(100.0 - ((sys_idle * 100.0) / sys_total));
        return static_cast<int>(cached_cpu_usage + 0.5f);
    }

    int get_local_ping() {
        if (!g_ctx->m_local_controller)
            return 0;
        if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
            return 0;
        return std::clamp(g_ctx->m_local_controller->m_iPing(), 0, 999);
    }

    int get_local_speed() {
        if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive())
            return 0;
        if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
            return 0;
        vec3_t vel = g_ctx->m_local_pawn->m_vecAbsVelocity();
        return static_cast<int>(std::sqrt(vel.x * vel.x + vel.y * vel.y));
    }
}

void c_overlay::draw_world_damage_markers() {
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!g_interfaces->m_global_vars)
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_3d_damage_markers)))
        return;

    const float current_time = g_interfaces->m_global_vars->m_curtime;

    for (int i = m_world_damage_markers.size() - 1; i >= 0; --i) {
        auto& marker = m_world_damage_markers[i];

        float time_alive = current_time - marker.m_spawn_time;
        if (time_alive > marker.m_lifetime) {
            m_world_damage_markers.erase(m_world_damage_markers.begin() + i);
            continue;
        }

        vec3_t pos = damage_pos + vec3_t(0, 0, time_alive);
        vec2_t screen_pos_vector;
        if (!g_math->world_to_screen(pos, screen_pos_vector))
            continue;

        hellvec2 screen_pos_hit_marker = { screen_pos_vector.x, screen_pos_vector.y + 16.5f };
        hellvec2 screen_pos_default = { screen_pos_vector.x, screen_pos_vector.y };

        float fade_in_duration = 0.20f;
        float fade_out_start = marker.m_lifetime * 0.7f;
        float alpha = 1.0f;

        if (time_alive < fade_in_duration) {
            float t = time_alive / fade_in_duration;
            alpha = t * t;
        }
        else if (time_alive > fade_out_start) {
            float t = (time_alive - fade_out_start) / (marker.m_lifetime - fade_out_start);
            alpha = 1.0f - (t * t);
        }

        static c_visuals::text_object_t text_object{};
        text_object.m_fg_color = GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_damage_markers_color));
        text_object.m_bg_color = GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_damage_markers_color_bg));
        text_object.m_font_type = (c_visuals::e_font_type)(GET_VAR(int, VISUALS_PATH(m_3d_damage_markers_font_type)));
        text_object.m_text_shadow_type = (c_visuals::e_text_shadow_type)(GET_VAR(int, VISUALS_PATH(m_3d_damage_markers_shadow_type)));

        std::string damage_str = std::to_string(marker.m_damage);
        text_object.m_text = damage_str.c_str();
        if (GET_VAR(bool, VISUALS_PATH(m_enabled_3d_hitmarkers)))
            g_visuals->draw_text(c_visuals::bb_t{ screen_pos_hit_marker, screen_pos_hit_marker }, text_object, alpha, ImGui::GetBackgroundDrawList());
        else
            g_visuals->draw_text(c_visuals::bb_t{ screen_pos_default, screen_pos_default }, text_object, alpha, ImGui::GetBackgroundDrawList());
    }
}

void c_overlay::add_hitmarker(const vec3_t& world_pos) {
    if (!g_interfaces->m_global_vars)
        return;

    hit_marker_t marker{};
    marker.m_world_origin = world_pos;
    marker.m_spawn_time = g_interfaces->m_global_vars->m_curtime;
    marker.m_lifetime = 1.8f;
    m_hit_markers.push_back(marker);
}

void c_overlay::draw_hitmarkers() {
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!g_interfaces->m_global_vars)
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_3d_hitmarkers)))
        return;

    const float current_time = g_interfaces->m_global_vars->m_curtime;
    auto* draw_list = ImGui::GetForegroundDrawList();

    const float max_size = 8.0f;
    const float base_thickness = 1.5f;
    const float gap = 3.0f;
    const hellcolor base_color = GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_hitmarkers_color));

    auto draw_gradient_line_3d = [&](const ImVec2& start, const ImVec2& end, const hellcolor& color_start, const hellcolor& color_end, float thickness, float anim_progress, int corner_type) {
        ImVec2 anim_start = start;
        ImVec2 anim_end = end;

        if (anim_progress < 1.0f) {
            float anim_offset = (max_size + gap) * 2.0f;
            ImVec2 corner_offset;
            if (corner_type == 0) {
                corner_offset = ImVec2(-anim_offset, -anim_offset);
            }
            else if (corner_type == 1) {
                corner_offset = ImVec2(-anim_offset, anim_offset);
            }
            else if (corner_type == 2) {
                corner_offset = ImVec2(anim_offset, -anim_offset);
            }
            else {
                corner_offset = ImVec2(anim_offset, anim_offset);
            }

            float ease = 1.0f - (1.0f - anim_progress) * (1.0f - anim_progress) * (1.0f - anim_progress);
            anim_start = ImVec2(
                start.x + corner_offset.x * (1.0f - ease),
                start.y + corner_offset.y * (1.0f - ease)
            );
            anim_end = ImVec2(
                end.x + corner_offset.x * (1.0f - ease),
                end.y + corner_offset.y * (1.0f - ease)
            );
        }

        const int segments = 8;
        for (int i = 0; i < segments; ++i) {
            float t1 = (float)i / (float)segments;
            float t2 = (float)(i + 1) / (float)segments;

            ImVec2 p1 = ImVec2(anim_start.x + (anim_end.x - anim_start.x) * t1, anim_start.y + (anim_end.y - anim_start.y) * t1);
            ImVec2 p2 = ImVec2(anim_start.x + (anim_end.x - anim_start.x) * t2, anim_start.y + (anim_end.y - anim_start.y) * t2);

            hellcolor segment_color = hellcolor(
                color_start.Value.x + (color_end.Value.x - color_start.Value.x) * t1,
                color_start.Value.y + (color_end.Value.y - color_start.Value.y) * t1,
                color_start.Value.z + (color_end.Value.z - color_start.Value.z) * t1,
                color_start.Value.w + (color_end.Value.w - color_start.Value.w) * t1
            );

            draw_list->AddLine(p1, p2, segment_color, thickness);
        }
        };

    for (std::size_t i = 0; i < m_hit_markers.size(); ) {
        auto& marker = m_hit_markers[i];
        const float time_alive = current_time - marker.m_spawn_time;

        if (time_alive > marker.m_lifetime) {
            m_hit_markers.erase(m_hit_markers.begin() + i);
            continue;
        }

        vec2_t screen;
        if (!g_math->world_to_screen(marker.m_world_origin, screen)) {
            ++i;
            continue;
        }

        const float alpha = 1.f - (time_alive / marker.m_lifetime);
        const float anim_duration = 0.15f;
        const float anim_progress = std::min(1.0f, time_alive / anim_duration);
        const hellvec2 center{ screen.x, screen.y };

        hellcolor core = base_color;
        core.Value.w = alpha;

        hellcolor fade_color = core;
        fade_color.Value.w = 0.0f;

        hellcolor glow_color = core;
        glow_color.Value.w = alpha * 0.3f;
        hellcolor glow_fade = glow_color;
        glow_fade.Value.w = 0.0f;

        float glow_radius = (max_size + gap) * 1.5f;

        const int glow_segments = 64;
        const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
        const unsigned int vtx_base = draw_list->_VtxCurrentIdx;

        draw_list->PrimReserve(glow_segments * 3, glow_segments + 1);
        draw_list->PrimWriteVtx(center, uv, glow_color);

        for (int j = 0; j < glow_segments; ++j) {
            float angle = (float)j / (float)glow_segments * 2.0f * IM_PI;
            ImVec2 pos = ImVec2(center.x + cosf(angle) * glow_radius, center.y + sinf(angle) * glow_radius);
            draw_list->PrimWriteVtx(pos, uv, glow_fade);
        }

        for (int j = 0; j < glow_segments; ++j) {
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + j));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((j + 1) % glow_segments)));
        }

        draw_gradient_line_3d(
            ImVec2(center.x - gap - max_size, center.y - gap - max_size),
            ImVec2(center.x - gap, center.y - gap),
            fade_color, core, base_thickness, anim_progress, 0
        );

        draw_gradient_line_3d(
            ImVec2(center.x - gap - max_size, center.y + gap + max_size),
            ImVec2(center.x - gap, center.y + gap),
            fade_color, core, base_thickness, anim_progress, 1
        );

        draw_gradient_line_3d(
            ImVec2(center.x + gap + max_size, center.y - gap - max_size),
            ImVec2(center.x + gap, center.y - gap),
            fade_color, core, base_thickness, anim_progress, 2
        );

        draw_gradient_line_3d(
            ImVec2(center.x + gap + max_size, center.y + gap + max_size),
            ImVec2(center.x + gap, center.y + gap),
            fade_color, core, base_thickness, anim_progress, 3
        );

        ++i;
    }
}


void c_overlay::clear_hitmarks() {
    m_hit_markers.clear();
}

void c_overlay::spread_circle() {
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    const auto local = g_ctx->m_local_pawn;
    if (!local || !local->is_alive())
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_spread_circle)))
        return;

    const auto cmd = g_ctx->m_cmd;
    const auto base_cmd = g_ctx->m_base;
    const auto weapon = g_ctx->m_active_weapon;
    if (!cmd || !base_cmd || !weapon)
        return;

    const auto& io = ImGui::GetIO();
    const hellvec2 center = ImGui::GetMainViewport()->GetCenter();

    static float s_radius = 0.f;
    static float s_scale = 2.f;

    float inaccuracy = weapon->get_inaccuracy();
    float spread = weapon->get_spread();
    float velocity_factor = g_ctx->m_local_pawn->m_vecAbsVelocity().length_2d();

    if (velocity_factor < 5.0f) {
        velocity_factor = 1.0f;
    }
    else {
        velocity_factor = velocity_factor / 250.0f;
    }

    const float target = (inaccuracy + spread) * velocity_factor * (io.DisplaySize.y / s_scale / 2.f);
    s_radius += (target - s_radius) * 0.15f;

    const auto col1 = ImGui::ColorConvertFloat4ToU32(GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_first)));
    const auto col2 = ImGui::ColorConvertFloat4ToU32(GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_last)));
    const auto uv = io.Fonts->TexUvWhitePixel;

    auto* dl = ImGui::GetBackgroundDrawList();
    constexpr int segs = 200;
    constexpr float step = 2.f * IM_PI / segs;
    const int base = dl->VtxBuffer.Size;

    dl->PrimReserve(segs * 3, segs + 1);
    dl->PrimWriteVtx(center, uv, col1);

    for (int i = 0; i < segs; ++i) {
        float a = i * step;
        dl->PrimWriteVtx({ center.x + std::cos(a) * s_radius, center.y + std::sin(a) * s_radius }, uv, col2);
    }

    for (int i = 0; i < segs; ++i) {
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx(base + 1 + i);
        dl->PrimWriteIdx(base + 1 + ((i + 1) % segs));
    }
}

static void draw_radial_gradient(ImDrawList* draw_list, const hellvec2& center, float radius, ImU32 col_inner, ImU32 col_outer, int segments = 64) {
    if (((col_inner | col_outer) & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    const float angle_step = 2.0f * IM_PI / segments;
    const hellvec2 uv = draw_list->_Data->TexUvWhitePixel;

    draw_list->PrimReserve(segments * 3, segments + 1);
    const unsigned int vtx_base = draw_list->_VtxCurrentIdx;


    draw_list->PrimWriteVtx(center, uv, col_inner);

    for (int i = 0; i < segments; ++i) {
        float angle = i * angle_step;
        float x = center.x + std::cos(angle) * radius;
        float y = center.y + std::sin(angle) * radius;
        draw_list->PrimWriteVtx(hellvec2(x, y), uv, col_outer);
    }

    for (int i = 0; i < segments; ++i) {
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + i));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((i + 1) % segments)));
    }
}

void c_overlay::push_hitbox(const c_hitbox_data& hitbox, ImU32 color, int segments) {
    m_capsules.push_back({ hitbox.m_mins, hitbox.m_maxs, hitbox.m_radius, color, segments });
}

void c_overlay::render_all() {
    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    for (const auto& cap : m_capsules) {
        const vec3_t axis = (cap.m_end - cap.m_start).normalized();

        vec3_t right = axis.cross(vec3_t(0, 0, 1));
        if (right.length_sqr() < 1e-3f)
            right = vec3_t(1, 0, 0);
        right.normalize();

        vec3_t up = axis.cross(right).normalized();

        constexpr int rings = 30;
        constexpr float fade_outer = 20.0f;

        for (int i = 0; i <= rings; ++i) {
            float t = (float)i / (float)rings;

            vec3_t ring_center = cap.m_start + (cap.m_end - cap.m_start) * t;

            vec2_t screen_center;
            if (!g_math->world_to_screen(ring_center, screen_center))
                continue;

            const hellvec2 center_2d(screen_center.x, screen_center.y);

            vec3_t offset_pos = ring_center + right * cap.m_radius;

            vec2_t screen_offset;
            if (!g_math->world_to_screen(offset_pos, screen_offset))
                continue;

            float visual_radius = (screen_offset - screen_center).length();

            hellcolor base(cap.m_color);
            hellcolor outer_color(base.Value.x, base.Value.y, base.Value.z, 0.0f);
            const ImU32 inner = cap.m_color;

            draw_radial_gradient(draw, center_2d, visual_radius, inner, outer_color);
        }
    }
}

void c_overlay::clear() {
    m_capsules.clear();
}

void circle_progress_bomb(ImDrawList* pDrawList, const ImVec2& center, float radius, const hellcolor& color, float thickness, float progress, float start_angle) {
    if (progress <= 0.0f) return;

    const float end_angle = start_angle + (IM_PI * 2.0f * progress);
    const int num_segments = 32;

    pDrawList->PathArcTo(ImVec2(center.x, center.y), radius, start_angle, end_angle, num_segments);
    pDrawList->PathStroke(color, 0, thickness);
}

void c_overlay::draw_bomb_hud()
{
    if (!GET_VAR(bool, VISUALS_PATH(m_bomb_hud_enabled)))
        return;

    auto local = g_ctx->m_local_pawn;

    C_PlantedC4* planted = bomb::get_planted_bomb();
    bool has_planted = planted && planted->m_bBombTicking();

    int damage = 0;
    float time_left = 0.0f;
    int site = 0;
    bool preview = false;

    if (has_planted)
    {
        if (!local || !local->is_alive())
            return;

        damage = bomb::get_bomb_damage_to_player(local);
        time_left = bomb::get_time_to_explode();
        if (time_left <= 0.0f)
            return;

        site = bomb::get_bomb_site();
        if (site < 0)
            site = 0;
    }
    else
    {
        if (!g_menu || !g_menu->m_menu_open)
            return;

        preview = true;
        damage = 50;
        time_left = 0.0f;
        site = 0;
    }

    char site_char = (site == 0 ? 'A' : 'B');

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    const ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    const float rect_width = 170.0f;
    const float rect_height = 60.0f;

    static ImVec2 rect_pos = ImVec2(-1.0f, -1.0f);
    if (rect_pos.x < 0.0f || rect_pos.y < 0.0f)
        rect_pos = ImVec2(screen_size.x * 0.5f - rect_width * 0.5f, screen_size.y * 0.2f);

    ImVec2 rect_min = rect_pos;
    ImVec2 rect_max = ImVec2(rect_min.x + rect_width, rect_min.y + rect_height);

    if (g_menu && g_menu->m_menu_open)
    {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        bool hovered = mouse_pos.x >= rect_min.x && mouse_pos.x <= rect_max.x &&
            mouse_pos.y >= rect_min.y && mouse_pos.y <= rect_max.y;

        static bool dragging = false;
        static ImVec2 drag_offset{};

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            dragging = true;
            drag_offset = ImVec2(mouse_pos.x - rect_min.x, mouse_pos.y - rect_min.y);
        }
        if (dragging)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                rect_pos = ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y);
                rect_min = rect_pos;
                rect_max = ImVec2(rect_min.x + rect_width, rect_min.y + rect_height);
            }
            else
            {
                dragging = false;
            }
        }
    }

    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
    ImVec2 outline_min = ImVec2(rect_min.x, rect_min.y - 1.f);
    ImVec2 outline_max = ImVec2(rect_max.x, rect_min.y + 4.f);
    draw_list->AddRectFilled(outline_min, outline_max, accent_color, 4.0f, ImDrawFlags_RoundCornersTop);

    hellcolor bg_color(29, 29, 29, 255);
    ImDrawFlags flags = ImDrawFlags_RoundCornersAll;
    draw_list->AddRectFilled(rect_min, rect_max, bg_color, 4.0f, flags);

    ImFont* font = g_font_manager->m_gheist_medium_14 ? g_font_manager->m_gheist_medium_14 : g_font_manager->m_calibri_12;

    ImFont* site_font = g_font_manager->m_inter_bold_large ? g_font_manager->m_inter_bold_large : g_font_manager->m_verdana_14;

    ImVec2 left_center = ImVec2(rect_min.x + 29.0f, rect_min.y + rect_height * 0.5f);
    float circle_radius = 16.0f;

    bool defusing = planted && planted->m_bBeingDefused() && !planted->m_bBombDefused();
    bool defuse_in_time = false;
    float defuse_remaining = 0.0f;
    float defuse_total = 0.0f;

    if (!preview && defusing && g_interfaces->m_global_vars)
    {
        float curtime = g_interfaces->m_global_vars->m_curtime;
        defuse_total = planted->m_flDefuseLength();
        defuse_remaining = planted->m_flDefuseCountDown() - curtime;
        if (defuse_remaining < 0.0f)
            defuse_remaining = 0.0f;

        if (defuse_total > 0.0f)
            defuse_in_time = defuse_remaining <= time_left;
    }

    bool show_defuse_icon = defusing && !preview;

    if (show_defuse_icon)
    {
        static icon_data_t defuse_icon = get_panorama_texture("icons/equipment/defuser");
        if (defuse_icon.texture_view && defuse_icon.width > 0 && defuse_icon.height > 0)
        {
            float icon_target_size = circle_radius * 1.2f;
            float aspect = static_cast<float>(defuse_icon.width) / static_cast<float>(defuse_icon.height);
            float icon_width = icon_target_size * aspect;
            float icon_height = icon_target_size;

            ImVec2 icon_min(
                left_center.x - icon_width * 0.5f,
                left_center.y - icon_height * 0.5f
            );
            ImVec2 icon_max(
                icon_min.x + icon_width,
                icon_min.y + icon_height
            );

            hellcolor icon_color(255, 255, 255, 255);
            draw_list->AddImage(
                (ImTextureID)defuse_icon.texture_view,
                icon_min,
                icon_max,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f),
                icon_color
            );
        }
    }
    else
    {
        char site_text[2] = { site_char, 0 };
        ImVec2 site_text_size = site_font->CalcTextSizeA(site_font->LegacySize, FLT_MAX, 0.0f, site_text);
        ImVec2 site_text_pos(left_center.x - site_text_size.x * 0.5f, left_center.y - site_text_size.y * 0.5f);
        if (site_char == 'A')
            site_text_pos.y -= 1.0f;
        else if (site_char == 'B')
            site_text_pos.x += 1.0f;
        hellcolor site_color(255, 255, 255, 255);
        draw_list->AddText(site_font, site_font->LegacySize, site_text_pos, site_color, site_text);
    }

    float progress = 0.5f;
    if (!preview)
    {
        float bomb_time_total = planted->m_flTimerLength();
        if (bomb_time_total > 0.1f)
            progress = std::clamp(time_left / bomb_time_total, 0.0f, 1.0f);
        else
            progress = 0.0f;
    }

    hellcolor progress_bg_color(20, 20, 20, 165);
    circle_progress_bomb(draw_list, left_center, circle_radius + 4.0f, progress_bg_color, 4.0f, 1.0f, -IM_PI * 0.5f);

    hellcolor progress_color = accent_color;

    if (defusing && defuse_in_time && defuse_total > 0.0f)
    {
        progress = std::clamp(defuse_remaining / defuse_total, 0.0f, 1.0f);
        progress_color = accent_color;
    }

    circle_progress_bomb(draw_list, left_center, circle_radius + 4.0f, progress_color, 2.0f, progress, -IM_PI * 0.5f);

    const float text_area_left = left_center.x + circle_radius + 8.0f;
    const float text_area_right = rect_max.x - 12.0f;

    const char* lethal_text = "Lethal";
    const char* defusing_text = "Defusing";

    bool lethal = false;
    if (!preview && local && local->is_alive() && damage > 0)
        lethal = damage >= local->m_iHealth();

    char damage_buffer[64];
    const char* damage_text = nullptr;

    if (!preview && defusing)
    {
        damage_text = defusing_text;
    }
    else if (!preview && lethal)
    {
        damage_text = lethal_text;
    }
    else if (damage <= 0)
    {
        std::snprintf(damage_buffer, sizeof(damage_buffer), "Damage: 0");
        damage_text = damage_buffer;
    }
    else
    {
        std::snprintf(damage_buffer, sizeof(damage_buffer), "Damage: %d", damage);
        damage_text = damage_buffer;
    }

    char time_buffer[64];
    if (!preview && planted)
    {
        float display_time = time_left;
        if (defusing && defuse_in_time && defuse_remaining > 0.0f)
            display_time = defuse_remaining;

        std::snprintf(time_buffer, sizeof(time_buffer), "Time: %.1f", display_time);
    }
    else
    {
        std::snprintf(time_buffer, sizeof(time_buffer), "Time: 10.0");
    }

    ImVec2 time_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, time_buffer);
    ImVec2 damage_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, damage_text);
    const char* defuse_label = "Defusing";
    hellcolor defuse_color = accent_color;

    ImVec2 defuse_size{};

    float text_center_x = (text_area_left + text_area_right) * 0.5f;
    float total_text_height = damage_size.y + 2.0f + time_size.y;
    float top_y = rect_min.y + rect_height * 0.5f - total_text_height * 0.5f;

    ImVec2 damage_pos(
        text_center_x - damage_size.x * 0.5f,
        top_y
    );

    ImVec2 time_pos(
        text_center_x - time_size.x * 0.5f,
        top_y + damage_size.y + 2.0f
    );

    hellcolor white_color(255, 255, 255, 255);
    hellcolor final_damage_color = white_color;
    hellcolor final_time_color = accent_color;

    if (!preview && lethal)
    {
        final_damage_color = accent_color;
    }

    char damage_label[32];
    char damage_value[32];
    if (!preview && (lethal || defusing))
    {
        strcpy_s(damage_label, sizeof(damage_label), "");
        strcpy_s(damage_value, sizeof(damage_value), damage_text);
    }
    else
    {
        strcpy_s(damage_label, sizeof(damage_label), "Damage: ");
        if (damage <= 0)
            strcpy_s(damage_value, sizeof(damage_value), "0");
        else
            std::snprintf(damage_value, sizeof(damage_value), "%d", damage);
    }

    char time_label[32] = "Time: ";
    char time_value[32];
    if (!preview && planted)
    {
        float display_time = time_left;
        if (defusing && defuse_in_time && defuse_remaining > 0.0f)
            display_time = defuse_remaining;
        std::snprintf(time_value, sizeof(time_value), "%.1f", display_time);
    }
    else
    {
        strcpy_s(time_value, sizeof(time_value), "10.0");
    }

    ImVec2 damage_label_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, damage_label);
    ImVec2 damage_value_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, damage_value);
    ImVec2 time_label_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, time_label);
    ImVec2 time_value_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, time_value);

    float damage_total_width = damage_label_size.x + damage_value_size.x;
    float time_total_width = time_label_size.x + time_value_size.x;

    ImVec2 damage_label_pos(
        text_center_x - damage_total_width * 0.5f,
        top_y
    );
    ImVec2 damage_value_pos(
        damage_label_pos.x + damage_label_size.x,
        top_y
    );

    ImVec2 time_label_pos(
        text_center_x - time_total_width * 0.5f,
        top_y + damage_size.y + 2.0f
    );
    ImVec2 time_value_pos(
        time_label_pos.x + time_label_size.x,
        top_y + damage_size.y + 2.0f
    );

    if (!preview && (lethal || defusing))
    {
        draw_list->AddText(font, font->LegacySize, damage_pos, accent_color, damage_text);
    }
    else
    {
        draw_list->AddText(font, font->LegacySize, damage_label_pos, white_color, damage_label);
        draw_list->AddText(font, font->LegacySize, damage_value_pos, accent_color, damage_value);
    }

    draw_list->AddText(font, font->LegacySize, time_label_pos, white_color, time_label);
    draw_list->AddText(font, font->LegacySize, time_value_pos, accent_color, time_value);
}

void c_overlay::draw_planted_bomb_world()
{
	bool bShowIcon = GET_VAR(bool, VISUALS_PATH(m_planted_bomb_world_icon));
	bool bShowTimer = GET_VAR(bool, VISUALS_PATH(m_planted_bomb_world_timer));

	if (!bShowIcon && !bShowTimer)
		return;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!g_ctx->m_local_pawn)
		return;

	if (!g_ctx->m_local_controller)
		return;

	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	C_PlantedC4* planted = bomb::get_planted_bomb();
	if (!planted || !planted->m_bBombTicking())
		return;

	int nMaxDistance = GET_VAR(int, VISUALS_PATH(m_planted_bomb_distance));
	vec3_t vLocalPos = g_ctx->m_local_pawn->get_world_space_center();

	c_game_scene_node* pGameSceneNode = planted->m_pGameSceneNode();
	if (!pGameSceneNode)
		return;

	vec3_t vBombPos = pGameSceneNode->m_vecAbsOrigin();
	float flDistance = vLocalPos.dist_to(vBombPos);

	if (flDistance > nMaxDistance)
		return;

	vec2_t vScreen;
	if (!g_math->world_to_screen(vBombPos, vScreen))
		return;

	float flAlpha = 1.0f;
	if (flDistance > 200.0f) {
		flAlpha = std::max(0.0f, 1.0f - (flDistance - 200.0f) / (nMaxDistance - 200.0f));
	}

	if (flAlpha < 0.01f)
		return;

	float time_left = bomb::get_time_to_explode();
	if (time_left <= 0.0f)
		return;

	ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();

	float flCurrentY = vScreen.y;

	if (bShowTimer) {
		hellcolor timerColor = GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color));
		timerColor.Value.w *= flAlpha;
		hellcolor timerBgColor = GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color_bg));
		timerBgColor.Value.w *= flAlpha;

		int nFontType = GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_font_type));
		int nShadowType = GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_shadow_type));

		c_visuals::e_font_type font_type = static_cast<c_visuals::e_font_type>(nFontType);
		ImFont* pFont = g_visuals->get_font(font_type);
		if (!pFont)
			return;

		char szTimer[32];
		int seconds = static_cast<int>(time_left);
		int milliseconds = static_cast<int>((time_left - seconds) * 10.0f);
		std::snprintf(szTimer, sizeof(szTimer), "%d.%d", seconds, milliseconds);

		float fontSize = pFont->LegacySize;
		ImVec2 textSize = pFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, szTimer);

		ImVec2 textPos = ImVec2(vScreen.x - textSize.x * 0.5f, flCurrentY);

		if (nShadowType == c_visuals::e_text_shadow_type::text_shadow_full) {
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					if (x == 0 && y == 0)
						continue;
					pDrawList->AddText(pFont, fontSize, ImVec2(textPos.x + x, textPos.y + y), timerBgColor, szTimer);
				}
			}
		}
		else if (nShadowType == c_visuals::e_text_shadow_type::text_shadow_drop) {
			hellcolor shadowColor = timerBgColor;
			shadowColor.Value.x = 0.0f;
			shadowColor.Value.y = 0.0f;
			shadowColor.Value.z = 0.0f;
			pDrawList->AddText(pFont, fontSize, ImVec2(textPos.x + 1, textPos.y + 1), shadowColor, szTimer);
		}

		pDrawList->AddText(pFont, fontSize, textPos, timerColor, szTimer);
		
		flCurrentY = textPos.y - 2.0f;
	}

	if (bShowIcon) {
		static icon_data_t bomb_icon = get_panorama_texture("icons/equipment/c4");
		
		if (bomb_icon.texture_view && bomb_icon.width > 0 && bomb_icon.height > 0) {
			int iTargetSize = 12;
			const auto flWtoHRatio = static_cast<float>(bomb_icon.width) / static_cast<float>(bomb_icon.height);
			auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
			auto Height = iTargetSize;
			
			ImVec2 vIconPos = ImVec2(vScreen.x - Width * 0.5f, flCurrentY - Height);
			
			hellcolor iconColor = GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_icon_color));
			iconColor.Value.w *= flAlpha;
			pDrawList->AddImage(bomb_icon.texture_view, vIconPos, ImVec2(vIconPos.x + Width, vIconPos.y + Height), ImVec2(0, 0), ImVec2(1, 1), iconColor);
		}
	}
}

void c_overlay::visualize_aimbot() {
    if (!GET_VAR(bool, VISUALS_PATH(m_visualize_hitboxes)))
        return;

    std::lock_guard<std::mutex> lock(m_aimbot_capsules_mutex);
    if (std::chrono::steady_clock::now() - m_aimbot_capsules_time > std::chrono::milliseconds(250))
        return;
    m_capsules.insert(m_capsules.end(), m_aimbot_capsules.begin(), m_aimbot_capsules.end());
}

void c_overlay::clear_aimbot_hitboxes() {
    std::lock_guard<std::mutex> lock(m_aimbot_capsules_mutex);
    m_aimbot_capsules.clear();
    m_aimbot_capsules_time = {};
}

void c_overlay::update_aimbot_hitboxes() {
    if (!GET_VAR(bool, VISUALS_PATH(m_visualize_hitboxes)) ||
        !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game() ||
        !g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive() ||
        !g_ctx->m_active_weapon || !g_ctx->m_active_weapon_data ||
        !g_ctx->m_local_controller || !g_interfaces->m_phys2world) {
        clear_aimbot_hitboxes();
        return;
    }

    vec3_t shoot_position = g_ctx->m_shoot_position;

    int weapon_rage_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
    if (weapon_rage_type == 10) {
        clear_aimbot_hitboxes();
        return;
    }

    fnv1a_t dmg_holder_id = tabs::aimbot::get_min_damage_holder_id(weapon_rage_type);
    int min_damage_setting = GET_VAR(int, dmg_holder_id);

    if (min_damage_setting <= 0) {
        clear_aimbot_hitboxes();
        return;
    }

    std::vector<debug_capsule_t> capsules;
    const ImU32 render_color = GET_VAR(hellcolor, VISUALS_PATH(m_visualize_hitbox_color));
    for (auto& player : g_entity_cache->m_players) {
        auto& records = player.m_lagcomp_data.m_lag_records;
        auto* pawn = player.m_pawn;
        if (!player.m_lagcomp_data.is_valid() || records.empty() || !pawn ||
            pawn == g_ctx->m_local_pawn || !pawn->is_alive() || !pawn->is_enemy())
            continue;

        auto& record = records.newest();
        if (record.m_pawn != pawn || record.m_visual_only || !record.is_valid())
            continue;

        const int target_health = pawn->m_iHealth();
        const int dmg_threshold = min_damage_setting > 100 ? target_health + min_damage_setting - 100 :
            std::min(min_damage_setting, target_health);

        auto penetration_ctx = player.m_penetration_context;

        for (auto& hitbox : record.m_rage_hitboxes) {
            if (!std::isfinite(hitbox.m_radius) || hitbox.m_radius <= 0.f ||
                !std::isfinite(hitbox.m_center.x) || !std::isfinite(hitbox.m_center.y) ||
                !std::isfinite(hitbox.m_center.z) || !std::isfinite(hitbox.m_mins.x) ||
                !std::isfinite(hitbox.m_mins.y) || !std::isfinite(hitbox.m_mins.z) ||
                !std::isfinite(hitbox.m_maxs.x) || !std::isfinite(hitbox.m_maxs.y) ||
                !std::isfinite(hitbox.m_maxs.z))
                continue;
            c_handle_bullet_penetration_data pen_data;
            int current_damage = 0;
            if (penetration_ctx.fire_bullet(shoot_position, hitbox.m_center, record.m_pawn, pen_data))
                current_damage = static_cast<int>(pen_data.m_damage);

            bool can_damage_current = current_damage >= dmg_threshold;
            bool predictive_damage = can_damage_current;

            if (!predictive_damage) {
                std::vector<vec3_t> positions_to_check = {
                    shoot_position + vec3_t(32.0f, 0.0f, 0.0f),
                    shoot_position + vec3_t(-32.0f, 0.0f, 0.0f),
                    shoot_position + vec3_t(0.0f, 32.0f, 0.0f),
                    shoot_position + vec3_t(0.0f, -32.0f, 0.0f),
                    shoot_position + vec3_t(0.0f, 0.0f, 24.0f),
                    shoot_position + vec3_t(0.0f, 0.0f, -24.0f),
                    shoot_position + vec3_t(24.0f, 24.0f, 0.0f),
                    shoot_position + vec3_t(-24.0f, 24.0f, 0.0f),
                    shoot_position + vec3_t(24.0f, -24.0f, 0.0f),
                    shoot_position + vec3_t(-24.0f, -24.0f, 0.0f)
                };

                for (vec3_t pos : positions_to_check) {
                    c_handle_bullet_penetration_data predictive_data;
                    int damage = 0;
                    if (penetration_ctx.fire_bullet(pos, hitbox.m_center, record.m_pawn, predictive_data))
                        damage = static_cast<int>(predictive_data.m_damage);

                    if (damage >= dmg_threshold) {
                        predictive_damage = true;
                        break;
                    }
                }
            }

            if (!predictive_damage)
                continue;

            capsules.push_back({hitbox.m_mins, hitbox.m_maxs, hitbox.m_radius, render_color, 5});
        }
    }
    std::lock_guard<std::mutex> lock(m_aimbot_capsules_mutex);
    m_aimbot_capsules.swap(capsules);
    m_aimbot_capsules_time = std::chrono::steady_clock::now();
}

void c_overlay::radial_gradient(ImDrawList* draw_list, const vec3_t& world_center, float radius, ImU32 center_color, ImU32 edge_color) {
    if (((center_color | edge_color) & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    constexpr int count = 100;

    vec2_t center_screen;
    if (!g_math->world_to_screen(world_center, center_screen))
        return;

    std::vector<hellvec2> screen_vertices;
    screen_vertices.reserve(count + 1);

    screen_vertices.emplace_back(center_screen.x, center_screen.y);

    constexpr float pi_f = static_cast<float>(A_PI);
    for (int i = 0; i < count; ++i) {
        const float angle = 2.0f * pi_f * i / count;
        const float x = radius * std::cos(angle);
        const float y = radius * std::sin(angle);

        vec3_t world_point = world_center + vec3_t(x, y, 0.0f);
        vec2_t screen_point;

        if (g_math->world_to_screen(world_point, screen_point)) {
            screen_vertices.emplace_back(screen_point.x, screen_point.y);
        }
    }

    const int actual_count = static_cast<int>(screen_vertices.size()) - 1;
    if (actual_count < 3)
        return;

    const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
    draw_list->PrimReserve(actual_count * 3, actual_count + 1);
    const unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimWriteVtx(ImVec2(screen_vertices[0].x, screen_vertices[0].y), uv, center_color);

    for (int i = 1; i <= actual_count; ++i) {
        draw_list->PrimWriteVtx(ImVec2(screen_vertices[i].x, screen_vertices[i].y), uv, edge_color);
    }

    for (int i = 0; i < actual_count; ++i) {
        draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(vtx_base));
        draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(vtx_base + 1 + i));
        draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(vtx_base + 1 + ((i + 1) % actual_count)));
    }
}

static ImVec2 RotateVertex(const ImVec2& p, const ImVec2& v, float angle) {
    float c = std::cos(DEG2RAD(angle));
    float s = std::sin(DEG2RAD(angle));
    return {
        p.x + (v.x - p.x) * c - (v.y - p.y) * s,
        p.y + (v.x - p.x) * s + (v.y - p.y) * c
    };
}

void draw_oof(c_cs_player_pawn* pPawn) {
    if (!pPawn || !pPawn->m_pGameSceneNode())
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_bEnableOOFArrows)))
        return;

    if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive())
        return;

    hellcolor triangleColor = GET_VAR(hellcolor, VISUALS_PATH(m_colOOFArrows));

    float baseSize = 8.0f;
    float screenDistance = 120.0f;

    vec3_t vecViewOrigin, vecTargetPos, vecDelta;
    vec2_t vecScreenPos;
    vec3_t vecOffScreenPos;
    float flLeeWayX, flLeeWayY, flRadius, flOffScreenRotation;
    bool bIsOnScreen;
    ImVec2 vP1, vP2, vP3, vBackCenter, vArrowScreenPos;

    auto RotatePoint = [](const ImVec2& vPoint, const ImVec2& vCenter, float flDeg) -> ImVec2 {
        flDeg = DEG2RAD(flDeg);
        const auto flCos = cosf(flDeg);
        const auto flSin = sinf(flDeg);
        ImVec2 vReturn = ImVec2();
        vReturn.x = flCos * (vPoint.x - vCenter.x) - flSin * (vPoint.y - vCenter.y);
        vReturn.y = flSin * (vPoint.x - vCenter.x) + flCos * (vPoint.y - vCenter.y);
        vReturn += vCenter;
        return vReturn;
        };

    auto GetOffScreenData = [](const vec3_t& vecDelta, float flRadiusX, float flRadiusY, vec3_t& vecOutOffScreenPos, float& flOutRot) {
        vec3_t view_angles(g_interfaces->m_csgo_input->get_view_angle());
        vec3_t fwd, right, up(0.f, 0.f, 1.f);
        g_math->angle_vectors(view_angles, &fwd, &right, nullptr);
        fwd.z = 0.f;
        fwd.normalize();
        right = up.cross(fwd);
        float front = vecDelta.dot(fwd);
        float side = vecDelta.dot(right);
        vecOutOffScreenPos.x = flRadiusX * -side;
        vecOutOffScreenPos.y = flRadiusY * -front;
        flOutRot = RAD2DEG(std::atan2(vecOutOffScreenPos.x, vecOutOffScreenPos.y) + A_PI);
        float yaw_rad = DEG2RAD(-flOutRot);
        float sa = std::sin(yaw_rad);
        float ca = std::cos(yaw_rad);
        vecOutOffScreenPos.x = (ImGui::GetIO().DisplaySize.x / 2.f) + (flRadiusX * sa);
        vecOutOffScreenPos.y = (ImGui::GetIO().DisplaySize.y / 2.f) - (flRadiusY * ca);
        };

    vecTargetPos = pPawn->m_pGameSceneNode()->m_vecAbsOrigin();
    bIsOnScreen = g_math->world_to_screen(vecTargetPos, vecScreenPos);
    flLeeWayX = ImGui::GetIO().DisplaySize.x / 18.f;
    flLeeWayY = ImGui::GetIO().DisplaySize.y / 18.f;

    if (!bIsOnScreen ||
        vecScreenPos.x < -flLeeWayX ||
        vecScreenPos.x >(ImGui::GetIO().DisplaySize.x + flLeeWayX) ||
        vecScreenPos.y < -flLeeWayY ||
        vecScreenPos.y >(ImGui::GetIO().DisplaySize.y + flLeeWayY)) {
        vecViewOrigin = g_ctx->m_local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
        vecDelta = (vecTargetPos - vecViewOrigin).normalized();
        float dist = (vecTargetPos - vecViewOrigin).length();
        float triangleSize = baseSize * std::clamp(600.f / dist, 0.8f, 1.2f);
        flRadius = screenDistance * (ImGui::GetIO().DisplaySize.y / 480.f);

        float flRadiusX = screenDistance * 100.0f / 100.f;
        float flRadiusY = screenDistance * 100.0f / 100.f;

        GetOffScreenData(vecDelta, flRadiusX, flRadiusY, vecOffScreenPos, flOffScreenRotation);
        flOffScreenRotation = -flOffScreenRotation;

        vArrowScreenPos = ImVec2(vecOffScreenPos.x, vecOffScreenPos.y);
        float flAngle = flOffScreenRotation + 180.0f;

        vP1 = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 18.f }, vArrowScreenPos, flAngle);
        vP2 = RotatePoint({ vArrowScreenPos.x - 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
        vP3 = RotatePoint({ vArrowScreenPos.x + 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
        vBackCenter = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 8.f }, vArrowScreenPos, flAngle);

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        ImU32 color = ImGui::ColorConvertFloat4ToU32(triangleColor.Value);

        hellcolor glow_color = triangleColor;
        glow_color.Value.w *= 0.6f;
        ImU32 glow_color_u32 = ImGui::ColorConvertFloat4ToU32(glow_color.Value);

        ImVec2 glow_points[3] = { vP1, vP2, vP3 };
        draw_list->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 15.0f, ImVec2(0, 0));
        draw_list->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 10.0f, ImVec2(0, 0));
        draw_list->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 6.0f, ImVec2(0, 0));

        draw_list->Flags |= ImDrawListFlags_AntiAliasedFill;
        draw_list->PathClear();
        draw_list->PathLineTo(ImVec2(vP1.x, vP1.y));
        draw_list->PathLineTo(ImVec2(vP2.x, vP2.y));
        draw_list->PathLineTo(vBackCenter);
        draw_list->PathLineTo(ImVec2(vP3.x, vP3.y));
        draw_list->PathFillConvex(color);
    }
}

void c_overlay::hitmarker() {
    if (!GET_VAR(bool, MISC_PATH(m_bHitMarker2d)))
        return;

    const float current_time = static_cast<float>(ImGui::GetTime());
    auto* draw_list = ImGui::GetBackgroundDrawList();

    const float fade_duration = 0.6f;
    const float max_size = 8.0f;
    const float base_thickness = 1.5f;
    const float gap = 3.0f;
    const hellcolor base_color_var = GET_VAR(hellcolor, MISC_PATH(m_colHitMarker2d));

    if (!m_hit_markers2.empty()) {
        auto it = std::remove_if(m_hit_markers2.begin(), m_hit_markers2.end(),
            [current_time, fade_duration](const hit_marker_tt& marker) {
                return (current_time - static_cast<float>(marker.time)) > fade_duration;
            });
        if (it != m_hit_markers2.end()) {
            m_hit_markers2.erase(it, m_hit_markers2.end());
        }
    }

    ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    auto draw_gradient_line_2d = [&](const ImVec2& start, const ImVec2& end, const hellcolor& color_start, const hellcolor& color_end, float thickness, float anim_progress, int corner_type) {
        ImVec2 anim_start = start;
        ImVec2 anim_end = end;

        if (anim_progress < 1.0f) {
            float anim_offset = (max_size + gap) * 2.0f;
            ImVec2 corner_offset;
            if (corner_type == 0) {
                corner_offset = ImVec2(-anim_offset, -anim_offset);
            }
            else if (corner_type == 1) {
                corner_offset = ImVec2(-anim_offset, anim_offset);
            }
            else if (corner_type == 2) {
                corner_offset = ImVec2(anim_offset, -anim_offset);
            }
            else {
                corner_offset = ImVec2(anim_offset, anim_offset);
            }

            float ease = 1.0f - (1.0f - anim_progress) * (1.0f - anim_progress) * (1.0f - anim_progress);
            anim_start = ImVec2(
                start.x + corner_offset.x * (1.0f - ease),
                start.y + corner_offset.y * (1.0f - ease)
            );
            anim_end = ImVec2(
                end.x + corner_offset.x * (1.0f - ease),
                end.y + corner_offset.y * (1.0f - ease)
            );
        }

        const int segments = 8;
        for (int i = 0; i < segments; ++i) {
            float t1 = (float)i / (float)segments;
            float t2 = (float)(i + 1) / (float)segments;

            ImVec2 p1 = ImVec2(anim_start.x + (anim_end.x - anim_start.x) * t1, anim_start.y + (anim_end.y - anim_start.y) * t1);
            ImVec2 p2 = ImVec2(anim_start.x + (anim_end.x - anim_start.x) * t2, anim_start.y + (anim_end.y - anim_start.y) * t2);

            hellcolor segment_color = hellcolor(
                color_start.Value.x + (color_end.Value.x - color_start.Value.x) * t1,
                color_start.Value.y + (color_end.Value.y - color_start.Value.y) * t1,
                color_start.Value.z + (color_end.Value.z - color_start.Value.z) * t1,
                color_start.Value.w + (color_end.Value.w - color_start.Value.w) * t1
            );

            draw_list->AddLine(p1, p2, segment_color, thickness);
        }
        };

    for (auto& marker : m_hit_markers2) {
        float elapsed = current_time - static_cast<float>(marker.time);
        float alpha = 1.0f - (elapsed / fade_duration);
        const float anim_duration = 0.15f;
        const float anim_progress = std::min(1.0f, elapsed / anim_duration);

        hellcolor base_color = base_color_var;
        base_color.Value.w = alpha;

        hellcolor core = base_color;
        hellcolor fade_color = core;
        fade_color.Value.w = 0.0f;

        hellcolor glow_color = core;
        glow_color.Value.w = alpha * 0.3f;
        hellcolor glow_fade = glow_color;
        glow_fade.Value.w = 0.0f;

        float glow_radius = (max_size + gap) * 1.5f;

        const int glow_segments = 64;
        const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
        const unsigned int vtx_base = draw_list->_VtxCurrentIdx;

        draw_list->PrimReserve(glow_segments * 3, glow_segments + 1);
        draw_list->PrimWriteVtx(center, uv, glow_color);

        for (int j = 0; j < glow_segments; ++j) {
            float angle = (float)j / (float)glow_segments * 2.0f * IM_PI;
            ImVec2 pos = ImVec2(center.x + cosf(angle) * glow_radius, center.y + sinf(angle) * glow_radius);
            draw_list->PrimWriteVtx(pos, uv, glow_fade);
        }

        for (int j = 0; j < glow_segments; ++j) {
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + j));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((j + 1) % glow_segments)));
        }

        draw_gradient_line_2d(
            ImVec2(center.x - gap - max_size, center.y - gap - max_size),
            ImVec2(center.x - gap, center.y - gap),
            fade_color, core, base_thickness, anim_progress, 0
        );

        draw_gradient_line_2d(
            ImVec2(center.x - gap - max_size, center.y + gap + max_size),
            ImVec2(center.x - gap, center.y + gap),
            fade_color, core, base_thickness, anim_progress, 1
        );

        draw_gradient_line_2d(
            ImVec2(center.x + gap + max_size, center.y - gap - max_size),
            ImVec2(center.x + gap, center.y - gap),
            fade_color, core, base_thickness, anim_progress, 2
        );

        draw_gradient_line_2d(
            ImVec2(center.x + gap + max_size, center.y + gap + max_size),
            ImVec2(center.x + gap, center.y + gap),
            fade_color, core, base_thickness, anim_progress, 3
        );
    }
}

void c_overlay::add_notification(std::string str_text, bool is_miss, bool is_config) {
    if (str_text.find("Shot was not registered by server") != std::string::npos ||
        str_text.find("target already dead") != std::string::npos) {
        return;
    }

    notification_t notification;
    notification.m_str_text = str_text;

    if (is_config) {
        notification.m_fl_remove_time = ImGui::GetTime() + 2.f;
        notification.m_fl_alpha = 0.f;
        notification.m_b_is_miss = is_miss;
        notification.m_b_is_config = is_config;
        notification.m_fl_spawn_time = ImGui::GetTime();
        notification.m_fl_anim_offset = 0.f;
        notification.m_fl_target_offset = 0.f;
        notification.m_fl_bounce_offset = 0.f;
        notification.m_fl_scale = 0.3f;
        notification.m_b_appearing = true;
        notification.m_b_disappearing = false;

        while (m_config_notifications.size() > 5)
            m_config_notifications.pop_back();

        m_config_notifications.emplace_front(notification);
    }
    else {
        if (!g_interfaces->m_global_vars)
            return;
        notification.m_fl_remove_time = g_interfaces->m_global_vars->m_curtime + 5.f;
        notification.m_fl_alpha = 0.f;
        notification.m_b_is_miss = is_miss;
        notification.m_b_is_config = is_config;
        notification.m_fl_spawn_time = g_interfaces->m_global_vars->m_curtime;
        notification.m_fl_anim_offset = 0.f;
        notification.m_fl_target_offset = 0.f;
        notification.m_fl_bounce_offset = 0.f;
        notification.m_fl_scale = 0.3f;
        notification.m_b_appearing = true;
        notification.m_b_disappearing = false;

        while (m_queue_notifications.size() > 15)
            m_queue_notifications.pop_back();

        m_queue_notifications.emplace_front(notification);
    }
}

void c_overlay::draw_notifications() {
    ImDrawList* p_draw_list = ImGui::GetBackgroundDrawList();
    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

    for (auto it = m_config_notifications.begin(); it != m_config_notifications.end(); ) {
        float current_time = ImGui::GetTime();
        float time_alive = current_time - it->m_fl_spawn_time;

        if (current_time > it->m_fl_remove_time && !it->m_b_disappearing) {
            it->m_b_disappearing = true;
            it->m_b_appearing = false;
        }

        if (it->m_b_appearing) {
            float appear_duration = 0.3f;
            float t = std::min(time_alive / appear_duration, 1.0f);

            float ease_out = 1.0f - powf(1.0f - t, 3.0f);
            it->m_fl_scale = 0.3f + (0.7f * ease_out);
            it->m_fl_alpha = t;
            it->m_fl_anim_offset = 0.f;

            if (t >= 1.0f) {
                it->m_b_appearing = false;
                it->m_fl_scale = 1.0f;
                it->m_fl_anim_offset = 0.f;
            }
        }
        else if (it->m_b_disappearing) {
            float disappear_duration = 0.3f;
            float disappear_start = current_time - it->m_fl_remove_time;
            float t = std::min(disappear_start / disappear_duration, 1.0f);

            it->m_fl_scale = 1.0f - (t * 0.3f);
            it->m_fl_alpha = 1.0f - t;
            it->m_fl_anim_offset = 0.f;
        }

        if (it->m_fl_alpha < 0.05f && it->m_b_disappearing) {
            it = m_config_notifications.erase(it);
        }
        else {
            ++it;
        }
    }

    if (!m_config_notifications.empty()) {
        ImFont* font = g_font_manager->m_gheist_medium_14;
        if (!font)
            font = g_font_manager->m_verdana_12;

        ImVec2 screen_size = ImGui::GetIO().DisplaySize;
        float padding_x = 8.0f;
        float padding_y = 6.0f;
        float rounding = 4.0f;
        float fl_start_y = screen_size.y - 10.0f;

        for (auto it = m_config_notifications.rbegin(); it != m_config_notifications.rend(); ++it) {
            auto& notification = *it;

            ImVec2 v_text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, notification.m_str_text.c_str());
            float rect_height = v_text_size.y + padding_y * 2.0f;
            float rect_width = padding_x + v_text_size.x + padding_x;
            float fl_start_x = screen_size.x - rect_width - 10.0f + notification.m_fl_anim_offset + notification.m_fl_bounce_offset;

            fl_start_y -= rect_height + 3.0f;

            ImVec2 gradient_min = ImVec2(fl_start_x, fl_start_y + 2.f);
            ImVec2 gradient_max = ImVec2(fl_start_x + rect_width, fl_start_y + 3.f + 5.f - 2.f);

            hellcolor gradient_accent_color = accent_color;
            gradient_accent_color.Value.w = notification.m_fl_alpha;
            p_draw_list->AddRectFilled(gradient_min, gradient_max, gradient_accent_color, rounding, ImDrawFlags_RoundCornersTop);

            ImVec2 rect_min = ImVec2(fl_start_x, fl_start_y + 3.0f);
            ImVec2 rect_max = ImVec2(fl_start_x + rect_width, fl_start_y + 3.0f + rect_height);

            hellcolor bg_color(29, 29, 29, static_cast<int>(255 * notification.m_fl_alpha));
            p_draw_list->AddRectFilled(rect_min, rect_max, bg_color, rounding, ImDrawFlags_RoundCornersAll);

            float x_pos = fl_start_x + padding_x;
            float y_pos = fl_start_y + 3.0f + padding_y;

            hellcolor base_text_color(255, 255, 255, static_cast<int>(255 * notification.m_fl_alpha));
            hellcolor accent_text_color = accent_color;
            accent_text_color.Value.w = notification.m_fl_alpha;

            std::string text = notification.m_str_text;

            auto draw_text_with_shadow = [&](const char* text_part, hellcolor color) {
                if (!text_part)
                    return;

                ImVec2 part_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, text_part);
                p_draw_list->AddText(font, font->LegacySize, ImVec2(x_pos, y_pos), color, text_part);
                x_pos += part_size.x;
                };

            size_t saved_pos = text.find("Saved the ");
            size_t loaded_pos = text.find("Loaded the ");
            size_t deleted_pos = text.find("Deleted the ");
            size_t created_pos = text.find("Created the ");

            if (saved_pos != std::string::npos) {
                draw_text_with_shadow("Saved the ", base_text_color);
                text = text.substr(10);
            }
            else if (loaded_pos != std::string::npos) {
                draw_text_with_shadow("Loaded the ", base_text_color);
                text = text.substr(11);
            }
            else if (deleted_pos != std::string::npos) {
                draw_text_with_shadow("Deleted the ", base_text_color);
                text = text.substr(12);
            }
            else if (created_pos != std::string::npos) {
                draw_text_with_shadow("Created the ", base_text_color);
                text = text.substr(12);
            }

            size_t config_pos = text.find(" config");
            if (config_pos != std::string::npos) {
                std::string config_name = text.substr(0, config_pos);
                draw_text_with_shadow(config_name.c_str(), accent_text_color);
                draw_text_with_shadow(" config", base_text_color);
            }
            else {
                draw_text_with_shadow(text.c_str(), base_text_color);
            }

            fl_start_y -= 2.0f;
        }
    }

    std::vector<bool> event_list = GET_VAR(std::vector<bool>, MISC_PATH(m_hitlog_modes));
    if (event_list.empty())
        return;

    const int hitlog_pos = std::clamp(GET_VAR(int, MISC_PATH(m_hitlog_position)), 0, 1);

    const bool is_in_game = g_interfaces->m_engine && g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game();
    const bool is_menu_open = g_menu && g_menu->m_menu_open;

    bool has_active_notifications = false;
    if (is_in_game && !m_queue_notifications.empty()) {
        for (auto it = m_queue_notifications.begin(); it != m_queue_notifications.end(); ) {
            bool should_show = false;
            if (it->m_b_is_miss) {
                should_show = (event_list.size() > 1) ? event_list[1] : false;
            }
            else {
                should_show = (event_list.size() > 0) ? event_list[0] : false;
            }

            if (should_show) {
                has_active_notifications = true;
                break;
            }
            ++it;
        }
    }

    const bool show_preview = is_menu_open && !has_active_notifications;

    if (!is_in_game && !show_preview)
        return;

    if (is_in_game && !has_active_notifications && !show_preview)
        return;

    if (!g_interfaces->m_global_vars)
        return;

    float fl_start_y = 10.f;
    ImVec2 hitlog_screen_size = ImGui::GetIO().DisplaySize;
    if (hitlog_pos == 0)
        fl_start_y = hitlog_screen_size.y * 0.5f + 270.f;

    if (show_preview && !has_active_notifications) {
        notification_t preview_hit;
        notification_t preview_miss;
        bool has_hit = false;
        bool has_miss = false;

        if (event_list.size() > 0 && event_list[0]) {
            preview_hit.m_str_text = "Hit PlayerName in the Head for 98 damage (12 remaining)";
            preview_hit.m_fl_remove_time = 0.0f;
            preview_hit.m_fl_alpha = 1.0f;
            preview_hit.m_b_is_miss = false;
            has_hit = true;
        }

        if (event_list.size() > 1 && event_list[1]) {
            preview_miss.m_str_text = "Missed PlayerName in the Head due to spread";
            preview_miss.m_fl_remove_time = 0.0f;
            preview_miss.m_fl_alpha = 1.0f;
            preview_miss.m_b_is_miss = true;
            has_miss = true;
        }

        if (has_hit || has_miss) {
            std::vector<notification_t> preview_notifications;
            if (has_hit) preview_notifications.push_back(preview_hit);
            if (has_miss) preview_notifications.push_back(preview_miss);

            ImFont* icon_font = g_font_manager->m_fa_small ? (ImFont*)g_font_manager->m_fa_small : nullptr;
            for (const auto& preview : preview_notifications) {
                ImFont* font = g_font_manager->m_gheist_medium_14;
                if (!font)
                    font = g_font_manager->m_verdana_12;

                ImVec2 v_text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, preview.m_str_text.c_str());

                float padding_x = 8.0f;
                float padding_y = 6.0f;
                const float icon_spacing = 6.0f;

                ImVec2 icon_size = icon_font ? icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, 0.0f, ICON_FA_CROSSHAIRS) : ImVec2(0.f, 0.f);

                float rect_height = v_text_size.y + padding_y * 2.0f;
                float rect_width = padding_x + (icon_font ? icon_size.x + icon_spacing : 0.f) + v_text_size.x + padding_x;

                float base_x = 10.f;
                if (hitlog_pos == 0)
                    base_x = hitlog_screen_size.x * 0.5f - rect_width * 0.5f;

                draw_gradient_rect(ImVec2(base_x, fl_start_y + 3.0f), ImVec2(rect_width, rect_height), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 180), p_draw_list);

                float x_pos = base_x + padding_x;
                float y_pos = fl_start_y + 3.0f + padding_y;

                hellcolor base_text_color(255, 255, 255, 255);
                hellcolor special_text_color;
                if (preview.m_b_is_miss) {
                    special_text_color = hellcolor(255, 150, 150, 255);
                }
                else {
                    special_text_color = accent_color;
                }

                hellcolor icon_color = preview.m_b_is_miss ? hellcolor(255, 150, 150, 255) : accent_color;
                if (icon_font) {
                    p_draw_list->AddText(icon_font, icon_font->LegacySize, ImVec2(x_pos, y_pos + (v_text_size.y - icon_size.y) * 0.5f), icon_color, ICON_FA_CROSSHAIRS);
                    x_pos += icon_size.x + icon_spacing;
                }

                std::string text = preview.m_str_text;
                float alpha_value = 1.0f;

                auto draw_text_with_shadow = [&](const char* text_part, hellcolor color) {
                    if (!text_part)
                        return;

                    ImVec2 part_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, text_part);
                    p_draw_list->AddText(font, font->LegacySize, ImVec2(x_pos, y_pos), color, text_part);
                    x_pos += part_size.x;
                    };

                if (preview.m_b_is_miss) {
                    size_t missed_pos = text.find("Missed ");
                    if (missed_pos != std::string::npos) {
                        draw_text_with_shadow("Missed ", base_text_color);
                        text = text.substr(7);
                    }

                    size_t in_the_pos = text.find(" in the ");
                    if (in_the_pos != std::string::npos) {
                        std::string player_part = text.substr(0, in_the_pos);
                        draw_text_with_shadow(player_part.c_str(), special_text_color);
                        draw_text_with_shadow(" in the ", base_text_color);
                        text = text.substr(in_the_pos + 8);

                        size_t due_to_pos = text.find(" due to ");
                        if (due_to_pos != std::string::npos) {
                            std::string hitbox_part = text.substr(0, due_to_pos);
                            draw_text_with_shadow(hitbox_part.c_str(), special_text_color);
                            draw_text_with_shadow(" due to ", base_text_color);
                            text = text.substr(due_to_pos + 8);

                            std::string reason = text;
                            if (!reason.empty()) {
                                reason[0] = std::toupper(reason[0]);
                            }
                            draw_text_with_shadow(reason.c_str(), special_text_color);
                        }
                    }
                }
                else {
                    size_t hit_pos = text.find("Hit ");
                    if (hit_pos != std::string::npos) {
                        draw_text_with_shadow("Hit ", base_text_color);
                        text = text.substr(4);
                    }

                    size_t in_the_pos = text.find(" in the ");
                    if (in_the_pos != std::string::npos) {
                        std::string player_part = text.substr(0, in_the_pos);
                        draw_text_with_shadow(player_part.c_str(), special_text_color);
                        draw_text_with_shadow(" in the ", base_text_color);
                        text = text.substr(in_the_pos + 8);

                        size_t for_pos = text.find(" for ");
                        if (for_pos != std::string::npos) {
                            std::string hitgroup_part = text.substr(0, for_pos);
                            draw_text_with_shadow(hitgroup_part.c_str(), special_text_color);
                            draw_text_with_shadow(" for ", base_text_color);
                            text = text.substr(for_pos + 5);

                            size_t damage_pos = text.find(" damage");
                            if (damage_pos != std::string::npos) {
                                std::string damage_part = text.substr(0, damage_pos);
                                draw_text_with_shadow(damage_part.c_str(), special_text_color);
                                draw_text_with_shadow(" damage", base_text_color);
                                std::string rest = text.substr(damage_pos + 7);
                                if (!rest.empty())
                                    draw_text_with_shadow(rest.c_str(), special_text_color);
                            }
                        }
                    }
                }

                fl_start_y += rect_height + 3.0f + 2.0f;
            }
        }
        return;
    }

    if (!is_in_game) {
        return;
    }

    if (m_queue_notifications.empty()) {
        return;
    }

    if (hitlog_pos == 0)
        fl_start_y = ImGui::GetIO().DisplaySize.y * 0.5f + 270.f;
    else
        fl_start_y = 10.f;

    ImFont* hitlog_icon_font = g_font_manager->m_fa_small ? (ImFont*)g_font_manager->m_fa_small : nullptr;

    for (auto it = m_queue_notifications.begin(); it != m_queue_notifications.end(); ) {
        bool should_show = false;
        if (it->m_b_is_miss) {
            should_show = (event_list.size() > 1) ? event_list[1] : false;
        }
        else {
            should_show = (event_list.size() > 0) ? event_list[0] : false;
        }

        if (!should_show) {
            ++it;
            continue;
        }

        if (!g_interfaces->m_global_vars) {
            ++it;
            continue;
        }

        float current_time = g_interfaces->m_global_vars->m_curtime;
        float time_alive = current_time - it->m_fl_spawn_time;

        if (current_time > it->m_fl_remove_time && !it->m_b_disappearing) {
            it->m_b_disappearing = true;
            it->m_b_appearing = false;
        }

        if (it->m_b_appearing) {
            float appear_duration = 0.3f;
            float t = std::min(time_alive / appear_duration, 1.0f);

            float ease_out = 1.0f - powf(1.0f - t, 3.0f);
            it->m_fl_scale = 0.3f + (0.7f * ease_out);
            it->m_fl_alpha = t;
            it->m_fl_anim_offset = 0.f;

            if (t >= 1.0f) {
                it->m_b_appearing = false;
                it->m_fl_scale = 1.0f;
                it->m_fl_anim_offset = 0.f;
            }
        }
        else if (it->m_b_disappearing) {
            float disappear_duration = 0.3f;
            float disappear_start = current_time - it->m_fl_remove_time;
            float t = std::min(disappear_start / disappear_duration, 1.0f);

            it->m_fl_scale = 1.0f - (t * 0.3f);
            it->m_fl_alpha = 1.0f - t;
            it->m_fl_anim_offset = 0.f;
        }

        if (it->m_fl_alpha < 0.05f && it->m_b_disappearing) {
            it = m_queue_notifications.erase(it);
        }
        else {
            ImFont* font = g_font_manager->m_gheist_medium_14;
            if (!font)
                font = g_font_manager->m_verdana_12;

            ImVec2 v_text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, it->m_str_text.c_str());

            const float icon_spacing = 6.0f;
            ImVec2 icon_size = hitlog_icon_font ? hitlog_icon_font->CalcTextSizeA(hitlog_icon_font->LegacySize, FLT_MAX, 0.0f, ICON_FA_CROSSHAIRS) : ImVec2(0.f, 0.f);

            float padding_x = 8.0f * it->m_fl_scale;
            float padding_y = 6.0f * it->m_fl_scale;
            float rect_height = (v_text_size.y + padding_y * 2.0f) * it->m_fl_scale;
            float rect_width = (padding_x + (hitlog_icon_font ? icon_size.x + icon_spacing : 0.f) + v_text_size.x + padding_x) * it->m_fl_scale;

            float scaled_font_size = font->LegacySize * it->m_fl_scale;
            float scaled_icon_size = hitlog_icon_font ? hitlog_icon_font->LegacySize * it->m_fl_scale : 0.f;

            float scale_offset = (1.0f - it->m_fl_scale) * rect_width * 0.5f;

            float base_x = 10.f;
            if (hitlog_pos == 0)
                base_x = ImGui::GetIO().DisplaySize.x * 0.5f - rect_width * 0.5f;
            base_x += it->m_fl_anim_offset + scale_offset;

            const int grad_alpha = std::min(static_cast<int>(180 * it->m_fl_alpha), 180);
            draw_gradient_rect(ImVec2(base_x, fl_start_y + 3.0f * it->m_fl_scale), ImVec2(rect_width, rect_height), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, grad_alpha), p_draw_list);

            float x_pos = base_x + padding_x;
            float y_pos = fl_start_y + 3.0f * it->m_fl_scale + padding_y;

            hellcolor base_text_color(255, 255, 255, static_cast<int>(255 * it->m_fl_alpha));
            hellcolor special_text_color;
            if (it->m_b_is_miss) {
                special_text_color = hellcolor(255, 150, 150, static_cast<int>(255 * it->m_fl_alpha));
            }
            else {
                special_text_color = accent_color;
                special_text_color.Value.w = it->m_fl_alpha;
            }

            std::string text = it->m_str_text;

            auto draw_text_with_shadow = [&](const char* text_part, hellcolor color) {
                if (!text_part)
                    return;

                ImVec2 part_size = font->CalcTextSizeA(scaled_font_size, FLT_MAX, 0.0f, text_part);
                p_draw_list->AddText(font, scaled_font_size, ImVec2(x_pos, y_pos), color, text_part);
                x_pos += part_size.x;
                };

            hellcolor hitlog_icon_color = it->m_b_is_miss ? hellcolor(255, 150, 150, static_cast<int>(255 * it->m_fl_alpha)) : accent_color;
            if (!it->m_b_is_miss)
                hitlog_icon_color.Value.w = it->m_fl_alpha;
            if (hitlog_icon_font && scaled_icon_size > 0.5f) {
                p_draw_list->AddText(hitlog_icon_font, scaled_icon_size, ImVec2(x_pos, y_pos + (v_text_size.y * it->m_fl_scale - icon_size.y * it->m_fl_scale) * 0.5f), hitlog_icon_color, ICON_FA_CROSSHAIRS);
                x_pos += (icon_size.x + icon_spacing) * it->m_fl_scale;
            }

            if (it->m_b_is_miss) {
                size_t missed_pos = text.find("Missed ");
                if (missed_pos != std::string::npos) {
                    draw_text_with_shadow("Missed ", base_text_color);
                    text = text.substr(7);
                }

                size_t in_the_pos = text.find(" in the ");
                if (in_the_pos != std::string::npos) {
                    std::string player_part = text.substr(0, in_the_pos);
                    draw_text_with_shadow(player_part.c_str(), special_text_color);
                    draw_text_with_shadow(" in the ", base_text_color);
                    text = text.substr(in_the_pos + 8);

                    size_t due_to_pos = text.find(" due to ");
                    if (due_to_pos != std::string::npos) {
                        std::string hitbox_part = text.substr(0, due_to_pos);
                        draw_text_with_shadow(hitbox_part.c_str(), special_text_color);
                        draw_text_with_shadow(" due to ", base_text_color);
                        text = text.substr(due_to_pos + 8);

                        std::string reason = text;
                        if (!reason.empty()) {
                            reason[0] = std::toupper(reason[0]);
                        }
                        draw_text_with_shadow(reason.c_str(), special_text_color);
                    }
                }
            }
            else {
                size_t hit_pos = text.find("Hit ");
                if (hit_pos != std::string::npos) {
                    draw_text_with_shadow("Hit ", base_text_color);
                    text = text.substr(4);
                }

                size_t in_the_pos = text.find(" in the ");
                if (in_the_pos != std::string::npos) {
                    std::string player_part = text.substr(0, in_the_pos);
                    draw_text_with_shadow(player_part.c_str(), special_text_color);
                    draw_text_with_shadow(" in the ", base_text_color);
                    text = text.substr(in_the_pos + 8);

                    size_t for_pos = text.find(" for ");
                    if (for_pos != std::string::npos) {
                        std::string hitgroup_part = text.substr(0, for_pos);
                        draw_text_with_shadow(hitgroup_part.c_str(), special_text_color);
                        draw_text_with_shadow(" for ", base_text_color);
                        text = text.substr(for_pos + 5);

                            size_t damage_pos = text.find(" damage");
                            if (damage_pos != std::string::npos) {
                                std::string damage_part = text.substr(0, damage_pos);
                                draw_text_with_shadow(damage_part.c_str(), special_text_color);
                                draw_text_with_shadow(" damage", base_text_color);
                                std::string rest = text.substr(damage_pos + 7);
                                if (!rest.empty())
                                    draw_text_with_shadow(rest.c_str(), special_text_color);
                            }
                        }
                    }
                }

            fl_start_y += rect_height + 3.0f * it->m_fl_scale + 2.0f;
            ++it;
        }
    }
}

void c_overlay::hotkey_list() {
    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_hotkey_list)))
        return;

    const bool is_in_game = g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game();
    const bool is_menu_open = g_menu && g_menu->m_menu_open;

    struct KeybindInfo {
        std::string display_text;
        std::string status_text;
        fnv1a_t holder_id;
        float activation_time;
    };

    static std::vector<KeybindInfo> persistent_keybinds;
    std::vector<KeybindInfo> current_active;

    auto& widget_keybinds = GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds));
    for (const auto& [widget_id, binds] : widget_keybinds) {
        for (const auto& keybind : binds) {
            if (keybind.m_key == -1) continue;
            std::string display_text = get_keybind_name(keybind.m_holder_id);
            if (display_text.empty()) continue;

            bool is_mindamage_keybind = (
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_light_pistol ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_deagle ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_revolver ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_smg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_lmg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_ar ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_shotgun ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_scout ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_autosniper ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_awp
                );

            bool is_hitchance_keybind = (
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_light_pistol ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_deagle ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_revolver ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_smg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_lmg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_ar ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_shotgun ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_scout ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_autosniper ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_awp
                );

            bool is_pointscale_keybind = (
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_light_pistol ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_deagle ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_revolver ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_smg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_lmg ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_ar ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_shotgun ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_scout ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_autosniper ||
                keybind.m_holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_awp
                );

            if (is_mindamage_keybind || is_hitchance_keybind || is_pointscale_keybind) {
                if (!g_ctx->m_active_weapon) continue;
                int weapon_rage_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
                if (weapon_rage_type == 10) continue;

                fnv1a_t current_weapon_holder_id;
                if (is_mindamage_keybind)
                    current_weapon_holder_id = tabs::aimbot::get_min_damage_holder_id(weapon_rage_type);
                else if (is_hitchance_keybind)
                    current_weapon_holder_id = tabs::aimbot::get_hitchance_holder_id(weapon_rage_type);
                else if (is_pointscale_keybind)
                    current_weapon_holder_id = tabs::aimbot::get_pointscale_holder_id(weapon_rage_type);

                if (keybind.m_holder_id != current_weapon_holder_id) continue;
            }

            bool key_pressed = g_input->key_down(keybind.m_key);
            bool should_show = false;
            std::string status_text;

            if (keybind.m_keybind_type == e_keybind_type::keybind_type_checkbox) {
                bool current_value = GET_VAR(bool, keybind.m_holder_id);
                if (keybind.m_mode == e_key_mode::key_mode_hold) {
                    if (key_pressed) {
                        should_show = true;
                        status_text = keybind.m_is_on_mode ? "ON" : "OFF";
                    }
                }
                else if (keybind.m_mode == e_key_mode::key_mode_toggle) {
                    if (keybind.m_is_on_mode) {
                        if (keybind.m_on_mode_activated && current_value) {
                            should_show = true;
                            status_text = "ON";
                        }
                    }
                    else {
                        if (keybind.m_keybind_active && !current_value) {
                            should_show = true;
                            status_text = "OFF";
                        }
                    }
                }
            }
            else if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_int) {
                if (keybind.m_keybind_active && std::holds_alternative<int>(keybind.m_override_val)) {
                    should_show = true;
                    int override_val = std::get<int>(keybind.m_override_val);
                    status_text = std::to_string(override_val);
                }
            }
            else if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_float) {
                if (keybind.m_keybind_active && std::holds_alternative<float>(keybind.m_override_val)) {
                    should_show = true;
                    float override_val = std::get<float>(keybind.m_override_val);
                    status_text = std::to_string((int)override_val);
                }
            }

            if (should_show) {
                auto existing = std::find_if(persistent_keybinds.begin(), persistent_keybinds.end(),
                    [&](const KeybindInfo& info) { return info.holder_id == keybind.m_holder_id; });

                if (existing != persistent_keybinds.end()) {
                    existing->display_text = display_text;
                    existing->status_text = status_text;
                    current_active.push_back(*existing);
                }
                else {
                    KeybindInfo new_bind = { display_text, status_text, keybind.m_holder_id, ImGui::GetTime() };
                    persistent_keybinds.push_back(new_bind);
                    current_active.push_back(new_bind);

                    m_hotkey_anim.keybind_activation_times[keybind.m_holder_id] = ImGui::GetTime();
                    m_hotkey_anim.target_text_alphas[keybind.m_holder_id] = 1.0f;
                }
            }
        }
    }

    persistent_keybinds.erase(std::remove_if(persistent_keybinds.begin(), persistent_keybinds.end(),
        [&](const KeybindInfo& info) {
            bool found = std::find_if(current_active.begin(), current_active.end(),
                [&](const KeybindInfo& active) { return active.holder_id == info.holder_id; }) != current_active.end();

            if (!found)
                m_hotkey_anim.target_text_alphas[info.holder_id] = 0.0f;
            return !found;
        }), persistent_keybinds.end());

    std::sort(current_active.begin(), current_active.end(),
        [](const KeybindInfo& a, const KeybindInfo& b) {
            return a.activation_time < b.activation_time;
        });

    bool has_active = !current_active.empty();
    m_hotkey_anim.has_active_keybinds = has_active;

    bool should_show_widget = has_active || is_menu_open;

    static bool prev_menu_state = false;
    static bool prev_has_active = false;

    if (has_active && !prev_has_active)
        m_hotkey_anim.target_widget_alpha = 1.0f;
    else if (!has_active && prev_has_active)
        m_hotkey_anim.target_widget_alpha = 0.0f;

    float lerp_speed = 8.0f * ImGui::GetIO().DeltaTime;

    if (is_menu_open) {
        if (!has_active)
            m_hotkey_anim.widget_alpha = 1.0f;
        else
            m_hotkey_anim.widget_alpha += (m_hotkey_anim.target_widget_alpha - m_hotkey_anim.widget_alpha) * lerp_speed;
    }
    else {
        if (has_active)
            m_hotkey_anim.widget_alpha += (m_hotkey_anim.target_widget_alpha - m_hotkey_anim.widget_alpha) * lerp_speed;
        else {
            if (prev_menu_state && !is_menu_open)
                m_hotkey_anim.widget_alpha = 0.0f;
            else
                m_hotkey_anim.widget_alpha += (m_hotkey_anim.target_widget_alpha - m_hotkey_anim.widget_alpha) * lerp_speed;
        }
    }

    prev_menu_state = is_menu_open;
    prev_has_active = has_active;

    if (!should_show_widget && m_hotkey_anim.widget_alpha < 0.01f) return;

    auto draw_list = ImGui::GetBackgroundDrawList();
    ImFont* font = g_font_manager->m_gheist_medium_14 ? g_font_manager->m_gheist_medium_14 : g_font_manager->m_inter_4002;
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    const float padding_x = 12.f;
    const float padding_y = 10.f;
    const float rounding = 4.f;
    const float header_height = 25.f;
    const float outline_height = 5.f;
    const float item_height = 24.f;
    const float base_width = 150.f;

    const float base_content_height = 20.f;

    float max_width = base_width;
    for (const auto& bind : current_active) {
        ImVec2 name_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, bind.display_text.c_str());
        
        std::string display_status = bind.status_text;
        if (display_status == "ON") {
            display_status = "on";
        }
        ImVec2 status_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, display_status.c_str());
        
        float total_width = name_size.x + status_size.x + padding_x * 3.f;
        max_width = std::max(max_width, total_width);
    }

    m_hotkey_anim.target_widget_width = max_width;
    m_hotkey_anim.widget_width = max_width;

    float content_height = has_active ? (current_active.size() * item_height) : (base_content_height * 0.5f);
    float total_height = header_height + 3.f + content_height;
    m_hotkey_anim.target_base_rect_height = total_height;

    m_hotkey_anim.base_rect_height += (m_hotkey_anim.target_base_rect_height - m_hotkey_anim.base_rect_height) * lerp_speed;

    static bool position_initialized = false;
    if (!position_initialized || m_hotkey_anim.widget_pos.x < 0.0f || m_hotkey_anim.widget_pos.y < 0.0f) {
        m_hotkey_anim.widget_pos = ImVec2(screen_size.x - 200.f, screen_size.y * 0.35f);
        position_initialized = true;
    }

    if (is_menu_open) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        ImVec2 rect_min = m_hotkey_anim.widget_pos;
        ImVec2 rect_max = ImVec2(rect_min.x + m_hotkey_anim.widget_width, rect_min.y + m_hotkey_anim.base_rect_height);

        bool hovered = mouse_pos.x >= rect_min.x && mouse_pos.x <= rect_max.x && mouse_pos.y >= rect_min.y && mouse_pos.y <= rect_max.y;

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_hotkey_anim.dragging = true;
            m_hotkey_anim.drag_offset = ImVec2(mouse_pos.x - rect_min.x, mouse_pos.y - rect_min.y);
        }

        if (m_hotkey_anim.dragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_hotkey_anim.widget_pos = ImVec2(mouse_pos.x - m_hotkey_anim.drag_offset.x,
                    mouse_pos.y - m_hotkey_anim.drag_offset.y);
            }
            else
                m_hotkey_anim.dragging = false;
        }
    }

    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

    ImVec2 outline_min = ImVec2(m_hotkey_anim.widget_pos.x, m_hotkey_anim.widget_pos.y + 2.f);
    ImVec2 outline_max = ImVec2(m_hotkey_anim.widget_pos.x + m_hotkey_anim.widget_width, m_hotkey_anim.widget_pos.y + 3.f + outline_height - 2.f);
    hellcolor outline_color = accent_color;
    outline_color.Value.w *= m_hotkey_anim.widget_alpha;
    draw_list->AddRectFilled(outline_min, outline_max, outline_color, rounding, ImDrawFlags_RoundCornersTop);

    ImVec2 header_min = ImVec2(m_hotkey_anim.widget_pos.x, m_hotkey_anim.widget_pos.y + 3.f);
    ImVec2 header_max = ImVec2(m_hotkey_anim.widget_pos.x + m_hotkey_anim.widget_width, m_hotkey_anim.widget_pos.y + 3.f + header_height);
    hellcolor header_bg_color(30, 30, 30, (int)(255 * m_hotkey_anim.widget_alpha));
    draw_list->AddRectFilled(header_min, header_max, header_bg_color, rounding, ImDrawFlags_RoundCornersTop);

    ImVec2 header_text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, "Hot Keys List");
    ImVec2 header_text_pos = ImVec2(header_min.x + (m_hotkey_anim.widget_width - header_text_size.x) * 0.5f, header_min.y + (header_height - header_text_size.y) * 0.5f);
    hellcolor header_text_color(255, 255, 255, (int)(255 * m_hotkey_anim.widget_alpha));
    draw_list->AddText(font, font->LegacySize, header_text_pos, header_text_color, "Hot Keys List");

    ImVec2 content_min = ImVec2(m_hotkey_anim.widget_pos.x, header_max.y);
    ImVec2 content_max = ImVec2(m_hotkey_anim.widget_pos.x + m_hotkey_anim.widget_width, m_hotkey_anim.widget_pos.y + m_hotkey_anim.base_rect_height);
    
    if (hell::blur::m_initialized) {
        ImVec2 rect_size = ImVec2(content_max.x - content_min.x, content_max.y - content_min.y);
        hell::blur::blur_effect.begin_blur();
        hell::blur::blur_effect.apply_blur(draw_list, content_min, rect_size, 0.0f, 6.0f, ImDrawFlags_RoundCornersBottom);
        hell::blur::blur_effect.end_blur();
    }
    hellcolor content_bg_color(29, 29, 29, (int)(150 * m_hotkey_anim.widget_alpha));
    
    draw_list->AddRectFilled(content_min, content_max, content_bg_color, rounding, ImDrawFlags_RoundCornersBottom);

    if (has_active) {
        ImVec2 header_only_max = ImVec2(m_hotkey_anim.widget_pos.x + m_hotkey_anim.widget_width, m_hotkey_anim.widget_pos.y + 3.f + header_height - 4.f);
        draw_list->PushClipRect(content_min, content_max, true);
        float current_y = content_min.y;

        for (const auto& bind : current_active) {
            if (m_hotkey_anim.text_alphas.find(bind.holder_id) == m_hotkey_anim.text_alphas.end())
                m_hotkey_anim.text_alphas[bind.holder_id] = 0.0f;

            if (m_hotkey_anim.target_text_alphas.find(bind.holder_id) == m_hotkey_anim.target_text_alphas.end())
                m_hotkey_anim.target_text_alphas[bind.holder_id] = 1.0f;

            float& alpha = m_hotkey_anim.text_alphas[bind.holder_id];
            float target_alpha = m_hotkey_anim.target_text_alphas[bind.holder_id];
            alpha += (target_alpha - alpha) * lerp_speed;

            if (alpha > 0.01f) {
                ImVec2 name_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, bind.display_text.c_str());
                
                std::string display_status = bind.status_text;
                if (display_status == "ON") {
                    display_status = "on";
                }
                ImVec2 status_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, display_status.c_str());

                ImVec2 name_pos = ImVec2(content_min.x + padding_x, current_y + (item_height - name_size.y) * 0.5f);
                ImVec2 status_pos = ImVec2(content_max.x - padding_x - status_size.x, current_y + (item_height - status_size.y) * 0.5f);

                hellcolor name_color(255, 255, 255, (int)(255 * alpha * m_hotkey_anim.widget_alpha));
                accent_color.Value.w *= alpha * m_hotkey_anim.widget_alpha;

                draw_list->AddText(font, font->LegacySize, name_pos, name_color, bind.display_text.c_str());
                draw_list->AddText(font, font->LegacySize, status_pos, accent_color, display_status.c_str());
            }

            current_y += item_height;
        }
        draw_list->PopClipRect();
    }
}

std::string c_overlay::get_keybind_name(fnv1a_t holder_id) {
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_enabled_ragebot) return "Ragebot";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_autofire) return "Auto Fire";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_silent_aim) return "Silent Aim";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_autostop) return "Auto Stop";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_taser_bot) return "Taser Bot";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_auto_scope) return "Auto Scope";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_duck_peek) return "Duck Peek";

    if (holder_id == g_vars->m_aimbot.m_antiaim.m_enabled_antiaim) return "Anti-Aim";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_at_target) return "At Target";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_hide_shots) return "Hide Shots";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_override_left) return "Override Left";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_override_right) return "Override Right";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_yaw_jitter) return "Yaw Jitter";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_pitch_jitter) return "Pitch Jitter";

    if (holder_id == g_vars->m_visuals.m_visualize_hitboxes) return "Visualize Hitboxes";
    if (holder_id == g_vars->m_visuals.m_shot_sparks_enabled) return "Shot Sparks";
    if (holder_id == g_vars->m_visuals.m_enabled_kill_effects) return "Kill Effects";
    if (holder_id == g_vars->m_visuals.m_chams_enabled) return "Chams";
    if (holder_id == g_vars->m_visuals.m_chams_enabled_bt) return "Backtrack Chams";
    if (holder_id == g_vars->m_visuals.m_chams_enabled_os) return "On Shot Chams";
    if (holder_id == g_vars->m_visuals.m_chams_local_enabled) return "Local Chams";
    if (holder_id == g_vars->m_visuals.m_chams_local_enabled_bt) return "Local BT Chams";
    if (holder_id == g_vars->m_visuals.m_esp_enabled) return "ESP";
    if (holder_id == g_vars->m_visuals.m_bbox) return "Bounding Box";
    if (holder_id == g_vars->m_visuals.m_visible_only) return "Visible Only";
    if (holder_id == g_vars->m_visuals.m_name) return "Name ESP";
    if (holder_id == g_vars->m_visuals.m_ping) return "Ping ESP";
    if (holder_id == g_vars->m_visuals.m_weapon_text) return "Weapon Text";
    if (holder_id == g_vars->m_visuals.m_weapon_icon) return "Weapon Icon";
    if (holder_id == g_vars->m_visuals.m_health_bar) return "Health Bar";
    if (holder_id == g_vars->m_visuals.m_armor_bar) return "Armor Bar";
    if (holder_id == g_vars->m_visuals.m_skeleton_esp_enabled) return "Skeleton ESP";
    if (holder_id == g_vars->m_visuals.m_third_person_enabled) return "Third Person";
    if (holder_id == g_vars->m_visuals.m_remove_visual_punch) return "No Visual Recoil";
    if (holder_id == g_vars->m_visuals.m_enable_world_modulation) return "World Modulation";
    if (holder_id == g_vars->m_visuals.m_enable_grenade_prediction) return "Grenade Prediction";
    if (holder_id == g_vars->m_visuals.m_enable_proximity_warnings) return "Proximity Warnings";
    if (holder_id == g_vars->m_visuals.m_enable_grenade_names) return "Grenade Names";
    if (holder_id == g_vars->m_visuals.m_enable_inferno_radius) return "Inferno Radius";
    if (holder_id == g_vars->m_visuals.m_local_impact_boxes) return "Impact Boxes";
    if (holder_id == g_vars->m_visuals.m_local_tracers) return "Bullet Tracers";
    if (holder_id == g_vars->m_visuals.m_local_glow) return "Local Glow";
    if (holder_id == g_vars->m_visuals.m_transparency_in_scope) return "Scope Transparency";
    if (holder_id == g_vars->m_visuals.m_teamate_glow) return "Teammate Glow";
    if (holder_id == g_vars->m_visuals.m_enemy_glow) return "Enemy Glow";
    if (holder_id == g_vars->m_visuals.m_scope_overlay) return "Scope Overlay";
    if (holder_id == g_vars->m_visuals.m_enabled_3d_damage_markers) return "3D Damage Markers";
    if (holder_id == g_vars->m_visuals.m_enabled_3d_hitmarkers) return "3D Hitmarkers";
    if (holder_id == g_vars->m_visuals.m_enabled_spread_circle) return "Spread Circle";
    if (holder_id == g_vars->m_visuals.m_bomb_hud_enabled) return "Bomb HUD";
    if (holder_id == g_vars->m_visuals.m_enabled_spread_gap) return "Spread Gap";
    if (holder_id == g_vars->m_visuals.m_enabled_hotkey_list) return "Hotkey List";

    if (holder_id == g_vars->m_misc.m_enabled_ramp_boost) return "Ramp Boost";
    if (holder_id == g_vars->m_misc.m_enabled_bunny_hop) return "Bunny Hop";
    if (holder_id == g_vars->m_misc.m_enabled_jump_bug) return "Jump Bug";
    if (holder_id == g_vars->m_misc.m_enabled_edge_jump) return "Edge Jump";
    if (holder_id == g_vars->m_misc.m_enabled_slow_walk) return "Slow Walk";
    if (holder_id == g_vars->m_misc.m_enabled_auto_peek) return "Auto Peek";
    if (holder_id == g_vars->m_misc.m_enabled_autostrafe) return "Auto Strafe";
    if (holder_id == g_vars->m_misc.m_enabled_hitsound) return "Hit Sound";
    if (holder_id == g_vars->m_misc.m_enabled_watermark) return "Watermark";
    if (holder_id == g_vars->m_misc.m_preserve_kill_feed) return "Preserve Kill Feed";

    if (holder_id == g_vars->m_skins.m_enabled_skinchanger) return "Skin Changer";

    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_light_pistol) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_deagle) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_revolver) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_smg) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_lmg) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_ar) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_shotgun) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_scout) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_autosniper) return "Min Damage Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_mindamage_awp) return "Min Damage Override";

    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_light_pistol) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_deagle) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_revolver) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_smg) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_lmg) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_ar) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_shotgun) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_scout) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_autosniper) return "Hit Chance Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_hitchance_awp) return "Hit Chance Override";

    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_light_pistol) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_deagle) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_revolver) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_smg) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_lmg) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_ar) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_shotgun) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_scout) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_autosniper) return "Point Scale Override";
    if (holder_id == g_vars->m_aimbot.m_ragebot.m_pointscale_awp) return "Point Scale Override";

    if (holder_id == g_vars->m_aimbot.m_antiaim.m_yaw) return "Yaw";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_pitch) return "Pitch";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_yaw_jitter_amount) return "Yaw Jitter Amount";
    if (holder_id == g_vars->m_aimbot.m_antiaim.m_pitch_jitter_amount) return "Pitch Jitter Amount";

    if (holder_id == g_vars->m_visuals.m_visualize_hitbox_fov) return "Hitbox FOV";
    if (holder_id == g_vars->m_visuals.m_kill_effects_type) return "Kill Effects Type";
    if (holder_id == g_vars->m_visuals.m_override_world_fov_value) return "World FOV";
    if (holder_id == g_vars->m_visuals.m_override_viewmodel_value) return "Viewmodel FOV";
    if (holder_id == g_vars->m_visuals.m_skeleton_esp_backtrack_enabled) return "Backtrack Skeleton";
    if (holder_id == g_vars->m_visuals.m_skeleton_esp_onshot_enabled) return "On Shot Skeleton";
    if (holder_id == g_vars->m_visuals.m_third_person_distance) return "Third Person Distance";
    if (holder_id == g_vars->m_visuals.m_scope_overlay_gap) return "Scope Gap";
    if (holder_id == g_vars->m_visuals.m_scope_overlay_length) return "Scope Length";
    
    if (holder_id == g_vars->m_misc.m_enabled_straight_throw) return "Straight Throw";
    if (holder_id == g_vars->m_misc.m_slow_walk_percent) return "Slow Walk %";
    if (holder_id == g_vars->m_misc.m_autostrafe_boost) return "Auto Strafe Boost";
    if (holder_id == g_vars->m_misc.m_hitsound_volume) return "Hit Sound Volume";
    if (holder_id == g_vars->m_misc.m_hitsound_selection) return "Hit Sound Type";
    if (holder_id == g_vars->m_misc.m_grenade_release_damage) return "Grenade Release DMG %";
    if (holder_id == g_vars->m_misc.m_enabled_grenade_release) return "Grenade Release";

    return "Unknown";
}

void c_overlay::crosshair_indicators() {
    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_hotkey_list)))
        return;

    if (!GET_VAR(bool, VISUALS_PATH(m_scope_overlay)))
        return;

    int scope_type = GET_VAR(int, VISUALS_PATH(m_scope_type));
    if (scope_type != e_scope_type::scope_type_static && scope_type != e_scope_type::scope_type_full)
        return;

    auto local_pawn = g_interfaces->m_entity_system->get_player_pawn(0);
    if (!local_pawn || local_pawn->m_iHealth() <= 0 || !local_pawn->m_bIsScoped())
        return;

    bool slow_condition = false;
    if (GET_VAR(bool, MISC_PATH(m_enabled_slow_walk))) {
        slow_condition = true;
    }

    bool left_condition = GET_VAR(bool, ANTIAIM_PATH(m_override_left)) && GET_VAR(bool, ANTIAIM_PATH(m_enabled_antiaim));
    bool right_condition = GET_VAR(bool, ANTIAIM_PATH(m_override_right)) && GET_VAR(bool, ANTIAIM_PATH(m_enabled_antiaim));

    bool dmg_condition = false;
    std::string dmg_text = "MD 0";
    if (g_ctx->m_active_weapon && GET_VAR(bool, RAGEBOT_PATH(m_enabled_ragebot))) {
        int weapon_rage_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
        if (weapon_rage_type != 10) {
            fnv1a_t dmg_holder_id = tabs::aimbot::get_min_damage_holder_id(weapon_rage_type);
            int min_damage = GET_VAR(int, dmg_holder_id);
            if (min_damage > 0) {
                dmg_condition = true;
                if (min_damage < 100) {
                    char buffer[32];
                    std::snprintf(buffer, sizeof(buffer), "MD %d", min_damage);
                    dmg_text = buffer;
                }
                else if (min_damage == 100) {
                    dmg_text = "HP";
                }
                else {
                    char buffer[32];
                    std::snprintf(buffer, sizeof(buffer), "HP+%d", min_damage - 100);
                    dmg_text = buffer;
                }
            }
        }
    }

    bool hc_condition = false;
    std::string hc_text = "HC 0";
    if (g_ctx->m_active_weapon && GET_VAR(bool, RAGEBOT_PATH(m_enabled_ragebot))) {
        int weapon_rage_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
        if (weapon_rage_type != 10) {
            fnv1a_t hc_holder_id = tabs::aimbot::get_hitchance_holder_id(weapon_rage_type);
            int hitchance = GET_VAR(int, hc_holder_id);
            if (hitchance > 0) {
                hc_condition = true;
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "HC %d", hitchance);
                hc_text = buffer;
            }
        }
    }

    bool peek_condition = GET_VAR(bool, MISC_PATH(m_enabled_auto_peek));

    std::vector<std::string> active_indicators;
    if (slow_condition) active_indicators.push_back("SLOW");
    if (left_condition) active_indicators.push_back("LEFT");
    if (right_condition) active_indicators.push_back("RIGHT");
    if (dmg_condition) active_indicators.push_back(dmg_text);
    if (hc_condition) active_indicators.push_back(hc_text);
    if (peek_condition) active_indicators.push_back("PEEK");

    if (active_indicators.empty())
        return;

    auto draw_list = ImGui::GetBackgroundDrawList();
    ImFont* font = g_font_manager->m_smallest_pixel;
    if (!font)
        return;

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    ImVec2 screen_center = ImVec2(screen_size.x * 0.5f, screen_size.y * 0.5f);

    float offset_x;
    if (scope_type == e_scope_type::scope_type_static) {
        float base_gap = (float)GET_VAR(int, VISUALS_PATH(m_scope_overlay_gap));
        float spread_addition = 0.f;

        if (GET_VAR(bool, VISUALS_PATH(m_enabled_spread_gap))) {
            const auto weapon = g_ctx->m_active_weapon;
            const auto local_pawn = g_ctx->m_local_pawn;
            if (weapon && local_pawn) {
                constexpr float s_scale = 233.f;
                float target_spread = (weapon->get_inaccuracy() + weapon->get_spread()) * local_pawn->m_vecAbsVelocity().length_2d() * (screen_size.y / s_scale / 2.f);
                static float s_spread_gap = 0.f;
                s_spread_gap += (target_spread - s_spread_gap) * 0.15f;
                spread_addition = s_spread_gap;
            }
        }

        float gap = base_gap + spread_addition;
        offset_x = gap + 0.5f;
    }
    else {
        offset_x = scope_type == e_scope_type::scope_type_full ? 5.0f : 20.0f;
    }
    const float offset_y = -8.0f;
    const float line_spacing = 2.0f;

    float total_height = 0.0f;
    for (const auto& text : active_indicators) {
        ImVec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, text.c_str());
        total_height += text_size.y;
        if (&text != &active_indicators.back()) {
            total_height += line_spacing;
        }
    }

    float start_y = screen_center.y + offset_y - total_height;
    float current_y = start_y;

    hellcolor text_color(255, 255, 255, 255);
    hellcolor shadow_color(0, 0, 0, 255);

    for (const auto& text : active_indicators) {
        ImVec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, text.c_str());
        ImVec2 text_pos(screen_center.x + offset_x, current_y);

        static const ImVec2 outline_offsets[8] = {
            { -1, -1 },
            { -1, 1 },
            { 1, -1 },
            { 1, 1 },
            { 0, 1 },
            { 1, 0 },
            { 0, -1 },
            { -1, 0 },
        };

        for (const ImVec2& off : outline_offsets) {
            draw_list->AddText(font, font->LegacySize, ImVec2(text_pos.x + off.x, text_pos.y + off.y), shadow_color, text.c_str());
        }

        draw_list->AddText(font, font->LegacySize, text_pos, text_color, text.c_str());

        current_y += text_size.y + line_spacing;
    }
}

void c_overlay::watermark() {
    if (!GET_VAR(bool, MISC_PATH(m_enabled_watermark)))
        return;

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    if (screen_size.x <= 0.f || screen_size.y <= 0.f)
        return;

    ImFont* font = g_font_manager->m_gheist_medium_14 ? g_font_manager->m_gheist_medium_14 : g_font_manager->m_inter_4002;
    if (!font)
        return;

    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

    static float last_fps = 60.f;
    static float time_accumulator = 0.f;
    time_accumulator += ImGui::GetIO().DeltaTime;
    if (time_accumulator >= 1.0f) {
        last_fps = ImGui::GetIO().Framerate;
        time_accumulator = 0.f;
    }

    const bool in_game = g_interfaces->m_engine && g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game();

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char time_str[64]{};
    struct tm time_info{};
    localtime_s(&time_info, &now_c);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", &time_info);

    std::string suffix = "  " + std::to_string((int)last_fps) + " fps  " + std::to_string(get_cpu_usage()) + "% cpu  ";
    if (in_game)
        suffix += std::to_string(get_local_ping()) + " ms  " + std::to_string(get_local_speed()) + " speed  ";
    suffix += std::string(time_str);

    const char* text1 = "Flow";
    const char* text2 = "sense";
    const char* username = "user";

    ImVec2 text1_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text1);
    ImVec2 text2_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text2);
    ImVec2 user_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, username);
    ImVec2 suffix_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, suffix.c_str());

    float total_width = text1_size.x + text2_size.x + user_size.x + suffix_size.x + 10.f;
    float total_height = text1_size.y;

    float pos_x = screen_size.x - total_width - 15.f;
    float pos_y = 15.f;

    ImVec2 bg_pos = ImVec2(pos_x - 5.f, pos_y - 2.5f);
    ImVec2 bg_size = ImVec2(total_width + 10.f, total_height + 6.f);

    auto draw_list = ImGui::GetBackgroundDrawList();

    draw_gradient_rect(bg_pos, bg_size, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 180), draw_list);

    float current_x = pos_x;
    hellcolor white_color(255, 255, 255, 255);

    draw_list->AddText(font, font->LegacySize, ImVec2(current_x, pos_y - 1.f), white_color, text1);
    current_x += text1_size.x;

    draw_list->AddText(font, font->LegacySize, ImVec2(current_x - 0.7f, pos_y - 1.f), accent_color, text2);
    current_x += text2_size.x;

    draw_list->AddText(font, font->LegacySize, ImVec2(current_x + 6.f, pos_y - 1.f), white_color, username);
    current_x += user_size.x + 3.f;

    draw_list->AddText(font, font->LegacySize, ImVec2(current_x + 3.f, pos_y - 1.f), white_color, suffix.c_str());
}

void c_overlay::indicators() {
    if (!GET_VAR(bool, MISC_PATH(m_enabled_indicators)))
        return;

    if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive())
        return;

    ImFont* font = g_font_manager->m_gheist_medium_14 ? g_font_manager->m_gheist_medium_14 : g_font_manager->m_inter_4002;
    ImFont* icon_font = g_font_manager->m_fa_small ? (ImFont*)g_font_manager->m_fa_small : nullptr;
    if (!font || !icon_font)
        return;

    std::vector<bool> modes = GET_VAR(std::vector<bool>, MISC_PATH(m_indicator_modes));
    auto mode_on = [&](size_t i) -> bool { return i < modes.size() ? modes[i] : false; };

    const bool rage_on = GET_VAR(bool, RAGEBOT_PATH(m_enabled_ragebot));
    const bool aa_on = GET_VAR(bool, ANTIAIM_PATH(m_enabled_antiaim));

    int hitchance = 0;
    int mindamage = 0;
    if (rage_on) {
        const int weapon_type = get_ragebot_weapon_type(g_ctx->m_weapon_def_index);
        if (weapon_type != 10) {
            hitchance = GET_VAR(int, tabs::aimbot::get_hitchance_holder_id(weapon_type));
            mindamage = GET_VAR(int, tabs::aimbot::get_min_damage_holder_id(weapon_type));
        }
    }

    std::string hc_text = hitchance > 0 ? std::to_string(hitchance) : "AUTO";
    std::string dmg_text = mindamage > 0 ? std::to_string(mindamage) : "LETHAL";

    const int ping = get_local_ping();

    std::string bomb_text;
    const bool bomb_planted = bomb::is_bomb_planted();
    if (bomb_planted) {
        const int site = bomb::get_bomb_site();
        const float time_left = std::max(0.f, bomb::get_time_to_explode());
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%s - %.1fs", site == 1 ? "B" : "A", (double)time_left);
        bomb_text = buf;
    }

    struct indicator_row_t {
        const char* icon;
        std::string text;
        bool active;
        int style; // 0 normal, 1 warning (ping), 2 bomb
    };

    indicator_row_t rows[10] = {
        { ICON_FA_BOLT, "RAGE", mode_on(0) && rage_on, 0 },
        { ICON_FA_EYE, "HIDE SHOTS", mode_on(1) && aa_on && GET_VAR(bool, ANTIAIM_PATH(m_hide_shots)), 0 },
        { ICON_FA_CROSSHAIRS, "FORCE SHOOT", mode_on(2) && GET_VAR(bool, RAGEBOT_PATH(m_force_shoot)), 0 },
        { ICON_FA_WIFI, "PING", mode_on(3) && ping >= 100, 1 },
        { ICON_FA_BULLSEYE, hc_text, mode_on(4) && rage_on, 0 },
        { ICON_FA_BOMB, bomb_text, mode_on(5) && bomb_planted, 2 },
        { ICON_FA_HEART_PULSE, dmg_text, mode_on(6) && rage_on, 0 },
        { ICON_FA_PERSON_RUNNING, "SLOW", mode_on(7) && GET_VAR(bool, MISC_PATH(m_enabled_slow_walk)), 0 },
        { ICON_FA_ARROW_LEFT, "LEFT", mode_on(8) && aa_on && GET_VAR(bool, ANTIAIM_PATH(m_override_left)), 0 },
        { ICON_FA_ARROW_RIGHT, "RIGHT", mode_on(9) && aa_on && GET_VAR(bool, ANTIAIM_PATH(m_override_right)), 0 },
    };

    auto draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

    static float slot_alpha[10] = { 0.f };
    static float slot_y[10] = { 0.f };
    static bool first_run = true;

    const float start_x = 15.f;
    const float start_y = screen_size.y * 0.5f + 30.f;
    const float spacing = 10.f;
    const float fade_speed = 0.08f;
    const float move_speed = 0.1f;

    if (first_run) {
        for (int i = 0; i < 10; i++)
            slot_y[i] = start_y;
        first_run = false;
    }

    float offset_y = 0.f;
    for (int i = 0; i < 10; i++) {
        auto& row = rows[i];
        const float target_y = start_y + offset_y;

        slot_y[i] = slot_y[i] * (1.f - move_speed) + target_y * move_speed;

        if (row.active) {
            slot_alpha[i] = std::min(1.f, slot_alpha[i] + fade_speed);
        }
        else {
            slot_alpha[i] = std::max(0.f, slot_alpha[i] - fade_speed);
            if (slot_alpha[i] <= 0.f)
                continue;
        }

        ImVec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, row.text.c_str());
        ImVec2 icon_size = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, 0.f, row.icon);

        float row_h = text_size.y + 6.f;
        if (row.style == 2)
            row_h += text_size.y + 2.f;

        ImVec2 pos = ImVec2(start_x, slot_y[i]);
        ImVec2 bg_size = ImVec2(icon_size.x + 4.f + text_size.x + 10.f, row_h);

        const int bg_alpha = static_cast<int>(180 * slot_alpha[i]);
        draw_gradient_rect(ImVec2(pos.x - 5.f, pos.y - 3.f), ImVec2(bg_size.x + 10.f, bg_size.y), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, bg_alpha), draw_list);

        hellcolor text_color(255, 255, 255, static_cast<int>(255 * slot_alpha[i]));
        ImU32 icon_color = IM_COL32(
            static_cast<int>(accent_color.Value.x * 255.f),
            static_cast<int>(accent_color.Value.y * 255.f),
            static_cast<int>(accent_color.Value.z * 255.f),
            static_cast<int>(255 * slot_alpha[i]));

        if (row.style == 1) {
            const float pulse = 0.2f + 0.8f * (0.5f + 0.5f * sinf(ImGui::GetTime() * 5.f));
            icon_color = ImColor(255, 255, 0, static_cast<int>(pulse * 255.f * slot_alpha[i]));
            text_color = hellcolor(255, 255, 0, static_cast<int>(pulse * 255.f * slot_alpha[i]));
        }

        ImVec2 icon_pos = ImVec2(pos.x + 5.f, pos.y + (row_h - icon_size.y) * 0.5f);
        ImVec2 text_pos = ImVec2(icon_pos.x + icon_size.x + 4.f, pos.y + 3.f);

        if (row.style == 2) {
            ImU32 orange = IM_COL32(255, 180, 0, static_cast<int>(255 * slot_alpha[i]));
            ImU32 red = IM_COL32(255, 0, 0, static_cast<int>(255 * slot_alpha[i]));
            draw_list->AddText(icon_font, icon_font->LegacySize, icon_pos, orange, row.icon);
            draw_list->AddText(font, font->LegacySize, text_pos, text_color, row.text.c_str());
            draw_list->AddText(font, font->LegacySize, ImVec2(text_pos.x, text_pos.y + text_size.y + 2.f), red, "FATAL");
        }
        else {
            draw_list->AddText(icon_font, icon_font->LegacySize, icon_pos, icon_color, row.icon);
            draw_list->AddText(font, font->LegacySize, text_pos, text_color, row.text.c_str());
        }

        if (row.active)
            offset_y += row_h + spacing;
    }
}

void c_overlay::observer_list() {
    if (!GET_VAR(bool, VISUALS_PATH(m_enabled_observer_list)))
        return;

    const bool is_in_game = g_interfaces->m_engine->is_connected() && g_interfaces->m_engine->in_game();
    const bool is_menu_open = g_menu && g_menu->m_menu_open;

    struct observer_info {
        std::string player_name;
        uint64_t steam_id;
        float activation_time;
    };

    static std::vector<observer_info> persistent_observers;
    std::vector<observer_info> current_active;

    if (is_in_game && g_ctx->m_local_pawn) {
        bool local_is_spectator = false;
        c_cs_player_pawn* spectator_target = nullptr;

        auto local_observer_services = reinterpret_cast<player_observer_services*>(g_ctx->m_local_pawn->m_pObserverServices());
        if (local_observer_services && local_observer_services->m_iObserverMode() > 0) {
            local_is_spectator = true;
            spectator_target = local_observer_services->m_hObserverTarget().get<c_cs_player_pawn>();
        }

        for (auto& player : g_entity_cache->m_players) {
            if (!player.m_controller || player.m_controller == g_ctx->m_local_controller)
                continue;

            if (!player.m_pawn)
                continue;

            auto observer_services = reinterpret_cast<player_observer_services*>(player.m_pawn->m_pObserverServices());
            if (!observer_services)
                continue;

            if (observer_services->m_iObserverMode() <= 0)
                continue;

            auto observer_target = observer_services->m_hObserverTarget().get<c_cs_player_pawn>();
            if (!observer_target)
                continue;

            bool should_add = false;

            if (local_is_spectator && spectator_target) {
                if (observer_target == spectator_target) {
                    should_add = true;
                }
            }
            else {
                if (observer_target == g_ctx->m_local_pawn) {
                    should_add = true;
                }
            }

            if (!should_add)
                continue;

            std::string player_name = player.m_controller->m_sSanitizedPlayerName() ?
                player.m_controller->m_sSanitizedPlayerName() : "Unknown";
            uint64_t steam_id = player.m_controller->m_steamID();

            auto existing = std::find_if(persistent_observers.begin(), persistent_observers.end(),
                [&](const observer_info& info) { return info.player_name == player_name; });

            if (existing != persistent_observers.end()) {
                current_active.push_back(*existing);
            }
            else {
                observer_info new_observer = { player_name, steam_id, ImGui::GetTime() };
                persistent_observers.push_back(new_observer);
                current_active.push_back(new_observer);

                m_observer_anim.observer_activation_times[player_name] = ImGui::GetTime();
                m_observer_anim.target_text_alphas[player_name] = 1.0f;
            }
        }

        if (local_is_spectator && spectator_target) {
            std::string local_name = g_ctx->m_local_controller->m_sSanitizedPlayerName() ?
                g_ctx->m_local_controller->m_sSanitizedPlayerName() : "You";
            uint64_t local_steam_id = g_ctx->m_local_controller->m_steamID();

            auto existing_local = std::find_if(persistent_observers.begin(), persistent_observers.end(),
                [&](const observer_info& info) { return info.player_name == local_name; });

            if (existing_local != persistent_observers.end()) {
                current_active.push_back(*existing_local);
            }
            else {
                observer_info local_observer = { local_name, local_steam_id, ImGui::GetTime() };
                persistent_observers.push_back(local_observer);
                current_active.push_back(local_observer);

                m_observer_anim.observer_activation_times[local_name] = ImGui::GetTime();
                m_observer_anim.target_text_alphas[local_name] = 1.0f;
            }
        }
    }

    persistent_observers.erase(std::remove_if(persistent_observers.begin(), persistent_observers.end(),
        [&](const observer_info& info) {
            bool found = std::find_if(current_active.begin(), current_active.end(),
                [&](const observer_info& active) { return active.player_name == info.player_name; }) != current_active.end();

            if (!found)
                m_observer_anim.target_text_alphas[info.player_name] = 0.0f;
            return !found;
        }), persistent_observers.end());

    std::sort(current_active.begin(), current_active.end(),
        [](const observer_info& a, const observer_info& b) {
            return a.activation_time < b.activation_time;
        });

    bool has_active = !current_active.empty();
    m_observer_anim.has_active_observers = has_active;

    bool should_show_widget = has_active || is_menu_open;

    static bool prev_menu_state = false;
    static bool prev_has_active = false;

    if (has_active && !prev_has_active)
        m_observer_anim.target_widget_alpha = 1.0f;
    else if (!has_active && prev_has_active)
        m_observer_anim.target_widget_alpha = 0.0f;

    float lerp_speed = 8.0f * ImGui::GetIO().DeltaTime;

    if (is_menu_open) {
        if (!has_active)
            m_observer_anim.widget_alpha = 1.0f;
        else
            m_observer_anim.widget_alpha += (m_observer_anim.target_widget_alpha - m_observer_anim.widget_alpha) * lerp_speed;
    }
    else {
        if (has_active)
            m_observer_anim.widget_alpha += (m_observer_anim.target_widget_alpha - m_observer_anim.widget_alpha) * lerp_speed;
        else {
            if (prev_menu_state && !is_menu_open)
                m_observer_anim.widget_alpha = 0.0f;
            else
                m_observer_anim.widget_alpha += (m_observer_anim.target_widget_alpha - m_observer_anim.widget_alpha) * lerp_speed;
        }
    }

    prev_menu_state = is_menu_open;
    prev_has_active = has_active;

    if (!should_show_widget && m_observer_anim.widget_alpha < 0.01f) return;

    auto draw_list = ImGui::GetBackgroundDrawList();
    ImFont* font = g_font_manager->m_gheist_medium_14 ? g_font_manager->m_gheist_medium_14 : g_font_manager->m_inter_4002;
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    const float padding_x = 12.f;
    const float padding_y = 10.f;
    const float rounding = 4.f;
    const float header_height = 25.f;
    const float outline_height = 5.f;
    const float item_height = 24.f;
    const float base_width = 150.f;

    const float base_content_height = 20.f;

    float max_width = base_width;
    const float avatar_size = 16.0f;
    const float avatar_padding = 6.0f;

    for (const auto& observer : current_active) {
        ImVec2 name_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, observer.player_name.c_str());
        float total_width = name_size.x + padding_x * 2.f;
        if (observer.steam_id != 0) {
            total_width += avatar_size + avatar_padding;
        }
        max_width = std::max(max_width, total_width);
    }

    m_observer_anim.target_widget_width = max_width;
    m_observer_anim.widget_width = max_width;

    float content_height = has_active ? (current_active.size() * item_height) : (base_content_height * 0.5f);
    float total_height = header_height + 3.f + content_height;
    m_observer_anim.target_base_rect_height = total_height;

    m_observer_anim.base_rect_height += (m_observer_anim.target_base_rect_height - m_observer_anim.base_rect_height) * lerp_speed;

    static bool position_initialized = false;
    if (!position_initialized || m_observer_anim.widget_pos.x < 0.0f || m_observer_anim.widget_pos.y < 0.0f) {
        m_observer_anim.widget_pos = ImVec2(screen_size.x - 200.f, screen_size.y * 0.55f);
        position_initialized = true;
    }

    if (is_menu_open) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        ImVec2 rect_min = m_observer_anim.widget_pos;
        ImVec2 rect_max = ImVec2(rect_min.x + m_observer_anim.widget_width, rect_min.y + m_observer_anim.base_rect_height);

        bool hovered = mouse_pos.x >= rect_min.x && mouse_pos.x <= rect_max.x && mouse_pos.y >= rect_min.y && mouse_pos.y <= rect_max.y;

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_observer_anim.dragging = true;
            m_observer_anim.drag_offset = ImVec2(mouse_pos.x - rect_min.x, mouse_pos.y - rect_min.y);
        }

        if (m_observer_anim.dragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_observer_anim.widget_pos = ImVec2(mouse_pos.x - m_observer_anim.drag_offset.x,
                    mouse_pos.y - m_observer_anim.drag_offset.y);
            }
            else
                m_observer_anim.dragging = false;
        }
    }

    hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

    ImVec2 outline_min = ImVec2(m_observer_anim.widget_pos.x, m_observer_anim.widget_pos.y + 2.f);
    ImVec2 outline_max = ImVec2(m_observer_anim.widget_pos.x + m_observer_anim.widget_width, m_observer_anim.widget_pos.y + 3.f + outline_height - 2.f);
    hellcolor outline_color = accent_color;
    outline_color.Value.w *= m_observer_anim.widget_alpha;
    draw_list->AddRectFilled(outline_min, outline_max, outline_color, rounding, ImDrawFlags_RoundCornersTop);

    ImVec2 header_min = ImVec2(m_observer_anim.widget_pos.x, m_observer_anim.widget_pos.y + 3.f);
    ImVec2 header_max = ImVec2(m_observer_anim.widget_pos.x + m_observer_anim.widget_width, m_observer_anim.widget_pos.y + 3.f + header_height);
    hellcolor header_bg_color(30, 30, 30, (int)(255 * m_observer_anim.widget_alpha));
    draw_list->AddRectFilled(header_min, header_max, header_bg_color, rounding, ImDrawFlags_RoundCornersTop);

    ImVec2 header_text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, "Observer List");
    ImVec2 header_text_pos = ImVec2(header_min.x + (m_observer_anim.widget_width - header_text_size.x) * 0.5f, header_min.y + (header_height - header_text_size.y) * 0.5f);
    hellcolor header_text_color(255, 255, 255, (int)(255 * m_observer_anim.widget_alpha));
    draw_list->AddText(font, font->LegacySize, header_text_pos, header_text_color, "Observer List");

    ImVec2 content_min = ImVec2(m_observer_anim.widget_pos.x, header_max.y);
    ImVec2 content_max = ImVec2(m_observer_anim.widget_pos.x + m_observer_anim.widget_width, m_observer_anim.widget_pos.y + m_observer_anim.base_rect_height);
    
    if (hell::blur::m_initialized) {
        ImVec2 rect_size = ImVec2(content_max.x - content_min.x, content_max.y - content_min.y);
        hell::blur::blur_effect.begin_blur();
        hell::blur::blur_effect.apply_blur(draw_list, content_min, rect_size, 0.0f, 6.0f, ImDrawFlags_RoundCornersBottom);
        hell::blur::blur_effect.end_blur();
    }
    hellcolor content_bg_color(29, 29, 29, (int)(150 * m_observer_anim.widget_alpha));
    
    draw_list->AddRectFilled(content_min, content_max, content_bg_color, rounding, ImDrawFlags_RoundCornersBottom);

    if (has_active) {
        draw_list->PushClipRect(content_min, content_max, true);
        float current_y = content_min.y;

        for (const auto& observer : current_active) {
            if (m_observer_anim.text_alphas.find(observer.player_name) == m_observer_anim.text_alphas.end())
                m_observer_anim.text_alphas[observer.player_name] = 0.0f;

            if (m_observer_anim.target_text_alphas.find(observer.player_name) == m_observer_anim.target_text_alphas.end())
                m_observer_anim.target_text_alphas[observer.player_name] = 1.0f;

            float& alpha = m_observer_anim.text_alphas[observer.player_name];
            float target_alpha = m_observer_anim.target_text_alphas[observer.player_name];
            alpha += (target_alpha - alpha) * lerp_speed;

            if (alpha > 0.01f) {
                const float avatar_size = 16.0f;
                const float avatar_padding = 6.0f;
                const float avatar_rounding = 3.0f;

                ImTextureID avatar;
                if (observer.steam_id != 0) {
                    avatar = get_avatar(observer.steam_id);
                }

                float text_x_offset = 0.0f;
                if (avatar) {
                    ImVec2 avatar_pos = ImVec2(content_min.x + padding_x - 2.0f, current_y + (item_height - avatar_size) * 0.5f);
                    ImVec2 avatar_max = ImVec2(avatar_pos.x + avatar_size, avatar_pos.y + avatar_size);

                    ImU32 avatar_color = IM_COL32(255, 255, 255, (int)(255 * alpha * m_observer_anim.widget_alpha));

                    draw_list->PushClipRect(avatar_pos, avatar_max, true);
                    draw_list->AddImageRounded(avatar, avatar_pos, avatar_max, ImVec2(0, 0), ImVec2(1, 1), avatar_color, avatar_rounding);
                    draw_list->PopClipRect();

                    text_x_offset = avatar_size + avatar_padding;
                }

                ImVec2 name_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, observer.player_name.c_str());
                ImVec2 name_pos = ImVec2(content_min.x + padding_x + text_x_offset - 2.0f, current_y + (item_height - name_size.y) * 0.5f);

                hellcolor name_color(255, 255, 255, (int)(255 * alpha * m_observer_anim.widget_alpha));
                draw_list->AddText(font, font->LegacySize, name_pos, name_color, observer.player_name.c_str());
            }

            current_y += item_height;
        }
        draw_list->PopClipRect();
    }
}

ImTextureID c_overlay::get_avatar(uint64_t steam_id) {
    if (avatar_cache.find(steam_id) != avatar_cache.end())
        return avatar_cache[steam_id];

    int iImage = g_interfaces->m_steam_friends->GetMediumFriendAvatar(steam_id);
    if (iImage == -1)
        iImage = g_interfaces->m_steam_friends->GetLargeFriendAvatar(steam_id);
    if (iImage == -1)
        return {};

    uint32 uAvatarWidth, uAvatarHeight;
    if (!g_interfaces->m_steam_utils->GetImageSize(iImage, &uAvatarWidth, &uAvatarHeight))
        return {};

    const int uImageSizeInBytes = uAvatarWidth * uAvatarHeight * 4;
    std::vector<uint8> avatar_rgba(uImageSizeInBytes);
    if (!g_interfaces->m_steam_utils->GetImageRGBA(iImage, avatar_rgba.data(), avatar_rgba.size()))
        return {};

    ID3D11ShaderResourceView* texture = nullptr;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = uAvatarWidth;
    desc.Height = uAvatarHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub_resource = {};
    sub_resource.pSysMem = avatar_rgba.data();
    sub_resource.SysMemPitch = uAvatarWidth * 4;

    ID3D11Texture2D* texture2d = nullptr;
    if (FAILED(g_interfaces->m_device->CreateTexture2D(&desc, &sub_resource, &texture2d)))
        return {};

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = desc.MipLevels;

    if (FAILED(g_interfaces->m_device->CreateShaderResourceView(texture2d, &srv_desc, &texture))) {
        texture2d->Release();
        return {};
    }

    texture2d->Release();

    ImTextureID avatar = reinterpret_cast<ImTextureID>(texture);
    avatar_cache[steam_id] = avatar;
    return avatar;
}
