#include "render_system_hooks.h"
#include <core/hooks/hooks.h>
#include <core/interfaces/interfaces.h>
#include <cheat/menu/menu.h>
#include <context.h>
#include <cheat/input.h>
#include <cheat/hooks/client/client_hooks.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/features/entity cache/entity_cache.h>
#include <cheat/features/ragebot/ragebot.h>
#include <cheat/menu/hell_gui/drawlist.h>
#include <cheat/features/visuals/overlay_features.h>
#include <cheat/menu/hell_gui/widgets/keybind.h>
#include <cheat/features/movement/movement.h>
#include <cheat/features/visuals/grenade.h>
#include <cheat/features/visuals/visual_events.h>
#include <cheat/features/visuals/dropped_weapons.h>
#include <cheat/features/misc/autobuy.h>
#include <cheat/features/skins/skins.h>
#include <cheat/features/misc/reveal_equipment.h>

HRESULT __stdcall c_render_system_hooks::present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
	static const auto original = g_hooks->m_render_system.m_present.get<decltype(&present)>();
	static bool imgui_initialized = false;
    if(g_interfaces->m_swap_chain->m_dxgi_swap_chain!=swap_chain) {
        if (imgui_initialized) {
            ImGui_ImplDX11_Shutdown();
            imgui_initialized = false;
        }
        g_interfaces->destroy_render_target();
        if(g_interfaces->m_device_context) g_interfaces->m_device_context->Release();
        if(g_interfaces->m_device) g_interfaces->m_device->Release();
        g_interfaces->m_device=nullptr;
        g_interfaces->m_device_context=nullptr;
        g_interfaces->m_swap_chain->m_dxgi_swap_chain->Release();
        swap_chain->AddRef();
        g_interfaces->m_swap_chain->m_dxgi_swap_chain=swap_chain;
    }

	if (!g_interfaces->m_render_target_view)
		g_interfaces->create_render_target();

	if (g_interfaces->m_render_target_view && g_interfaces->m_device_context)
		g_interfaces->m_device_context->OMSetRenderTargets(1, &g_interfaces->m_render_target_view, nullptr);

	try {
		if (!imgui_initialized) {
			if (!g_menu->m_init) {
				g_menu->init();
			}
            if (g_menu->m_init && g_interfaces->m_device && g_interfaces->m_device_context) {
                if (!ImGui::GetIO().BackendRendererUserData)
                    imgui_initialized = ImGui_ImplDX11_Init(g_interfaces->m_device, g_interfaces->m_device_context);
                else
                    imgui_initialized = true;
            }
		}

		if (imgui_initialized && g_interfaces->m_device_context) {
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			static bool logged_present = false;
			if (!logged_present) { LOG("present: first frame"); logged_present = true; }

			g_menu->render();
			static bool logged_menu = false;
			if (!logged_menu) { LOG("present: menu rendered"); logged_menu = true; }
			// without a matching client.dll build only the menu runs:
			// every feature below touches client.dll-dependent state.
			if (!g_ctx->m_client_valid) {
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				return original(swap_chain, sync_interval, flags);
			}
			g_autobuy->update();
			g_skins->update();
			g_overlay->present();
			static bool logged_overlay = false;
			if (!logged_overlay) { LOG("present: overlay done"); logged_overlay = true; }
			g_GrenadePrediction->Run();
			g_GrenadePrediction->ProximityWarning();
			g_GrenadePrediction->DrawGrenadeNames();
			g_dropped_weapons->draw();
			g_visual_events->create_delayed_tracer();
			hell::drawlist::on_present();
			g_visuals->present();
			hell::render_keybind_list(GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds)));

			ImGui::EndFrame();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
	}
	catch (const std::exception& ex) {
	}

	return original(swap_chain, sync_interval, flags);
}

HRESULT __fastcall c_render_system_hooks::resize_buffers(IDXGISwapChain* swap_chain, uint32_t buffer_count, uint32_t width, uint32_t height, DXGI_FORMAT new_format, uint32_t flags) {
	static const auto original = g_hooks->m_render_system.m_resize_buffers.get<decltype(&resize_buffers)>();

	g_interfaces->destroy_render_target();
	auto result = original(swap_chain, buffer_count, width, height, new_format, flags);
	if (SUCCEEDED(result))
		g_interfaces->create_render_target();

	return result;
}

HRESULT __stdcall c_render_system_hooks::create_swap_chain(IDXGIFactory* factory, IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** swap_chain) {
	static const auto original = g_hooks->m_render_system.m_create_swap_chain.get<decltype(&create_swap_chain)>();

	g_interfaces->destroy_render_target();

	return original(factory, device, desc, swap_chain);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
long c_render_system_hooks::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	g_input->wnd_proc(msg, wparam, lparam);

	if (g_input->key_released(VK_INSERT) || g_input->key_released(VK_DELETE)) {
		g_menu->m_menu_open = !g_menu->m_menu_open;

		static const auto fn_is_relative_mouse_mode = g_hooks->m_client.m_is_relative_mouse_mode.get<decltype(&c_client_hooks::is_relative_mouse_mode)>();
		if (fn_is_relative_mouse_mode)
			fn_is_relative_mouse_mode(g_interfaces->m_input_system, g_menu->m_menu_open ? false : g_menu->m_input_active);
	}

	if (g_menu->m_menu_open) {
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		if ((wparam != 'R' &&
			wparam != '1' &&
			wparam != '2' &&
			wparam != '3' &&
			wparam != '4' &&
			wparam != '5' &&
			wparam != 'W' &&
			wparam != 'A' &&
			wparam != 'S' &&
			wparam != 'D' &&
			wparam != VK_SHIFT &&
			wparam != VK_CONTROL &&
			wparam != VK_TAB &&
			wparam != VK_SPACE) ||
			ImGui::GetIO().WantTextInput)
		{
			return true;
		}
	}

	if (g_menu->m_menu_open && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEMOVE))
		return false;

	return static_cast<long>(CallWindowProcW(g_ctx->m_old_wnd_proc, hwnd, msg, wparam, lparam));
}