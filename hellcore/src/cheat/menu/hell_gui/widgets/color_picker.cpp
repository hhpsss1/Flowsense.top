#include "color_picker.h"
#include <cheat/menu/animation/anim.h>
#include <cheat/menu/hell_gui/colors.h>
#include "checkbox.h"

using namespace ImGui;

static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V) {
	ImGuiContext& g = *GImGui;
	IM_ASSERT(g.ColorEditCurrentID != 0);
	if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ColorConvertFloat4ToU32(hellvec4(col[0], col[1], col[2], 0)))
		return;

	// When S == 0, H is undefined.
	// When H == 1 it wraps around to 0.
	if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
		*H = g.ColorEditSavedHue;

	// When V == 0, S is undefined.
	if (*V == 0.0f)
		*S = g.ColorEditSavedSat;
}

bool ColorButton(const char* szDescId, const hellvec4& imCol, ImGuiColorEditFlags imColorEditFlags, const hellvec2& imSize) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(szDescId);
	const float default_size = GetFrameHeight();
	const hellvec2 size(imSize.x == 0.0f ? default_size : imSize.x, imSize.y == 0.0f ? default_size : imSize.y);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (imColorEditFlags & ImGuiColorEditFlags_NoAlpha)
		imColorEditFlags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

	hellvec4 col_rgb = imCol;
	if (imColorEditFlags & ImGuiColorEditFlags_InputHSV)
		ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

	float grid_step = ImMin(size.x, size.y) / 2.99f;
	float rounding = 2;
	ImRect bb_inner = bb;
	float off = 0.0f;
	if ((imColorEditFlags & ImGuiColorEditFlags_NoBorder) == 0) {
		off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
		bb_inner.Expand(off);
	}

	RenderColorRectWithAlphaCheckerboard(window->DrawList, bb_inner.Min, bb_inner.Max, GetColorU32(imCol), grid_step, hellvec2(off, off), rounding);
	window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(imCol), rounding);

	RenderNavCursor(bb, id);

	return pressed;
}

static void RenderArrowsForVerticalBar(ImDrawList* draw_list, hellvec2 pos, hellvec2 half_sz, float bar_w, float alpha) {
	ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + half_sz.x + 1, pos.y), hellvec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + bar_w - half_sz.x - 1, pos.y), hellvec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
}

static bool color_button(const char* desc_id, hellcolor col, hellvec2 position, hellvec2 size_arg = { }) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(desc_id);

	const float default_size = GetFrameHeight() - 2;
	hellvec2 size = {
		size_arg.x == 0.0f ? default_size : size_arg.x,
		size_arg.y == 0.0f ? default_size : size_arg.y
	};

	const ImRect bb(position, position + size);
	//ItemSize( bb );
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	hellvec2 center = (bb.Min + bb.Max) * 0.5f;
	float radius = ImMin(size.x, size.y) * 0.3f;

	window->DrawList->AddCircleFilled(center, radius, col, 32);
	window->DrawList->AddCircle(center, radius + 1.f, hellcolor(80, 80, 80, 200), 32);

	return pressed;
}

bool hell::color_picker(const char* id, hellcolor& color, int pickers_inline, bool checkbox_in_line, bool alpha_bar) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	PushID(id);

	const float picker_size = GetFrameHeight();
	const float checkbox_width = hell::calc_checkbox_size();
	const float checkbox_offset = checkbox_in_line
		? checkbox_width + style.FramePadding.x
		: 0.0f;

	const float total_button_width = (picker_size - style.FramePadding.x) * pickers_inline;
	const float inline_offset = GetContentRegionAvail().x - checkbox_offset - total_button_width;

	const hellvec2 button_pos = { window->DC.CursorPos.x + inline_offset, window->DC.CursorPos.y - 1.f };

	hellvec4& col = color.Value;
	bool value_changed = false;

	if (color_button(xx("##ColorButton"), color, button_pos, { picker_size, picker_size })) {
		g.ColorPickerRef = col;
		OpenPopup(xx("picker"));
		SetNextWindowPos(g.LastItemData.Rect.GetBL() + hellvec2(0.0f, style.ItemSpacing.y));
	}

	SetNextWindowBgAlpha(1.0f);
	PushStyleColor(ImGuiCol_WindowBg, hellcolor(55, 55, 55).Value);
	PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

	if (BeginPopup(xx("picker"), ImGuiWindowFlags_NoMove)) {
		hellvec4 temp = col;
		
		ImGuiColorEditFlags flags = ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV;
		if (!alpha_bar) flags |= ImGuiColorEditFlags_NoAlpha;
		
		if (ColorPicker4("##picker", (float*)&temp, flags)) {
			col = temp;
			value_changed = true;
		}
		EndPopup();
	}

	PopStyleColor();
	PopStyleVar();

	PopID();
	return value_changed;
}