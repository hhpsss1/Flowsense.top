#include "reveal_equipment.h"
#include <algorithm>
#include <format>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/panorama.h>
#include <context.h>
#include <cheat/features/entity cache/entity_cache.h>

static std::unordered_map<int, weapon_type> weapon_type_map = {
	{1, WEAPONTYPE_PISTOL}, {2, WEAPONTYPE_PISTOL}, {3, WEAPONTYPE_PISTOL}, {4, WEAPONTYPE_PISTOL},
	{7, WEAPONTYPE_RIFLE}, {8, WEAPONTYPE_RIFLE}, {9, WEAPONTYPE_SNIPER_RIFLE}, {10, WEAPONTYPE_RIFLE},
	{11, WEAPONTYPE_SNIPER_RIFLE}, {13, WEAPONTYPE_RIFLE}, {14, WEAPONTYPE_MACHINEGUN}, {16, WEAPONTYPE_RIFLE},
	{17, WEAPONTYPE_SUBMACHINEGUN}, {19, WEAPONTYPE_SUBMACHINEGUN}, {20, WEAPONTYPE_EQUIPMENT},
	{23, WEAPONTYPE_SUBMACHINEGUN}, {24, WEAPONTYPE_SUBMACHINEGUN}, {25, WEAPONTYPE_SHOTGUN}, {26, WEAPONTYPE_SUBMACHINEGUN},
	{27, WEAPONTYPE_SHOTGUN}, {28, WEAPONTYPE_MACHINEGUN}, {29, WEAPONTYPE_SHOTGUN}, {30, WEAPONTYPE_PISTOL},
	{31, WEAPONTYPE_TASER}, {32, WEAPONTYPE_PISTOL}, {33, WEAPONTYPE_SUBMACHINEGUN}, {34, WEAPONTYPE_SUBMACHINEGUN},
	{35, WEAPONTYPE_SHOTGUN}, {36, WEAPONTYPE_PISTOL}, {37, WEAPONTYPE_EQUIPMENT}, {38, WEAPONTYPE_SNIPER_RIFLE},
	{39, WEAPONTYPE_RIFLE}, {40, WEAPONTYPE_SNIPER_RIFLE}, {41, WEAPONTYPE_KNIFE}, {42, WEAPONTYPE_KNIFE},
	{43, WEAPONTYPE_GRENADE}, {44, WEAPONTYPE_GRENADE}, {45, WEAPONTYPE_GRENADE}, {46, WEAPONTYPE_GRENADE},
	{47, WEAPONTYPE_GRENADE}, {48, WEAPONTYPE_GRENADE}, {49, WEAPONTYPE_C4}, {50, WEAPONTYPE_EQUIPMENT},
	{51, WEAPONTYPE_EQUIPMENT}, {52, WEAPONTYPE_EQUIPMENT}, {55, WEAPONTYPE_EQUIPMENT}, {56, WEAPONTYPE_EQUIPMENT},
	{57, WEAPONTYPE_STACKABLEITEM}, {59, WEAPONTYPE_KNIFE}, {60, WEAPONTYPE_RIFLE}, {61, WEAPONTYPE_PISTOL},
	{63, WEAPONTYPE_PISTOL}, {64, WEAPONTYPE_PISTOL}, {68, WEAPONTYPE_GRENADE}, {69, WEAPONTYPE_FISTS},
	{70, WEAPONTYPE_BREACHCHARGE}, {72, WEAPONTYPE_TABLET}, {74, WEAPONTYPE_MELEE}, {75, WEAPONTYPE_MELEE},
	{76, WEAPONTYPE_MELEE}, {78, WEAPONTYPE_MELEE}, {80, WEAPONTYPE_KNIFE}, {81, WEAPONTYPE_GRENADE},
	{82, WEAPONTYPE_GRENADE}, {83, WEAPONTYPE_GRENADE}, {84, WEAPONTYPE_GRENADE}, {85, WEAPONTYPE_BUMPMINE},
	{500, WEAPONTYPE_KNIFE}, {503, WEAPONTYPE_KNIFE}, {505, WEAPONTYPE_KNIFE}, {506, WEAPONTYPE_KNIFE},
	{507, WEAPONTYPE_KNIFE}, {508, WEAPONTYPE_KNIFE}, {509, WEAPONTYPE_KNIFE}, {512, WEAPONTYPE_KNIFE},
	{514, WEAPONTYPE_KNIFE}, {515, WEAPONTYPE_KNIFE}, {516, WEAPONTYPE_KNIFE}, {517, WEAPONTYPE_KNIFE},
	{518, WEAPONTYPE_KNIFE}, {519, WEAPONTYPE_KNIFE}, {520, WEAPONTYPE_KNIFE}, {521, WEAPONTYPE_KNIFE},
	{522, WEAPONTYPE_KNIFE}, {523, WEAPONTYPE_KNIFE}, {525, WEAPONTYPE_KNIFE}
};

