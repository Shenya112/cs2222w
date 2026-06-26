#pragma once
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <sdk/interfaces/material_system.h>

static const char* arr_skybox_names[] = {
"Default",
"Sunset",
"Overcast",
"Night",
"Aztec",
"Jungle",
"Moon",
"Hell",
"The space",
"Stars",
"Aurora",
"Space gas",
"Space cells",
"Stones"
};

static constexpr std::array<std::string_view, 13> arr_skybox_paths = {
     "materials/skybox/cs_italy_s2_skybox_sunset_2_exr_e56cedf6.vtex",
     "materials/skybox/sky_overcast_01_exr_da4019b1.vtex",
     "materials/skybox/tests/src/lightingtest_sky_night_exr_2c5e8c62.vtex",
     "materials/skybox/sky_hr_aztec_02_exr_f84f8de9.vtex",
     "materials/skybox/jungle_cube_pfm_bc16d813.vtex",
     "materials/skybox/mr_moon_cube_pfm_f3262f9.vtex",
     "materials/skybox/hell_hdri_png_795ff36e.vtex",
     "materials/skybox/mr_21_cube_pfm_ea3e7de9.vtex",
     "materials/skybox/starmap_random_2020_4k_exr_5cc2c022.vtex",
     "materials/skybox/auroraborealis_cube_pfm_84850c18.vtex",
     "materials/skybox/space_skybox_jpg_6e5f57ce.vtex",
     "materials/skybox/dzy5_cube_pfm_ef2ee53f.vtex",
     "materials/skybox/space_13_cube_pfm_766b5180.vtex"
};


class c_skybox_changer {
public:
	void run( );
	void refresh_custom_skyboxes( );  
	std::vector<std::string> get_all_skyboxes( ); 
	void init_on_level_load( );
	void restore_default_skybox( );
	void cleanup( );

	bool m_need_update_material = false;
	c_material_2* m_default_skybox_material = nullptr;
	c_material_2* m_custom_skybox_material = nullptr;
	bool m_has_custom_skybox = false;
	int m_last_selected_skybox = -1;
private:
	std::vector<std::string> m_custom_skyboxes;
};

inline auto g_skybox_changer = std::make_unique<c_skybox_changer>( );
