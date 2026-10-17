#include "visuals.h"
#include <cheat/config/vars.h>
#include <utils/fonts/font_manager.h>
#include <context.h>
#include <cheat/menu/animation/anim.h>
#include <sdk/interfaces/csgo_input.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <cheat/features/ragebot/ragebot.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/features/visuals/chams.h>
#include <math/math.h>
#include <core/interfaces/interfaces.h>
#include <sdk/constants.h>
#include <algorithm>
#include <string>
#include <vector>
#include <cmath>
#include <utils/stb_image.h>
#include <utils/fonts/compressed_fonts/test_avatar.h>
#include <d3d11.h>
bool c_visuals::get_bb(c_cs_player_pawn* entity, bb_t& bb) {
    if (!entity)
        return false;
    auto collision = entity->m_pCollision();
    auto game_scene_node = entity->m_pGameSceneNode();
    if (!collision || !game_scene_node)
        return false;
    c_transform node_to_world = game_scene_node->m_nodeToWorld();
    auto transform = node_to_world.m_quat_orientation.to_matrix(node_to_world.m_vec_position);
    vec3_t mins = collision->m_vecMins();
    vec3_t maxs = collision->m_vecMaxs();
    vec2_t min_screen = { FLT_MAX, FLT_MAX };
    vec2_t max_screen = { -FLT_MAX, -FLT_MAX };
    for (int i = 0; i < 8; ++i) {
        vec3_t point = { i & 1 ? maxs.x : mins.x, i & 2 ? maxs.y : mins.y, i & 4 ? maxs.z : mins.z };
        vec3_t transform_point = point.transform(transform);

        vec2_t point_screen{};
        if (!g_math->world_to_screen(transform_point, point_screen))
            return true;

        min_screen.x = std::min(min_screen.x, point_screen.x);
        min_screen.y = std::min(min_screen.y, point_screen.y);
        max_screen.x = std::max(max_screen.x, point_screen.x);
        max_screen.y = std::max(max_screen.y, point_screen.y);
    }
    bb.m_min = { min_screen.x, min_screen.y };
    bb.m_max = { max_screen.x, max_screen.y };
    return false;
}
c_visuals::bb_t c_visuals::get_element_bb(bb_t anchor, e_element_bb_position position, int row, float padding, float size) {
    bb_t obj{ {},{} };
    switch (position) {
    case e_element_bb_position::left:
        obj.m_min = { anchor.m_min.x - (padding * (float)row - size), anchor.m_min.y };
        obj.m_max = { anchor.m_min.x - (padding * (float)row), anchor.m_min.y + abs(anchor.m_min.y - anchor.m_max.y) };
        break;
    case e_element_bb_position::top:
        obj.m_min = { anchor.m_min.x, anchor.m_min.y - (padding * (float)row - size) };
        obj.m_max = { anchor.m_min.x + abs(anchor.m_min.x - anchor.m_max.x), anchor.m_min.y - (padding * (float)row) };
        break;
    case e_element_bb_position::right:
        obj.m_min = { anchor.m_max.x + (padding * (float)row - size), anchor.m_min.y };
        obj.m_max = { anchor.m_max.x + (padding * (float)row), anchor.m_max.y };
        break;
    case e_element_bb_position::bottom:
        obj.m_min = { anchor.m_min.x, anchor.m_max.y + (padding * (float)row) };
        obj.m_max = { anchor.m_max.x, anchor.m_max.y + (padding * (float)row - size) };
        break;
    }
    return obj.normalize();
}
void c_visuals::draw_bar(bb_t owner_bb, bar_object_t& obj, float alpha_modifier, ImDrawList* draw_list) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    bb_t bb = this->get_element_bb(owner_bb, obj.m_element_position, obj.m_element_index, obj.m_padding, obj.m_size);
    bb.normalize();

    bool is_vertical = (obj.m_element_position == e_element_bb_position::left || obj.m_element_position == e_element_bb_position::right);
    bool is_horizontal = (obj.m_element_position == e_element_bb_position::top || obj.m_element_position == e_element_bb_position::bottom);

    bb_t fg_bb = bb;

    if (is_vertical) {
        float full_height = fg_bb.m_max.y - fg_bb.m_min.y;
        float filled_height = (full_height * obj.m_value) / 100.f;
        fg_bb.m_min.y = fg_bb.m_max.y - filled_height;
    }
    else if (is_horizontal) {
        float full_width = fg_bb.m_max.x - fg_bb.m_min.x;
        float filled_width = (full_width * obj.m_value) / 100.f;
        fg_bb.m_max.x = fg_bb.m_min.x + filled_width;
    }

    if (obj.m_glow_enabled && obj.m_glow_intensity > 0.0f) {
        float shadow_thickness = 10.0f * obj.m_glow_intensity;
        float shadow_alpha = 0.9f * obj.m_glow_intensity * alpha_modifier;

        hellcolor shadow_color = obj.m_fg_color;
        shadow_color.Value.w = shadow_alpha;
        ImU32 shadow_col = ImGui::ColorConvertFloat4ToU32(shadow_color.Value);

        draw_list->AddShadowRect(bb.m_min, bb.m_max, shadow_col, shadow_thickness, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
    }

    bb_t bg_bb = bb;
    bg_bb.expand(1.f, 1.f);
    draw_list->AddRectFilled(bg_bb.m_min, bg_bb.m_max, obj.m_bg_color.AlphaOverride(alpha_modifier));

    if (obj.m_gradient_enabled && fg_bb.m_min.x < fg_bb.m_max.x && fg_bb.m_min.y < fg_bb.m_max.y) {
        const int segments = 20;
        for (int i = 0; i < segments; ++i) {
            float t1 = (float)i / (float)segments;
            float t2 = (float)(i + 1) / (float)segments;

            bb_t segment_bb = fg_bb;
            if (is_vertical) {
                float seg_height = (fg_bb.m_max.y - fg_bb.m_min.y) / segments;
                segment_bb.m_min.y = fg_bb.m_min.y + seg_height * i;
                segment_bb.m_max.y = fg_bb.m_min.y + seg_height * (i + 1);
            } else {
                float seg_width = (fg_bb.m_max.x - fg_bb.m_min.x) / segments;
                segment_bb.m_min.x = fg_bb.m_min.x + seg_width * i;
                segment_bb.m_max.x = fg_bb.m_min.x + seg_width * (i + 1);
            }

            hellcolor color_start, color_end;
            if (obj.m_gradient_reverse) {
                color_start = obj.m_gradient_color;
                color_end = obj.m_fg_color;
            } else {
                color_start = obj.m_fg_color;
                color_end = obj.m_gradient_color;
            }

            hellcolor seg_color = hellcolor(
                color_start.Value.x + (color_end.Value.x - color_start.Value.x) * t1,
                color_start.Value.y + (color_end.Value.y - color_start.Value.y) * t1,
                color_start.Value.z + (color_end.Value.z - color_start.Value.z) * t1,
                (color_start.Value.w + (color_end.Value.w - color_start.Value.w) * t1) * alpha_modifier
            );

            draw_list->AddRectFilled(segment_bb.m_min, segment_bb.m_max, seg_color);
        }
    } else {
        draw_list->AddRectFilled(fg_bb.m_min, fg_bb.m_max, obj.m_fg_color.AlphaOverride(alpha_modifier));
    }
    if (obj.m_value > 0 && obj.m_value <= 99 && obj.m_number_value_enabled) {
        auto font = this->get_font(obj.m_number_value_text.m_font_type);
        std::string text_str = std::to_string(obj.m_value);
        const char* text = text_str.c_str();
        hellvec2 text_pos{};
        hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text);
        if (obj.m_element_position == e_element_bb_position::left || obj.m_element_position == e_element_bb_position::right) {
            hellvec2 top_mid = { fg_bb.get_center().x, fg_bb.m_min.y };
            text_pos = top_mid - (text_size * 0.5f);
        }
        else {
            hellvec2 center = fg_bb.get_center();
            text_pos = fg_bb.m_max - (text_size * 0.5f);
        }
        for (hellvec2& outline_pos : m_outline_pos_arr) {
            draw_list->AddText(font, font->LegacySize, text_pos + outline_pos, obj.m_number_value_text.m_bg_color.AlphaOverride(alpha_modifier), text);
        }
        draw_list->AddText(font, font->LegacySize, text_pos, obj.m_number_value_text.m_fg_color.AlphaOverride(alpha_modifier), text);
    }
}