scoreboard_weapon_manager::scoreboard_weapon_manager() {
}

scoreboard_weapon_manager::~scoreboard_weapon_manager() {
	shutdown();
}

void scoreboard_weapon_manager::initialize() {
	enabled_ = true;
	player_weapons_.clear();
	active_weapons_.clear();
	display_data_.clear();
	ui_engine_ = nullptr;
	panel_scoreboard_ = nullptr;
	last_update_frame_ = 0;
}

void scoreboard_weapon_manager::shutdown() {
	enabled_ = false;
	player_weapons_.clear();
	active_weapons_.clear();
	display_data_.clear();
	ui_engine_ = nullptr;
	panel_scoreboard_ = nullptr;
}

void scoreboard_weapon_manager::level_init() {
	if (!g_interfaces || !g_interfaces->m_panorama_ui_engine)
		return;

	c_ui_engine* ui_engine_ptr = g_interfaces->m_panorama_ui_engine->get_ui_engine();
	if (!ui_engine_ptr) return;

	this->ui_engine_ = ui_engine_ptr;

	c_ui_panel* wish_panel = nullptr;
	for (int i = 0; i < ui_engine_ptr->m_panel_count; i++) {
		panel_data_t* panel_data = &ui_engine_ptr->m_panels_array[i];
		if (!panel_data || !panel_data->m_panel) continue;
		c_ui_panel* panel = panel_data->m_panel;
		if (!panel->m_panel_name) continue;
		if (fnv1a::hash_32(panel->m_panel_name) == fnv1a::hash_32("Scoreboard")) {
			wish_panel = panel;
			break;
		}
	}

	if (!wish_panel) return;
	panel_scoreboard_ = wish_panel;

	std::string main_script = R"PANORAMA(
(function () {

    if (typeof SClient !== "undefined") {
        SClient = undefined;
    }

    SClient = (function () {

        var handlers = {};

        return {
            register_handler: function (type, callback) {
                handlers[type] = callback;
            },

            receive: function (msg) {
                if (msg && handlers[msg.type]) {
                    handlers[msg.type](msg);
                }
            }
        };

    })();

    SWeaponManager = (function () {

        function getScoreboard() {
            var root = $.GetContextPanel();
            return root.FindChildTraverse("Scoreboard") || (root.id === "Scoreboard" ? root : null);
        }

        function getRow(sb, xuid) {
            return sb.FindChildTraverse("player-" + xuid) || sb.FindChildTraverse("id-" + xuid);
        }

        return {

            update: function (xuid, weapons, active_path) {

                var sb = getScoreboard();
                if (!sb) return;

                var row = getRow(sb, xuid);
                if (!row) return;

                var nameIcons = row.FindChildTraverse("id-sb-name__nameicons");
                if (!nameIcons) return;

                var containerId = "custom-weapons-container-" + xuid;
                var container = nameIcons.FindChildTraverse(containerId);

                if (!weapons || weapons.length === 0) {
                    if (container) {
                        container.style.visibility = "collapse";
                    }
                    return;
                }

                if (!container) {
                    container = $.CreatePanel("Panel", nameIcons, containerId);
                    container.AddClass("custom-weapons-container");
                    container.style.flowChildren = "right";
                    container.style.height = "100%";
                    container.style.verticalAlign = "center";
                    container.style.paddingLeft = "2px";
                }

                container.style.visibility = "visible";
                container.RemoveAndDeleteChildren();

                var primary = [];
                var pistols = [];
                var grenades = [];
                var equipment = [];

                weapons.forEach(function (w) {
                    if (w.type === 2 || w.type === 3 || w.type === 4 || w.type === 5 || w.type === 6) {
                        primary.push(w);
                    } else if (w.type === 1) {
                        pistols.push(w);
                    } else if (w.type === 9 || w.type === 11 || w.type === 7) {
                        grenades.push(w);
                    } else if (w.type === 8 || w.type === 11) {
                        equipment.push(w);
                    } else {
                        equipment.push(w);
                    }
                });

                var sortedWeapons = primary.concat(pistols).concat(grenades).concat(equipment);

                sortedWeapons.forEach(function (w) {

                    var id = "wep_" + w.path.replace(/[^a-zA-Z0-9]/g, "_");
                    var slot = $.CreatePanel("Panel", container, id);
                    slot.style.height = "100%";
                    slot.style.width = "fit-children";
                    slot.style.verticalAlign = "center";
                    slot.style.margin = "0px 1px";

                    var img = $.CreatePanel("Image", slot, "img");
                    img.style.height = "18px";
                    img.style.verticalAlign = "center";
                    img.scaling = "stretch-aspect-preserve";

                    var finalPath = w.path;

                    if (finalPath.indexOf("file://") !== 0) {
                        if (finalPath.indexOf("icons/equipment") === -1) {
                            finalPath = "icons/equipment/" + finalPath;
                        }
                        if (finalPath.indexOf(".svg") === -1 && finalPath.indexOf(".vsvg") === -1) {
                            finalPath += ".svg";
                        }
                        finalPath = "file://{images}/" + finalPath;
                    }

                    img.SetImage(finalPath);

                    var weaponType = w.type;
                    var weaponPath = w.path;
                    
                    if (weaponType === 1) {
                        if (weaponPath.indexOf("usp_silencer") !== -1) {
                            img.style.width = "45px";
                            slot.style.width = "45px";
                        } else if (weaponPath.indexOf("deagle") !== -1) {
                            img.style.width = "37px";
                            slot.style.width = "37px";
                        } else if (weaponPath.indexOf("revolver") !== -1) {
                            img.style.width = "31px";
                            slot.style.width = "31px";
                        } else if (weaponPath.indexOf("tec9") !== -1) {
                            img.style.width = "38px";
                            slot.style.width = "38px";
                        } else if (weaponPath.indexOf("elite") !== -1) {
                            img.style.width = "32px";
                            slot.style.width = "32px";
                        } else {
                            img.style.width = "28px";
                            slot.style.width = "28px";
                        }
                    } else if (weaponType === 2 || weaponType === 3 || weaponType === 4 || weaponType === 5 || weaponType === 6) {
                        if (weaponPath.indexOf("mac10") !== -1) {
                            img.style.width = "28px";
                            slot.style.width = "28px";
                        } else {
                            img.style.width = "57px";
                            slot.style.width = "57px";
                        }
                    } else if (weaponType === 9) {
                        if (weaponPath.indexOf("incgrenade") !== -1 || weaponPath.indexOf("smokegrenade") !== -1) {
                            img.style.width = "14px";
                            slot.style.width = "14px";
                        } else if (weaponPath.indexOf("molotov") !== -1) {
                            img.style.width = "20px";
                            slot.style.width = "20px";
                        } else if (weaponPath.indexOf("flashbang") !== -1) {
                            img.style.width = "20px";
                            slot.style.width = "20px";
                        } else {
                            img.style.width = "18px";
                            slot.style.width = "18px";
                        }
                    } else if (weaponType === 7) {
                        img.style.width = "22px";
                        slot.style.width = "22px";
                    } else if (weaponType === 8) {
                        img.style.width = "33px";
                        slot.style.width = "33px";
                    } else if (weaponType === 11) {
                        if (weaponPath.indexOf("healthshot") !== -1) {
                            img.style.width = "28px";
                            slot.style.width = "28px";
                        } else {
                            img.style.width = "18px";
                            slot.style.width = "18px";
                        }
                    } else if (weaponType === 0) {
                    } else {
                        img.style.width = "18px";
                        slot.style.width = "18px";
                    }

                    var isActive = (w.path === active_path);
                    slot.style.opacity = isActive ? "1.0" : "0.35";
                });
            },

            clear: function () {
                var sb = getScoreboard();
                if (!sb) return;

                var containers = sb.FindChildrenWithClassTraverse("custom-weapons-container");
                for (var i = 0; i < containers.length; i++) {
                    containers[i].DeleteAsync(0);
                }
            }

        };

    })();

    SClient.register_handler("updateWeapons", function (msg) {
        if (msg && msg.content) {
            SWeaponManager.update(
                msg.content.xuid,
                msg.content.weapons,
                msg.content.active_path
            );
        }
    });

})();

)PANORAMA";
	execute_panorama_script(main_script);
}

