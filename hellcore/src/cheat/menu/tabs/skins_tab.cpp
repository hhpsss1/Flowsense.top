#include "skins_tab.h"

#include <includes.h>
#include <cheat/config/config_system.h>
#include <cheat/config/vars.h>
#include <cheat/input.h>
#include <unordered_map>

#include <cheat/menu/hell_gui/hell_gui.h>
#include <cheat/features/skins/skins.h>
#include <cheat/features/skins/custom_models.h>
#include <utils/fonts/font_manager.h>
#include <cheat/menu/hell_gui/colors.h>
#include <utils/vtex_parser.h>

static bool prepare_skin_selection(int& selected, const std::vector<c_dumped_item>& items,
    size_t model_count, std::vector<c_vars::skins_t::skin_settings_t>& settings) {
    const size_t count = std::min(items.size(), model_count);
    if (!count) {
        selected = 0;
        return false;
    }
    if (selected < 0 || static_cast<size_t>(selected) >= count)
        selected = 0;
    if (settings.size() < count)
        settings.resize(count);
    for (size_t i = 0; i < count; ++i)
        settings[i].m_item_definition_index = items[i].def_index;
    return true;
}

constexpr int column_count = 4;

int get_skin_index_by_id( const std::vector<c_dumped_skin>& skins, int id ) {
	for ( int i = 0; i < skins.size( ); ++i )
		if ( skins[ i ].id == id )
			return i;
	return 0;
}

hellcolor get_rarity_color(uint32_t rarity) {
	switch (rarity) {
		case 0: return hellcolor(255, 255, 255, 255);
		case 1: return hellcolor(176, 195, 217, 255);
		case 2: return hellcolor(94, 152, 217, 255);
		case 3: return hellcolor(75, 105, 255, 255);
		case 4: return hellcolor(136, 71, 255, 255);
		case 5: return hellcolor(211, 44, 230, 255);
		case 6: return hellcolor(235, 75, 75, 255);
		case 7: return hellcolor(228, 174, 57, 255);
		default: return hellcolor(255, 255, 255, 255);
	}
}

uint32_t get_paint_kit_rarity(int paint_kit_id) {
	auto econ_item_schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
	if (!econ_item_schema)
		return 1;

	auto paint_kit = econ_item_schema->get_paint_kits().find_by_key(paint_kit_id);
	if (paint_kit.has_value()) {
		return paint_kit.value()->paint_kit_rarity();
	}

	return 1;
}

uint32_t get_agent_rarity(int agent_id) {
	auto econ_item_schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
	if (!econ_item_schema)
		return 1;
	
	auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(agent_id);
	if (item_def.has_value())
		return item_def.value()->m_iterarity;
	
	return 1;
}

