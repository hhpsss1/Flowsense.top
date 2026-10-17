#pragma once

#include <includes.h>

class c_render_system_hooks {
public:
	static HRESULT WINAPI present( IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags );
	static HRESULT __fastcall resize_buffers( IDXGISwapChain* swap_chain, uint32_t buffer_count, uint32_t width, uint32_t height, DXGI_FORMAT new_format, uint32_t flags );
	static HRESULT WINAPI create_swap_chain( IDXGIFactory* factory, IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** swap_chain );
	static long wnd_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
};