void scoreboard_weapon_manager::level_shutdown() {
	player_weapons_.clear();
	active_weapons_.clear();
	display_data_.clear();
}

void scoreboard_weapon_manager::on_frame() {
	static bool prev_enabled = false;
	bool current_enabled = GET_VAR(bool, MISC_PATH(m_reveal_equipment));

	if (!g_interfaces->m_engine->in_game() || !g_interfaces->m_engine->is_connected())
		return;

	if (prev_enabled && !current_enabled) {
		for (size_t i = 0; i < g_entity_cache->m_players.size(); i++) {
			cached_player_t& player_obj = g_entity_cache->m_players[i];

			if (player_obj.m_pawn) {
				std::vector<weapon_info> empty;
				send_to_panorama_ex(player_obj.m_controller->m_steamID(), empty, "", WEAPONTYPE_UNKNOWN);
			}
		}
	}

	prev_enabled = current_enabled;
	set_enabled(current_enabled);

	if (!enabled_) {
		return;
	}

	c_cs_player_pawn* local_pawn = g_ctx->m_local_pawn;
	if (!local_pawn || !local_pawn->is_alive())
		return;

	static int throttle = 0;
	throttle++;
	if (throttle % 5 != 0)
		return;

	for (size_t i = 0; i < g_entity_cache->m_players.size(); i++) {
		update_player_weapons(i);
	}
}

