#pragma comment(lib,"d3d11.lib")
#include "interfaces.h"
#include <core/hooks/modules.h>
#include <context.h>

template<typename T>
inline T c_interfaces::find_interface( void* module_handle, const char* name ) {
	using fn = T( * )( const char*, int* );
	const auto func = reinterpret_cast<fn>( GetProcAddress( (HMODULE)module_handle, ( xx( "CreateInterface" ) ) ) );
	if ( !func )
		return nullptr;

	auto res = func( name, nullptr );

	LOG( std::format( "{}{} at {}", xx( "found interface: " ), name, static_cast<void*>( res ) ).c_str( ) );

	return res;
}

void c_interfaces::init( void ) {
    static c_swap_chain_dx_11 bootstrap{};
    m_swap_chain=&bootstrap;
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount=1;
    desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // game window first: GetForegroundWindow at inject time is often the
    // injector itself or nothing, which fails swapchain creation.
    desc.OutputWindow = g_ctx->m_window ? g_ctx->m_window : GetForegroundWindow();
    if (!desc.OutputWindow)
        desc.OutputWindow = GetDesktopWindow();
    desc.SampleDesc.Count=1;
    desc.Windowed=TRUE;
    desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;

    // the dummy swapchain is only used for its vtable (shared by every
    // DX11 swapchain), so any driver type works. HARDWARE can fail on
    // weak GPUs, RDP sessions or VMs and during game startup: fall back
    // to WARP and retry instead of dying.
    static const D3D_DRIVER_TYPE driver_fallback[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    HRESULT hr = E_FAIL;
    for (int attempt = 0; attempt < 6 && FAILED(hr); ++attempt) {
        if (attempt > 0)
            Sleep(300);
        for (auto driver : driver_fallback) {
            if (m_device) { m_device->Release(); m_device = nullptr; }
            if (m_device_context) { m_device_context->Release(); m_device_context = nullptr; }
            if (bootstrap.m_dxgi_swap_chain) { bootstrap.m_dxgi_swap_chain->Release(); bootstrap.m_dxgi_swap_chain = nullptr; }
            hr = D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                &desc, &bootstrap.m_dxgi_swap_chain, &m_device, nullptr, &m_device_context);
            if (SUCCEEDED(hr) && bootstrap.m_dxgi_swap_chain)
                break;
        }
    }
    if(FAILED(hr) || !bootstrap.m_dxgi_swap_chain) {
        LOG_ERROR("D3D11 bootstrap failed");
        MessageBoxA(nullptr, "hellcore: D3D11 bootstrap failed (no usable GPU driver)", "hellcore", MB_OK | MB_ICONERROR);
        return;
    }
	using fn_get_csgo_input = c_csgo_input * (__fastcall*)();
	auto csgo_input_fn = g_modules->m_client.find(xx("E8 ? ? ? ? 48 8B 56 ? 48 8B C8 E8 ? ? ? ? 4C 89 7E")).relative(1, 5).as<fn_get_csgo_input>();
	this->m_csgo_input = csgo_input_fn ? csgo_input_fn() : nullptr;
	if (!this->m_csgo_input)
		LOG_ERROR("failed to resolve csgo_input");
	refresh_global_vars();
    refresh_level_state();


	//this->m_hitbox_system = *g_modules->m_client.find( xx( "48 8B 0D ? ? ? ? 48 8B D0 C7 44 24 ? ? ? ? ? 48 C7 44 24 ? ? ? ? ? E8 ? ? ? ? F3 0F 10 85" ) ).relative( 3, 7 ).as<c_hitbox_system**>( );


	this->m_game_event_manager = nullptr;
	if (auto ev_slot = g_modules->m_client.find(xx("48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50 ? 4C 8B 05")).relative(3, 7).as<c_game_event_manager2**>(); ev_slot)
		this->m_game_event_manager = *ev_slot;
	if (!this->m_game_event_manager)
		LOG_ERROR("failed to resolve game_event_manager");
	this->m_pvs = g_modules->m_engine2.find(xx("48 8D 0D ? ? ? ? 33 D2 FF 50")).relative(3, 7).as<c_engine_pvs_manager*>();



	this->m_schema_system = find_interface<c_schema_system*>(g_modules->m_schemasystem.get(), xx("SchemaSystem_001"));
	this->m_vphysics2 = find_interface<c_vhpysics*>(g_modules->m_vphysics.get(), xx("VPhysics2_Interface_001"));
	this->m_input_system = find_interface<c_input_system*>(g_modules->m_inputsystem.get(), xx("InputSystemVersion001"));

	this->m_localize = find_interface<c_localize*>(g_modules->m_localize.get(), xx("Localize_001"));
	this->m_engine_convar = find_interface<c_engine_cvar*>(g_modules->m_tier0.get(), xx("VEngineCvar007"));

	this->m_material_system = find_interface<c_material_system_2*>(g_modules->m_materialsystem2.get(), xx("VMaterialSystem2_001"));
	this->m_network_client_services = find_interface<c_network_game_client_services*>(g_modules->m_engine2.get(), xx("NetworkClientService_001"));
	this->m_engine = find_interface<c_engine_client*>(g_modules->m_engine2.get(), xx("Source2EngineToClient001"));
	this->m_prediction = find_interface<c_prediction*>(g_modules->m_client.get(), xx("Source2ClientPrediction001"));
	this->m_source2_client = find_interface<c_source2_client*>(g_modules->m_client.get(), xx("Source2Client002"));
	this->m_file_system = find_interface<c_file_system*>(g_modules->m_filesystem_stdio.get(), xx("VFileSystem017"));
	this->m_mesh_system = find_interface<c_mesh_system*>(g_modules->m_mesh_system.get(), xx("MeshSystem001"));
	this->m_scene_system = find_interface<c_scene_system*>(g_modules->m_scenesystem.get(), xx("SceneSystem_002"));
	this->m_resource_system = find_interface<c_resource_system*>(g_modules->m_resourcesystem.get(), xx("ResourceSystem013"));
	this->m_particle_system_mgr = find_interface<c_particle_system_mgr*>(g_modules->m_particles.get(), xx("ParticleSystemMgr003"));
	this->m_panorama_ui_engine = find_interface<c_panorama_ui_engine*>(g_modules->m_panorama.get(), xx("PanoramaUIEngine001"));

	auto steamApi = GetModuleHandleA("steam_api64.dll");
	if (!steamApi) {
		LOG_ERROR("steam_api64.dll not loaded");
		return;
	}

	auto steam_client_fn = reinterpret_cast<fn_steam_client>(GetProcAddress((HMODULE)steamApi, ("SteamClient")));
	auto get_user_fn = reinterpret_cast<tt>(GetProcAddress((HMODULE)steamApi, ("SteamAPI_GetHSteamUser")));
	auto get_pipe_fn = reinterpret_cast<tt>(GetProcAddress((HMODULE)steamApi, ("SteamAPI_GetHSteamPipe")));
	if (!steam_client_fn || !get_user_fn || !get_pipe_fn) {
		LOG_ERROR("steam exports missing");
		return;
	}

	ISteamClient* m_steam_client = steam_client_fn();

	std::uint64_t steam_user = get_user_fn();
	std::uint64_t steam_pipe = get_pipe_fn();
	if (!m_steam_client) {
		LOG_ERROR("ISteamClient null");
		return;
	}

	this->m_steam_friends = m_steam_client->GetISteamFriends((std::uint64_t)steam_user, (std::uint64_t)steam_pipe, "SteamFriends017");
	this->m_steam_utils = m_steam_client->GetISteamUtils((std::uint64_t)steam_pipe, "SteamUtils010");
}

