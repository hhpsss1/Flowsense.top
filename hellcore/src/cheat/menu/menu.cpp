#include "menu.h"
#include <context.h>
#include <core/interfaces/interfaces.h>

#include "tabs/config_tab.h"
#include "tabs/misc_tab.h"
#include "tabs/visuals_tab.h"
#include "tabs/aimbot_tab.h"

#include "animation/anim.h"

#include <sdk/interfaces/global_variables.h>
#include <utils/fonts/font_manager.h>

#include "hell_gui/hell_gui.h"
#include "hell_gui/colors.h"
#include "tabs/skins_tab.h"
#include <utils/fonts/compressed_fonts/font_awesome.h>
#include <shellapi.h>

#include <utils/stb_image.h>
#include <utils/fonts/compressed_fonts/test_avatar.h>

void c_menu::init(void) {
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(g_ctx->m_window);
	ImGui_ImplDX11_Init(g_interfaces->m_device, g_interfaces->m_device_context);

	g_font_manager->init();

	hell::blur::m_initialized = hell::blur::blur_effect.initialize(g_interfaces->m_device, g_interfaces->m_device_context);

	m_init = true;
}

icon_data_t c_menu::load_avatar_texture() {
	static icon_data_t avatar_texture;

	if (avatar_texture.texture_view)
		return avatar_texture;

	int width, height, channels;
	unsigned char* data = stbi_load_from_memory(test_avatar, sizeof(test_avatar), &width, &height, &channels, 4);
	if (!data)
		return {};

	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* texture_view = nullptr;

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
	sub_resource.pSysMem = data;
	sub_resource.SysMemPitch = desc.Width * 4;
	sub_resource.SysMemSlicePitch = 0;

	auto device = g_interfaces->m_device;
	device->CreateTexture2D(&desc, &sub_resource, &texture);

	if (texture) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = desc.MipLevels;
		srv_desc.Texture2D.MostDetailedMip = 0;

		device->CreateShaderResourceView(texture, &srv_desc, &texture_view);
		texture->Release();
	}

	stbi_image_free(data);

	if (texture_view) {
		avatar_texture = icon_data_t(texture_view, width, height);
	}

	return avatar_texture;
}

