#include "skins.h"

#include <cheat/config/vars.h>
#include <sdk/interfaces/resource_system.h>
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <Windows.h>

namespace {
    void set_glove_bodygroup(void* pawn) {
        using fn_t = void(__fastcall*)(void*, int, unsigned int);
        static auto fn = g_modules->m_client.find("85 D2 0F 88 ? ? ? ? 55 56 57").as<fn_t>();
        if (pawn && fn) {
            fn(pawn, 0, 1u);
            if (g_ctx->m_local_pawn && g_ctx->m_local_pawn->m_iTeamNum() == 3)
                fn(pawn, 1, 1u);
        }
    }

    void attr_remove(c_econ_item_view* item) {
        using fn_t = void(__fastcall*)(c_econ_item_view*, int);
        static auto fn = g_modules->m_client.find("40 53 48 83 EC 20 48 63 81 ? ? ? ? 44 0F B7 CA").as<fn_t>();
        if (item && fn)
            for (int index : {6, 7, 8})
                fn(item, index);
    }

    void attr_create(c_econ_item_view* item, int paint_kit, float wear, int seed) {
        using fn_t = void(__fastcall*)(c_econ_item_view*, const char*, float);
        static auto fn = g_modules->m_client.find("40 53 48 83 EC 20 48 8B D9 48 81 C1 08 02 00 00").as<fn_t>();
        if (!item || paint_kit <= 0 || !fn || !std::isfinite(wear))
            return;
        fn(item, "set item texture prefab", static_cast<float>(paint_kit));
        fn(item, "set item texture seed", static_cast<float>(std::max(seed, 0)));
        fn(item, "set item texture wear", std::clamp(wear, 0.f, 1.f));
    }

}

static const std::unordered_map<int, uint32_t> k_knife_subclass_hashes = {
    {500, 3933374535u}, {503, 3787235507u}, {505, 4046390180u}, {506, 2047704618u},
    {507, 1731408398u}, {508, 1638561588u}, {509, 2282479884u}, {512, 3412259219u},
    {514, 2511498851u}, {515, 1353709123u}, {516, 4269888884u}, {517, 1105782941u},
    {518, 275962944u},  {519, 1338637359u}, {520, 3230445913u}, {521, 3206681373u},
    {522, 2595277776u}, {523, 4029975521u}, {525, 365028728u},  {526, 3845286452u},
};

static const std::unordered_map<int, const char*> k_fallback_knife_models = {
    {500, "weapons/models/knife/knife_bayonet/weapon_knife_bayonet.vmdl"},
    {503, "weapons/models/knife/knife_css/weapon_knife_css.vmdl"},
    {505, "weapons/models/knife/knife_flip/weapon_knife_flip.vmdl"},
    {506, "weapons/models/knife/knife_gut/weapon_knife_gut.vmdl"},
    {507, "weapons/models/knife/knife_karambit/weapon_knife_karambit.vmdl"},
    {508, "weapons/models/knife/knife_m9/weapon_knife_m9.vmdl"},
    {509, "weapons/models/knife/knife_tactical/weapon_knife_tactical.vmdl"},
    {512, "weapons/models/knife/knife_falchion/weapon_knife_falchion.vmdl"},
    {514, "weapons/models/knife/knife_bowie/weapon_knife_bowie.vmdl"},
    {515, "weapons/models/knife/knife_butterfly/weapon_knife_butterfly.vmdl"},
    {516, "weapons/models/knife/knife_push/weapon_knife_push.vmdl"},
    {517, "weapons/models/knife/knife_cord/weapon_knife_cord.vmdl"},
    {518, "weapons/models/knife/knife_canis/weapon_knife_canis.vmdl"},
    {519, "weapons/models/knife/knife_ursus/weapon_knife_ursus.vmdl"},
    {520, "weapons/models/knife/knife_navaja/weapon_knife_navaja.vmdl"},
    {521, "weapons/models/knife/knife_outdoor/weapon_knife_outdoor.vmdl"},
    {522, "weapons/models/knife/knife_stiletto/weapon_knife_stiletto.vmdl"},
    {523, "weapons/models/knife/knife_talon/weapon_knife_talon.vmdl"},
    {525, "weapons/models/knife/knife_skeleton/weapon_knife_skeleton.vmdl"},
    {526, "weapons/models/knife/knife_kukri/weapon_knife_kukri.vmdl"},
};

