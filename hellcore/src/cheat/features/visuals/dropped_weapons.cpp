#include "dropped_weapons.h"
#include <includes.h>
#include <cheat/config/vars.h>
#include <context.h>
#include <core/interfaces/interfaces.h>
#include <sdk/entity/entity.h>
#include <sdk/entity/weapon.h>
#include <math/math.h>
#include <imgui/imgui.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <utils/fonts/font_manager.h>

static const char* clean_weapon_name(const char* class_name)
{
	if (!class_name)
		return "";

	std::string name = class_name;
	
	if (name.find("C_Weapon") == 0) {
		name = name.substr(8);
	}
	else if (name.find("C_") == 0) {
		name = name.substr(2);
	}

	static std::string result;
	result = name;
	return result.c_str();
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

void c_dropped_weapons::draw()
{
	bool bShowNames = GET_VAR(bool, VISUALS_PATH(m_dropped_items_names));
	bool bShowIcons = GET_VAR(bool, VISUALS_PATH(m_dropped_items_icons));

	if (!bShowNames && !bShowIcons)
		return;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;

	if (!g_ctx->m_local_pawn)
		return;

	if (!g_ctx->m_local_controller)
		return;

	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	int nMaxDistance = GET_VAR(int, VISUALS_PATH(m_dropped_items_distance));
	vec3_t vLocalPos = g_ctx->m_local_pawn->get_world_space_center();

	c_weapon_base* pLocalWeapon = g_ctx->m_active_weapon;

	for (auto& weapon_obj : g_entity_cache->m_weapon_entity) {
		c_base_entity* pWeapon = weapon_obj.m_pEntity;
		if (!pWeapon)
			continue;

		if (pWeapon == pLocalWeapon)
			continue;

		c_base_handle hOwner = pWeapon->m_hOwnerEntity();
		if (hOwner.is_valid())
			continue;

		c_game_scene_node* pGameSceneNode = pWeapon->m_pGameSceneNode();
		if (!pGameSceneNode)
			continue;

		vec3_t vWeaponPos = pGameSceneNode->m_vecAbsOrigin();
		float flDistance = vLocalPos.dist_to(vWeaponPos);

		if (flDistance > nMaxDistance)
			continue;

		vec2_t vScreen;
		if (!g_math->world_to_screen(vWeaponPos, vScreen))
			continue;

		float flAlpha = 1.0f;
		if (flDistance > 200.0f) {
			flAlpha = std::max(0.0f, 1.0f - (flDistance - 200.0f) / (nMaxDistance - 200.0f));
		}

		if (flAlpha < 0.01f)
			continue;

		const char* szWeaponClass = pWeapon->get_class_name();
		if (!szWeaponClass)
			continue;

		const char* szWeaponName = clean_weapon_name(szWeaponClass);

		ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();

		float flCurrentY = vScreen.y;

		if (bShowNames) {
			hellcolor nameColor = GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color));
			nameColor.Value.w *= flAlpha;
			hellcolor nameBgColor = GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color_bg));
			nameBgColor.Value.w *= flAlpha;

			int nFontType = GET_VAR(int, VISUALS_PATH(m_dropped_items_names_font_type));
			int nShadowType = GET_VAR(int, VISUALS_PATH(m_dropped_items_names_shadow_type));

			c_visuals::e_font_type font_type = static_cast<c_visuals::e_font_type>(nFontType);
			ImFont* pFont = g_visuals->get_font(font_type);
			if (!pFont)
				continue;

			float fontSize = pFont->LegacySize;
			ImVec2 textSize = pFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, szWeaponName);

			ImVec2 textPos = ImVec2(vScreen.x - textSize.x * 0.5f, flCurrentY);

			if (nShadowType == c_visuals::e_text_shadow_type::text_shadow_full) {
				for (int x = -1; x <= 1; x++) {
					for (int y = -1; y <= 1; y++) {
						if (x == 0 && y == 0)
							continue;
						pDrawList->AddText(pFont, fontSize, ImVec2(textPos.x + x, textPos.y + y), nameBgColor, szWeaponName);
					}
				}
			}
			else if (nShadowType == c_visuals::e_text_shadow_type::text_shadow_drop) {
				hellcolor shadowColor = nameBgColor;
				shadowColor.Value.x = 0.0f;
				shadowColor.Value.y = 0.0f;
				shadowColor.Value.z = 0.0f;
				pDrawList->AddText(pFont, fontSize, ImVec2(textPos.x + 1, textPos.y + 1), shadowColor, szWeaponName);
			}

			pDrawList->AddText(pFont, fontSize, textPos, nameColor, szWeaponName);
			
			flCurrentY = textPos.y - 2.0f;
		}

		if (bShowIcons) {
			icon_data_t icon_data;
			
			c_weapon_base* pWeaponBase = reinterpret_cast<c_weapon_base*>(pWeapon);
			if (pWeaponBase && pWeaponBase->weapon_data()) {
				const char* weapon_name = pWeaponBase->weapon_data()->m_szName();
				if (weapon_name)
					icon_data = get_weapon_icon_by_name(weapon_name);
			}
			
			if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0) {
				int iTargetSize = 12;
				const auto flWtoHRatio = static_cast<float>(icon_data.width) / static_cast<float>(icon_data.height);
				auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
				auto Height = iTargetSize;
				
				ImVec2 vIconPos = ImVec2(vScreen.x - Width * 0.5f, flCurrentY - Height);
				
				hellcolor iconColor = GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_icons_color));
				iconColor.Value.w *= flAlpha;
				pDrawList->AddImage(icon_data.texture_view, vIconPos, ImVec2(vIconPos.x + Width, vIconPos.y + Height), ImVec2(0, 0), ImVec2(1, 1), iconColor);
			}
		}
	}
}
