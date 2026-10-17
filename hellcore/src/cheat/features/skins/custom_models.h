#pragma once

#include <string>
#include <vector>
#include <memory>

class c_custom_models {
public:
    struct entry_t {
        std::string name;
        std::string path;
    };

    std::vector<entry_t> m_models;
    bool m_scanned = false;

    void scan();
    void update();
};
inline auto g_custom_models = std::make_unique<c_custom_models>();