static const char* localized_item_name(const char* token)
{
    if (!token || !*token)
        return "";
    const char* name = g_interfaces->m_localize ? g_interfaces->m_localize->find_safe(token) : nullptr;
    return name && *name ? name : token;
}

static bool is_paint_kit_from_weapon(c_paint_kit* paint_kit, const char* weapon_id)
{
    if (!paint_kit || !weapon_id || !*weapon_id || !g_interfaces->m_file_system)
        return false;
    const char* paint_name = paint_kit->paint_kit_name();
    if (!paint_name || !*paint_name)
        return false;
    auto path = "panorama/images/econ/default_generated/" + std::string(weapon_id) + "_" + paint_name + "_light_png.vtex_c";
    return g_interfaces->m_file_system->exists(path.c_str(), "GAME");
}

std::vector<c_dumped_skin> c_skins::process_paint_kits(c_econ_item_schema* econ_item_schema, c_econ_item_definition* item)
{
    std::vector<c_dumped_skin> skins;
    if (!econ_item_schema || !item)
        return skins;
    const char* weapon_name = item->get_simple_weapon_name();
    if (!weapon_name || !*weapon_name)
        return skins;
    for (const auto& paint_kit_entry : econ_item_schema->get_paint_kits())
    {
        c_paint_kit* paint_kit = paint_kit_entry.m_value;
        if (paint_kit && paint_kit->paint_kit_id() != 0 && paint_kit->paint_kit_id() != 9001)
        {
            if (!is_paint_kit_from_weapon(paint_kit, weapon_name))
                continue;

            uint32_t item_rarity = item->m_iterarity;
            uint32_t paint_kit_rarity = paint_kit->paint_kit_rarity();
            uint32_t final_rarity = std::clamp((int)(item_rarity + paint_kit_rarity - 1), 0, (paint_kit_rarity == 7) ? 7 : 6);

            skins.emplace_back(c_dumped_skin{
                localized_item_name(paint_kit->paint_kit_description_tag() ? paint_kit->paint_kit_description_tag() : paint_kit->paint_kit_name()),
                (int)paint_kit->paint_kit_id(),
                final_rarity
                });
        }
    }
    return skins;
}

void c_skins::add_item_to_category(c_econ_item_schema* econ_item_schema, c_econ_item_definition* item, bool is_special_item, std::vector<c_dumped_item>& category)
{
    c_dumped_item dumped_item(
        localized_item_name(item->m_item_base_name),
        item->get_item_definition_index(),
        is_special_item
    );

    dumped_item.skins = process_paint_kits(econ_item_schema, item);

    if (!dumped_item.skins.empty())
        std::sort(dumped_item.skins.begin(), dumped_item.skins.end(), [](const c_dumped_skin& a, const c_dumped_skin& b) {
        return a.rarity > b.rarity;
            });

    category.emplace_back(dumped_item);
}

