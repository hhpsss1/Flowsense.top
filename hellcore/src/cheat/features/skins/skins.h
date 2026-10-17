#pragma once

#include <context.h>
#include <sdk/constants.h>

#include <core/interfaces/interfaces.h>

inline short get_skin_config(short item_def)
{
    switch (item_def)
    {
    case WEAPON_DESERT_EAGLE:
        return CONFIG_DEAGLE;
    case WEAPON_DUAL_BERETTAS:
        return CONFIG_DUAL_BERETTAS;
    case WEAPON_FIVE_SEVEN:
        return CONFIG_FIVE_SEVEN;
    case WEAPON_GLOCK_18:
        return CONFIG_GLOCK;
    case WEAPON_AK_47:
        return CONFIG_AK47;
    case WEAPON_AUG:
        return CONFIG_AUG;
    case WEAPON_AWP:
        return CONFIG_AWP;
    case WEAPON_FAMAS:
        return CONFIG_FAMAS;
    case WEAPON_G3SG1:
        return CONFIG_G3SG1;
    case WEAPON_GALIL_AR:
        return CONFIG_GALIL;
    case WEAPON_M249:
        return CONFIG_M249;
    case WEAPON_M4A4:
        return CONFIG_M4A4;
    case WEAPON_MAC_10:
        return CONFIG_MAC10;
    case WEAPON_P90:
        return CONFIG_P90;
    case WEAPON_UMP_45:
        return CONFIG_UMP;
    case WEAPON_MP5_SD:
        return CONFIG_MP5SD;
    case WEAPON_XM1014:
        return CONFIG_XM1024;
    case WEAPON_PP_BIZON:
        return CONFIG_BIZON;
    case WEAPON_MAG_7:
        return CONFIG_MAG7;
    case WEAPON_NEGEV:
        return CONFIG_NEGEV;
    case WEAPON_SAWED_OFF:
        return CONFIG_SAWEDOFF;
    case WEAPON_TEC_9:
        return CONFIG_TEC9;
    case WEAPON_ZEUS_X27:
        return CONFIG_TASER;
    case WEAPON_P2000:
        return CONFIG_HKP2000;
    case WEAPON_MP7:
        return CONFIG_MP7;
    case WEAPON_MP9:
        return CONFIG_MP9;
    case WEAPON_NOVA:
        return CONFIG_NOVA;
    case WEAPON_P250:
        return CONFIG_P250;
    case WEAPON_SCAR_20:
        return CONFIG_SCAR20;
    case WEAPON_SG_553:
        return CONFIG_SG553;
    case WEAPON_SSG_08:
        return CONFIG_SSG08;
    case WEAPON_M4A1_S:
        return CONFIG_M4A1_S;
    case WEAPON_USP_S:
        return CONFIG_USP_S;
    case WEAPON_CZ75_AUTO:
        return CONFIG_CZ75;
    case WEAPON_R8_REVOLVER:
        return CONFIG_REVOLVER;
    }

    return CONFIG_UNKNOWN;
}

class c_dumped_skin {
public:
    std::string name;
    int id = 0;
    uint32_t rarity = 0;

    c_dumped_skin(const std::string& skin_name, int skin_id, uint32_t skin_rarity)
        : name(skin_name), id(skin_id), rarity(skin_rarity) {
    }
};

class c_dumped_item {
public:
    std::string name;
    uint16_t def_index = 0;
    bool unusual_item = false;
    std::vector<c_dumped_skin> skins;
    c_dumped_skin* selected_skin = nullptr;

    c_dumped_item(const std::string& item_name, uint16_t item_def_index, bool is_unusual)
        : name(item_name), def_index(item_def_index), unusual_item(is_unusual) {
    }
};

struct c_gloves {
    std::vector<const char*> models;
    std::vector<const char*> names;
    std::vector<int> indexes;
};

struct c_knives {
    std::vector<const char*> models;
    std::vector<const char*> names;
    std::vector<int> indexes;
};

struct c_agents
{
    std::vector<const char*> agent_models;
    std::vector<const char*> agent_names;
    std::vector<int> agent_ids;
};

class c_skins
{
    std::vector<c_dumped_skin> process_paint_kits(c_econ_item_schema* econ_item_schema, c_econ_item_definition* item);
    void add_item_to_category(c_econ_item_schema* econ_item_schema, c_econ_item_definition* item, bool is_special_item, std::vector<c_dumped_item>& category);
public:
    std::vector<c_dumped_item> items;
    std::vector<c_dumped_item> glove_items;
    std::vector<c_dumped_item> knives_items;

    c_agents agents;
    c_gloves gloves;
    c_knives knives;

    bool need_update_knifes = false;
    bool need_update_gloves = false;

    void dump_items();
    void update();
    void on_round_start();
    uintptr_t* find_hud_element(const char* name);
    void clear_hud_weapon();
    void agent_changer();
    void knifes(int stage);
    void weapons();
    void glove_changer();
    
private:
    bool m_was_alive = false;
    int m_last_team = 0;
    bool m_pending_update = false;
    bool m_waiting_for_round_start_update = false;
};
inline auto g_skins = std::make_unique<c_skins>();