#include "skybox_changer.h"
#include <sdk/interfaces/resource_system.h>
#include <core/interfaces/interfaces.h>
#include "chams.h"
#include <sdk/datatypes/key_values.h>
#include <sdk/datatypes/c_buffer_string.h>

inline const char* skybox_template = R"(
        <!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
        {
            Shader = "sky.vfx"
            g_flBrightnessExposureBias = 0.000000
            g_flRenderOnlyExposureBias = 0.000000
            SkyTexture = resource:"%s"
            g_tSkyTexture = resource:"%s"
        }
)";

auto create_material = []( const char* material_name, const char vmat[] ) -> c_strong_handle<c_material_2> {
    if ( !material_name || !vmat )
        return {};

    c_key_values_3* keyval = c_key_values_3::create_material_resource( );
    if ( !keyval )
        return {};

    keyval->load_from_buffer( vmat );

    c_strong_handle<c_material_2> custom_material{};
    using fn_create_material = __int64( __fastcall* )(void*, void*, const char*, c_key_values_3*, unsigned int, unsigned int);
    static fn_create_material create_material = g_modules->m_materialsystem2.find( xx( "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 8B F2" ) ).as<fn_create_material>( );

    if ( !create_material )
        return {};

    create_material( nullptr, &custom_material, material_name, keyval, 0, 1 );

    return custom_material;
};

void c_skybox_changer::refresh_custom_skyboxes() {
    std::error_code error;
    char client_path[MAX_PATH]{};
    if (!GetModuleFileNameA(GetModuleHandleA("client.dll"), client_path, MAX_PATH)) return;
    auto folder = std::filesystem::path(client_path).parent_path().parent_path().parent_path() / "materials" / "skybox";
    std::vector<std::string> found;
    for (std::filesystem::directory_iterator it(folder, error), end; !error && it != end; it.increment(error))
        if (it->is_regular_file(error) && it->path().extension() == ".vtex_c")
            found.push_back(it->path().stem().string());
    std::sort(found.begin(), found.end());
    m_custom_skyboxes = std::move(found);
}
std::vector<std::string> c_skybox_changer::get_all_skyboxes() {
    std::vector<std::string> result(std::begin(arr_skybox_names), std::end(arr_skybox_names));
    result.insert(result.end(), m_custom_skyboxes.begin(), m_custom_skyboxes.end());
    return result;
}
c_material_2* c_skybox_changer::get_sky_material() {
    std::lock_guard<std::mutex> lock(m_sky_mutex);
    return m_custom_skybox_material;
}
void c_skybox_changer::run() {
    std::lock_guard<std::mutex> lock(m_sky_mutex);
    if (!g_interfaces->m_engine || !g_interfaces->m_engine->in_game()) return;
    const int index = GET_VAR(int, VISUALS_PATH(m_selected_skybox));
    if (!GET_VAR(bool, VISUALS_PATH(m_enable_custom_sky)) || index <= 0) {
        m_has_custom_skybox = false;
        m_custom_skybox_material = nullptr;
        m_custom_skybox_handle = {};
        m_last_selected_skybox = -1;
        m_need_update_material = true;
        return;
    }
    if (index == m_last_selected_skybox && m_has_custom_skybox && !m_need_update_material) return;
    m_has_custom_skybox = false;
    m_custom_skybox_material = nullptr;
    m_custom_skybox_handle = {};
    const int builtin_count = static_cast<int>(std::size(arr_skybox_names));
    std::string path;
    if (index < builtin_count) path = arr_skybox_paths[index - 1];
    else {
        const int custom = index - builtin_count;
        if (custom < 0 || custom >= static_cast<int>(m_custom_skyboxes.size())) return;
        path = "materials/skybox/" + m_custom_skyboxes[custom] + ".vtex";
    }
    auto cached = m_material_cache.find(path);
    if (cached != m_material_cache.end()) m_custom_skybox_handle = cached->second;
    else {
        if (!g_interfaces->m_resource_system || GetTickCount64() < m_next_retry) return;
        m_next_retry = GetTickCount64() + 1000;
        auto* texture = g_interfaces->m_resource_system->precache_resource(path.c_str(), 0x78657476ull);
        if (!texture || !texture->data) return;
        char material[2048]{};
        sprintf_s(material, skybox_template, path.c_str(), path.c_str());
        const std::string name = "materials/dev/hellcore_sky_" + std::to_string(index) + ".vmat";
        auto handle = create_material(name.c_str(), material);
        if (!handle.binding || !handle) return;
        m_material_cache.emplace(path, handle);
        m_custom_skybox_handle = handle;
    }
    m_custom_skybox_material = m_custom_skybox_handle;
    m_has_custom_skybox = m_custom_skybox_material != nullptr;
    if (m_has_custom_skybox) { m_last_selected_skybox = index; m_need_update_material = false; }
}
void c_skybox_changer::init_on_level_load() {
    cleanup();
    refresh_custom_skyboxes();
}

void c_skybox_changer::restore_default_skybox() {
    std::lock_guard<std::mutex> lock(m_sky_mutex);
    m_has_custom_skybox = false;
    m_custom_skybox_material = nullptr;
    m_custom_skybox_handle = {};
    m_last_selected_skybox = -1;
    m_need_update_material = true;
}

void c_skybox_changer::cleanup( ) {
    std::lock_guard<std::mutex> lock(m_sky_mutex);
    m_default_skybox_material = nullptr;
    m_custom_skybox_material = nullptr;
    m_custom_skybox_handle = {};
    m_has_custom_skybox = false;
    m_last_selected_skybox = -1;
    m_need_update_material = true;
    m_material_cache.clear();
    m_next_retry = 0;
}