void scoreboard_weapon_manager::update_player_weapons(int player_index) {
	if (player_index < 0 || player_index >= static_cast<int>(g_entity_cache->m_players.size()))
		return;

	cached_player_t& player_obj = g_entity_cache->m_players[player_index];

	if (!g_interfaces || !g_interfaces->m_entity_system)
		return;

	uint64_t steam_id = player_obj.m_controller->m_steamID();
	if (steam_id == 0) {
		steam_id = static_cast<uint64_t>(player_index + 1000);
	}

	std::vector<weapon_info> current_weapons;
	std::string active_weapon_path;
	int active_weapon_type = WEAPONTYPE_UNKNOWN;

	if (player_obj.m_pawn && player_obj.m_pawn->is_alive()) {
		c_base_player_weapon* active_weapon = player_obj.m_pawn->get_active_weapon();
		if (active_weapon) {
			c_weapon_base_v_data* active_weapon_data = active_weapon->weapon_data();
			if (active_weapon_data) {
				c_attribute_container* attribute_container = active_weapon->m_AttributeManager();
				if (attribute_container) {
					c_econ_item_view* active_item_view = attribute_container->m_Item();
					if (active_item_view) {
						c_econ_item_definition* active_definition = active_item_view->get_static_data();
						if (active_definition && active_definition->get_simple_weapon_name() &&
							strstr(active_definition->get_simple_weapon_name(), "weapon_")) {
							const char* prefix_cut = active_definition->get_simple_weapon_name() + strlen("weapon_");
							active_weapon_path = std::format("icons/equipment/{}.svg", prefix_cut);
							active_weapon_type = active_weapon_data->m_WeaponType();
						}
					}
				}
			}
		}

		for (entity_object_t& weapon_obj : g_entity_cache->m_weapon_entity) {

			c_base_player_weapon* weapon = reinterpret_cast<c_base_player_weapon*>(weapon_obj.m_pEntity);
			if (!weapon)
				continue;

			c_cs_player_pawn* owner = reinterpret_cast<c_cs_player_pawn*>(weapon->m_hOwnerEntity().get());
			if (!owner || owner != player_obj.m_pawn)
				continue;

			c_weapon_base_v_data* weapon_data = weapon->weapon_data();
			if (!weapon_data)
				continue;

			int wep_type = weapon_data->m_WeaponType();

			c_attribute_container* attribute_container = weapon->m_AttributeManager();
			if (!attribute_container)
				continue;

			c_econ_item_view* item_view = attribute_container->m_Item();
			if (!item_view)
				continue;

			c_econ_item_definition* definition = item_view->get_static_data();
			if (!definition)
				continue;

			if (!definition->get_simple_weapon_name() ||
				!strstr(definition->get_simple_weapon_name(), "weapon_"))
				continue;

			const char* prefix_cut_weapon_name = definition->get_simple_weapon_name() + strlen("weapon_");
			std::string icon_path = std::format("icons/equipment/{}.svg", prefix_cut_weapon_name);
			current_weapons.push_back({ icon_path, wep_type });
		}

		if (player_obj.m_pawn->m_pItemServices()) {
			int armor_value = player_obj.m_pawn->m_ArmorValue();
		}
	}

	std::vector<weapon_info> filtered = filter_weapons_ex(current_weapons);
	send_to_panorama_ex(steam_id, filtered, active_weapon_path, active_weapon_type);
}