void c_skins::dump_items()
{
    auto* econ_system = g_interfaces->m_source2_client->get_econ_item_system();
    if (!econ_system)
        return;
    auto* econ_item_schema = econ_system->get_econ_item_schema();
    if (!econ_item_schema || !items.empty())
        return;

    for (const auto& it : econ_item_schema->get_sorted_item_definition_map())
    {
        c_econ_item_definition* item = it.m_value;
        if (!item || !item->m_item_base_name || item->m_item_base_name[0] == '\0')
            continue;

        if (item->is_agent())
        {
            const char* model = item->get_model_name();
            if (!model || !*model)
                continue;
            agents.agent_models.emplace_back(model);
            agents.agent_names.emplace_back(localized_item_name(item->m_item_base_name));
            agents.agent_ids.emplace_back(item->get_item_definition_index());
        }
        else if (item->is_knife(true))
        {
            const int def_index = item->get_item_definition_index();
            if (k_knife_subclass_hashes.find(def_index) == k_knife_subclass_hashes.end())
                continue;

            const char* model = item->get_model_name();
            const auto model_exists = [](const char* path) {
                return path && *path && g_interfaces->m_file_system &&
                    g_interfaces->m_file_system->exists((std::string(path) + "_c").c_str(), "GAME");
            };
            if (!model_exists(model)) {
                auto fb = k_fallback_knife_models.find(def_index);
                model = (fb != k_fallback_knife_models.end()) ? fb->second : model;
            }

            if (!model_exists(model))
                continue;
            knives.models.emplace_back(model);
            knives.names.emplace_back(localized_item_name(item->m_item_base_name));
            knives.indexes.emplace_back(def_index);

            add_item_to_category(econ_item_schema, item, true, knives_items);
        }
        else if (item->is_glove(true))
        {
            const char* model = item->get_model_name();
            gloves.models.emplace_back(model ? model : "");
            gloves.names.emplace_back(localized_item_name(item->m_item_base_name));
            gloves.indexes.emplace_back(item->get_item_definition_index());

            add_item_to_category(econ_item_schema, item, true, glove_items);
        }
        else if (item->is_weapon())
        {
            add_item_to_category(econ_item_schema, item, false, items);
        }
    }

    std::vector<std::tuple<const char*, const char*, int, uint32_t>> agent_data;
    for (size_t i = 0; i < agents.agent_ids.size(); ++i) {
        auto item_def = econ_item_schema->get_sorted_item_definition_map().find_by_key(agents.agent_ids[i]);
        uint32_t rarity = item_def.has_value() ? item_def.value()->m_iterarity : 0;
        agent_data.emplace_back(agents.agent_models[i], agents.agent_names[i], agents.agent_ids[i], rarity);
    }
    
    std::sort(agent_data.begin(), agent_data.end(), [](const auto& a, const auto& b) {
        return std::get<3>(a) > std::get<3>(b);
    });
    
    agents.agent_models.clear();
    agents.agent_names.clear();
    agents.agent_ids.clear();
    
    for (const auto& data : agent_data) {
        agents.agent_models.emplace_back(std::get<0>(data));
        agents.agent_names.emplace_back(std::get<1>(data));
        agents.agent_ids.emplace_back(std::get<2>(data));
    }

    agents.agent_models.insert(agents.agent_models.begin(), nullptr);
    agents.agent_names.insert(agents.agent_names.begin(), "Default");
    agents.agent_ids.insert(agents.agent_ids.begin(), 0);
    g_ctx->m_dumped_skins = true;
}

uintptr_t* c_skins::find_hud_element(const char* name)
{
    using fn_find_hud_element = uintptr_t * (__fastcall*)(const char*);
    static fn_find_hud_element find_hud_element_ = g_modules->m_client.find(xx("40 53 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 74 ?? 48 89 5C 24")).as< fn_find_hud_element >();

    return find_hud_element_(name);
}

void c_skins::clear_hud_weapon()
{
    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    auto hud_weapon_selection = (uintptr_t)find_hud_element(xx("HudWeaponSelection"));
    if (!hud_weapon_selection)
        return;

    uintptr_t hud_weapons = hud_weapon_selection - 0x98;
    if (!hud_weapons)
        return;

    using fn_clear_hud_weapon = int(__fastcall*)(uintptr_t, int, void*);
    static fn_clear_hud_weapon fn_clear_hud_weapon_ = nullptr;

    if (!fn_clear_hud_weapon_)
        return;

    if (!g_interfaces->m_world)
        return;

    fn_clear_hud_weapon_(hud_weapons, 0, g_interfaces->m_world);
}

