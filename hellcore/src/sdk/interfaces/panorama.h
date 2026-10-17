#pragma once

#include <context.h>

// Thanks EB


struct c_panel_2d {
	
};

struct c_top_level_window_source_2 {

};

//class c_panel_style {
//
//};

class c_ui_panel {
public:
	void* m_vtable; //0x0000
	c_panel_2d* m_panel; //0x0008
	char* m_panel_name; //0x0010
	c_panel_2d* m_pParentPanel; //0x0018
	c_top_level_window_source_2* m_ui_window; //0x0020
	int32_t m_child_ui_panel_count; //0x0028
	PAD( 4 ); //0x002C
	c_ui_panel* m_children_ui_panel_count; //0x0030
	PAD( 28 ); //0x0038
	float m_width; //0x0054
	float m_height; //0x0058
}; 

struct panel_data_t {
	PAD( 8 );
	uint32_t m_flags;
	uint32_t m_visible;
	c_ui_panel* m_panel;
	uint32_t m_panel_id;
	PAD( 4 );
};

class c_ui_engine {
public:
	PAD(160); //0x0000
	int32_t m_top_level_window_count; //0x00A0
	PAD(4); //0x00A4
	c_top_level_window_source_2** m_top_level_window_array; //0x00A8
	PAD(88); //0x00B0
	void* m_style_factory_wrapper; //0x0108
	PAD(0x228); //0x0110
	panel_data_t* m_panels_array; //0x0130
	int32_t m_panel_count; //0x0138
	PAD(84); //0x013C
	void* m_ui_input_engine; //0x0190
	void* m_layout_manager; //0x0198
	PAD(1704); //0x01A0
		
	// @eb: Runs script within the context of a given panel
	void run_script( c_ui_panel* panel, const char* script ) {
		g_memory->call_virtual<void>( this, 79, panel, script, nullptr, 0 );
		return;
	}
};

class c_panorama_ui_engine {
private:
	PAD( 40 ); //0x0000
	c_ui_engine* m_ui_engine; //0x0028
public:
	c_ui_engine* get_ui_engine( ) { return g_memory->call_virtual<c_ui_engine*>( this, 13 ); }
};