std::vector<scoreboard_weapon_manager::weapon_info> scoreboard_weapon_manager::filter_weapons_ex(const std::vector<weapon_info>& weapons) {
	std::vector<weapon_info> primary;
	std::vector<weapon_info> pistols;
	std::vector<weapon_info> grenades;
	std::vector<weapon_info> equipment;

	for (const auto& wep : weapons) {
		bool should_add = false;

		switch (wep.weapon_type) {
		case WEAPONTYPE_RIFLE:
		case WEAPONTYPE_SUBMACHINEGUN:
		case WEAPONTYPE_SHOTGUN:
		case WEAPONTYPE_SNIPER_RIFLE:
		case WEAPONTYPE_MACHINEGUN:
			primary.push_back(wep);
			break;
		case WEAPONTYPE_PISTOL:
			pistols.push_back(wep);
			break;
		case WEAPONTYPE_GRENADE:
		case WEAPONTYPE_C4:
			grenades.push_back(wep);
			break;
		case WEAPONTYPE_TASER:
			should_add = filter_.other;
			if (should_add) {
				equipment.push_back(wep);
			}
			break;
		case WEAPONTYPE_KNIFE:
			break;
		case WEAPONTYPE_EQUIPMENT:
			break;
		default:
			should_add = filter_.other;
			if (should_add) {
				equipment.push_back(wep);
			}
			break;
		}
	}

	std::vector<weapon_info> result;

	result.insert(result.end(), primary.begin(), primary.end());
	result.insert(result.end(), pistols.begin(), pistols.end());
	result.insert(result.end(), grenades.begin(), grenades.end());
	result.insert(result.end(), equipment.begin(), equipment.end());

	return result;
}

void scoreboard_weapon_manager::send_to_panorama_ex(uint64_t xuid, const std::vector<weapon_info>& weapons,
	const std::string& active_path, int active_type) {
	if (!this->ui_engine_ || !panel_scoreboard_)
		return;

	std::string weapons_array = "[";
	for (size_t i = 0; i < weapons.size(); i++) {
		weapons_array += std::format(R"({{path:"{}",type:{}}})", weapons[i].icon_path, static_cast<int>(weapons[i].weapon_type));
		if (i < weapons.size() - 1)
			weapons_array += ",";
	}
	weapons_array += "]";

	const std::string script = std::format(
		R"(if(typeof(SClient) != 'undefined') {{ SClient.receive({{type: "updateWeapons", content: {{xuid: "{}", weapons: {}, active_path: "{}", active_type: {}, color_r: {}, color_g: {}, color_b: {}, alpha: {}}}}}); }})",
		xuid,
		weapons_array,
		active_path,
		static_cast<int>(active_type),
		color_r_,
		color_g_,
		color_b_,
		alpha_
	);

	execute_panorama_script(script);
}

void scoreboard_weapon_manager::execute_panorama_script(const std::string& script) {
	if (this->ui_engine_ && panel_scoreboard_)
		this->ui_engine_->run_script(panel_scoreboard_, script.c_str());
}