void c_visuals::draw_skeleton(c_cs_player_pawn* pawn, hellcolor col_color, hellcolor col_outline, ImDrawList* draw_list) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    
    if (!pawn)
        return;
    
    auto* game_scene_node = pawn->m_pGameSceneNode();
    if (!game_scene_node)
        return;
    
    auto* skeleton_instance = game_scene_node->get_skeleton_instace();
    if (!skeleton_instance || !skeleton_instance->get_bone_matrix() || skeleton_instance->get_bone_count() <= 0)
        return;
    
    static const std::vector<std::vector<int>> bone_joint_list = {
        { 5, 4, 0 },
        { 5, 8, 9, 10 },
        { 5, 13, 14, 15 },
        { 0, 22, 23, 24 },
        { 0, 25, 26, 27 }
    };
    
    for (const auto& bone_chain : bone_joint_list) {
        vec2_t previous_screen{};
        bool has_previous = false;
        
        for (int bone_index : bone_chain) {
            if (bone_index >= skeleton_instance->get_bone_count() || bone_index < 0)
                continue;
            
            vec3_t bone_pos = skeleton_instance->get_bone_matrix()[bone_index].get_origin(0);
            vec2_t current_screen{};
            
            if (!g_math->world_to_screen(bone_pos, current_screen))
                continue;
            
            if (has_previous) {
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0)
                            continue;
                        draw_list->AddLine(
                            hellvec2(previous_screen.x + i, previous_screen.y + j),
                            hellvec2(current_screen.x + i, current_screen.y + j),
                            col_outline,
                            1.0f
                        );
                    }
                }
                draw_list->AddLine(
                    hellvec2(previous_screen.x, previous_screen.y),
                    hellvec2(current_screen.x, current_screen.y),
                    col_color,
                    1.0f
                );
            }
            
            previous_screen = current_screen;
            has_previous = true;
        }
    }
}

void c_visuals::draw_skeleton_backtrack(c_cs_player_pawn* pawn, ImDrawList* draw_list, float alpha_modifier) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    
    if (!pawn || !pawn->is_alive())
        return;
    
    cached_player_t& player = g_entity_cache->find(pawn);
    auto& lagcomp_data = player.m_lagcomp_data;
    
    if (lagcomp_data.m_lag_records.empty())
        return;
    
    auto& lag_records = lagcomp_data.m_lag_records;
    auto& record_newest = lag_records.newest();
    
    static const std::vector<std::vector<int>> bone_joint_list = {
        { 5, 4, 0 },
        { 5, 8, 9, 10 },
        { 5, 13, 14, 15 },
        { 0, 22, 23, 24 },
        { 0, 25, 26, 27 }
    };
    
    hellcolor col_color = GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color)).AlphaOverride(alpha_modifier);
    hellcolor col_outline = GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color_outline)).AlphaOverride(alpha_modifier);
    
    int skeleton_type = GET_VAR(int, VISUALS_PATH(m_skeleton_esp_backtrack_type));
    int max_records = static_cast<int>(lag_records.size());
    int size = (skeleton_type == e_backtrack_cham_type::backtrack_cham_all) ? std::min(12, max_records) : 1;
    
    int records_processed = 0;
    
    for (int i = 0; i < max_records && records_processed < size; i++) {
        auto& record_current = lagcomp_data.m_lag_records[i];
        if (!record_current.is_valid())
            continue;

        if (record_current.m_origin.dist_to(record_newest.m_origin) < 0.25f)
            continue;

        if (!record_current.m_bones)
            continue;

        for (const auto& bone_chain : bone_joint_list) {
            vec2_t previous_screen{};
            bool has_previous = false;
            
            for (int bone_index : bone_chain) {
                if (bone_index >= 128 || bone_index < 0)
                    continue;
                
                vec3_t bone_pos = record_current.m_bones[bone_index].to_matrix().origin();
                vec2_t current_screen{};
                
                if (!g_math->world_to_screen(bone_pos, current_screen))
                    continue;
                
                if (has_previous) {
                    for (int j = -1; j <= 1; j++) {
                        for (int k = -1; k <= 1; k++) {
                            if (j == 0 && k == 0)
                                continue;
                            draw_list->AddLine(
                                hellvec2(previous_screen.x + j, previous_screen.y + k),
                                hellvec2(current_screen.x + j, current_screen.y + k),
                                col_outline,
                                1.0f
                            );
                        }
                    }
                    draw_list->AddLine(
                        hellvec2(previous_screen.x, previous_screen.y),
                        hellvec2(current_screen.x, current_screen.y),
                        col_color,
                        1.0f
                    );
                }
                
                previous_screen = current_screen;
                has_previous = true;
            }
        }
        
        records_processed++;
        
        if (skeleton_type == e_backtrack_cham_type::backtrack_cham_last) {
            break;
        }
    }
}

void c_visuals::draw_skeleton_onshot(c_cs_player_pawn* pawn, ImDrawList* draw_list, float alpha_modifier) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    
    if (!pawn)
        return;
    
    prune_old_onshot_skeletons(pawn);
    
    auto skeleton_it = s_onshot_skeletons.find(pawn);
    if (skeleton_it == s_onshot_skeletons.end())
        return;
    
    auto& skeleton_array = skeleton_it->second;
    
    static const std::vector<std::vector<int>> bone_joint_list = {
        { 5, 4, 0 },
        { 5, 8, 9, 10 },
        { 5, 13, 14, 15 },
        { 0, 22, 23, 24 },
        { 0, 25, 26, 27 }
    };
    
    int duration = GET_VAR(int, VISUALS_PATH(m_onshot_skeleton_duration));
    float curtime = g_interfaces->m_global_vars->m_curtime;
    
    for (size_t i = 0; i < skeleton_array.size(); i++) {
        skeleton_onshot_data_t& skeleton_data = skeleton_array[i];
        
        float elapsed = curtime - skeleton_data.m_draw_begin_time;
        float alpha_mult = 1.0f;
        
        if (duration > 1) {
            if (elapsed > 1.0f) {
                float fade_start = 1.0f;
                float fade_duration = (float)duration - fade_start;
                if (fade_duration > 0.0f) {
                    float fade_progress = (elapsed - fade_start) / fade_duration;
                    alpha_mult = std::clamp(1.0f - fade_progress, 0.0f, 1.0f);
                }
            }
        } else {
            float fade_duration = (float)duration;
            if (fade_duration > 0.0f) {
                float fade_progress = elapsed / fade_duration;
                alpha_mult = std::clamp(1.0f - fade_progress, 0.0f, 1.0f);
            }
        }
        
        hellcolor col_color = GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color));
        hellcolor col_outline = GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color_outline));
        
        col_color.Value.w *= alpha_mult * alpha_modifier;
        col_outline.Value.w *= alpha_mult * alpha_modifier;
        
        for (const auto& bone_chain : bone_joint_list) {
            vec2_t previous_screen{};
            bool has_previous = false;
            
            for (int bone_index : bone_chain) {
                if (bone_index >= skeleton_data.m_bone_count || bone_index < 0)
                    continue;
                
                vec3_t bone_pos = skeleton_data.m_bones[bone_index].m_pos;
                vec2_t current_screen{};
                
                if (!g_math->world_to_screen(bone_pos, current_screen))
                    continue;
                
                if (has_previous) {
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            if (i == 0 && j == 0)
                                continue;
                            draw_list->AddLine(
                                hellvec2(previous_screen.x + i, previous_screen.y + j),
                                hellvec2(current_screen.x + i, current_screen.y + j),
                                col_outline,
                                1.0f
                            );
                        }
                    }
                    draw_list->AddLine(
                        hellvec2(previous_screen.x, previous_screen.y),
                        hellvec2(current_screen.x, current_screen.y),
                        col_color,
                        1.0f
                    );
                }
                
                previous_screen = current_screen;
                has_previous = true;
            }
        }
    }
}

