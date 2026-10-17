#pragma once
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <mutex>
#include <sdk/interfaces/material_system.h>

static const char* arr_skybox_names[] = {
    "Default", "Sunset", "Overcast", "Night", "Aztec", "Jungle", "Cloudy",
    "Overpass", "Nuke", "Office", "Anubis", "Train", "Vertigo"
};
static constexpr std::array<std::string_view, 12> arr_skybox_paths = {
    "materials/skybox/cs_italy_s2_skybox_sunset_2_exr_e56cedf6.vtex",
    "materials/skybox/sky_overcast_01_exr_da4019b1.vtex",
    "materials/skybox/sky_rain_night_01_exr_8d775aee.vtex",
    "materials/skybox/sky_hr_aztec_02_exr_f84f8de9.vtex",
    "materials/skybox/jungle_cube_pfm_bc16d813.vtex",
    "materials/skybox/sky_csgo_cloudy01_cube_pfm_f9a0b177.vtex",
    "materials/skybox/sky_de_overpass_01_exr_f8534391.vtex",
    "materials/skybox/sky_de_nuke_exr_f04e84b2.vtex",
    "materials/skybox/sky_cs_office_45_0_exr_d0152542.vtex",
    "materials/skybox/sky_de_annubis_exr_2c5e0b53.vtex",
    "materials/skybox/sky_de_train03_exr_4fdb8a38.vtex",
    "materials/skybox/sky_de_vertigo_exr_c70a3937.vtex"
};

class c_skybox_changer {
public:
	void run( );
    c_material_2* get_sky_material();
	void refresh_custom_skyboxes( );  
	std::vector<std::string> get_all_skyboxes( ); 
	void init_on_level_load( );
	void restore_default_skybox( );
	void cleanup( );

	bool m_need_update_material = false;
	c_material_2* m_default_skybox_material = nullptr;
	c_material_2* m_custom_skybox_material = nullptr;
    c_strong_handle<c_material_2> m_custom_skybox_handle{};
	bool m_has_custom_skybox = false;
	int m_last_selected_skybox = -1;
private:
	std::mutex m_sky_mutex;
    std::vector<std::string> m_custom_skyboxes;
    std::unordered_map<std::string, c_strong_handle<c_material_2>> m_material_cache;
    uint64_t m_next_retry = 0;
};

inline auto g_skybox_changer = std::make_unique<c_skybox_changer>( );
