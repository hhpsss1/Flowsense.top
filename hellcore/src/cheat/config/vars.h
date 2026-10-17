#pragma once

#include "config_system.h"
#include <cheat/features/visuals/visuals.h>

#define RAGEBOT_PATH( variable ) g_vars->m_aimbot.m_ragebot.variable
#define ANTIAIM_PATH( variable ) g_vars->m_aimbot.m_antiaim.variable
#define LEGITBOT_PATH( variable ) g_vars->m_aimbot.m_legitbot.variable
#define VISUALS_PATH( variable ) g_vars->m_visuals.variable
#define MISC_PATH( variable ) g_vars->m_misc.variable
#define SKINS_PATH( variable ) g_vars->m_skins.variable
#define CONFIG_PATH( variable ) g_vars->m_config.variable


enum eweapontype : uint32_t
{
	weapontype_knife = 0x0,
	weapontype_pistol = 0x1,
	weapontype_submachinegun = 0x2,
	weapontype_rifle = 0x3,
	weapontype_shotgun = 0x4,
	weapontype_sniper_rifle = 0x5,
	weapontype_machinegun = 0x6,
	weapontype_c4 = 0x7,
	weapontype_taser = 0x8,
	weapontype_grenade = 0x9,
	weapontype_equipment = 0xa,
	weapontype_stackableitem = 0xb,
	weapontype_fists = 0xc,
	weapontype_breachcharge = 0xd,
	weapontype_bumpmine = 0xe,
	weapontype_tablet = 0xf,
	weapontype_melee = 0x10,
	weapontype_shield = 0x11,
	weapontype_zone_repulsor = 0x12,
	weapontype_unknown = 0x13,
};

enum eitemdefinitionindex : short
{
	weapon_desert_eagle = 1,
	weapon_dual_berettas = 2,
	weapon_five_seven = 3,
	weapon_glock = 4,
	weapon_ak47 = 7,
	weapon_aug = 8,
	weapon_awp_ = 9,
	weapon_famas = 10,
	weapon_g3sg1 = 11,
	weapon_galil = 13,
	weapon_m249 = 14,
	weapon_m4a4 = 16,
	weapon_mac10 = 17,
	weapon_p90 = 19,
	weapon_ump = 24,
	weapon_mp5sd = 23,
	weapon_xm1024 = 25,
	weapon_bizon = 26,
	weapon_mag7 = 27,
	weapon_negev = 28,
	weapon_sawedoff = 29,
	weapon_tec9 = 30,
	weapon_taser = 31,
	weapon_hkp2000 = 32,
	weapon_mp7 = 33,
	weapon_mp9 = 34,
	weapon_nova = 35,
	weapon_p250 = 36,
	weapon_scar20 = 38,
	weapon_sg553 = 39,
	weapon_ssg08 = 40,
	weapon_flashbang = 43,
	weapon_hegrenade = 44,
	weapon_smoke = 45,
	weapon_molotov = 46,
	weapon_decoy = 47,
	weapon_incdendiary = 48,
	weapon_healthshot = 57,
	weapon_m4a1_s = 60,
	weapon_usp_s = 61,
	weapon_cz75 = 63,
	weapon_r8 = 64,
	weapon_bayonet = 500,
	weapon_knifecss = 503,
	weapon_knifeflip = 505,
	weapon_knifegut = 506,
	weapon_knifekarambit = 507,
	weapon_knifem9bayonet = 508,
	weapon_knifetactical = 509,
	weapon_knifefalchion = 512,
	weapon_knifesurvivalbowie = 514,
	weapon_knifebutterfly = 515,
	weapon_knifepush = 516,
	weapon_knifecord = 517,
	weapon_knifecanis = 518,
	weapon_knifeursus = 519,
	weapon_knifegypsyjackknife = 520,
	weapon_knifeoutdoor = 521,
	weapon_knifestiletto = 522,
	weapon_knifewidowmaker = 523,
	weapon_knifeskeleton = 525,
	weapon_knifekukri = 526,
};

enum e_materials : unsigned int {
	material_default = 0,
	material_latex,
	material_glow,
	material_ghost,
	material_flat,
	material_max
};

enum e_ragebot_weapons : unsigned int {
	weapon_light_pistol = 0,
	weapon_deagle,
	weapon_revolver,
	weapon_smg,
	weapon_lmg,
	weapon_ar,
	weapon_shotgun,
	weapon_scout,
	weapon_autosniper,
	weapon_awp,
	weapon_not_wanted, // not wanted like me
	weapon_max
};