void c_visuals::create_onshot_skeleton_data(c_cs_player_pawn* pawn) {
    if (!pawn)
        return;

    auto* game_scene_node = pawn->m_pGameSceneNode();
    if (!game_scene_node)
        return;

    auto* skeleton_instance = game_scene_node->get_skeleton_instace();
    if (!skeleton_instance || !skeleton_instance->get_bone_matrix() || skeleton_instance->get_bone_count() <= 0)
        return;

    skeleton_onshot_data_t skeleton_data;
    skeleton_data.m_bone_count = std::min(skeleton_instance->get_bone_count(), 128);
    skeleton_data.m_draw_begin_time = g_interfaces->m_global_vars->m_curtime;

    vec3_t impact_pos = vec3_t(0, 0, 0);
    bool found_impact = false;
    
    for (auto& player : g_entity_cache->m_players) {
        if (player.m_pawn == pawn && player.m_lagcomp_data.is_valid()) {
            auto& lag_records = player.m_lagcomp_data.m_lag_records;
            if (!lag_records.empty()) {
                lag_record_t* best_record = nullptr;
                float best_distance = FLT_MAX;
                
                vec3_t current_pos = pawn->get_world_space_center();
                
                for (auto& record : lag_records) {
                    if (!record.is_valid())
                        continue;
                        
                    vec3_t record_pos = record.m_origin;
                    record_pos.z += 64.0f;
                    
                    float distance = current_pos.dist_to(record_pos);
                    if (distance < best_distance) {
                        best_distance = distance;
                        best_record = &record;
                    }
                }
                
                if (best_record) {
                    for (int i = 0; i < skeleton_data.m_bone_count && i < 128; i++) {
                        skeleton_data.m_bones[i].m_pos = best_record->m_bones[i].to_matrix().origin();
                        skeleton_data.m_bones[i].m_rot = vec4_t(0, 0, 0, 1);
                    }
                    found_impact = true;
                }
            }
            break;
        }
    }
    
    if (!found_impact) {
        for (int i = 0; i < skeleton_data.m_bone_count; i++) {
            vec3_t bone_pos = skeleton_instance->get_bone_matrix()[i].get_origin(0);
            skeleton_data.m_bones[i].m_pos = bone_pos;
            skeleton_data.m_bones[i].m_rot = vec4_t(0, 0, 0, 1);
        }
    }

    s_onshot_skeletons[pawn].emplace_back(skeleton_data);
}

void c_visuals::prune_old_onshot_skeletons(c_cs_player_pawn* pawn) {
    if (!pawn || !pawn->get_handle().is_valid()) {
        return;
    }

    int duration = GET_VAR(int, VISUALS_PATH(m_onshot_skeleton_duration));
    float curtime = g_interfaces->m_global_vars->m_curtime;

    auto& skeleton_array = s_onshot_skeletons[pawn];
    for (size_t i = 0; i < skeleton_array.size(); i++) {
        auto& skeleton_data = skeleton_array[i];
        
        if (fabsf(skeleton_data.m_draw_begin_time - curtime) > duration) {
            skeleton_array.erase(skeleton_array.begin() + i);
            i--;
        }
    }

    if (skeleton_array.empty()) {
        s_onshot_skeletons.erase(pawn);
    }
}

void c_visuals::cleanup_onshot_skeletons(c_cs_player_pawn* pawn) {
    if (!pawn)
        return;
    
    auto skeleton_it = s_onshot_skeletons.find(pawn);
    if (skeleton_it != s_onshot_skeletons.end()) {
        s_onshot_skeletons.erase(skeleton_it);
    }
}

void c_visuals::draw_text(bb_t owner_bb, text_object_t& obj, float alpha_modifier, ImDrawList* draw_list) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    bb_t bb = this->get_element_bb(owner_bb, obj.m_element_position, obj.m_element_index, obj.m_padding, obj.m_size);
    auto font = this->get_font(obj.m_font_type);
    hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, obj.m_text);
    hellvec2 text_pos = { 0.f, bb.get_center().y - (text_size.y * 0.5f) };
    switch (obj.m_text_alignment) {
    case e_text_alignment::text_alignment_center:
        text_pos.x = bb.get_center().x - (text_size.x * 0.5f) + obj.m_text_offset_x;
        break;
    case e_text_alignment::text_alignment_left:
        text_pos.x = bb.m_min.x + obj.m_text_offset_x;
        break;
    case e_text_alignment::text_alignment_right:
        text_pos.x = bb.m_max.x - text_size.x + obj.m_text_offset_x;
        break;
    }
    if (obj.m_text_shadow_type & e_text_shadow_type::text_shadow_drop) {
        hellvec2 outline_pos = { text_pos.x + 1, text_pos.y + 1 };
        draw_list->AddText(font, font->LegacySize, outline_pos, obj.m_bg_color.AlphaOverride(alpha_modifier), obj.m_text);
    }
    if (obj.m_text_shadow_type & e_text_shadow_type::text_shadow_full) {
        constexpr int arr_size = IM_ARRAYSIZE(m_outline_pos_arr);
        for (int i = 0; i < arr_size; i++) {
            float alpha_mod = 1.f - ((float)i / (float)arr_size);
            hellvec2& outline_pos = m_outline_pos_arr[i];
            draw_list->AddText(font, font->LegacySize, text_pos + outline_pos, obj.m_bg_color.AlphaOverride(alpha_modifier * alpha_mod), obj.m_text);
        }
    }

    draw_list->AddText(font, font->LegacySize, text_pos, obj.m_fg_color.AlphaOverride(alpha_modifier), obj.m_text);
}
static icon_data_t get_weapon_icon_by_name(const std::string& weapon_name)
{
    if (weapon_name.empty())
        return {};

    std::string clean_name = weapon_name;
    if (clean_name.length() >= 7 && clean_name.substr(0, 7) == "weapon_")
        clean_name.erase(0, 7);

    std::vector<std::string> paths_to_try;
    paths_to_try.push_back("icons/equipment/" + clean_name);

    std::string name_without_numbers = clean_name;
    name_without_numbers.erase(std::remove_if(name_without_numbers.begin(), name_without_numbers.end(), ::isdigit), name_without_numbers.end());
    if (!name_without_numbers.empty() && name_without_numbers != clean_name)
        paths_to_try.push_back("icons/equipment/" + name_without_numbers);

    if (clean_name.find("_") != std::string::npos)
    {
        std::string name_without_underscore = clean_name;
        name_without_underscore.erase(std::remove(name_without_underscore.begin(), name_without_underscore.end(), '_'), name_without_underscore.end());
        if (!name_without_underscore.empty() && name_without_underscore != clean_name)
            paths_to_try.push_back("icons/equipment/" + name_without_underscore);
    }

    for (const auto& path : paths_to_try)
    {
        auto icon_data = get_panorama_texture(path);
        if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
            return icon_data;
    }

    return {};
}