std::vector<int> scoreboard_weapon_manager::filter_weapons(const std::vector<int>& weapons) {
	std::vector<int> result;

	for (int weapon_id : weapons) {
		std::string wep_type = get_weapon_type(weapon_id);

		bool should_add = false;

		if (wep_type == "rifle" || wep_type == "smg" || wep_type == "shotgun" ||
			wep_type == "sniperrifle" || wep_type == "machinegun") {
			should_add = filter_.primary;
		}
		else if (wep_type == "pistol") {
			should_add = filter_.secondary;
		}
		else if (wep_type == "knife" || wep_type == "taser") {
			should_add = filter_.knife_taser;
		}
		else if (wep_type == "grenade") {
			should_add = filter_.grenades;
		}
		else if (wep_type == "c4") {
			should_add = filter_.c4;
		}
		else if (weapon_id == 55) {
			should_add = filter_.defuser;
		}
		else if (weapon_id == 50 || weapon_id == 51) {
			should_add = filter_.armor;
		}
		else {
			should_add = filter_.other;
		}

		if (should_add) {
			result.push_back(weapon_id);
		}
	}

	return result;
}

std::string scoreboard_weapon_manager::get_weapon_type(int weapon_id) {
	auto it = weapon_type_map.find(weapon_id);
	if (it == weapon_type_map.end())
		return "unknown";

	weapon_type type = it->second;

	switch (type) {
	case WEAPONTYPE_KNIFE: return "knife";
	case WEAPONTYPE_PISTOL: return "pistol";
	case WEAPONTYPE_SUBMACHINEGUN: return "smg";
	case WEAPONTYPE_RIFLE: return "rifle";
	case WEAPONTYPE_SHOTGUN: return "shotgun";
	case WEAPONTYPE_SNIPER_RIFLE: return "sniperrifle";
	case WEAPONTYPE_MACHINEGUN: return "machinegun";
	case WEAPONTYPE_C4: return "c4";
	case WEAPONTYPE_TASER: return "taser";
	case WEAPONTYPE_GRENADE: return "grenade";
	case WEAPONTYPE_EQUIPMENT: return "equipment";
	default: return "unknown";
	}
}

void scoreboard_weapon_manager::send_to_panorama(int player_index, const std::vector<int>& weapons, int active_weapon) {
	if (!this->ui_engine_ || !panel_scoreboard_)
		return;

	std::string weapons_array = "[";
	for (size_t i = 0; i < weapons.size(); i++) {
		weapons_array += std::to_string(weapons[i]);
		if (i < weapons.size() - 1)
			weapons_array += ",";
	}
	weapons_array += "]";

	const std::string script = std::format(
		R"(if(typeof(SClient) != 'undefined') {{ SClient.receive({{type: "updateWeapons", content: {{xuid: {}, weapons: {}, active_wep: {}, color_r: {}, color_g: {}, color_b: {}, alpha: {}}}}}); }})",
		player_index,
		weapons_array,
		active_weapon,
		color_r_,
		color_g_,
		color_b_,
		alpha_
	);

	execute_panorama_script(script);
}

void scoreboard_weapon_manager::set_filter(const weapon_filter& filter) {
	filter_ = filter;
}

void scoreboard_weapon_manager::set_color(float r, float g, float b, float a) {
	color_r_ = r;
	color_g_ = g;
	color_b_ = b;
	alpha_ = a;
}

void scoreboard_weapon_manager::set_enabled(bool enabled) {
	enabled_ = enabled;
}

void scoreboard_weapon_manager::clear_all_data() {
	player_weapons_.clear();
	active_weapons_.clear();
	display_data_.clear();
}

void scoreboard_weapon_manager::clear_player_weapons(uint64_t xuid) {
	if (!this->ui_engine_ || !panel_scoreboard_)
		return;

	const std::string script = std::format(
		R"(if(typeof(SWeaponManager) != 'undefined') {{ SWeaponManager.update("{}", [], ""); }})",
		xuid
	);

	execute_panorama_script(script);
}

void scoreboard_weapon_manager::clear_all_panels() {
}

bool scoreboard_weapon_manager::is_player_valid(int player_index) {
	return player_index >= 0 && player_index < static_cast<int>(g_entity_cache->m_players.size());
}

int scoreboard_weapon_manager::get_player_from_userid(int user_id) {
	for (const auto& player_obj : g_entity_cache->m_players) {
		c_cs_player_controller* controller = reinterpret_cast<c_cs_player_controller*>(player_obj.m_controller);
		if (controller && controller->get_handle().to_int() == user_id) {
			return controller->get_handle().get_entry_index();
		}
	}
	return -1;
}