bool skin_selectable(const char* label, bool selected, hellcolor text_color) {
	ImGuiWindow* im_window = ImGui::GetCurrentWindow();
	if (im_window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& im_style = g.Style;

	hellvec2 im_label_size = ImGui::CalcTextSize(label, NULL, true);
	hellvec2 im_avail_region = ImGui::GetContentRegionAvail();

	hellvec2 im_item_size(
		im_avail_region.x,
		im_label_size.y
	);

	hellvec2 im_pos = im_window->DC.CursorPos;
	ImGui::ItemSize(im_item_size, 0.0f);

	ImRect im_bb(im_pos, im_pos + im_item_size);
	if (!ImGui::ItemAdd(im_bb, im_window->GetID(label)))
		return false;

	bool b_hovered, b_held;
	bool b_pressed = ImGui::ButtonBehavior(im_bb, im_window->GetID(label), &b_hovered, &b_held);

	ImDrawList* im_draw_list = im_window->DrawList;
	hellvec2 im_text_pos = { im_bb.Min.x, im_bb.GetCenter().y - (im_label_size.y * 0.5f) };

	std::string str_new_label(label);
	size_t pos = str_new_label.find(xx("##"));

	if (pos != std::string::npos) {
		str_new_label = str_new_label.substr(0, pos);
	}

	ImGuiID id = im_window->GetID(label);

	float* select_progress = ImGui::GetStateStorage()->GetFloatRef(id, 0.0f);
	float* hover_progress = ImGui::GetStateStorage()->GetFloatRef(id + 1, 0.0f);
	float* color_transition = ImGui::GetStateStorage()->GetFloatRef(id + 2, 0.0f);

	*select_progress = ImSaturate(*select_progress + g.IO.DeltaTime * (selected ? 15.0f : -15.0f));
	*hover_progress = ImSaturate(*hover_progress + g.IO.DeltaTime * (b_hovered && !selected ? 15.0f : -15.0f));

	hellcolor normal_color = text_color;
	hellcolor hover_color = text_color;

	hellcolor target_color = normal_color;
	if (selected)
		target_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
	else if (b_hovered)
		target_color = hover_color;

	*color_transition = ImLerp(*color_transition, target_color.Value.w, g.IO.DeltaTime * 10.0f);
	target_color.Value.w = *color_transition;

	float x_offset = ImLerp(0.0f, 3.3f, *select_progress) + ImLerp(0.0f, 1.5f, *hover_progress);
	im_text_pos.x += x_offset;

	im_draw_list->AddText(im_text_pos, target_color, str_new_label.c_str());

	return b_pressed;
}

enum e_skin_type : int {
	skin_knife = 0,
	skin_glove,
	skin_weapon,
	skin_agent,
	skin_custom,
	skin_max
};

const char* s_skin_type_arr[ skin_max ] = {
	xx( "Knife" ),
	xx( "Glove" ),
	xx( "Weapon" ),
	xx( "Agent" ),
	xx( "Custom" ),
};

static e_skin_type s_skin_type = skin_knife;

void tabs::render_skins( ) {
	if ( !g_ctx->m_dumped_skins ) {
		hellvec2 avail = ImGui::GetContentRegionAvail( );
		const char* text = xx( "Dumping skins.." );
		hellvec2 text_size = ImGui::CalcTextSize( text );
		hellvec2 center = avail * 0.5f;
		hellvec2 text_pos = center - text_size * 0.5f;
		ImGui::SetCursorPos( text_pos );
		hell::label( text );
		return;
	}
	hell::set_cursor_pos_relative_y( );

	bool enabled = GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger));
	
	hellvec2 child_size = hell::calculate_column_size( 3 );
	float max_height = hell::get_available_height() - 40.f;

	float spacing = 18.f;
	float start_y = ImGui::GetCursorPosY( );
	float col_spacing = ImGui::GetStyle( ).WindowPadding.x;

	float col1_x = ImGui::GetCursorPosX( );
	float col2_x = col1_x + child_size.x + col_spacing;
	float col3_x = col2_x + child_size.x + col_spacing;

	float skin_changer_h = hell::calculate_child_height( [ & ] {
		hell::checkbox( xx( "Enabled" ), SKINS_PATH( m_enabled_skinchanger ) );
		} );

	ImGui::SetCursorPos( { col1_x, start_y } );
	hell::child( xx( "Skin changer" ), { child_size.x, skin_changer_h }, [ & ] {
		hell::checkbox( xx( "Enabled" ), SKINS_PATH( m_enabled_skinchanger ) );
		}
	);

	if (!enabled)
		return;

	float skin_type_h = hell::calculate_child_height( [ & ] {
		for (int i = 0; i < skin_max; ++i) {
			hell::selectable(s_skin_type_arr[i], s_skin_type == i);
		}
		} );

	float dummy_wear = 0.01f;
	int dummy_seed = 0;
	float settings_h = 0.f;
	float icon_h = 140.f;

	if (s_skin_type != skin_agent && s_skin_type != skin_glove && s_skin_type != skin_custom) {
		settings_h = hell::calculate_child_height( [ & ] {
			hell::slider_float_passthrough(xx("Wear"), &dummy_wear, 0.01f, 1.f);
			hell::slider_int_passthrough(xx("Pattern"), &dummy_seed, 0, 1000);
		} );
	}

	ImGui::SetCursorPos( { col1_x, start_y + skin_changer_h + col_spacing } );
	hell::child( xx( "Skin type" ), { child_size.x, std::min(skin_type_h, max_height / 2.f) }, [ & ] {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
		hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
		for (int i = 0; i < skin_max; ++i) {
			if (hell::selectable(s_skin_type_arr[i], s_skin_type == i)) {
				s_skin_type = (e_skin_type)i;
			}
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		}
	);

	ImGui::SetCursorPos( { col1_x, start_y + skin_changer_h + std::min(skin_type_h, max_height / 2.f) + col_spacing * 2 } );
	if (s_skin_type != skin_agent && s_skin_type != skin_glove && s_skin_type != skin_custom) {
		hell::child( xx( "Settings" ), { child_size.x, settings_h }, [ & ] {
			switch (s_skin_type) {
			case e_skin_type::skin_knife: {
				int& selected_knife = GET_VAR(int, SKINS_PATH(m_knife_selected));
				auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
			if (!prepare_skin_selection(selected_knife, g_skins->knives_items, g_skins->knives.names.size(), settings)) break;
				auto& setting = settings[selected_knife];
				hell::slider_float_passthrough(xx("Wear"), &setting.m_wear, 0.01f, 1.f, 0, false, "%.2f");
				hell::slider_int_passthrough(xx("Pattern"), &setting.m_seed, 0, 1000);
				break;
			}
			case e_skin_type::skin_weapon: {
				int& selected_weapon = GET_VAR(int, SKINS_PATH(m_selected_weapon));
				auto& items = g_skins->items;
				if (selected_weapon >= items.size()) break;
				auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
				if (selected_weapon >= settings.size()) break;
				auto& setting = settings[selected_weapon];
				hell::slider_float_passthrough(xx("Wear"), &setting.m_wear, 0.01f, 1.f, 0, false, "%.2f");
				hell::slider_int_passthrough(xx("Pattern"), &setting.m_seed, 0, 1000);
				break;
			}
			}
		});
	}

	ImGui::SetCursorPos( { col1_x, start_y + skin_changer_h + std::min(skin_type_h, max_height / 2.f) + settings_h + col_spacing * 3 } );
	hell::child( xx( "Icon" ), { child_size.x, icon_h }, [ & ] {
		static std::unordered_map<std::string, ID3D11ShaderResourceView*> texture_cache;
		
		ID3D11ShaderResourceView* skin_texture = nullptr;
		std::string texture_path;
		
		auto econ_item_schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
		if (!econ_item_schema) return;
		
		switch (s_skin_type) {
		case e_skin_type::skin_knife: {
			int& selected_knife = GET_VAR(int, SKINS_PATH(m_knife_selected));
			auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
			if (!prepare_skin_selection(selected_knife, g_skins->knives_items, g_skins->knives.names.size(), settings)) break;
			auto& knife_skins = g_skins->knives_items[selected_knife].skins;
			auto& setting = settings[selected_knife];
			
			if (!knife_skins.empty()) {
				int current_index = get_skin_index_by_id(knife_skins, setting.m_paint_kit);
				if (current_index >= 0 && current_index < knife_skins.size()) {
					auto& selected_skin = knife_skins[current_index];
					auto paint_kit = econ_item_schema->get_paint_kits().find_by_key(selected_skin.id);
					if (paint_kit.has_value()) {
						auto& item = g_skins->knives_items[selected_knife];
						auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(item.def_index);
						if (item_def.has_value()) {
							texture_path = "panorama/images/econ/default_generated/" + 
								std::string(item_def.value()->get_simple_weapon_name()) + "_" + 
								paint_kit.value()->paint_kit_name() + "_light_png.vtex_c";
						}
					}
				}
			}
			break;
		}
		case e_skin_type::skin_weapon: {
			int& selected_weapon = GET_VAR(int, SKINS_PATH(m_selected_weapon));
			auto& items = g_skins->items;
			if (selected_weapon >= items.size()) break;
			auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
			if (selected_weapon >= settings.size()) break;
			auto& item = items[selected_weapon];
			auto& setting = settings[selected_weapon];
			auto& skins = item.skins;
			
			if (!skins.empty()) {
				int current_index = get_skin_index_by_id(skins, setting.m_paint_kit);
				if (current_index >= 0 && current_index < skins.size()) {
					auto& selected_skin = skins[current_index];
					auto paint_kit = econ_item_schema->get_paint_kits().find_by_key(selected_skin.id);
					if (paint_kit.has_value()) {
						auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(item.def_index);
						if (item_def.has_value()) {
							texture_path = "panorama/images/econ/default_generated/" + 
								std::string(item_def.value()->get_simple_weapon_name()) + "_" + 
								paint_kit.value()->paint_kit_name() + "_light_png.vtex_c";
						}
					}
				}
			}
			break;
		}
		case e_skin_type::skin_glove: {
			int& selected_glove = GET_VAR(int, SKINS_PATH(m_glove_selected));
			auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
			if (!prepare_skin_selection(selected_glove, g_skins->glove_items, g_skins->gloves.names.size(), settings)) break;
			auto& glove_skins = g_skins->glove_items[selected_glove].skins;
			auto& setting = settings[selected_glove];
			
			if (!glove_skins.empty()) {
				int current_index = get_skin_index_by_id(glove_skins, setting.m_paint_kit);
				if (current_index >= 0 && current_index < glove_skins.size()) {
					auto& selected_skin = glove_skins[current_index];
					auto paint_kit = econ_item_schema->get_paint_kits().find_by_key(selected_skin.id);
					if (paint_kit.has_value()) {
						auto& item = g_skins->glove_items[selected_glove];
						auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(item.def_index);
						if (item_def.has_value()) {
							texture_path = "panorama/images/econ/default_generated/" + 
								std::string(item_def.value()->get_simple_weapon_name()) + "_" + 
								paint_kit.value()->paint_kit_name() + "_light_png.vtex_c";
						}
					}
				}
			}
			break;
		}
		case e_skin_type::skin_agent: {
			int& selected_ct = GET_VAR(int, SKINS_PATH(m_agent_selected_ct));
			int& selected_t = GET_VAR(int, SKINS_PATH(m_agent_selected_t));
			static int selected_team = 0;
			
			auto& agents = g_skins->agents;
			int selected_agent = (selected_team == 0) ? selected_ct : selected_t;
			
			if (selected_agent >= 0 && selected_agent < agents.agent_ids.size()) {
				int agent_id = agents.agent_ids[selected_agent];
				auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(agent_id);
				if (item_def.has_value()) {
					std::string agent_name = item_def.value()->get_simple_weapon_name();
					
					std::vector<std::string> possible_paths = {
						"panorama/images/econ/default_generated/" + agent_name + "_png.vtex_c",
						"panorama/images/econ/characters/" + agent_name + ".vtex_c",
						"panorama/images/econ/characters/" + agent_name + "_png.vtex_c",
						"panorama/images/econ/default_generated/" + agent_name + ".vtex_c"
					};
					
					for (const auto& path : possible_paths) {
						if (g_interfaces->m_file_system->exists(path.c_str(), "GAME")) {
							texture_path = path;
							break;
						}
					}
				}
			}
			break;
		}
		}
		
		if (!texture_path.empty()) {
			auto it = texture_cache.find(texture_path);
			if (it != texture_cache.end()) {
				skin_texture = it->second;
			} else {
				if (g_interfaces->m_file_system->exists(texture_path.c_str(), "GAME")) {
					auto vtex = vtex_parser::load(texture_path, g_interfaces->m_file_system);
					if (!vtex.data.empty()) {
						ID3D11Texture2D* texture = nullptr;
						D3D11_TEXTURE2D_DESC desc = {};
						desc.Width = vtex.w;
						desc.Height = vtex.h;
						desc.MipLevels = 1;
						desc.ArraySize = 1;
						desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						desc.SampleDesc.Count = 1;
						desc.Usage = D3D11_USAGE_DEFAULT;
						desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
						
						D3D11_SUBRESOURCE_DATA sub_resource = {};
						sub_resource.pSysMem = vtex.data.data();
						sub_resource.SysMemPitch = desc.Width * 4;
						sub_resource.SysMemSlicePitch = 0;
						
						if (SUCCEEDED(g_interfaces->m_device->CreateTexture2D(&desc, &sub_resource, &texture))) {
							D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
							srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
							srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
							srv_desc.Texture2D.MipLevels = desc.MipLevels;
							srv_desc.Texture2D.MostDetailedMip = 0;
							
							if (SUCCEEDED(g_interfaces->m_device->CreateShaderResourceView(texture, &srv_desc, &skin_texture))) {
								texture_cache[texture_path] = skin_texture;
							}
							texture->Release();
						}
					}
				}
			}
		}
		
		if (skin_texture) {
			hellvec2 avail = ImGui::GetContentRegionAvail();
			
			D3D11_TEXTURE2D_DESC desc;
			ID3D11Texture2D* texture;
			((ID3D11ShaderResourceView*)skin_texture)->GetResource((ID3D11Resource**)&texture);
			texture->GetDesc(&desc);
			texture->Release();
			
			float tex_width = (float)desc.Width;
			float tex_height = (float)desc.Height;
			
			float max_size = std::min(avail.x, avail.y) * 1.5f;
			float scale = max_size / std::max(tex_width, tex_height);
			
			hellvec2 image_size = { tex_width * scale, tex_height * scale };
			
			hellvec2 center = avail * 0.5f;
			hellvec2 image_pos = center - image_size * 0.5f;
			image_pos.y += 10.0f;
			
			ImGui::SetCursorPos(image_pos);
			ImGui::Image((void*)skin_texture, image_size);
		}
	});

	switch ( s_skin_type ) {
	case e_skin_type::skin_knife: {
		int& selected_knife = GET_VAR( int, SKINS_PATH( m_knife_selected ) );
		auto& settings = GET_VAR_VEC( c_vars::skins_t::skin_settings_t, SKINS_PATH( m_knives_settings ) );
			if (!prepare_skin_selection(selected_knife, g_skins->knives_items, g_skins->knives.names.size(), settings)) break;
		auto& knife_skins = g_skins->knives_items[ selected_knife ].skins;
		auto& setting = settings[ selected_knife ];

		const int displayed_selection = selected_knife;
		float models_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < g_skins->knives.names.size(); ++i) {
				hell::selectable(g_skins->knives.names[i], selected_knife == i);
			}
			} );

		float skins_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < knife_skins.size(); ++i) {
				hell::selectable(knife_skins[i].name.c_str(), false);
			}
			} );

		ImGui::SetCursorPos( { col2_x, start_y } );
		hell::child( xx( "Models" ), { child_size.x, std::min(models_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			for (int i = 0; i < g_skins->knives.names.size(); ++i) {
				if (hell::selectable(g_skins->knives.names[i], selected_knife == i)) {
					selected_knife = i;
				}
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);

		if (selected_knife != displayed_selection) break;
		ImGui::SetCursorPos( { col3_x, start_y } );
		hell::child( xx( "Skins" ), { child_size.x, std::min(skins_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			int current_index = get_skin_index_by_id(knife_skins, setting.m_paint_kit);
			std::unordered_map<std::string, int> name_counts;
			for (int i = 0; i < knife_skins.size(); ++i) {
				bool is_selected = (i == current_index);
				hellcolor rarity_color = get_rarity_color(knife_skins[i].rarity);
				
				std::string display_name = knife_skins[i].name;
				name_counts[knife_skins[i].name]++;
				if (name_counts[knife_skins[i].name] > 1) {
					display_name += " " + std::to_string(name_counts[knife_skins[i].name]);
				}
				
				ImGui::PushID(i);
				if (skin_selectable(display_name.c_str(), is_selected, rarity_color)) {
					setting.m_paint_kit = knife_skins[i].id;
				}
				ImGui::PopID();
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);
		break;
	}
	case e_skin_type::skin_glove: {
		int& selected_glove = GET_VAR( int, SKINS_PATH( m_glove_selected ) );
		auto& settings = GET_VAR_VEC( c_vars::skins_t::skin_settings_t, SKINS_PATH( m_glove_settings ) );
			if (!prepare_skin_selection(selected_glove, g_skins->glove_items, g_skins->gloves.names.size(), settings)) break;
		auto& glove_skins = g_skins->glove_items[ selected_glove ].skins;
		auto& setting = settings[ selected_glove ];

		const int displayed_selection = selected_glove;
		float models_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < g_skins->gloves.names.size(); ++i) {
				hell::selectable(g_skins->gloves.names[i], selected_glove == i);
			}
			} );

		float skins_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < glove_skins.size(); ++i) {
				hell::selectable(glove_skins[i].name.c_str(), false);
			}
			} );

		ImGui::SetCursorPos( { col2_x, start_y } );
		hell::child( xx( "Models" ), { child_size.x, std::min(models_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			for (int i = 0; i < g_skins->gloves.names.size(); ++i) {
				if (hell::selectable(g_skins->gloves.names[i], selected_glove == i)) {
					selected_glove = i;
				}
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);

		if (selected_glove != displayed_selection) break;
		ImGui::SetCursorPos( { col3_x, start_y } );
		hell::child( xx( "Skins" ), { child_size.x, std::min(skins_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			int current_index = get_skin_index_by_id(glove_skins, setting.m_paint_kit);
			std::unordered_map<std::string, int> name_counts;
			for (int i = 0; i < glove_skins.size(); ++i) {
				bool is_selected = (i == current_index);
				hellcolor rarity_color = get_rarity_color(glove_skins[i].rarity);
				
				std::string display_name = glove_skins[i].name;
				name_counts[glove_skins[i].name]++;
				if (name_counts[glove_skins[i].name] > 1) {
					display_name += " " + std::to_string(name_counts[glove_skins[i].name]);
				}
				
				ImGui::PushID(i);
				if (skin_selectable(display_name.c_str(), is_selected, rarity_color)) {
					setting.m_paint_kit = glove_skins[i].id;
				}
				ImGui::PopID();
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);
		break;
	}
	case e_skin_type::skin_weapon: {
		auto& items = g_skins->items;
		int& selected_weapon = GET_VAR( int, SKINS_PATH( m_selected_weapon ) );
		if ( selected_weapon >= items.size( ) ) return;
		auto& settings = GET_VAR_VEC( c_vars::skins_t::skin_settings_t, SKINS_PATH( m_skin_settings ) );
		if ( selected_weapon >= settings.size( ) ) return;
		auto& item = items[ selected_weapon ];
		auto& setting = settings[ selected_weapon ];
		auto& skins = item.skins;

		float models_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < items.size(); ++i) {
				hell::selectable(items[i].name.c_str(), selected_weapon == i);
			}
			} );

		float skins_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < skins.size(); ++i) {
				hell::selectable(skins[i].name.c_str(), false);
			}
			} );

		ImGui::SetCursorPos( { col2_x, start_y } );
		hell::child( xx( "Models" ), { child_size.x, std::min(models_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			for (int i = 0; i < items.size(); ++i) {
				if (hell::selectable(items[i].name.c_str(), selected_weapon == i)) {
					selected_weapon = i;
				}
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);

		ImGui::SetCursorPos( { col3_x, start_y } );
		hell::child( xx( "Skins" ), { child_size.x, std::min(skins_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			if (!skins.empty()) {
				int current_index = get_skin_index_by_id(skins, setting.m_paint_kit);
				std::unordered_map<std::string, int> name_counts;
				for (int i = 0; i < skins.size(); ++i) {
					bool is_selected = (i == current_index);
					hellcolor rarity_color = get_rarity_color(skins[i].rarity);
					
					std::string display_name = skins[i].name;
					name_counts[skins[i].name]++;
					if (name_counts[skins[i].name] > 1) {
						display_name += " " + std::to_string(name_counts[skins[i].name]);
					}
					
					ImGui::PushID(i);
					if (skin_selectable(display_name.c_str(), is_selected, rarity_color)) {
						setting.m_paint_kit = skins[i].id;
					}
					ImGui::PopID();
				}
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);
		break;
	}
	case e_skin_type::skin_agent: {
		int& selected_ct = GET_VAR( int, SKINS_PATH( m_agent_selected_ct ) );
        int& selected_t = GET_VAR( int, SKINS_PATH( m_agent_selected_t ) );
		static int selected_team = 0;

		float models_h = hell::calculate_child_height( [ & ] {
			hell::selectable("CT Agents", selected_team == 0);
			hell::selectable("T Agents", selected_team == 1);
			} );

		float skins_h = hell::calculate_child_height( [ & ] {
			for (int i = 0; i < g_skins->agents.agent_names.size(); ++i) {
				hell::selectable(g_skins->agents.agent_names[i], false);
			}
			} );

		ImGui::SetCursorPos( { col2_x, start_y } );
		hell::child( xx( "Models" ), { child_size.x, std::min(models_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			if (hell::selectable("CT Agents", selected_team == 0)) {
				selected_team = 0;
			}
			if (hell::selectable("T Agents", selected_team == 1)) {
				selected_team = 1;
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);

		ImGui::SetCursorPos( { col3_x, start_y } );
		hell::child( xx( "Skins" ), { child_size.x, std::min(skins_h, max_height) }, [ & ] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			for (int i = 0; i < g_skins->agents.agent_names.size(); ++i) {
				bool is_selected = (selected_team == 0 && selected_ct == i) || (selected_team == 1 && selected_t == i);
				hellcolor rarity_color = get_rarity_color(get_agent_rarity(g_skins->agents.agent_ids[i]));
				
				ImGui::PushID(i);
				if (skin_selectable(g_skins->agents.agent_names[i], is_selected, rarity_color)) {
					if (selected_team == 0) {
						selected_ct = i;
					} else {
						selected_t = i;
					}
				}
				ImGui::PopID();
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);
		break;
	}
	case e_skin_type::skin_custom: {
		if (!g_custom_models->m_scanned)
			g_custom_models->scan();

		int& selected_custom = GET_VAR(int, SKINS_PATH(m_custom_model_selected));
		if (selected_custom < 0 || static_cast<size_t>(selected_custom) >= g_custom_models->m_models.size())
			selected_custom = 0;

		float models_h = hell::calculate_child_height([&] {
			for (size_t i = 0; i < g_custom_models->m_models.size(); ++i) {
				hell::selectable(g_custom_models->m_models[i].name.c_str(), false);
			}
			});

		float custom_h = hell::calculate_child_height([&] {
			hell::button(xx("Refresh models"), { -1.f, 0.f });
			} );

		ImGui::SetCursorPos({ col2_x, start_y });
		hell::child(xx("Models"), { child_size.x, std::min(models_h, max_height) }, [&] {
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
			hellcolor accent_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(accent_color.Value.x / 255.0f, accent_color.Value.y / 255.0f, accent_color.Value.z / 255.0f, 0.9f));
			for (int i = 0; i < (int)g_custom_models->m_models.size(); ++i) {
				if (hell::selectable(g_custom_models->m_models[i].name.c_str(), selected_custom == i)) {
					selected_custom = i;
				}
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			}
		);

		ImGui::SetCursorPos({ col3_x, start_y });
		hell::child(xx("Custom"), { child_size.x, custom_h }, [&] {
			if (hell::button(xx("Refresh models"), { -1.f, 0.f })) {
				g_custom_models->scan();
				selected_custom = 0;
			}
			const int found = (int)g_custom_models->m_models.size() > 0 ? (int)g_custom_models->m_models.size() - 1 : 0;
			ImGui::Text("Found: %d", found);
			if (selected_custom > 0 && selected_custom < (int)g_custom_models->m_models.size())
				ImGui::TextWrapped("%s", g_custom_models->m_models[selected_custom].path.c_str());
			else
				ImGui::Text("Model changer off");
			}
		);
		break;
	}
	}
}