static icon_data_t get_weapon_icon(uint16_t weapon_index)
{
    std::string weapon_path;
    switch (weapon_index)
    {
    case WEAPON_AK_47: weapon_path = "icons/equipment/ak47"; break;
    case WEAPON_M4A4: weapon_path = "icons/equipment/m4a4"; break;
    case WEAPON_M4A1_S: weapon_path = "icons/equipment/m4a1_silencer"; break;
    case WEAPON_AUG: weapon_path = "icons/equipment/aug"; break;
    case WEAPON_FAMAS: weapon_path = "icons/equipment/famas"; break;
    case WEAPON_GALIL_AR: weapon_path = "icons/equipment/galil"; break;
    case WEAPON_SG_553: weapon_path = "icons/equipment/sg556"; break;
    case WEAPON_AWP: weapon_path = "icons/equipment/awp"; break;
    case WEAPON_SSG_08: weapon_path = "icons/equipment/ssg08"; break;
    case WEAPON_G3SG1: weapon_path = "icons/equipment/g3sg1"; break;
    case WEAPON_SCAR_20: weapon_path = "icons/equipment/scar20"; break;
    case WEAPON_P90: weapon_path = "icons/equipment/p90"; break;
    case WEAPON_MP7: weapon_path = "icons/equipment/mp7"; break;
    case WEAPON_MP9: weapon_path = "icons/equipment/mp9"; break;
    case WEAPON_MP5_SD: weapon_path = "icons/equipment/mp5sd"; break;
    case WEAPON_UMP_45: weapon_path = "icons/equipment/ump45"; break;
    case WEAPON_MAC_10: weapon_path = "icons/equipment/mac10"; break;
    case WEAPON_PP_BIZON: weapon_path = "icons/equipment/bizon"; break;
    case WEAPON_M249: weapon_path = "icons/equipment/m249"; break;
    case WEAPON_NEGEV: weapon_path = "icons/equipment/negev"; break;
    case WEAPON_XM1014: weapon_path = "icons/equipment/xm1014"; break;
    case WEAPON_SAWED_OFF: weapon_path = "icons/equipment/sawedoff"; break;
    case WEAPON_MAG_7: weapon_path = "icons/equipment/mag7"; break;
    case WEAPON_NOVA: weapon_path = "icons/equipment/nova"; break;
    case WEAPON_DESERT_EAGLE: weapon_path = "icons/equipment/deagle"; break;
    case WEAPON_R8_REVOLVER: weapon_path = "icons/equipment/revolver"; break;
    case WEAPON_GLOCK_18: weapon_path = "icons/equipment/glock"; break;
    case WEAPON_P2000: weapon_path = "icons/equipment/p2000"; break;
    case WEAPON_USP_S: weapon_path = "icons/equipment/usp_silencer"; break;
    case WEAPON_P250: weapon_path = "icons/equipment/p250"; break;
    case WEAPON_FIVE_SEVEN: weapon_path = "icons/equipment/fiveseven"; break;
    case WEAPON_TEC_9: weapon_path = "icons/equipment/tec9"; break;
    case WEAPON_CZ75_AUTO: weapon_path = "icons/equipment/cz75a"; break;
    case WEAPON_DUAL_BERETTAS: weapon_path = "icons/equipment/elite"; break;
    case WEAPON_ZEUS_X27: weapon_path = "icons/equipment/taser"; break;
    default: return {};
    }
    return get_panorama_texture(weapon_path);
}

static void draw_flag_text(
    c_visuals::player_visual_data_t& data,
    const char* text,
    hellcolor fg_color,
    bool right_side,
    float side_offset,
    int row,
    float alpha_modifier,
    ImDrawList* draw_list
) {
    if (!text || !*text || !draw_list)
        return;

    int font_type_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_font_type));
    int shadow_type_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_shadow_type));
    int size_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_size));
    int padding_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_padding));

    c_visuals::e_font_type font_type = (c_visuals::e_font_type)font_type_cfg;
    c_visuals::e_text_shadow_type shadow_type = (c_visuals::e_text_shadow_type)shadow_type_cfg;

    ImFont* font = g_visuals->get_font(font_type);
    if (!font)
        return;

    hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text);

    float row_height = text_size.y + (float)size_cfg;
    float y = data.m_bb.m_min.y + (row - 1) * row_height;

    float padding = (float)padding_cfg;
    float x = right_side
        ? data.m_bb.m_max.x + side_offset + padding
        : data.m_bb.m_min.x - side_offset - padding - text_size.x;

    hellvec2 text_pos = { x, y };

    hellcolor fg = fg_color.AlphaOverride(alpha_modifier);
    hellcolor bg = hellcolor(0, 0, 0, 150).AlphaOverride(alpha_modifier);

    if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_drop) {
        hellvec2 outline_pos = { text_pos.x + 1.f, text_pos.y + 1.f };
        draw_list->AddText(font, font->LegacySize, outline_pos, bg, text);
    } else if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_full) {
        static const hellvec2 outline_offsets[8] = {
            { -1, -1 },
            { -1, 1 },
            { 1, -1 },
            { 1, 1 },
            { 0, 1 },
            { 1, 0 },
            { 0, -1 },
            { -1, 0 },
        };

        for (const hellvec2& off : outline_offsets) {
            draw_list->AddText(font, font->LegacySize, text_pos + off, bg, text);
        }
    }

    draw_list->AddText(font, font->LegacySize, text_pos, fg, text);
}

static void draw_ping_text(
    c_visuals::player_visual_data_t& data,
    const char* text,
    hellcolor fg_color,
    int row,
    float alpha_modifier,
    ImDrawList* draw_list
) {
    if (!text || !*text || !draw_list)
        return;

    int font_type_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_font_type));
    int shadow_type_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_shadow_type));
    int size_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_size));
    int padding_cfg = GET_VAR(int, VISUALS_PATH(m_esp_flags_padding));

    c_visuals::e_font_type font_type = (c_visuals::e_font_type)font_type_cfg;
    c_visuals::e_text_shadow_type shadow_type = (c_visuals::e_text_shadow_type)shadow_type_cfg;

    ImFont* font = g_visuals->get_font(font_type);
    if (!font)
        return;

    hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text);

    float y = data.m_bb.m_min.y - (text_size.y + 8.f);

    float x = data.m_bb.get_center().x - (text_size.x * 0.5f);

    hellvec2 text_pos = { x, y };

    hellcolor fg = fg_color.AlphaOverride(alpha_modifier);
    hellcolor bg = hellcolor(0, 0, 0, 150).AlphaOverride(alpha_modifier);

    if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_drop) {
        hellvec2 outline_pos = { text_pos.x + 1.f, text_pos.y + 1.f };
        draw_list->AddText(font, font->LegacySize, outline_pos, bg, text);
    } else if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_full) {
        static const hellvec2 outline_offsets[8] = {
            { -1, -1 },
            { -1, 1 },
            { 1, -1 },
            { 1, 1 },
            { 0, 1 },
            { 1, 0 },
            { 0, -1 },
            { -1, 0 },
        };

        for (const hellvec2& off : outline_offsets) {
            draw_list->AddText(font, font->LegacySize, text_pos + off, bg, text);
        }
    }

    draw_list->AddText(font, font->LegacySize, text_pos, fg, text);
}