enum e_ragebot_hitboxes : unsigned int {
	rage_hitbox_head = 0, // HITBOX_HEAD
	rage_hitbox_upper_chest, // HITBOX_UPPER_CHEST
	rage_hitbox_chest, // HITBOX_CHEST
	rage_hitbox_stomach, // HITBOX_STOMACH
	rage_hitbox_pelvis, // HITBOX_PELVIS
	rage_hitbox_arms, // HITBOX left/right FOREARM
	rage_hitbox_legs, // HITBOX left/right CALVes
	rage_hitbox_feet, // HITBOX left/right FEET
	rage_hitbox_max
};

enum e_ragebot_target_types : unsigned int {
	rage_target_damage = 0,
	rage_target_hitchance,
	rage_target_low_health,
	rage_target_max
};

enum e_ragebot_hitbox_preference_types : unsigned int {
	rage_hitbox_preference_damage = 0,
	rage_hitbox_preference_hitchance,
	rage_hitbox_preference_max
};

enum e_ragebot_autostop_types : unsigned int {
	rage_autostop_early = 0,
	rage_autostop_in_air,
	rage_autostop_max
};


enum e_removals : unsigned int {
	removal_scope = 0,
	removal_local_name,
	removal_team_names,
	removal_smoke,
	removal_flash,
	removal_legs,
	removal_aimpunch,
	removal_max
};

enum e_hitmarker : unsigned int {
	hitmarker_2d,
	hitmarker_3d,
	hitmarker_max
};

enum e_hitsounds : unsigned int {
	hitsounds_coin = 0,
	hitsounds_click,
	hitsounds_shot,
	hitsounds_assembly,
	hitsounds_star,
	hitsounds_max
};

enum e_scope_type : int {
	scope_type_static = 0,
	scope_type_full
};

enum e_esp_flags : unsigned int {
	esp_flag_scoped = 0,
	esp_flag_bomb,
	esp_flag_taser,
	esp_flag_defuse,
	esp_flag_defusing,
	esp_flag_armor,
	esp_flag_armor_helmet,
	esp_flag_reloading,
	esp_flag_ping,
	esp_flag_max
};

enum class e_key_state : int {
	none = 0,
	down,
	up,
	released
};

enum e_key_mode : unsigned int {
	key_mode_toggle = 0,
	key_mode_hold
};

enum e_backtrack_cham_type : unsigned int {
	backtrack_cham_last = 0,
	backtrack_cham_all,
	backtrack_cham_max,
};

enum e_particle_type : unsigned int {
	particle_firework_1 = 0,
	particle_firework_2,
	particle_feathers,
	particle_explosion,
	particle_glow_stars,
	particle_sakura2,
	particle_sakura3,
	particle_max
};

struct player_visual_data_t;

struct keybind_t;
typedef std::unordered_map<ImGuiID, std::vector<keybind_t>> kb_map_t;

class c_vars {
public:
	struct {
		struct {

			ADD_VAR(bool, m_enabled_ragebot, false);
			ADD_VAR(bool, m_autofire, false);
			ADD_VAR(bool, m_silent_aim, false);
			ADD_VAR(bool, m_autostop, false);
			ADD_VAR(bool, m_force_shoot, false);
			ADD_VAR(int, m_forceshoot_iterations, 3);
			ADD_VAR(int, m_forceshot_spread_scale, 50);
			ADD_VAR(bool, m_autowall, true);
			ADD_VAR(bool, m_taser_bot, false);
			ADD_VAR(bool, m_auto_scope, false);
			ADD_VAR(hellcolor, m_taser_bot_radius_color, hellcolor(255, 255, 255, 255));
			ADD_VAR_VEC(bool, e_ragebot_autostop_types::rage_autostop_max, m_autostop_modes, false);

			ADD_VAR(int, m_mindamage_light_pistol, 0);
			ADD_VAR(int, m_mindamage_deagle, 0);
			ADD_VAR(int, m_mindamage_revolver, 0);
			ADD_VAR(int, m_mindamage_smg, 0);
			ADD_VAR(int, m_mindamage_lmg, 0);
			ADD_VAR(int, m_mindamage_ar, 0);
			ADD_VAR(int, m_mindamage_shotgun, 0);
			ADD_VAR(int, m_mindamage_scout, 0);
			ADD_VAR(int, m_mindamage_autosniper, 0);
			ADD_VAR(int, m_mindamage_awp, 0);
			ADD_VAR(bool, m_duck_peek, false);

