#include "slider.h"
#include <cheat/menu/hell_gui/colors.h>
#include <cheat/menu/hell_gui/hell_gui.h>
#include "keybind.h"

using namespace ImGui;

bool hell::slider_int( const char* label, fnv1a_t holder_id, int min, int max, int custom_id, bool force_custom_id, const char* format, ImGuiSliderFlags flags ) {
	_registered_internal::register_min_max( holder_id, min, max );

	return slider_scalar( label, ImGuiDataType_S32, holder_id, &min, &max, format, flags, custom_id, force_custom_id, false );
}

bool hell::slider_int_passthrough( const char* label, int* v, int min, int max, int custom_id, bool force_custom_id, const char* format, ImGuiSliderFlags flags ) {
	return slider_scalar( label, ImGuiDataType_S32, (void*)v, &min, &max, format, flags, custom_id, force_custom_id, true );
}

bool hell::slider_float( const char* label, fnv1a_t holder_id, float min_, float max_, int custom_id, bool force_custom_id, const char* format, ImGuiSliderFlags flags ) {

	static float min, max;
	min = min_;
	max = max_;
	_registered_internal::register_min_max( holder_id, min_, max_ );
	return slider_scalar( label, ImGuiDataType_Float, holder_id, &min, &max, format, flags, custom_id, force_custom_id, false );
}

bool hell::slider_float_passthrough( const char* label, float* v, float min, float max, int custom_id, bool force_custom_id, const char* format, ImGuiSliderFlags flags ) {
	return slider_scalar( label, ImGuiDataType_Float, (void*)v, &min, &max, format, flags, custom_id, force_custom_id, true );
}

bool hell::slider_none( const char* label, fnv1a_t holder_id, int min, int max, int custom_id, const char* format, ImGuiSliderFlags flags ) {
	char displayBuf[ 64 ];

	int& val = GET_VAR( int, holder_id );

	if ( val <= min )
		_snprintf_s( displayBuf, sizeof( displayBuf ), xx( "None" ) );
	else
		_snprintf_s( displayBuf, sizeof( displayBuf ), format, val );

	_registered_internal::register_min_max( holder_id, min, max );

	return slider_scalar( label, ImGuiDataType_S32, holder_id, &min, &max, displayBuf, flags, custom_id, false, false );
}

bool hell::min_dmg_slider( const char* label, fnv1a_t holder_id, int min, int max, int custom_id, const char* format, ImGuiSliderFlags flags ) {
	char displayBuf[ 64 ];

	int& val = GET_VAR( int, holder_id );

	if ( val > 100 )
		_snprintf_s( displayBuf, sizeof( displayBuf ), xx( "hp + %d" ), val - 100 );
	else
		_snprintf_s( displayBuf, sizeof( displayBuf ), xx( "%dhp" ), val );

	_registered_internal::register_min_max( holder_id, min, max );

	return slider_scalar( label, ImGuiDataType_S32, holder_id, &min, &max, displayBuf, flags, custom_id, false, false );
}