void c_interfaces::create_render_target( ) {
	destroy_render_target();


	if ( !m_swap_chain || !m_swap_chain->m_dxgi_swap_chain ) {
		LOG_ERROR( xx( "invalid swap chain" ) );
		return;
	}

	if ( !m_device ) {
		if ( FAILED( m_swap_chain->m_dxgi_swap_chain->GetDevice( __uuidof( ID3D11Device ), (void**)&m_device ) ) ) {
			LOG_ERROR( xx( "failed to get device" ) );
			return;
		}
	}

	if ( !m_device ) {
		LOG_ERROR( xx( "device is null" ) );
		return;
	}

	if ( !m_device_context ) {
		m_device->GetImmediateContext( &m_device_context );
	}

	if ( !m_device_context ) {
		LOG_ERROR( xx( "failed to get device context" ) );
		return;
	}

	static const auto GetCorrectDXGIFormat = [ ]( DXGI_FORMAT eCurrentFormat ) {
		switch ( eCurrentFormat ) {
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		return eCurrentFormat;
		};

	DXGI_SWAP_CHAIN_DESC sd;
	if ( FAILED( m_swap_chain->m_dxgi_swap_chain->GetDesc( &sd ) ) ) {
		LOG_ERROR( xx( "failed to get swap chain desc" ) );
		return;
	}

	ID3D11Texture2D* pBackBuffer = nullptr;
	HRESULT hr = m_swap_chain->m_dxgi_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
	if ( FAILED( hr ) ) {
		LOG_ERROR( xx( "failed to get back buffer" ) );
		return;
	}

	if ( pBackBuffer ) {
		D3D11_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = static_cast<DXGI_FORMAT>( GetCorrectDXGIFormat( sd.BufferDesc.Format ) );
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		
		hr = m_device->CreateRenderTargetView( pBackBuffer, &desc, &m_render_target_view );
		if ( FAILED( hr ) ) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			hr = m_device->CreateRenderTargetView( pBackBuffer, &desc, &m_render_target_view );
			if ( FAILED( hr ) ) {
				LOG_ERROR( xx( "failed to create render target view with D3D11_RTV_DIMENSION_TEXTURE2D" ) );
				LOG_ERROR( xx( "retrying.." ) );

				hr = m_device->CreateRenderTargetView( pBackBuffer, NULL, &m_render_target_view );
				if ( FAILED( hr ) ) {
					LOG_ERROR( xx( "failed to create render target view" ) );
				}
			}
		}
		
		pBackBuffer->Release( );
		pBackBuffer = nullptr;
	}
}