void c_visuals::draw_player_flags_runtime(player_visual_data_t& data) {
    if (!data.m_pawn)
        return;

    auto& flags_vec = GET_VAR(std::vector<bool>, VISUALS_PATH(m_esp_flags));

    c_cs_player_pawn* pawn = data.m_pawn;
    c_weapon_base* weapon = pawn->get_active_weapon();
    c_player_item_services* item_services = pawn->m_pItemServices();
    c_player_weapon_services* weapon_services = pawn->m_pWeaponServices();

    bool is_scoped = pawn->m_bIsScoped();
    bool is_defusing = pawn->m_bIsDefusing();
    bool has_armor = pawn->m_ArmorValue() > 0;
    bool has_helmet = item_services && item_services->m_bHasHelmet();
    bool has_defuse_kit = item_services && item_services->m_bHasDefuser();

    bool is_reloading = false;
    bool has_bomb = false;
    bool has_taser = false;

    if (weapon) {
        is_reloading = weapon->m_bInReload();
    }

    if (weapon_services) {
        auto& weapons = weapon_services->m_hMyWeapons();
        for (int i = 0; i < weapons.size; ++i) {
            c_weapon_base* w = weapons.elements[i].get<c_weapon_base>();
            if (!w)
                continue;

            auto attr = w->m_AttributeManager();
            if (!attr)
                continue;

            auto item = attr->m_Item();
            if (!item)
                continue;

            uint16_t def_index = item->m_iItemDefinitionIndex();
            if (def_index == WEAPON_C4_EXPLOSIVE)
                has_bomb = true;
            if (def_index == WEAPON_ZEUS_X27)
                has_taser = true;
        }
    }

    hellcolor generic_color = GET_VAR(hellcolor, VISUALS_PATH(m_esp_flags_color)).AlphaOverride(data.m_alpha_modifier);
    hellcolor bomb_color = hellcolor(255, 120, 120, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor defuse_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor scope_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor reload_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor taser_color = hellcolor(255, 225, 115, 230).AlphaOverride(data.m_alpha_modifier);

    float offset_right = 0.f;
    float offset_left = 0.f;

    auto update_side_offsets_from_element = [&](c_visuals::e_element_bb_position pos, int index, int padding, float size) {
        if (pos == c_visuals::e_element_bb_position::right) {
            auto bb = g_visuals->get_element_bb(data.m_bb, pos, index, (float)padding, size);
            bb.normalize();
            offset_right = std::max(offset_right, bb.m_max.x - data.m_bb.m_max.x);
        } else if (pos == c_visuals::e_element_bb_position::left) {
            auto bb = g_visuals->get_element_bb(data.m_bb, pos, index, (float)padding, size);
            bb.normalize();
            offset_left = std::max(offset_left, data.m_bb.m_min.x - bb.m_min.x);
        }
    };

    update_side_offsets_from_element(data.m_health_bar.m_element_position, data.m_health_bar.m_element_index, data.m_health_bar.m_padding, (float)data.m_health_bar.m_size);
    update_side_offsets_from_element(data.m_armor_bar.m_element_position, data.m_armor_bar.m_element_index, data.m_armor_bar.m_padding, (float)data.m_armor_bar.m_size);
    
    if (GET_VAR(bool, VISUALS_PATH(m_name)) && data.m_name_text.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_name_text.m_element_position, data.m_name_text.m_element_index, data.m_name_text.m_padding, (float)data.m_name_text.m_size);
    }
    if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_weapon_text.m_element_position, data.m_weapon_text.m_element_index, data.m_weapon_text.m_padding, (float)data.m_weapon_text.m_size);
    }
    if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon)) && data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_weapon_icon.m_element_position, data.m_weapon_icon.m_element_index, data.m_weapon_icon.m_padding, (float)data.m_weapon_icon.m_size);
    }

    int current_row_right = 1;
    int current_row_left = 1;

    auto flag_enabled = [&](e_esp_flags flag) -> bool {
        size_t idx = static_cast<size_t>(flag);
        return idx < flags_vec.size() && flags_vec[idx];
    };

    if (flag_enabled(e_esp_flags::esp_flag_scoped) && is_scoped) {
        draw_flag_text(data, "SCOPE", scope_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_defusing) && is_defusing) {
        draw_flag_text(data, "DEFUSE", generic_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (has_armor) {
        if (flag_enabled(e_esp_flags::esp_flag_armor_helmet) && has_helmet) {
            draw_flag_text(data, "HK", generic_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
        } else if (flag_enabled(e_esp_flags::esp_flag_armor) && !has_helmet) {
            draw_flag_text(data, "K", generic_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
        }
    }

    if (flag_enabled(e_esp_flags::esp_flag_reloading) && is_reloading) {
        draw_flag_text(data, "RELOAD", reload_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_bomb) && has_bomb) {
        draw_flag_text(data, "C4", bomb_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_defuse) && has_defuse_kit) {
        draw_flag_text(data, "KIT", defuse_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_taser) && has_taser) {
        draw_flag_text(data, "ZEUS", taser_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_ping)) {
        int ping = data.m_controller->m_iPing();
        data.m_ping_storage = std::to_string(ping) + "MS";
        hellcolor ping_color = hellcolor(255, 0, 0, 220);
        if (ping > 30 && ping <= 60) {
            ping_color = hellcolor(255, 165, 0, 220);
        } else if (ping > 60) {
            ping_color = hellcolor(0, 255, 0, 220);
        }
        
        int ping_row = 1;
        int max_top_index = 0;
        
        if (GET_VAR(bool, VISUALS_PATH(m_name)) && data.m_name_text.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_name_text.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_weapon_text.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon)) && data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_weapon_icon.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_health_bar)) && data.m_health_bar.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_health_bar.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_armor_bar)) && data.m_armor_bar.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_armor_bar.m_element_index);
        }
        
        ping_row = max_top_index + 1;
        
        draw_ping_text(data, data.m_ping_storage.c_str(), ping_color, ping_row, data.m_alpha_modifier, data.m_draw_list);
    }
}

void c_visuals::draw_player_flags_preview(player_visual_data_t& data) {
    auto& flags_vec = GET_VAR(std::vector<bool>, VISUALS_PATH(m_esp_flags));

    hellcolor generic_color = GET_VAR(hellcolor, VISUALS_PATH(m_esp_flags_color)).AlphaOverride(data.m_alpha_modifier);
    hellcolor bomb_color = hellcolor(255, 120, 120, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor defuse_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor scope_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor reload_color = hellcolor(133, 167, 255, 230).AlphaOverride(data.m_alpha_modifier);
    hellcolor taser_color = hellcolor(255, 225, 115, 230).AlphaOverride(data.m_alpha_modifier);

    float offset_right = 0.f;
    float offset_left = 0.f;

    auto update_side_offsets_from_element = [&](c_visuals::e_element_bb_position pos, int index, int padding, float size) {
        if (pos == c_visuals::e_element_bb_position::right) {
            auto bb = g_visuals->get_element_bb(data.m_bb, pos, index, (float)padding, size);
            bb.normalize();
            offset_right = std::max(offset_right, bb.m_max.x - data.m_bb.m_max.x);
        } else if (pos == c_visuals::e_element_bb_position::left) {
            auto bb = g_visuals->get_element_bb(data.m_bb, pos, index, (float)padding, size);
            bb.normalize();
            offset_left = std::max(offset_left, data.m_bb.m_min.x - bb.m_min.x);
        }
    };

    update_side_offsets_from_element(data.m_health_bar.m_element_position, data.m_health_bar.m_element_index, data.m_health_bar.m_padding, (float)data.m_health_bar.m_size);
    update_side_offsets_from_element(data.m_armor_bar.m_element_position, data.m_armor_bar.m_element_index, data.m_armor_bar.m_padding, (float)data.m_armor_bar.m_size);
    
    if (GET_VAR(bool, VISUALS_PATH(m_name)) && data.m_name_text.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_name_text.m_element_position, data.m_name_text.m_element_index, data.m_name_text.m_padding, (float)data.m_name_text.m_size);
    }
    if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_weapon_text.m_element_position, data.m_weapon_text.m_element_index, data.m_weapon_text.m_padding, (float)data.m_weapon_text.m_size);
    }
    if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon)) && data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::left) {
        update_side_offsets_from_element(data.m_weapon_icon.m_element_position, data.m_weapon_icon.m_element_index, data.m_weapon_icon.m_padding, (float)data.m_weapon_icon.m_size);
    }

    int current_row_right = 1;
    int current_row_left = 1;

    auto flag_enabled = [&](e_esp_flags flag) -> bool {
        size_t idx = static_cast<size_t>(flag);
        return idx < flags_vec.size() && flags_vec[idx];
    };

    if (flag_enabled(e_esp_flags::esp_flag_scoped)) {
        draw_flag_text(data, "SCOPE", scope_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }
    if (flag_enabled(e_esp_flags::esp_flag_defusing)) {
        draw_flag_text(data, "DEFUSE", defuse_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }
    if (flag_enabled(e_esp_flags::esp_flag_armor_helmet)) {
        draw_flag_text(data, "HK", generic_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    } else if (flag_enabled(e_esp_flags::esp_flag_armor)) {
        draw_flag_text(data, "K", generic_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }
    if (flag_enabled(e_esp_flags::esp_flag_reloading)) {
        draw_flag_text(data, "RELOAD", reload_color, true, offset_right, current_row_right++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_bomb)) {
        draw_flag_text(data, "C4", bomb_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }
    if (flag_enabled(e_esp_flags::esp_flag_defuse)) {
        draw_flag_text(data, "KIT", defuse_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }
    if (flag_enabled(e_esp_flags::esp_flag_taser)) {
        draw_flag_text(data, "ZEUS", taser_color, false, offset_left, current_row_left++, data.m_alpha_modifier, data.m_draw_list);
    }

    if (flag_enabled(e_esp_flags::esp_flag_ping)) {
        int ping_row = 1;
        int max_top_index = 0;
        
        if (GET_VAR(bool, VISUALS_PATH(m_name)) && data.m_name_text.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_name_text.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_weapon_text.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon)) && data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_weapon_icon.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_health_bar)) && data.m_health_bar.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_health_bar.m_element_index);
        }
        if (GET_VAR(bool, VISUALS_PATH(m_armor_bar)) && data.m_armor_bar.m_element_position == c_visuals::e_element_bb_position::top) {
            max_top_index = std::max(max_top_index, data.m_armor_bar.m_element_index);
        }
        
        ping_row = max_top_index + 1;
        
        draw_ping_text(data, "14MS", hellcolor(255, 0, 0, 220), ping_row, data.m_alpha_modifier, data.m_draw_list);
    }
}