bool hell::slider_scalar( const char* label, ImGuiDataType data_type, std::variant<fnv1a_t, void*> v, const void* min, const void* max, const char* format, ImGuiSliderFlags flags, int custom_id, bool force_custom_id, bool passthrough ) {
	static SliderObj_t obj;

	fnv1a_t holder_id = -1;
	e_keybind_type keybind_type = e_keybind_type::keybind_type_unset;
	void* data = nullptr;

	if ( passthrough ) {
		data = std::get<void*>( v );
	} else {
		holder_id = std::get<fnv1a_t>( v );
		keybind_type = ( data_type == ImGuiDataType_S32 )
			? e_keybind_type::keybind_type_slider_int
			: e_keybind_type::keybind_type_slider_float;
		data = ( keybind_type == e_keybind_type::keybind_type_slider_int )
			? (void*)&GET_VAR( int, holder_id )
			: (void*)&GET_VAR( float, holder_id );
	}

	ImGuiWindow* im_window = GetCurrentWindow( );
	if ( im_window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID im_id = force_custom_id ? custom_id : im_window->GetID( label ) + custom_id;
	const hellvec2 im_label_size = CalcTextSize( label, NULL, true );
	const float padding = 6.0f;
	const float slider_height = im_label_size.y;

	hellvec2 im_label_pos = im_window->DC.CursorPos;
	im_label_pos.y += 1;

	float slider_width = hell::config::m_widget_width;
	float slider_start_x = im_window->WorkRect.Max.x - slider_width;

	ImRect slider_pos = { { slider_start_x, im_label_pos.y }, { slider_start_x + slider_width, im_label_pos.y + slider_height } };

	ImRect im_frame_bb( slider_pos.Min, { slider_start_x + slider_width, im_label_pos.y + GetFrameHeight( ) - 2.0f } );
	const ImRect im_total_bb( im_label_pos, im_frame_bb.Max );

	const bool temp_input_allowed = ( flags & ImGuiSliderFlags_NoInput ) == 0;
	ItemSize( im_total_bb, style.FramePadding.y );
	if ( !ItemAdd( im_total_bb, im_id, &im_frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0 ) )
		return false;

	if ( format == NULL )
		format = DataTypeGetInfo( data_type )->PrintFmt;

	bool hovered = false, held = false;
	bool clicked = ButtonBehavior( im_total_bb, im_id, &hovered, &held );
	const bool item_active = IsItemActive( );

	bool temp_input_active = temp_input_allowed && TempInputIsActive( im_id );
	if ( !temp_input_active ) {
		const bool make_active = ( clicked || g.NavActivateId == im_id );
		if ( make_active && clicked )
			SetKeyOwner( ImGuiKey_MouseLeft, im_id );
		if ( make_active ) {
			SetActiveID( im_id, im_window );
			SetFocusID( im_id, im_window );
			FocusWindow( im_window );
			g.ActiveIdUsingNavDirMask |= ( 1 << ImGuiDir_Left ) | ( 1 << ImGuiDir_Right );
		}
	}

	ImRect im_grab_bb;
	const bool value_changed = SliderBehavior( im_frame_bb, im_id, data_type, data, min, max, format, flags, &im_grab_bb );
	if ( value_changed )
		MarkItemEdited( im_id );

	// Get current value as float for animation
	float current_value = 0.0f;
	if ( data_type == ImGuiDataType_S32 )
		current_value = (float)( *(int*)data );
	else if ( data_type == ImGuiDataType_Float )
		current_value = *(float*)data;
	
	float* animated_value = ImGui::GetStateStorage( )->GetFloatRef( im_id + 1, current_value );
	*animated_value = ImLerp( *animated_value, current_value, ImGui::GetIO( ).DeltaTime * obj.flAnimationTime_Active );

	float target_alpha = hell::colors::m_text_unhovered.Value.w;
	if ( item_active )
		target_alpha = hell::colors::m_text_selected.Value.w;
	else if ( hovered )
		target_alpha = hell::colors::m_text_hovered.Value.w;

	float* label_alpha = ImGui::GetStateStorage( )->GetFloatRef( im_id + 2000, target_alpha );
	float label_anim_speed = ImGui::GetIO( ).DeltaTime * obj.flAnimationTime_Hover;
	*label_alpha = ImLerp( *label_alpha, target_alpha, label_anim_speed );

	float* value_alpha = ImGui::GetStateStorage( )->GetFloatRef( im_id + 3000, 0.0f );
	float value_speed = ImGui::GetIO( ).DeltaTime * obj.flAnimationTime_Value;
	*value_alpha = ImClamp( *value_alpha + ( ( hovered || item_active ) ? value_speed : -value_speed ), 0.0f, 1.0f );

	// Calculate fraction based on data type
	float min_f, max_f;
	if ( data_type == ImGuiDataType_S32 ) {
		min_f = (float)( *(int*)min );
		max_f = (float)( *(int*)max );
	} else {
		min_f = *(float*)min;
		max_f = *(float*)max;
	}
	float fraction = ImClamp( ( *animated_value - min_f ) / ( max_f - min_f ), 0.0f, 1.0f );
	float animated_x = ImLerp( slider_pos.Min.x, slider_pos.Max.x, fraction );
	hellvec2 end_slider_pos = hellvec2( animated_x, slider_pos.Max.y );

	// background
	im_window->DrawList->AddRectFilled(
		slider_pos.Min + hellvec2( 0, 3 ),
		slider_pos.Max - hellvec2( 0, 3 ),
		obj.imBackgroundColor,
		obj.flFrameRounding
	);

	// fill
	im_window->DrawList->AddRectFilled(
		hellvec2( slider_pos.Min.x, slider_pos.Min.y + 3 ),
		hellvec2( animated_x, slider_pos.Max.y - 3 ),
		GET_VAR(hellcolor, MISC_PATH(m_accent_color)),
		obj.flFrameRounding
	);

	if ( *value_alpha > 0.0f ) {
		char value_buf[ 64 ];
		const char* value_buf_end = value_buf + DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, data, format );
		hellvec2 value_text_size = CalcTextSize( value_buf, value_buf_end );

		hellvec2 padding = { 4.0f, 2.0f };
		hellvec2 box_size = value_text_size + padding * 2.0f;

		hellvec2 value_text_pos = hellvec2( animated_x - box_size.x / 2.0f + padding.x, im_frame_bb.Min.y - value_text_size.y - 6 );

		hellvec2 bg_min = value_text_pos - padding;
		hellvec2 bg_max = value_text_pos + value_text_size + padding;

		float box_left = bg_min.x;
		float box_right = bg_max.x;
		float slider_left = im_frame_bb.Min.x;
		float slider_right = im_frame_bb.Max.x;

		float shift = 0.0f;
		if ( box_left < slider_left )
			shift = slider_left - box_left;
		else if ( box_right > slider_right )
			shift = slider_right - box_right;

		value_text_pos.x += shift;
		bg_min.x += shift;
		bg_max.x += shift;

		GetForegroundDrawList( )->AddRectFilled( bg_min, bg_max, ImColor( 35, 35, 35, static_cast<int>( *value_alpha * 255.f ) ), 4.0f );
		GetForegroundDrawList( )->AddText( value_text_pos, ImColor( hell::colors::m_text_selected.Value.x, hell::colors::m_text_selected.Value.y, hell::colors::m_text_selected.Value.z, *value_alpha ), value_buf );
	}

	im_window->DrawList->AddText( im_label_pos, ImColor( hell::colors::m_text_selected.Value.x, hell::colors::m_text_selected.Value.y, hell::colors::m_text_selected.Value.z, *label_alpha ), label );

	if ( obj.bDrawDebug )
		im_window->DrawList->AddRect( im_total_bb.Min, im_total_bb.Max, IM_COL32( 255, 0, 0, 80 ) );

	if ( !passthrough ) {
		hell::keybind_handler( label, im_total_bb.GetCenter( ), holder_id, keybind_type );
		std::string id_string_popup = std::string( label ) + xx( "##popup_kb" );
		const char* popup_id = id_string_popup.c_str( );

		ImGuiID id = GetID( popup_id );

		if ( value_changed ) {
			auto& widget_keybinds = GET_VAR( kb_map_t, CONFIG_PATH( m_widget_keybinds ) );
			for ( auto& widget_keybind : widget_keybinds ) {
				if ( widget_keybind.first == id ) {
					for ( keybind_t& keybind : widget_keybind.second ) {
						keybind.m_default_val = GET_VAR( int, holder_id );
					}
				}
			}
		}
	}

	return value_changed;
}