void c_skins::agent_changer()
{
    if (!g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0 || !GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger)))
        return;

    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    int team_num = g_ctx->m_local_pawn->m_iTeamNum();
    const bool is_ct = team_num == 3, is_t = team_num == 2;

    int menu_selected_ct = GET_VAR(int, SKINS_PATH(m_agent_selected_ct));
    int menu_selected_t = GET_VAR(int, SKINS_PATH(m_agent_selected_t));

    if (agents.agent_models.empty())
        return;

    const int selected = is_ct ? menu_selected_ct : menu_selected_t;
    if ((!is_ct && !is_t) || selected < 0 || static_cast<std::size_t>(selected) >= agents.agent_models.size())
        return;
    auto* node = g_ctx->m_local_pawn->m_pGameSceneNode();
    auto* skeleton = node ? node->get_skeleton_instace() : nullptr;
    if (!skeleton || !agents.agent_models[selected] || !*agents.agent_models[selected])
        return;
    const auto handle = g_ctx->m_local_pawn->get_handle().index;
    if (handle == INVALID_EHANDLE_INDEX)
        return;
    static std::uint32_t last_handle = INVALID_EHANDLE_INDEX;
    static int last_selected = -1, last_team = -1;
    static const resource_binding_t* last_model = nullptr;
    const auto* model = skeleton->m_modelState().m_model_handle.binding;
    if (handle == last_handle && selected == last_selected && team_num == last_team && model == last_model)
        return;
    g_ctx->m_local_pawn->set_model(agents.agent_models[selected]);
    last_handle = handle;
    last_selected = selected;
    last_team = team_num;
    last_model = skeleton->m_modelState().m_model_handle.binding;
}

#define TOLOWERU(c) ((uint32_t)(((c >= 'A') && (c <= 'Z')) ? c + 32 : c))
#define STRINGTOKEN_MURMURHASH_SEED 0x31415926

inline uint32_t MurmurHash2LowerCaseA1(const char* pString, int len, uint32_t nSeed)
{
    std::string lowered(static_cast<std::size_t>(len), '\0');
    char* p = lowered.data();
    for (int i = 0; i < len; i++)
    {
        p[i] = TOLOWERU(pString[i]);
    }
    return MurmurHash2(p, len, nSeed);
}

class c_utl_string_token
{
public:
    std::uint32_t m_nHashCode;
#if DEBUG_STRINGTOKENS
    const char* m_pDebugName;
#endif

    c_utl_string_token(const char* szString)
    {
        this->SetHashCode1(this->MakeStringToken1(szString));
    }

    bool operator==(const c_utl_string_token& other) const
    {
        return (other.m_nHashCode == m_nHashCode);
    }

    bool operator!=(const c_utl_string_token& other) const
    {
        return (other.m_nHashCode != m_nHashCode);
    }

    bool operator<(const c_utl_string_token& other) const
    {
        return (m_nHashCode < other.m_nHashCode);
    }

    uint32_t GetHashCode1(void) const
    {
        return m_nHashCode;
    }

    void SetHashCode1(uint32_t nCode)
    {
        m_nHashCode = nCode;
    }

    __forceinline std::uint32_t MakeStringToken1(const char* szString, int nLen)
    {
        std::uint32_t nHashCode = MurmurHash2LowerCaseA1(szString, nLen, STRINGTOKEN_MURMURHASH_SEED);
        return nHashCode;
    }

    __forceinline std::uint32_t MakeStringToken1(const char* szString)
    {
        return MakeStringToken1(szString, (int)strlen(szString));
    }

    //__forceinline std::uint32_t MakeStringToken(CUtlString& str)
    //{
    //    return MakeStringToken(str.Get(), str.Length());
    //}

    c_utl_string_token()
    {
        m_nHashCode = 0;
    }
};