void c_visuals::draw_player_flags(player_visual_data_t& data) {
    if (!GET_VAR(bool, VISUALS_PATH(m_esp_enabled)))
        return;

    if (data.m_preview_mode) {
        draw_player_flags_preview(data);
    } else {
        draw_player_flags_runtime(data);
    }
}
void c_visuals::draw_icon(bb_t owner_bb, icon_object_t& obj, const icon_data_t& icon_data, float alpha_modifier, ImDrawList* draw_list) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    if (!icon_data.texture_view)
        return;
    bb_t bb = this->get_element_bb(owner_bb, obj.m_element_position, obj.m_element_index, obj.m_padding, (float)obj.m_size);
    bb.normalize();
    int iTargetSize = obj.m_size;
    float flWtoHRatio = 1.0f;
    if (icon_data.height > 0)
        flWtoHRatio = static_cast<float>(icon_data.width) / static_cast<float>(icon_data.height);
    auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
    auto Height = iTargetSize;

    ImVec2 vPos = bb.get_center();
    vPos.x -= Width * 0.5f;
    vPos.y -= Height * 0.5f;
    hellcolor iconColor = obj.m_color;
    iconColor.Value.w *= alpha_modifier;
    draw_list->AddImage((ImTextureID)icon_data.texture_view, ImVec2(vPos.x, vPos.y),
        ImVec2(vPos.x + Width, vPos.y + Height),
        ImVec2(0, 0), ImVec2(1, 1),
        iconColor);
}