void c_interfaces::destroy_render_target( ) {
	if ( m_render_target_view ) {
		m_render_target_view->Release( );
		m_render_target_view = nullptr;
	}
}

bool c_interfaces::refresh_global_vars() {
    static auto** slot = g_modules->m_client.find(xx("48 89 15 ?? ?? ?? ?? 48 89 42")).relative(3, 7).as<c_global_variables**>();
    m_global_vars = slot ? *slot : nullptr;
    return m_global_vars != nullptr;
}

void c_interfaces::refresh_level_state() {
    static auto** rules_slot = g_modules->m_client.find(xx("48 8B 1D ? ? ? ? 48 8D 54 24 ? 0F 28 D0 48 8D 4C 24 ?")).relative(3, 7).as<c_game_rules**>();
    m_game_rules = rules_slot ? *rules_slot : nullptr;
    static auto** m_entity_system_slot = g_modules->m_client.find(xx("48 8B 0D ? ? ? ? 48 8B D3 E8 ? ? ? ? FF C5")).relative(3, 7).as<c_entity_system**>();
    m_entity_system = m_entity_system_slot ? *m_entity_system_slot : nullptr;
    static auto** m_phys2world_slot = g_modules->m_client.find(xx("4C 8B 35 ? ? ? ? 24 ? 0C ? 66 0F 7F 45 ? 88 45 ? 48 8B CB 48 8D 05 ? ? ? ? 89 7D ? 48 89 45 ? 89 7D ? C7 45 ? ? ? ? ? 66 C7 45 ? ? ? 44 88 7D")).relative(3, 7).as<c_vphys2world**>();
    m_phys2world = m_phys2world_slot ? *m_phys2world_slot : nullptr;
    static auto** m_world_slot = g_modules->m_client.find(xx("4C 8B 05 ? ? ? ? 4D 85 C0 74 ? 83 F9 ? 74 ? 8B C1 25 ? ? ? ? 8B D0 48 C1 E8 ? ? ? ? ? 4D 85 DB")).relative(3, 7).as<c_world**>();
    m_world = m_world_slot ? *m_world_slot : nullptr;
    static auto** m_game_particle_manager_slot = g_modules->m_client.find(xx("48 8B 0D ? ? ? ? 41 B8 ? ? ? ? F3 0F 11 74 24")).relative(3, 7).as<c_game_particle_manager**>();
    m_game_particle_manager = m_game_particle_manager_slot ? *m_game_particle_manager_slot : nullptr;
}