void c_menu::render(void) {
	if (!g_menu->m_menu_open)
		return;

	hell::window::m_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
	ImGui::SetNextWindowContentSize({ 800, 510 });
	hell::begin_window(1, { 800, 510 }, { 25, 25, 25, 220 }, 7.f, true);
	{
		ImGui::PushFont(g_font_manager->m_gheist_medium_14);

		hellvec2 window_pos = ImGui::GetWindowPos();
		hellvec2 window_size = ImGui::GetWindowSize();

		hellvec2 logo_size = hellvec2(38, 38);
		ImRect logo_rect = { window_pos, window_pos + logo_size };
		float tab_size_y_pad = logo_size.y + ImGui::GetStyle().WindowPadding.y;

		hell::window::m_window_drawlist->AddRectFilled(logo_rect.Min, logo_rect.Max, hellcolor(25, 25, 25, 150), 7.f, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight);

		constexpr const char* logo_text = xx("A");
		hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));

		hellvec2 logo_text_size = g_font_manager->m_inter_bold_large->CalcTextSizeA(g_font_manager->m_inter_bold_large->LegacySize, FLT_MAX, 0.f, logo_text);
		hellvec2 logo_pos = logo_rect.GetCenter() - logo_text_size * 0.5f;
		hell::label_shadow(logo_pos, g_font_manager->m_inter_bold_large, logo_text, accent_color);

		ImRect tab_rect = { hellvec2(logo_rect.Max.x + 6.f, logo_rect.Min.y), hellvec2(window_pos.x + hell::window::m_window_size.x, logo_rect.Max.y) };
		hell::window::m_window_drawlist->AddRectFilled(tab_rect.Min, tab_rect.Max, hellcolor(25, 25, 25, 150), 7.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopRight);

		auto avatar_texture = load_avatar_texture();
		if (avatar_texture.texture_view) {
			constexpr float avatar_size = 26.f;
			constexpr float user_info_padding = 12.f;

			hellvec2 avatar_pos = { tab_rect.Max.x - user_info_padding - avatar_size, tab_rect.GetCenter().y - avatar_size * 0.5f };

			hell::window::m_window_drawlist->AddImageRounded((ImTextureID)avatar_texture.texture_view, avatar_pos, { avatar_pos.x + avatar_size, avatar_pos.y + avatar_size }, { 0, 0 }, { 1, 1 }, IM_COL32_WHITE, avatar_size * 0.25f);

			const char* username = "user";
			const char* until_label = "Until: ";
			const char* until_date = "23.01.2026";

			hellvec2 username_size = g_font_manager->m_gheist_medium_12->CalcTextSizeA(g_font_manager->m_gheist_medium_12->LegacySize, FLT_MAX, 0.f, username);
			hellvec2 until_label_size = g_font_manager->m_gheist_medium_12->CalcTextSizeA(g_font_manager->m_gheist_medium_12->LegacySize, FLT_MAX, 0.f, until_label);
			hellvec2 until_date_size = g_font_manager->m_gheist_medium_12->CalcTextSizeA(g_font_manager->m_gheist_medium_12->LegacySize, FLT_MAX, 0.f, until_date);
			float until_total_width = until_label_size.x + until_date_size.x;
			
			float text_right_edge = avatar_pos.x - 8.f;

			hellvec2 username_pos = { text_right_edge - username_size.x, avatar_pos.y };
			float until_y = avatar_pos.y + username_size.y + 1.f;
			hellvec2 until_label_pos = { text_right_edge - until_total_width, until_y };
			hellvec2 until_date_pos = { until_label_pos.x + until_label_size.x, until_y };

			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_12, g_font_manager->m_gheist_medium_12->LegacySize, username_pos, hellcolor(255, 255, 255), username);
			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_12, g_font_manager->m_gheist_medium_12->LegacySize, until_label_pos, hellcolor(140, 140, 140), until_label);
			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_12, g_font_manager->m_gheist_medium_12->LegacySize, until_date_pos, accent_color, until_date);
		}

		static std::array<const char*, 4> tab_names = {
			xx("RAGE"),
			xx("VISUALS"),
			xx("SKINS"),
			xx("MISC")
		};

		static std::array<const char*, 4> tab_icons = {
			ICON_FA_CROSSHAIRS,
			ICON_FA_EYE,
			ICON_FA_PALETTE,
			ICON_FA_SLIDERS
		};

		float tab_start_x = tab_rect.Min.x + ImGui::GetStyle().WindowPadding.x;
		float tab_y = tab_rect.GetCenter().y;
		float tab_spacing = 8.f;
		float tab_padding_x = 12.f;
		float tab_padding_y = 6.f;

		hellcolor inactive_color = hellcolor(180, 180, 180, 255);
		hellcolor active_color = hellcolor(255, 255, 255, 255);
		hellcolor bg_color = hellcolor(45, 45, 45, 120);

		float current_x = tab_start_x;

		for (int i = 0; i < 4; i++) {
			bool is_selected = (int)m_current_tab == i;

			ImGui::PushFont(g_font_manager->m_fa_large);
			hellvec2 icon_size = ImGui::CalcTextSize(tab_icons[i]);
			ImGui::PopFont();

			hellvec2 text_size = g_font_manager->m_gheist_medium_14->CalcTextSizeA(g_font_manager->m_gheist_medium_14->LegacySize, FLT_MAX, 0.f, tab_names[i]);

			float total_width = icon_size.x + 4.f + text_size.x + tab_padding_x * 2.f;
			float total_height = ImMax(icon_size.y, text_size.y) + tab_padding_y * 2.f;

			hellvec2 tab_min = hellvec2(current_x, tab_y - total_height * 0.5f);
			hellvec2 tab_max = hellvec2(current_x + total_width, tab_y + total_height * 0.5f);

			ImGui::SetCursorScreenPos(tab_min);
			ImGui::InvisibleButton(tab_names[i], hellvec2(total_width, total_height));

			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			if (clicked) {
				m_current_tab = (e_menu_tabs)i;
			}

			if (is_selected) {
				hell::window::m_window_drawlist->AddRectFilled(tab_min, tab_max, bg_color, 4.f);
			}

			hellcolor icon_color = is_selected ? accent_color : (hovered ? hellcolor(200, 200, 200, 255) : inactive_color);
			hellcolor text_color = is_selected ? active_color : (hovered ? hellcolor(220, 220, 220, 255) : inactive_color);

			hellvec2 icon_pos = hellvec2(tab_min.x + tab_padding_x, tab_y - icon_size.y * 0.5f);
			ImGui::PushFont(g_font_manager->m_fa_large);
			hell::window::m_window_drawlist->AddText(icon_pos, icon_color, tab_icons[i]);
			ImGui::PopFont();

			hellvec2 text_pos = hellvec2(icon_pos.x + icon_size.x + 4.f, tab_y - text_size.y * 0.5f);
			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_14, g_font_manager->m_gheist_medium_14->LegacySize, text_pos, text_color, tab_names[i]);

			current_x += total_width + tab_spacing;
		}

		float footer_height = 20.f;
		hell::window::m_footer_height = footer_height + ImGui::GetStyle().WindowPadding.y + 4.f;

		ImGui::SetCursorPosY(tab_size_y_pad);

		switch (m_current_tab) {
		case e_menu_tabs::tab_aimbot:
			tabs::render_aimbot();
			break;
		case e_menu_tabs::tab_visuals:
			tabs::render_visuals();
			break;
		case e_menu_tabs::tab_skins:
			tabs::render_skins();
			break;
		case e_menu_tabs::tab_misc:
			tabs::render_misc();
			break;
		case e_menu_tabs::tab_config:
			tabs::render_config();
			break;
		default:
			break;
		}

		ImRect footer_rect = { hellvec2(window_pos.x, window_pos.y + window_size.y - footer_height), hellvec2(window_pos.x + window_size.x, window_pos.y + window_size.y) };

		hell::window::m_window_drawlist->AddRectFilled(footer_rect.Min, footer_rect.Max, hellcolor(25, 25, 25, 150), 7.f, ImDrawFlags_RoundCornersBottom);

		constexpr const char* version_text = xx("AirFlow alpha v1.0.0 // distributed by MIRA.KIM");
		hellvec2 version_text_size = g_font_manager->m_gheist_medium_14->CalcTextSizeA(g_font_manager->m_gheist_medium_14->LegacySize, FLT_MAX, 0.f, version_text);
		hellvec2 version_pos = hellvec2(footer_rect.Min.x + 12.f, footer_rect.GetCenter().y - version_text_size.y * 0.5f);
		hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_14, g_font_manager->m_gheist_medium_14->LegacySize, version_pos, hellcolor(140, 140, 140, 255), version_text);

		float icon_btn_padding = 8.f;
		hellcolor footer_icon_color = hellcolor(140, 140, 140, 255);
		hellcolor footer_icon_hover_color = hellcolor(255, 255, 255, 255);

		ImGui::PushFont(g_font_manager->m_fa_large);
		hellvec2 folder_icon_size = ImGui::CalcTextSize(ICON_FA_FOLDER_OPEN);
		hellvec2 folder_btn_pos = hellvec2(footer_rect.Max.x - 12.f - folder_icon_size.x, footer_rect.GetCenter().y - folder_icon_size.y * 0.5f);

		ImGui::SetCursorScreenPos(folder_btn_pos);
		ImGui::InvisibleButton(xx("##open_folder_btn"), folder_icon_size);
		bool folder_hovered = ImGui::IsItemHovered();
		bool folder_clicked = ImGui::IsItemClicked();

		if (folder_clicked) {
			ShellExecuteW(NULL, L"open", L"C:\\hellcore-alpha\\configs", NULL, NULL, SW_SHOWNORMAL);
		}

		hell::window::m_window_drawlist->AddText(folder_btn_pos, folder_hovered ? footer_icon_hover_color : footer_icon_color, ICON_FA_FOLDER_OPEN);
		ImGui::PopFont();

		ImGui::PushFont(g_font_manager->m_fa_large);
		hellvec2 gear_icon_size = ImGui::CalcTextSize(ICON_FA_GEAR);
		hellvec2 gear_btn_pos = hellvec2(folder_btn_pos.x - icon_btn_padding - gear_icon_size.x, footer_rect.GetCenter().y - gear_icon_size.y * 0.5f);

		ImGui::SetCursorScreenPos(gear_btn_pos);
		ImGui::InvisibleButton(xx("##config_tab_btn"), gear_icon_size);
		bool gear_hovered = ImGui::IsItemHovered();
		bool gear_clicked = ImGui::IsItemClicked();

		if (gear_clicked) {
			m_current_tab = e_menu_tabs::tab_config;
		}

		bool is_config_tab_active = m_current_tab == e_menu_tabs::tab_config;
		hellcolor gear_color = is_config_tab_active ? accent_color : (gear_hovered ? footer_icon_hover_color : footer_icon_color);
		hell::window::m_window_drawlist->AddText(gear_btn_pos, gear_color, ICON_FA_GEAR);
		ImGui::PopFont();

		ImGui::PopFont();
	}
	hell::end_window();
}