void c_visuals::draw_grenade_icons(bb_t owner_bb, icon_object_t& obj, const std::vector<icon_data_t>& icons, float alpha_modifier, ImDrawList* draw_list) {
    if (!draw_list)
        draw_list = ImGui::GetBackgroundDrawList();
    if (icons.empty())
        return;

    bb_t bb = this->get_element_bb(owner_bb, obj.m_element_position, obj.m_element_index, obj.m_padding, (float)obj.m_size);
    bb.normalize();

    float icon_height = (float)obj.m_size;
    float gap = 2.f;

    float total_width = 0.f;
    for (const auto& icon : icons) {
        if (!icon.texture_view || icon.width <= 0 || icon.height <= 0)
            continue;
        float aspect = (float)icon.width / (float)icon.height;
        total_width += icon_height * aspect + gap;
    }
    if (total_width > 0.f)
        total_width -= gap;

    float y = bb.get_center().y - icon_height * 0.5f;
    float x = bb.get_center().x - total_width * 0.5f;

    hellcolor base_col = obj.m_color.AlphaOverride(alpha_modifier);

    for (const auto& icon : icons) {
        if (!icon.texture_view || icon.width <= 0 || icon.height <= 0)
            continue;

        float aspect = (float)icon.width / (float)icon.height;
        float icon_width = icon_height * aspect;

        hellvec2 min = { x, y };
        hellvec2 max = { x + icon_width, y + icon_height };

        draw_list->AddImage(
            (ImTextureID)icon.texture_view,
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            ImVec2(0, 0),
            ImVec2(1, 1),
            base_col
        );

        x += icon_width + gap;
    }
}
void c_visuals::draw_player_visual_data(player_visual_data_t& data) {
    if (!GET_VAR(bool, VISUALS_PATH(m_esp_enabled)))
        return;
    if (!data.m_draw_list)
        data.m_draw_list = ImGui::GetBackgroundDrawList();
    if (!data.m_preview_mode) {
        if (!data.m_controller || !data.m_pawn)
            return;
    }
    if (data.m_bb.m_oob)
        return;
    data.m_bb.align();

    if (GET_VAR(bool, VISUALS_PATH(m_bbox))) {
        data.m_draw_list->AddRect(
            data.m_bb.m_min + hellvec2(1.0f, 1.0f),
            data.m_bb.m_max - hellvec2(1.0f, 1.0f),
            data.m_bb.m_bb_bg_color.AlphaOverride(data.m_alpha_modifier),
            0.f,
            0,
            1.f
        );
        data.m_draw_list->AddRect(
            data.m_bb.m_min - hellvec2(1.0f, 1.0f),
            data.m_bb.m_max + hellvec2(1.0f, 1.0f),
            data.m_bb.m_bb_bg_color.AlphaOverride(data.m_alpha_modifier),
            0.f,
            0,
            1.f
        );
        data.m_draw_list->AddRect(
            data.m_bb.m_min,
            data.m_bb.m_max,
            data.m_bb.m_bb_fg_color.AlphaOverride(data.m_alpha_modifier),
            0.f,
            0,
            1.f
        );
    }
    if (GET_VAR(bool, VISUALS_PATH(m_name))) {
        if (GET_VAR(bool, VISUALS_PATH(m_enabled_steam_avatar)) && (data.m_steam_id != 0 || data.m_preview_mode)) {
            const float avatar_size = 10.0f;
            const float avatar_rounding = 3.0f;
            const float avatar_padding = 0.0f;
            
            ImTextureID avatar;
            if (data.m_preview_mode) {
                static ImTextureID preview_avatar;
                if (!preview_avatar) {
                    int width, height, channels;
                    unsigned char* avatar_data = stbi_load_from_memory(test_avatar, sizeof(test_avatar), &width, &height, &channels, 4);
                    if (avatar_data) {
                        ID3D11ShaderResourceView* texture = nullptr;
                        D3D11_TEXTURE2D_DESC desc = {};
                        desc.Width = width;
                        desc.Height = height;
                        desc.MipLevels = 1;
                        desc.ArraySize = 1;
                        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        desc.SampleDesc.Count = 1;
                        desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                        D3D11_SUBRESOURCE_DATA sub_resource = {};
                        sub_resource.pSysMem = avatar_data;
                        sub_resource.SysMemPitch = width * 4;

                        ID3D11Texture2D* texture2d = nullptr;
                        if (SUCCEEDED(g_interfaces->m_device->CreateTexture2D(&desc, &sub_resource, &texture2d))) {
                            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                            srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                            srv_desc.Texture2D.MipLevels = desc.MipLevels;

                            if (SUCCEEDED(g_interfaces->m_device->CreateShaderResourceView(texture2d, &srv_desc, &texture))) {
                                preview_avatar = reinterpret_cast<ImTextureID>(texture);
                            }
                            texture2d->Release();
                        }
                        stbi_image_free(avatar_data);
                    }
                }
                avatar = preview_avatar;
            } else {
                avatar = g_overlay->get_avatar(data.m_steam_id);
            }
            
            if (avatar) {
                bb_t name_bb = this->get_element_bb(data.m_bb, data.m_name_text.m_element_position, data.m_name_text.m_element_index, data.m_name_text.m_padding, data.m_name_text.m_size);
                auto font = this->get_font(data.m_name_text.m_font_type);
                hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, data.m_name_text.m_text);
                float total_width = avatar_size + text_size.x;
                hellvec2 combined_pos = { 0.f, name_bb.get_center().y - (text_size.y * 0.5f) };
                
                switch (data.m_name_text.m_text_alignment) {
                case e_text_alignment::text_alignment_center:
                    combined_pos.x = name_bb.get_center().x - (total_width * 0.5f);
                    break;
                case e_text_alignment::text_alignment_left:
                    combined_pos.x = name_bb.m_min.x;
                    break;
                case e_text_alignment::text_alignment_right:
                    combined_pos.x = name_bb.m_max.x - total_width;
                    break;
                }
                
                ImVec2 avatar_pos = ImVec2(combined_pos.x - 2.0f, combined_pos.y + (text_size.y - avatar_size) * 0.5f);
                ImVec2 avatar_max = ImVec2(avatar_pos.x + avatar_size, avatar_pos.y + avatar_size);
                
                ImU32 avatar_color = IM_COL32(255, 255, 255, (int)(255 * data.m_alpha_modifier));
                
                data.m_draw_list->PushClipRect(avatar_pos, avatar_max, true);
                data.m_draw_list->AddImageRounded(avatar, avatar_pos, avatar_max, ImVec2(0, 0), ImVec2(1, 1), avatar_color, avatar_rounding);
                data.m_draw_list->PopClipRect();
                
                data.m_name_text.m_text_offset_x = avatar_size - 4.0f;
            }
        } else {
            data.m_name_text.m_text_offset_x = 0.0f;
        }
        
        draw_text(
            data.m_bb,
            data.m_name_text,
            data.m_alpha_modifier,
            data.m_draw_list
        );
    }

    if (GET_VAR(bool, VISUALS_PATH(m_weapon_text))) {
        draw_text(
            data.m_bb,
            data.m_weapon_text,
            data.m_alpha_modifier,
            data.m_draw_list
        );
    }
    if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon))) {
        if (!data.m_preview_mode)
        {
            auto* active_weapon = data.m_pawn->get_active_weapon();
            if (active_weapon)
            {
                icon_data_t icon_data = {};
                if (active_weapon->weapon_data())
                {
                    const char* weapon_name = active_weapon->weapon_data()->m_szName();
                    if (weapon_name)
                        icon_data = get_weapon_icon_by_name(weapon_name);
                }
                
                if (!icon_data.texture_view && active_weapon->m_AttributeManager() && active_weapon->m_AttributeManager()->m_Item())
                {
                    uint16_t weapon_index = active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
                    icon_data = get_weapon_icon(weapon_index);
                }
                
                if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
                {
                    draw_icon(
                        data.m_bb,
                        data.m_weapon_icon,
                        icon_data,
                        data.m_alpha_modifier,
                        data.m_draw_list
                    );
                }
            }
        }
    }

    if (GET_VAR(bool, VISUALS_PATH(m_grenade_icons))) {
        if (!data.m_preview_mode) {
            std::vector<icon_data_t> grenade_icons;

            c_player_weapon_services* weapon_services = data.m_pawn->m_pWeaponServices();
            if (weapon_services) {
                auto& weapons = weapon_services->m_hMyWeapons();
                bool has_he = false, has_flash = false, has_molotov = false, has_smoke = false, has_decoy = false;

                for (int i = 0; i < weapons.size; ++i) {
                    c_weapon_base* w = weapons.elements[i].get<c_weapon_base>();
                    if (!w)
                        continue;

                    auto attr = w->m_AttributeManager();
                    if (!attr)
                        continue;

                    auto item = attr->m_Item();
                    if (!item)
                        continue;

                    uint16_t def_index = item->m_iItemDefinitionIndex();

                    if (def_index == WEAPON_HIGH_EXPLOSIVE_GRENADE) has_he = true;
                    if (def_index == WEAPON_FLASHBANG) has_flash = true;
                    if (def_index == WEAPON_MOLOTOV || def_index == WEAPON_INCENDIARY_GRENADE) has_molotov = true;
                    if (def_index == WEAPON_SMOKE_GRENADE) has_smoke = true;
                    if (def_index == WEAPON_DECOY_GRENADE) has_decoy = true;
                }

                if (has_he)       grenade_icons.push_back(get_panorama_texture("icons/equipment/hegrenade"));
                if (has_flash)    grenade_icons.push_back(get_panorama_texture("icons/equipment/flashbang"));
                if (has_molotov)  grenade_icons.push_back(get_panorama_texture("icons/equipment/incgrenade"));
                if (has_smoke)    grenade_icons.push_back(get_panorama_texture("icons/equipment/smokegrenade"));
                if (has_decoy)    grenade_icons.push_back(get_panorama_texture("icons/equipment/decoy"));
            }

            if (!grenade_icons.empty()) {
                draw_grenade_icons(
                    data.m_bb,
                    data.m_grenade_icons,
                    grenade_icons,
                    data.m_alpha_modifier,
                    data.m_draw_list
                );
            }
        }
    }
    if (GET_VAR(bool, VISUALS_PATH(m_health_bar))) {
        draw_bar(
            data.m_bb,
            data.m_health_bar,
            data.m_alpha_modifier,
            data.m_draw_list
        );
    }

    if (GET_VAR(bool, VISUALS_PATH(m_armor_bar))) {
        draw_bar(
            data.m_bb,
            data.m_armor_bar,
            data.m_alpha_modifier,
            data.m_draw_list
        );
    }

    if (GET_VAR(bool, VISUALS_PATH(m_skeleton_esp_enabled)) && !data.m_preview_mode) {
        draw_skeleton(
            data.m_pawn,
            GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_color)).AlphaOverride(data.m_alpha_modifier),
            GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_outline_color)).AlphaOverride(data.m_alpha_modifier),
            data.m_draw_list
        );
    }

    draw_player_flags(data);
}
void visualize_hitboxes() {
}
void c_visuals::present() {
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;
    draw_custom_scope();
    visualize_hitboxes();
    for (auto& cached_entity : g_entity_cache->m_players) {
        if (!cached_entity.check_and_update_pawn())
            continue;
        c_cs_player_controller* controller = cached_entity.m_controller;
        c_cs_player_pawn* pawn = cached_entity.m_pawn;
        if (!pawn)
            continue;
        if (!pawn->is_enemy())
            continue;
        if (!pawn->get_active_weapon())
            continue;
        if (!pawn->get_active_weapon()->weapon_data())
            continue;
        if (!pawn->get_active_weapon()->weapon_data()->m_szName())
            continue;
        int health = pawn->m_iHealth();
        if (health <= 0)
            continue;
        c_player_weapon_services* weapon_services = pawn->m_pWeaponServices();
        if (!weapon_services)
            continue;

        const char* weapon_name = pawn->get_active_weapon()->weapon_data()->m_szName();

        player_visual_data_t data = GET_VAR(c_visuals::player_visual_data_t, VISUALS_PATH(m_visual_data_settings));
        data.m_preview_mode = false;
        data.m_alpha_modifier = 1.f;
        data.m_draw_list = ImGui::GetBackgroundDrawList();
        data.m_cached_entity = &cached_entity;
        data.m_controller = controller;
        data.m_pawn = pawn;
        data.m_steam_id = controller->m_steamID();
        data.m_name_text.m_text = controller->m_sSanitizedPlayerName();
        data.m_name_text.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_name_position)));
        data.m_name_text.m_element_index = GET_VAR(int, VISUALS_PATH(m_name_index));
        
        data.m_weapon_text.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_weapon_text_position)));
        data.m_weapon_text.m_element_index = GET_VAR(int, VISUALS_PATH(m_weapon_text_index));
        
        data.m_armor_bar.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_armor_bar_position)));
        data.m_armor_bar.m_element_index = GET_VAR(int, VISUALS_PATH(m_armor_bar_index));
        
        if (data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::bottom) {
            if (GET_VAR(bool, VISUALS_PATH(m_armor_bar)) && data.m_armor_bar.m_element_position == c_visuals::e_element_bb_position::bottom) {
                if (data.m_weapon_text.m_element_index <= data.m_armor_bar.m_element_index) {
                    data.m_weapon_text.m_element_index = data.m_armor_bar.m_element_index + 1;
                }
            }
        }

        int ping = controller->m_iPing();
        data.m_ping_storage = std::to_string(ping) + "MS";

        data.m_weapon_name_storage = weapon_name ? weapon_name : "";
        if (data.m_weapon_name_storage.length() >= 7 && data.m_weapon_name_storage.substr(0, 7) == "weapon_")
        {
            data.m_weapon_name_storage.erase(0, 7);
        }
        data.m_weapon_text.m_text = data.m_weapon_name_storage.c_str();
        data.m_weapon_icon.m_color = GET_VAR(hellcolor, VISUALS_PATH(m_weapon_icon_color));
        data.m_weapon_icon.m_size = GET_VAR(int, VISUALS_PATH(m_weapon_icon_size));
        data.m_weapon_icon.m_padding = GET_VAR(int, VISUALS_PATH(m_weapon_icon_padding));
        data.m_weapon_icon.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_weapon_icon_position)));
        data.m_weapon_icon.m_element_index = GET_VAR(int, VISUALS_PATH(m_weapon_icon_index));
        
        if (data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::bottom) {
            if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::bottom) {
                if (data.m_weapon_icon.m_element_index <= data.m_weapon_text.m_element_index) {
                    data.m_weapon_icon.m_element_index = data.m_weapon_text.m_element_index + 1;
                }
            }
        }
        
        if (data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::bottom) {
            if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::bottom) {
                if (data.m_weapon_icon.m_element_index <= data.m_weapon_text.m_element_index) {
                    data.m_weapon_icon.m_element_index = data.m_weapon_text.m_element_index + 1;
                }
            }
        }
        data.m_grenade_icons.m_color = GET_VAR(hellcolor, VISUALS_PATH(m_grenade_icons_color));
        data.m_grenade_icons.m_size = GET_VAR(int, VISUALS_PATH(m_grenade_icon_size));
        data.m_grenade_icons.m_padding = GET_VAR(int, VISUALS_PATH(m_grenade_icon_padding));
        data.m_grenade_icons.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_grenade_icon_position)));
        data.m_grenade_icons.m_element_index = GET_VAR(int, VISUALS_PATH(m_grenade_icon_index));

        data.m_health_bar.m_value = health;
        data.m_health_bar.m_fg_color = GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color));
        data.m_health_bar.m_bg_color = GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color_bg));
        data.m_health_bar.m_glow_enabled = GET_VAR(bool, VISUALS_PATH(m_health_bar_glow_enabled));
        data.m_health_bar.m_glow_intensity = 1.2f;
        data.m_health_bar.m_gradient_enabled = GET_VAR(bool, VISUALS_PATH(m_health_bar_gradient_enabled));
        data.m_health_bar.m_gradient_color = GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_gradient_color));
        data.m_health_bar.m_gradient_reverse = false;
        
        data.m_armor_bar.m_value = pawn->m_ArmorValue();
        data.m_armor_bar.m_fg_color = GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color));
        data.m_armor_bar.m_bg_color = GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color_bg));
        data.m_armor_bar.m_glow_enabled = GET_VAR(bool, VISUALS_PATH(m_armor_bar_glow_enabled));
        data.m_armor_bar.m_glow_intensity = 1.2f;
        data.m_armor_bar.m_gradient_enabled = GET_VAR(bool, VISUALS_PATH(m_armor_bar_gradient_enabled));
        data.m_armor_bar.m_gradient_color = GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_gradient_color));
        data.m_armor_bar.m_gradient_reverse = true;
        
        data.m_bb.m_oob = this->get_bb(pawn, data.m_bb);
        this->draw_player_visual_data(data);
        
        if (GET_VAR(bool, VISUALS_PATH(m_bEnableOOFArrows))) {
            draw_oof(pawn);
        }
    }

    for (auto& cached_entity : g_entity_cache->m_players) {
        if (!cached_entity.check_and_update_pawn())
            continue;
        c_cs_player_pawn* pawn = cached_entity.m_pawn;
        if (!pawn || !pawn->is_enemy())
            continue;

        if (GET_VAR(bool, VISUALS_PATH(m_skeleton_esp_backtrack_enabled))) {
            draw_skeleton_backtrack(pawn, ImGui::GetBackgroundDrawList(), 1.0f);
        }

        if (GET_VAR(bool, VISUALS_PATH(m_skeleton_esp_onshot_enabled))) {
            draw_skeleton_onshot(pawn, ImGui::GetBackgroundDrawList(), 1.0f);
        }
    }
}
void c_visuals::draw_custom_scope() {
    auto local_pawn = g_interfaces->m_entity_system->get_player_pawn(0);
    if (!local_pawn || local_pawn->m_iHealth() <= 0 || !local_pawn->m_bIsScoped())
        return;
    if (!GET_VAR(bool, VISUALS_PATH(m_scope_overlay)))
        return;
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    hellvec2 screen_size = ImGui::GetIO().DisplaySize;
    hellvec2 screen_center = screen_size * 0.5f;
    if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_static) {
        float line_length = (float)GET_VAR(int, VISUALS_PATH(m_scope_overlay_length));
        float base_gap = (float)GET_VAR(int, VISUALS_PATH(m_scope_overlay_gap));
        
        static float s_spread_gap = 0.f;
        float spread_addition = 0.f;
        
        if (GET_VAR(bool, VISUALS_PATH(m_enabled_spread_gap))) {
            const auto weapon = g_ctx->m_active_weapon;
            const auto local_pawn = g_ctx->m_local_pawn;
            if (weapon && local_pawn) {
                constexpr float s_scale = 233.f;
                float target_spread = (weapon->get_inaccuracy() + weapon->get_spread()) * local_pawn->m_vecAbsVelocity().length_2d() * (screen_size.y / s_scale / 2.f);
                s_spread_gap += (target_spread - s_spread_gap) * 0.15f;
                spread_addition = s_spread_gap;
            }
        } else {
            s_spread_gap = 0.f;
        }
        
        float gap = base_gap + spread_addition;
        float fade_out_gap = gap / 2.f;
        auto col_inside = GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_inside));
        col_inside.Value.w = 1.f;
        auto col_outside = GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_outside));
        col_outside.Value.w = 0.f;
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - 0.5f, screen_center.y - gap - line_length },
            { screen_center.x + 0.5f, screen_center.y - gap },
            col_outside, col_outside, col_inside, col_inside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - 0.5f, screen_center.y + gap },
            { screen_center.x + 0.5f, screen_center.y + gap + line_length },
            col_inside, col_inside, col_outside, col_outside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - gap - line_length, screen_center.y - 0.5f },
            { screen_center.x - gap, screen_center.y + 0.5f },
            col_outside, col_inside, col_inside, col_outside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x + gap, screen_center.y - 0.5f },
            { screen_center.x + gap + line_length, screen_center.y + 0.5f },
            col_inside, col_outside, col_outside, col_inside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - 0.5f, screen_center.y - gap },
            { screen_center.x + 0.5f, screen_center.y - gap + fade_out_gap },
            col_inside, col_inside, col_outside, col_outside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - 0.5f, screen_center.y + gap - fade_out_gap },
            { screen_center.x + 0.5f, screen_center.y + gap },
            col_outside, col_outside, col_inside, col_inside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x - gap, screen_center.y - 0.5f },
            { screen_center.x - gap + fade_out_gap, screen_center.y + 0.5f },
            col_inside, col_outside, col_outside, col_inside
        );
        draw_list->AddRectFilledMultiColor(
            { screen_center.x + gap - fade_out_gap, screen_center.y - 0.5f },
            { screen_center.x + gap, screen_center.y + 0.5f },
            col_outside, col_inside, col_inside, col_outside
        );
    }
    else if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_full) {
        draw_list->AddLine({ 0.f, screen_center.y }, { screen_size.x, screen_center.y }, hellcolor(0, 0, 0));
        draw_list->AddLine({ screen_center.x, 0.f }, { screen_center.x, screen_size.y }, hellcolor(0, 0, 0));
    }
}
hellvec2 c_visuals::bb_t::get_anchor(e_anchor_type anchor_type, hellvec2 offset) const {
    float x_center = (m_min.x + m_max.x) * 0.5f;
    float y_center = (m_min.y + m_max.y) * 0.5f;
    hellvec2 return_val{};
    switch (anchor_type) {
    case e_anchor_type::top_left:
        return_val = hellvec2(m_min.x, m_min.y);
        break;
    case e_anchor_type::top_center:
        return_val = hellvec2(x_center, m_min.y);
        break;
    case e_anchor_type::top_right:
        return_val = hellvec2(m_max.x, m_min.y);
        break;
    case e_anchor_type::mid_left:
        return_val = hellvec2(m_min.x, y_center);
        break;
    case e_anchor_type::mid_center:
        return_val = hellvec2(x_center, y_center);
        break;
    case e_anchor_type::mid_right:
        return_val = hellvec2(m_max.x, y_center);
        break;
    case e_anchor_type::bottom_left:
        return_val = hellvec2(m_min.x, m_max.y);
        break;
    case e_anchor_type::bottom_center:
        return_val = hellvec2(x_center, m_max.y);
        break;
    case e_anchor_type::bottom_right:
        return_val = hellvec2(m_max.x, m_max.y);
        break;
    default:
        return_val = hellvec2(0, 0);
        break;
    }
    return return_val + offset;
}