			ADD_VAR(int, m_pointscale_light_pistol, 0);
			ADD_VAR(int, m_pointscale_deagle, 0);
			ADD_VAR(int, m_pointscale_revolver, 0);
			ADD_VAR(int, m_pointscale_smg, 0);
			ADD_VAR(int, m_pointscale_lmg, 0);
			ADD_VAR(int, m_pointscale_ar, 0);
			ADD_VAR(int, m_pointscale_shotgun, 0);
			ADD_VAR(int, m_pointscale_scout, 0);
			ADD_VAR(int, m_pointscale_autosniper, 0);
			ADD_VAR(int, m_pointscale_awp, 0);

			ADD_VAR(int, m_hitchance_light_pistol, 0);
			ADD_VAR(int, m_hitchance_deagle, 0);
			ADD_VAR(int, m_hitchance_revolver, 0);
			ADD_VAR(int, m_hitchance_smg, 0);
			ADD_VAR(int, m_hitchance_lmg, 0);
			ADD_VAR(int, m_hitchance_ar, 0);
			ADD_VAR(int, m_hitchance_shotgun, 0);
			ADD_VAR(int, m_hitchance_scout, 0);
			ADD_VAR(int, m_hitchance_autosniper, 0);
			ADD_VAR(int, m_hitchance_awp, 0);

			ADD_VAR_VEC(e_ragebot_target_types, e_ragebot_weapons::weapon_max, m_target_type, e_ragebot_target_types::rage_target_damage);
			ADD_VAR_VEC(e_ragebot_hitbox_preference_types, e_ragebot_weapons::weapon_max, m_hitbox_preference, e_ragebot_hitbox_preference_types::rage_hitbox_preference_hitchance);


			ADD_VAR_VEC(std::vector<int>, e_ragebot_weapons::weapon_max, m_selected_hitboxes, {});
			ADD_VAR_VEC(std::vector<int>, e_ragebot_weapons::weapon_max, m_multipoint_hitboxes, {});
		} m_ragebot;

		struct {
			ADD_VAR(bool, m_enabled_antiaim, false);
			ADD_VAR(bool, m_at_target, false);
			ADD_VAR(bool, m_hide_shots, false);

			ADD_VAR(bool, m_override_left, false);
			ADD_VAR(bool, m_override_right, false);

			ADD_VAR(int, m_yaw, 0);
			ADD_VAR(int, m_pitch, 0);

			ADD_VAR(bool, m_yaw_jitter, false);
			ADD_VAR(bool, m_pitch_jitter, false);
			ADD_VAR(int, m_yaw_jitter_amount, 0);
			ADD_VAR(int, m_pitch_jitter_amount, 0);
		} m_antiaim;

		struct {
		} m_legitbot;
	} m_aimbot;
	struct {

		ADD_VAR(bool, m_visualize_hitboxes, false);
		ADD_VAR(hellcolor, m_visualize_hitbox_color, hellcolor(255, 0, 0));
		ADD_VAR(int, m_visualize_hitbox_fov, 20);

		ADD_VAR(hellcolor, m_shot_sparks_color, hellcolor(147, 159, 217));
		ADD_VAR(bool, m_shot_sparks_enabled, false);

		ADD_VAR(bool, m_enabled_kill_effects, false);
		ADD_VAR(int, m_kill_effects_type, e_particle_type::particle_explosion);

		ADD_VAR(bool, m_chams_enabled, false);
		ADD_VAR(bool, m_glow_chams_local_visible, false);
		ADD_VAR(bool, m_glow_chams_local_invisible, false);
		ADD_VAR(bool, m_glow_chams_local_bt_visible, false);
		ADD_VAR(bool, m_glow_chams_local_bt_invisible, false);
		ADD_VAR(bool, m_glow_chams_enemy_visible, false);
		ADD_VAR(bool, m_glow_chams_enemy_invisible, false);
		ADD_VAR(bool, m_glow_chams_enemy_os_visible, false);
		ADD_VAR(bool, m_glow_chams_enemy_os_invisible, false);
		ADD_VAR(bool, m_glow_chams_enemy_bt_visible, false);
		ADD_VAR(bool, m_glow_chams_enemy_bt_invisible, false);

		ADD_VAR(bool, m_particle_modulation, false);
		ADD_VAR(hellcolor, m_molotov_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_explosion_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_taser_stim_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_sakura_color, hellcolor(255, 255, 255, 255));