void c_skins::knifes(int stage)
{
    if (!g_ctx->m_local_pawn || !g_ctx->m_local_controller)
        return;

    auto weapon_services = g_ctx->m_local_pawn->m_pWeaponServices();
    if (!weapon_services)
        return;

    c_base_view_model* view_model = g_ctx->m_local_pawn->get_view_model_brutforce();
    c_network_utl_vector<c_base_handle>& my_weapons = weapon_services->m_hMyWeapons();
    if (!my_weapons.elements || my_weapons.size > 64)
        return;

    for (auto i = 0; i < my_weapons.size; i++)
    {
        c_econ_entity* weapon = my_weapons.elements[i].get<c_econ_entity>();
        if (!weapon)
            continue;

        c_attribute_container* attribute_manager = weapon->m_AttributeManager();
        if (!attribute_manager)
            continue;

        c_econ_item_view* item = attribute_manager->m_Item();
        if (!item)
            continue;

        const uint16_t cur_def = item->m_iItemDefinitionIndex();
        const bool is_knife_entity = (cur_def == 42 || cur_def == 59 || (cur_def >= 500 && cur_def <= 526));

        if (!is_knife_entity)
            continue;

        if (!GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger))) {
            weapon->m_nFallbackPaintKit() = 0;
            weapon->m_flFallbackWear() = 0.0001f;
            weapon->m_nFallbackSeed() = 0;
            continue;
        }

        const int selected_knife = GET_VAR(int, SKINS_PATH(m_knife_selected));

        if (selected_knife < 0 || selected_knife >= static_cast<int>(g_skins->knives_items.size()) ||
            selected_knife >= static_cast<int>(knives.indexes.size()) ||
            selected_knife >= static_cast<int>(knives.models.size()))
            continue;

        const auto& knives_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));

        if (selected_knife < 0 || selected_knife >= knives_settings.size())
            return;

        const auto& selected_setting = knives_settings[selected_knife];
        auto current_skin = selected_setting.m_paint_kit;

        const auto hash_it = k_knife_subclass_hashes.find(knives.indexes[selected_knife]);
        if (hash_it == k_knife_subclass_hashes.end())
            continue;

        const uint32_t target_subclass = hash_it->second;
        const int target_def_index = knives.indexes[selected_knife];

        static int last_knife_index = -1;
        bool knife_changed = (last_knife_index != selected_knife);

        c_weapon_base* weapon_base = (c_weapon_base*)weapon;

        bool weapon_needs_update = knife_changed
            || need_update_knifes
            || weapon->m_nFallbackPaintKit() != current_skin
            || weapon->m_flFallbackWear() != selected_setting.m_wear
            || weapon->m_nFallbackSeed() != selected_setting.m_seed
            || item->m_iItemDefinitionIndex() != static_cast<uint16_t>(target_def_index)
            || weapon_base->m_nSubclassID() != target_subclass;

        const bool active_knife = g_ctx->m_local_pawn->get_active_weapon() == weapon;
        auto* weapon_node = weapon->m_pGameSceneNode();
        auto* view_node = active_knife && view_model ? view_model->m_pGameSceneNode() : nullptr;
        auto* weapon_skeleton = weapon_node ? weapon_node->get_skeleton_instace() : nullptr;
        auto* view_skeleton = view_node ? view_node->get_skeleton_instace() : nullptr;
        const bool view_needs_update = view_skeleton && weapon_skeleton &&
            view_skeleton->m_modelState().m_model_handle.binding != weapon_skeleton->m_modelState().m_model_handle.binding;
        if (!weapon_needs_update && !view_needs_update)
            continue;
        if (!weapon_node || !g_interfaces->m_resource_system)
            continue;
        c_buffer_string model_path(knives.models[selected_knife], static_cast<uint64_t>(0x6C646D76));
        auto* model_resource = g_interfaces->m_resource_system->blocking_load_resource_by_name(&model_path, "");
        if (!model_resource || !model_resource->data)
            continue;

        item->m_iItemIDHigh() = 0xFFFFFFFFu;
        item->m_iItemIDLow() = 0u;
        item->m_bInitialized() = true;
        item->m_iEntityQuality() = 3;
        item->m_iItemDefinitionIndex() = static_cast<uint16_t>(target_def_index);

        weapon_base->m_nSubclassID() = target_subclass;


        weapon->m_nFallbackPaintKit() = current_skin;
        weapon->m_flFallbackWear() = selected_setting.m_wear;
        weapon->m_nFallbackSeed() = selected_setting.m_seed;
        weapon->m_nFallbackStatTrak() = -1;



        if (weapon_needs_update)
            weapon_base->update_subclass();
        weapon->set_model(knives.models[selected_knife]);
        if (active_knife && view_node)
            view_model->set_model(knives.models[selected_knife]);
        if (weapon_needs_update)
            weapon_base->regen_weapon_skin();

        const uint64_t mesh_mask = (target_def_index == 523) ? 2ull : 1ull;
        weapon_node->set_mesh_group_mask(mesh_mask);
        if (view_node)
            view_node->set_mesh_group_mask(mesh_mask);

        clear_hud_weapon();
        need_update_knifes = false;
        last_knife_index = selected_knife;
    }
}

