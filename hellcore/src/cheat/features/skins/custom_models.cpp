#include "custom_models.h"

#include <algorithm>
#include <filesystem>

#include <context.h>
#include <core/interfaces/interfaces.h>
#include <cheat/config/vars.h>
#include <sdk/entity/entity.h>
#include <sdk/entity/base_handle.h>
#include <sdk/interfaces/resource_system.h>
#include <sdk/datatypes/c_buffer_string.h>

void c_custom_models::scan() {
    m_models.clear();
    m_models.push_back({ "[ OFF ]", "" });

    std::filesystem::path root;
    if (!g_ctx->m_running_path.empty()) {
        root = std::filesystem::path(g_ctx->m_running_path).parent_path().parent_path().parent_path()
            / "csgo" / "characters" / "models";
    }
    if (root.empty() || !std::filesystem::exists(root))
        root = std::filesystem::current_path() / "csgo" / "characters" / "models";

    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        m_scanned = true;
        return;
    }

    for (auto& p : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (ec)
            break;
        if (!p.is_regular_file(ec) || ec)
            continue;
        if (p.path().extension() != ".vmdl_c")
            continue;

        std::string full = p.path().string();
        size_t pos = full.find("characters\\");
        if (pos == std::string::npos)
            pos = full.find("characters/");
        if (pos == std::string::npos)
            continue;

        std::string rel = full.substr(pos);
        std::replace(rel.begin(), rel.end(), '\\', '/');
        size_t ext = rel.rfind(".vmdl_c");
        if (ext != std::string::npos)
            rel.replace(ext, 7, ".vmdl");

        m_models.push_back({ p.path().stem().string(), rel });
    }

    m_scanned = true;
}

void c_custom_models::update() {
    static std::uint32_t last_handle = INVALID_EHANDLE_INDEX;
    static int last_selected = -1;
    static const resource_binding_t* last_model = nullptr;

    if (!g_ctx->m_local_pawn || g_ctx->m_local_pawn->m_iHealth() <= 0 || !GET_VAR(bool, SKINS_PATH(m_enabled_skinchanger))) {
        last_handle = INVALID_EHANDLE_INDEX;
        last_selected = -1;
        last_model = nullptr;
        return;
    }

    if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;

    if (!g_interfaces->m_resource_system)
        return;

    int selected = GET_VAR(int, SKINS_PATH(m_custom_model_selected));
    if (m_models.empty() || selected <= 0 || static_cast<std::size_t>(selected) >= m_models.size()) {
        last_handle = INVALID_EHANDLE_INDEX;
        last_selected = -1;
        last_model = nullptr;
        return;
    }

    const std::string& path = m_models[selected].path;
    if (path.empty())
        return;

    auto* node = g_ctx->m_local_pawn->m_pGameSceneNode();
    auto* skeleton = node ? node->get_skeleton_instace() : nullptr;
    if (!skeleton)
        return;

    const auto handle = g_ctx->m_local_pawn->get_handle().index;
    if (handle == INVALID_EHANDLE_INDEX)
        return;

    const auto* model = skeleton->m_modelState().m_model_handle.binding;
    if (handle == last_handle && selected == last_selected && model == last_model)
        return;

    c_buffer_string model_path(path.c_str(), static_cast<std::uint64_t>(0x6C646D76));
    auto* resource = g_interfaces->m_resource_system->blocking_load_resource_by_name(&model_path, "");
    if (!resource || !resource->data)
        return;

    g_ctx->m_local_pawn->set_model(path.c_str());

    last_handle = handle;
    last_selected = selected;
    last_model = skeleton->m_modelState().m_model_handle.binding;
}
