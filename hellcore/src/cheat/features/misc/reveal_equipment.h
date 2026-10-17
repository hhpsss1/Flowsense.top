#pragma once

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>

class c_ui_engine;
class c_ui_panel;

struct weapon_filter {
    bool primary = true;
    bool secondary = true;
    bool knife_taser = true;
    bool grenades = true;
    bool c4 = true;
    bool defuser = true;
    bool armor = true;
    bool other = true;
};

class scoreboard_weapon_manager {
public:
    struct weapon_info {
        std::string icon_path;
        int weapon_type;

        bool operator==(const weapon_info& other) const {
            return icon_path == other.icon_path && weapon_type == other.weapon_type;
        }
    };

    struct player_display_data {
        std::vector<weapon_info> weapons;
        std::string active_path;
        int active_type;

        bool operator==(const player_display_data& other) const {
            return weapons == other.weapons &&
                active_path == other.active_path &&
                active_type == other.active_type;
        }
    };

    struct player_weapon_cache {
        std::vector<weapon_info> weapons;
        std::string active_path;

        bool operator==(const player_weapon_cache& other) const {
            return weapons == other.weapons && active_path == other.active_path;
        }
    };

    scoreboard_weapon_manager();

    ~scoreboard_weapon_manager();

    void initialize();
    void shutdown();
    void level_init();
    void level_shutdown();
    void on_frame();

    void update_player_weapons(int player_index);
    std::vector<int> filter_weapons(const std::vector<int>& weapons);
    std::string get_weapon_type(int weapon_id);
    void send_to_panorama(int player_index, const std::vector<int>& weapons, int active_weapon);
    void set_filter(const weapon_filter& filter);
    void set_color(float r, float g, float b, float a);
    void set_enabled(bool enabled);

    void clear_all_data();

    bool is_player_valid(int player_index);

    void remove_all_icons();

    int get_player_from_userid(int user_id);

private:
    std::vector<weapon_info> filter_weapons_ex(const std::vector<weapon_info>& weapons);
    std::string escape_js_string(const std::string& input);
    void send_to_panorama_ex(uint64_t xuid, const std::vector<weapon_info>& weapons, const std::string& active_path, int active_type);
    void clear_player_weapons(uint64_t xuid);
    void clear_all_panels();
    void execute_panorama_script(const std::string& script);

    bool enabled_ = false;
    bool just_enabled_ = false;
    bool script_initialized_;
    weapon_filter filter_;
    float color_r_ = 1.0f;
    float color_g_ = 1.0f;
    float color_b_ = 0.0f;
    float alpha_ = 1.0f;

    std::unordered_map<int, std::vector<int>> player_weapons_;
    std::unordered_map<int, int> active_weapons_;
    std::unordered_map<uint64_t, player_display_data> display_data_;
    std::unordered_map<uint64_t, player_weapon_cache> last_weapon_cache_;

    c_ui_engine* ui_engine_;
    c_ui_panel* panel_scoreboard_;
    std::string main_script_;
    int last_update_frame_;
};
inline const std::unique_ptr<scoreboard_weapon_manager> g_scoreboard = std::make_unique<scoreboard_weapon_manager>();