void c_skins::weapons()
{
    if (!g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0)
        return;

    auto weapon_service = g_ctx->m_local_pawn->m_pWeaponServices();
    if (!weapon_service)
        return;


    c_base_view_model* view_model = g_ctx->m_local_pawn->get_view_model_brutforce();
    if (!view_model)
        return;

    c_network_utl_vector<c_base_handle>& my_weapons = weapon_service->m_hMyWeapons();
    if (!my_weapons.elements || my_weapons.size > 64)
        return;

    for (auto i = 0; i < my_weapons.size; i++)
    {
        c_econ_entity* weapon = my_weapons.elements[i].get<c_econ_entity>();
        if (!weapon || !weapon->m_pGameSceneNode() || !view_model->m_pGameSceneNode())
            continue;

        c_attribute_container* attribute_manager = weapon->m_AttributeManager();
        if (!attribute_manager)
            continue;

        c_econ_item_view* item = attribute_manager->m_Item();
        if (!item)
            continue;

        auto item_schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
        if (!item_schema)
            continue;

        int item_definition_index = item->m_iItemDefinitionIndex();

        int current_weapon = get_skin_config(item_definition_index);
        if (current_weapon == CONFIG_UNKNOWN)
            continue;

        if (!GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger))) {
            weapon->m_nFallbackPaintKit() = 0;
            weapon->m_flFallbackWear() = 0.0001f;
            weapon->m_nFallbackSeed() = 0;
            continue;
        }

        const auto& skins_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));

        if (current_weapon < 0 || static_cast<std::size_t>(current_weapon) >= skins_settings.size())
            continue;
        auto& current_skin = skins_settings[current_weapon];

        bool wants_update = weapon->m_nFallbackPaintKit() != current_skin.m_paint_kit
            || weapon->m_flFallbackWear() != current_skin.m_wear
            || weapon->m_nFallbackSeed() != current_skin.m_seed;

        if (current_skin.m_paint_kit > 0) {
            auto paint_kit = item_schema->get_paint_kits().find_by_key(current_skin.m_paint_kit);
            bool use_old_model = paint_kit.has_value() && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(paint_kit.value()) + 0xAE);

            uint64_t mesh_mask = 1 + static_cast<uint64_t>(use_old_model);


            c_weapon_base* weapon_base = (c_weapon_base*)weapon;
            if (wants_update)
                weapon_base->m_nSubclassID() = c_utl_string_token(std::to_string(item->m_iItemDefinitionIndex()).c_str()).GetHashCode1();

            weapon->m_nFallbackPaintKit() = current_skin.m_paint_kit;
            weapon->m_flFallbackWear() = current_skin.m_wear;
            weapon->m_nFallbackSeed() = current_skin.m_seed;

            item->m_iItemIDHigh() = -1;

            weapon->m_pGameSceneNode()->set_mesh_group_mask(mesh_mask);
            view_model->m_pGameSceneNode()->set_mesh_group_mask(mesh_mask);
        }

        if (wants_update) {
            weapon->m_nFallbackPaintKit() = current_skin.m_paint_kit;
            weapon->m_flFallbackWear() = current_skin.m_wear;
            weapon->m_nFallbackSeed() = current_skin.m_seed;
            c_weapon_base* weapon_base = static_cast<c_weapon_base*>(weapon);



            weapon_base->update_subclass();

            weapon_base->regen_weapon_skin();



            clear_hud_weapon();
        }
    }
}