		ADD_VAR(bool, m_chams_enemy_ragdoll, false);
		ADD_VAR(hellcolor, m_chams_enemy_color, hellcolor(0, 255, 0));
		ADD_VAR(hellcolor, m_chams_enemy_color_invis, hellcolor(255, 0, 0));
		ADD_VAR(int, m_chams_enemy_material, e_materials::material_latex);
		ADD_VAR(int, m_chams_enemy_material_invis, e_materials::material_latex);

		ADD_VAR(int, m_chams_type_bt, e_backtrack_cham_type::backtrack_cham_last);
		ADD_VAR(int, m_chams_local_type_bt, e_backtrack_cham_type::backtrack_cham_last);

		ADD_VAR(bool, m_chams_enabled_bt, false);
		ADD_VAR(hellcolor, m_chams_enemy_color_bt, hellcolor(0, 255, 0));
		ADD_VAR(hellcolor, m_chams_enemy_color_invis_bt, hellcolor(255, 0, 0));
		ADD_VAR(int, m_chams_enemy_material_bt, e_materials::material_latex);
		ADD_VAR(int, m_chams_enemy_material_invis_bt, e_materials::material_latex);

		ADD_VAR(bool, m_chams_enabled_os, false);
		ADD_VAR(hellcolor, m_chams_enemy_color_os, hellcolor(0, 255, 0));
		ADD_VAR(hellcolor, m_chams_enemy_color_invis_os, hellcolor(255, 0, 0));
		ADD_VAR(int, m_chams_enemy_material_os, e_materials::material_latex);
		ADD_VAR(int, m_chams_enemy_material_invis_os, e_materials::material_latex);
		ADD_VAR(int, m_chams_os_expiration, 3);

		ADD_VAR(bool, m_chams_local_attachments_enabled, false);
		ADD_VAR(hellcolor, m_chams_local_attachments_color, hellcolor(255, 255, 255));
		ADD_VAR(int, m_chams_local_attachments_material, e_materials::material_latex);

		ADD_VAR(bool, m_chams_local_enabled, false);
		ADD_VAR(hellcolor, m_chams_local_color, hellcolor(0, 255, 0));
		ADD_VAR(hellcolor, m_chams_local_color_invis, hellcolor(255, 0, 0));
		ADD_VAR(int, m_chams_local_material, e_materials::material_latex);
		ADD_VAR(int, m_chams_local_material_invis, e_materials::material_latex);

		ADD_VAR(bool, m_chams_local_enabled_bt, false);
		ADD_VAR(hellcolor, m_chams_local_color_bt, hellcolor(0, 255, 0));
		ADD_VAR(hellcolor, m_chams_local_color_invis_bt, hellcolor(255, 0, 0));
		ADD_VAR(int, m_chams_local_material_bt, e_materials::material_latex);
		ADD_VAR(int, m_chams_local_material_invis_bt, e_materials::material_latex);