void c_skins::glove_changer()
{
    if (!g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0)
        return;

    if (!GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger))) {
        c_econ_item_view* item = &g_ctx->m_local_pawn->m_EconGloves();
        if (item) {
            item->m_iItemDefinitionIndex() = 0;
            item->m_bInitialized() = false;
        }
        return;
    }

    const int selected_glove = GET_VAR(int, SKINS_PATH(m_glove_selected));
    if (selected_glove < 0 || selected_glove >= static_cast<int>(g_skins->glove_items.size()) ||
        selected_glove >= static_cast<int>(gloves.indexes.size()))
        return;

    const auto& gloves_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
    if (selected_glove >= gloves_settings.size())
        return;

    const auto& selected_setting = gloves_settings[selected_glove];
    auto current_skin = selected_setting.m_paint_kit;

    if (current_skin <= 0)
        return;

    c_econ_item_schema* econ_item_schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
    if (!econ_item_schema)
        return;

    c_econ_item_view* item = &g_ctx->m_local_pawn->m_EconGloves();
    if (!item)
        return;

    int current_model = gloves.indexes[selected_glove];

    int menu_selected_ct = GET_VAR(int, SKINS_PATH(m_agent_selected_ct));
    int menu_selected_t = GET_VAR(int, SKINS_PATH(m_agent_selected_t));

    static int last_skin = -1;
    static int last_seed = -1;
    static float last_wear = -1.f;
    static int last_agent_ct = -1;
    static int last_agent_t = -1;
    static float last_spawn_time = -1.f;

    static int s_update_frames = 0;
    static int last_glove = -1;
    static c_base_handle last_pawn;

    const float spawn_now = g_ctx->m_local_pawn->m_flLastSpawnTimeIndex();
    const bool changed = (selected_glove != last_glove) || (current_skin != last_skin)
        || (selected_setting.m_seed != last_seed) || (selected_setting.m_wear != last_wear)
        || (spawn_now != last_spawn_time) || last_pawn != g_ctx->m_local_pawn->get_handle() || need_update_gloves;
    const bool engine_reset = (item->m_iItemDefinitionIndex() != current_model) || !item->m_bInitialized();

    if (changed) {
        s_update_frames = 6;
    }
    else if (engine_reset) {
        s_update_frames = 6;
    }
    need_update_gloves = false;

    if (s_update_frames <= 0)
        return;

    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    item->m_iItemDefinitionIndex() = current_model;
    item->m_iEntityQuality() = 3;

    attr_remove(item);
    attr_create(item, current_skin, selected_setting.m_wear, selected_setting.m_seed);

    item->m_iItemID() = 0xF000000000000010ull;
    item->m_iItemIDHigh() = 0xF0000000u;
    item->m_iItemIDLow() = 0x10u;
    item->m_bDisallowSOC() = true;
    item->m_bRestoreCustomMaterialAfterPrecache() = true;
    if (g_ctx->m_local_controller)
        item->m_iAccountID() = static_cast<uint32_t>(g_ctx->m_local_controller->m_steamID() & 0xFFFFFFFF);
    item->m_bInitialized() = true;
    using invalidate_t = void(__fastcall*)(c_econ_item_view*);
    static auto invalidate = g_modules->m_client.find("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D B9 ? ? ? ? 48 8B F1").as<invalidate_t>();
    if (invalidate) invalidate(item);
    g_ctx->m_local_pawn->m_bNeedToReApplyGloves() = true;
    using update_t = void(__fastcall*)(void*, int);
    auto update = g_memory->get_method<update_t>(g_ctx->m_local_pawn, 10);
    if (update) update(g_ctx->m_local_pawn, 1);
    set_glove_bodygroup(g_ctx->m_local_pawn);

    last_glove = selected_glove;
    last_skin = current_skin;
    last_seed = selected_setting.m_seed;
    last_wear = selected_setting.m_wear;
    last_spawn_time = spawn_now;
    last_pawn = g_ctx->m_local_pawn->get_handle();
    last_agent_t = menu_selected_t;
    last_agent_ct = menu_selected_ct;
    need_update_gloves = false;
    s_update_frames--;
}

void c_skins::update() {
    if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;
        
    bool is_alive = g_ctx->m_local_pawn && g_ctx->m_local_pawn->is_alive();
    int current_team = g_ctx->m_local_pawn ? g_ctx->m_local_pawn->m_iTeamNum() : 0;
    
    if (m_waiting_for_round_start_update) {
        need_update_knifes = true;
        need_update_gloves = true;
        m_waiting_for_round_start_update = false;
    }
    
    if (!m_waiting_for_round_start_update) {
        if (!m_was_alive && is_alive) {
            need_update_knifes = true;
            need_update_gloves = true;
        }
        
        if (m_last_team != current_team && current_team != 0) {
            need_update_knifes = true;
            need_update_gloves = true;
        }
    }
    
    m_was_alive = is_alive;
    m_last_team = current_team;
}

void c_skins::on_round_start() {
    m_waiting_for_round_start_update = true;
}