		ADD_VAR(bool, m_esp_enabled, false);
		ADD_VAR(bool, m_bbox, false);
		ADD_VAR(bool, m_visible_only, false);
		ADD_VAR(bool, m_name, false);
		ADD_VAR(bool, m_ping, false);
		ADD_VAR(bool, m_weapon_text, false);
		ADD_VAR(bool, m_weapon_icon, false);
		ADD_VAR(float, m_bar_glow_intensity, 1.0f);
		ADD_VAR(bool, m_bar_glow_enabled, false);
		ADD_VAR(bool, m_health_bar, false);
		ADD_VAR(bool, m_armor_bar, false);
		ADD_VAR(bool, m_skeleton_esp_enabled, false);
		ADD_VAR(bool, m_skeleton_esp_onshot_enabled, false);
		ADD_VAR(bool, m_skeleton_esp_backtrack_enabled, false);
		ADD_VAR(int, m_skeleton_esp_backtrack_type, e_backtrack_cham_type::backtrack_cham_last);
		ADD_VAR(bool, m_bEnableOOFArrows, false);
		ADD_VAR(bool, m_bOOFArrowsGlow, false);
		ADD_VAR(int, m_onshot_skeleton_duration, 3);
		ADD_VAR(hellcolor, m_colOOFArrows, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_onshot_color, hellcolor(255, 0, 0, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_onshot_color_outline, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_backtrack_color, hellcolor(0, 255, 0, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_backtrack_color_outline, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_skeleton_esp_outline_color, hellcolor(0, 0, 0, 180));
		ADD_VAR(hellcolor, m_bb_fg_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_bb_bg_color, hellcolor(0, 0, 0, 150));
		ADD_VAR(hellcolor, m_weapon_name_color, hellcolor(255, 255, 255, 220));
		ADD_VAR(hellcolor, m_weapon_name_color_bg, hellcolor(0, 0, 0, 255));
		ADD_VAR(hellcolor, m_weapon_icon_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(int, m_weapon_icon_position, (int)c_visuals::e_element_bb_position::bottom);
		ADD_VAR(int, m_weapon_icon_index, 3);
		ADD_VAR(int, m_weapon_icon_size, 16);
		ADD_VAR(int, m_weapon_icon_padding, 8);
		ADD_VAR(bool, m_grenade_icons, false);
		ADD_VAR(hellcolor, m_grenade_icons_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(int, m_grenade_icon_position, (int)c_visuals::e_element_bb_position::bottom);
		ADD_VAR(int, m_grenade_icon_index, 2);
		ADD_VAR(int, m_grenade_icon_size, 12);
		ADD_VAR(int, m_grenade_icon_padding, 4);
		ADD_VAR(hellcolor, m_name_color, hellcolor(255, 255, 255, 220));
		ADD_VAR(hellcolor, m_name_color_bg, hellcolor(0, 0, 0, 150));
		ADD_VAR(int, m_name_position, (int)c_visuals::e_element_bb_position::top);
		ADD_VAR(int, m_name_index, 1);
		ADD_VAR(int, m_weapon_text_position, (int)c_visuals::e_element_bb_position::bottom);
		ADD_VAR(int, m_weapon_text_index, 2);
		ADD_VAR(int, m_armor_bar_position, (int)c_visuals::e_element_bb_position::bottom);
		ADD_VAR(int, m_armor_bar_index, 1);
		ADD_VAR(hellcolor, m_health_bar_color, hellcolor(54, 153, 34));
		ADD_VAR(hellcolor, m_armor_bar_color, hellcolor(33, 80, 156));
		ADD_VAR(hellcolor, m_health_bar_color_bg, hellcolor(0, 0, 0, 120));
		ADD_VAR(hellcolor, m_armor_bar_color_bg, hellcolor(0, 0, 0, 120));
		ADD_VAR(bool, m_health_bar_glow, false);
		ADD_VAR(float, m_health_bar_glow_intensity, 1.0f);
		ADD_VAR(bool, m_armor_bar_glow, false);
		ADD_VAR(float, m_armor_bar_glow_intensity, 1.0f);
		ADD_VAR(bool, m_health_bar_glow_enabled, false);
		ADD_VAR(bool, m_armor_bar_glow_enabled, false);
		ADD_VAR(bool, m_health_bar_gradient_enabled, false);
		ADD_VAR(bool, m_armor_bar_gradient_enabled, false);
		ADD_VAR(hellcolor, m_health_bar_gradient_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_armor_bar_gradient_color, hellcolor(255, 255, 255, 255));

		ADD_VAR(hellcolor, m_esp_flags_color, hellcolor(255, 255, 255, 220));
		ADD_VAR_VEC(bool, e_esp_flags::esp_flag_max, m_esp_flags, {});
		ADD_VAR(int, m_esp_flags_font_type, (int)c_visuals::e_font_type::font_type_smallest_pixel);
		ADD_VAR(int, m_esp_flags_shadow_type, (int)c_visuals::e_text_shadow_type::text_shadow_full);
		ADD_VAR(int, m_esp_flags_size, 2);
		ADD_VAR(int, m_esp_flags_padding, 4);
		ADD_VAR(int, m_left_flags_icon_size, 14);

		ADD_VAR(bool, m_left_flags_icon_type, false);

		ADD_VAR(int, m_aspect_ratio, 0);

		ADD_VAR(bool, m_override_world_fov, false);
		ADD_VAR(int, m_override_world_fov_value, 90);
		ADD_VAR(int, m_override_world_fov_value_first_scope, 0);
		ADD_VAR(int, m_override_world_fov_value_second_scope, 0);
		ADD_VAR(bool, m_dynamic_fov, false);

		ADD_VAR(bool, m_override_viewmodel_fov, false);
		ADD_VAR(int, m_override_viewmodel_value, 1);
		ADD_VAR(int, m_override_viewmodel_value_fov_x, 1);
		ADD_VAR(int, m_override_viewmodel_value_fov_y, 1);
		ADD_VAR(int, m_override_viewmodel_value_fov_z, 1);

		ADD_VAR(bool, m_third_person_enabled, false);
		ADD_VAR(int, m_third_person_distance, 100);
		ADD_VAR(bool, m_remove_visual_punch, false);

		ADD_VAR(bool, m_enable_world_modulation, false);
		ADD_VAR(hellcolor, m_world_color, hellcolor(255, 255, 255, 220));
		ADD_VAR(bool, m_enable_lightnings_modulation, false);
		ADD_VAR(hellcolor, m_lightings_color, hellcolor(255, 255, 255, 220));
		ADD_VAR(bool, m_enable_sky_modulation, false);
		ADD_VAR(bool, m_enable_custom_sky, false);
		ADD_VAR(bool, m_enable_world_weather, false);
		ADD_VAR(bool, m_enable_world_wetness, false);
		ADD_VAR(bool, m_enable_world_blur, false);
		ADD_VAR(float, m_world_blur_near_start, 0.f);
		ADD_VAR(float, m_world_blur_near_end, 0.f);
		ADD_VAR(float, m_world_blur_far_start, 0.f);
		ADD_VAR(float, m_world_blur_far_end, 0.f);
		ADD_VAR(hellcolor, m_ash_color, hellcolor(147, 159, 217));
		ADD_VAR(int, m_weather_index, 0);
		ADD_VAR(hellcolor, m_sky_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_clouds_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_sun_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_grenade_prediction_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(bool, m_enable_grenade_prediction, false);
		ADD_VAR(bool, m_grenade_prediction_pulsating, true);
		ADD_VAR(bool, m_grenade_prediction_glow, true);
		ADD_VAR(hellcolor, m_proximity_warnings_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(bool, m_enable_proximity_warnings, false);
		ADD_VAR(bool, m_enable_grenade_names, false);
		ADD_VAR(bool, m_grenade_names_show_icon, true);
		ADD_VAR(hellcolor, m_grenade_names_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_grenade_names_color_bg, hellcolor(0, 0, 0, 180));
		ADD_VAR(int, m_grenade_names_font_type, 1);
		ADD_VAR(int, m_grenade_names_shadow_type, 2);
		ADD_VAR(bool, m_dropped_items_names, false);
		ADD_VAR(bool, m_dropped_items_icons, false);
		ADD_VAR(hellcolor, m_dropped_items_names_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_dropped_items_names_color_bg, hellcolor(0, 0, 0, 180));
		ADD_VAR(hellcolor, m_dropped_items_icons_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(int, m_dropped_items_names_font_type, 1);
		ADD_VAR(int, m_dropped_items_names_shadow_type, 2);
		ADD_VAR(int, m_dropped_items_distance, 300);
		ADD_VAR(bool, m_planted_bomb_overlay, false);
		ADD_VAR(bool, m_planted_bomb_world_icon, false);
		ADD_VAR(bool, m_planted_bomb_world_timer, false);
		ADD_VAR(hellcolor, m_planted_bomb_icon_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_planted_bomb_timer_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_planted_bomb_timer_color_bg, hellcolor(0, 0, 0, 180));
		ADD_VAR(int, m_planted_bomb_timer_font_type, 1);
		ADD_VAR(int, m_planted_bomb_timer_shadow_type, 2);
		ADD_VAR(int, m_planted_bomb_distance, 300);
		ADD_VAR(hellcolor, m_inferno_radius_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(bool, m_enable_inferno_radius, false);
		ADD_VAR(int, m_from_middle_alpha, 130);
		ADD_VAR(int, m_to_middle_alpha, 220);
		ADD_VAR(hellcolor, m_smoke_radius_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(bool, m_enable_smoke_radius, false);
		ADD_VAR(int, m_smoke_from_middle_alpha, 130);
		ADD_VAR(int, m_smoke_to_middle_alpha, 220);

		ADD_VAR_VEC(bool, e_removals::removal_max, m_removals, {});
		ADD_VAR(int, m_world_exposure, 0);
		ADD_VAR(int, m_skybox_index, 0);
		ADD_VAR(bool, m_enable_custom_skyboxes, false);

		ADD_VAR(bool, m_enable_custom_fog, false);
		ADD_VAR(float, m_custom_fog_start_distance, 100.f);
		ADD_VAR(float, m_custom_fog_end_distance, 3000.0f);
		ADD_VAR(float, m_custom_fog_falloff, 1.0f);
		ADD_VAR(float, m_custom_fog_brightness, 1.0f);
		ADD_VAR(hellcolor, m_custom_fog_color, hellcolor(147, 159, 217, 255));


		ADD_VAR(bool, m_bVolumetricFog, false);
		ADD_VAR(float, m_flVolumetricScattering, 1.0f);
		ADD_VAR(hellcolor, m_volumetricFogColor, hellcolor(255, 255, 255, 255));
		ADD_VAR(float, m_flVolumetricAnisotropy, 0.0f);
		ADD_VAR(float, m_flVolumetricDrawDistance, 8000.0f);
		ADD_VAR(float, m_flVolumetricFadeStart, 0.0f);
		ADD_VAR(float, m_flVolumetricFadeEnd, 1000.0f);

		ADD_VAR(c_visuals::player_visual_data_t, m_visual_data_settings, {});

		ADD_VAR(bool, m_local_impact_boxes, false);
		ADD_VAR(hellcolor, m_local_impact_boxes_color, hellcolor(255, 255, 255));
		ADD_VAR(int, m_local_impact_boxes_duration, 4);
		ADD_VAR(int, m_local_impact_boxes_size, 1);

		ADD_VAR(bool, m_local_tracers, false);
		ADD_VAR(bool, m_local_tracers_only_topoint, false);
		ADD_VAR(hellcolor, m_local_impact_tracers_color, hellcolor(255, 255, 255));
		ADD_VAR(int, m_local_impact_tracers_duration, 1);
		ADD_VAR(int, m_local_impact_tracers_width, 5);

		ADD_VAR(bool, m_local_glow, false);
		ADD_VAR(hellcolor, m_transparency_color, hellcolor(0, 0, 0));
		ADD_VAR(bool, m_transparency_in_scope, false);
		ADD_VAR(hellcolor, m_local_glow_color, hellcolor(255, 255, 255));
		ADD_VAR(bool, m_teamate_glow, false);
		ADD_VAR(hellcolor, m_teamate_glow_color, hellcolor(255, 255, 255));
		ADD_VAR(bool, m_enemy_glow, false);
		ADD_VAR(hellcolor, m_enemy_glow_color, hellcolor(255, 255, 255));


		ADD_VAR(int, m_selected_skybox, 0);

		ADD_VAR(bool, m_scope_overlay, false);
		ADD_VAR(int, m_scope_overlay_gap, 40);
		ADD_VAR(int, m_scope_overlay_length, 100);
		ADD_VAR(int, m_scope_type, e_scope_type::scope_type_full);
		ADD_VAR(hellcolor, m_scope_col_inside, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_scope_col_outside, hellcolor(255, 255, 255, 255));



		ADD_VAR(hellcolor, m_enabled_3d_hitmarkers_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_enabled_spread_circle_color_first, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_enabled_spread_circle_color_last, hellcolor(255, 255, 255, 255));

		ADD_VAR_VEC(bool, e_hitmarker::hitmarker_max, m_hitmarkers, {});
		ADD_VAR(int, m_hitmarker_size, 5);
		ADD_VAR(int, m_hitmarker_gap, 2);
		ADD_VAR(int, m_hitmarker_thickness, 1);
		ADD_VAR(int, m_3d_damage_markers_font_type, 2);
		ADD_VAR(int, m_3d_damage_markers_shadow_type, 0);

		ADD_VAR(bool, m_enabled_3d_damage_markers, false);
		ADD_VAR(hellcolor, m_enabled_3d_damage_markers_color, hellcolor(255, 255, 255, 255));
		ADD_VAR(hellcolor, m_enabled_3d_damage_markers_color_bg, hellcolor(0, 0, 0, 180));

		ADD_VAR(bool, m_enabled_3d_hitmarkers, false);
		ADD_VAR(bool, m_enabled_spread_circle, false);

		ADD_VAR(bool, m_bomb_hud_enabled, false);
		ADD_VAR(bool, m_enabled_spread_gap, false);
		ADD_VAR(bool, m_enabled_hotkey_list, false);
		ADD_VAR(bool, m_enabled_observer_list, false);
		ADD_VAR(bool, m_enabled_steam_avatar, false);
		ADD_VAR(hellcolor, m_indicator_icon_color, hellcolor(255, 255, 255, 255));
	} m_visuals;

	struct {

		ADD_VAR(bool, m_menu_click_sounds, true);
		ADD_VAR(bool, m_enabled_bunny_hop, false);
		ADD_VAR(bool, m_enabled_jump_bug, false);
		ADD_VAR(bool, m_enabled_long_jump, false);
		ADD_VAR(bool, m_enabled_edge_jump, false);
		ADD_VAR(bool, m_enabled_quick_stop, false);
		ADD_VAR(bool, m_enabled_slow_walk, false);
		ADD_VAR(bool, m_enabled_auto_peek, false);
		ADD_VAR(int, m_auto_peek_fade, 1);
		ADD_VAR(hellcolor, m_auto_peek_color, hellcolor(135, 206, 250, 255));
		ADD_VAR(hellcolor, m_auto_peek_color_retreating, hellcolor(255, 100, 100, 255));
		ADD_VAR(int, m_slow_walk_percent, 50);
		ADD_VAR(bool, m_enabled_autostrafe, false);
		ADD_VAR(int, m_autostrafe_boost, 100);
		ADD_VAR(bool, m_enabled_hitsound, false);
		ADD_VAR(int, m_hitsound_volume, 50);
		ADD_VAR(int, m_hitsound_selection, e_hitsounds::hitsounds_coin);
		ADD_VAR(bool, m_bHitMarker2d, false);
		ADD_VAR(hellcolor, m_colHitMarker2d, hellcolor(255, 255, 255, 255));
		ADD_VAR(std::vector<bool>, m_hitlog_modes, std::vector<bool>(2, false));
		ADD_VAR(int, m_hitlog_position, 0);
		ADD_VAR(bool, m_enabled_indicators, false);
		ADD_VAR(std::vector<bool>, m_indicator_modes, std::vector<bool>(10, true));
		ADD_VAR(hellcolor, m_accent_color, hellcolor(135, 185, 255, 255));
		ADD_VAR(bool, m_enabled_watermark, false);
		ADD_VAR(bool, m_preserve_kill_feed, false);
		ADD_VAR(bool, m_reveal_equipment, false);
		ADD_VAR(bool, m_enabled_straight_throw, false);
		ADD_VAR(bool, m_enabled_grenade_release, false);
		ADD_VAR(int, m_grenade_release_damage, 25);

		ADD_VAR(bool, m_enabled_autobuy, false);
		ADD_VAR(bool, m_enabled_ramp_boost, false);
		ADD_VAR(int, m_autobuy_primary, 0);
		ADD_VAR(int, m_autobuy_secondary, 0);
		ADD_VAR(std::vector<bool>, m_autobuy_additional, std::vector<bool>(6, false));
	} m_misc;

	struct skins_t
	{
		ADD_VAR(int, m_agent_selected_ct, 0);
		ADD_VAR(int, m_agent_selected_t, 0);
		ADD_VAR(int, m_custom_model_selected, 0);

		struct skin_settings_t
		{
			int m_item_definition_index = 0;
			int m_paint_kit = 0;
			int m_previous_skin = -1;
			float m_wear = 0.01f;
			int m_seed = 0;
		};
		ADD_VAR_VEC(c_vars::skins_t::skin_settings_t, 100, m_skin_settings, {});
		ADD_VAR_VEC(c_vars::skins_t::skin_settings_t, 100, m_glove_settings, {});
		ADD_VAR_VEC(c_vars::skins_t::skin_settings_t, 100, m_knives_settings, {});

		ADD_VAR(int, m_knife_selected, 0);
		ADD_VAR(int, m_glove_selected, 0);

		ADD_VAR(bool, m_enabled_skinchanger, 0);
		ADD_VAR(int, m_selected_weapon, 0);
		ADD_VAR(int, m_selected_type, 0);
	} m_skins;

	struct {
		ADD_VAR(kb_map_t, m_widget_keybinds, {});
	} m_config;
};
inline auto g_vars = std::make_unique<c_vars>();

enum e_keybind_type : unsigned int {
	keybind_type_unset = 0,
	keybind_type_checkbox,
	keybind_type_slider_int,
	keybind_type_slider_float,
};

struct keybind_t {
	e_key_mode m_mode = e_key_mode::key_mode_hold;
	fnv1a_t m_holder_id = -1;
	int m_key = -1;

	e_keybind_type m_keybind_type = e_keybind_type::keybind_type_unset;

	std::variant<bool, int, float> m_default_val;
	std::variant<std::monostate, bool, int, float> m_override_val;

	bool m_is_on_mode = true;
	bool m_keybind_active = false;
	bool m_on_mode_activated = false;

	keybind_t() = default;

	keybind_t(ImGuiID widget_id, fnv1a_t holder_id, int key, e_keybind_type kb_type, std::variant<bool, int, float> default_val)
		: m_mode(e_key_mode::key_mode_hold), m_holder_id(holder_id), m_key(key), m_keybind_type(kb_type), m_default_val(default_val), m_override_val(std::monostate{}), m_is_on_mode(true), m_keybind_active(false), m_on_mode_activated(false) {
	}
};