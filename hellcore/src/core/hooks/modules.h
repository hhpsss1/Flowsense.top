#pragma once
#include <cstdint>
#include <memory>
#include <string_view>
#include <Windows.h>
#include <vector>
#include <utils/encryption/xor.h>

struct address_t {
private:
	uintptr_t m_pointer;

public:
	address_t( ) : m_pointer( 0x0 ) {}

	template<typename T = uintptr_t>
	address_t( T pointer )
	{
		this->m_pointer = reinterpret_cast< uintptr_t >( pointer );
	}

	address_t add( ptrdiff_t offset )
	{
		if ( m_pointer < 0x10000 )
			return address_t( );
		m_pointer += offset;
		return *this;
	}

	address_t sub( ptrdiff_t offset )
	{
		if ( m_pointer < 0x10000 )
			return address_t( );
		m_pointer -= offset;
		return *this;
	}

	address_t deref( )
	{
		if ( m_pointer < 0x10000 )
			return address_t( );
		m_pointer = *reinterpret_cast< uintptr_t* >( m_pointer );
		return *this;
	}

	address_t relative( int offset, int size )
	{
		if ( m_pointer < 0x10000 )
			return address_t( );
		m_pointer += *reinterpret_cast< int32_t* >( m_pointer + offset );
		m_pointer += size;

		return *this;
	}

	address_t absolute( int pre, int post ) {
		if ( m_pointer < 0x10000 )
			return address_t( );
		m_pointer += pre;
		m_pointer += sizeof( int32_t ) + *reinterpret_cast< int32_t* >( m_pointer );
		m_pointer += post;
		return *this;
	}

	bool valid( ) const {
		return m_pointer >= 0x10000;
	}

	template<typename T>
	T as( )
	{
		return reinterpret_cast< T >( m_pointer );
	}

	uintptr_t raw( )
	{
		return m_pointer;
	}
};

struct module_t {
private:
	void* m_ptr;
public:
	module_t( ) = default;

	module_t( const char* module_name ) {
		m_ptr = GetModuleHandleA( module_name );
	}

	void* get( ) { return m_ptr; }

	static std::vector<int> ida_to_bytes( const std::string_view& pattern )
	{
		std::vector<int> bytes{};

		char* start = const_cast< char* >( pattern.data( ) );
		char* end = const_cast< char* >( pattern.data( ) ) + pattern.size( );

		for ( char* current = start; current < end; ++current )
		{
			if ( *current == '?' )
			{
				++current;

				if ( *current == '?' )
					++current;

				bytes.push_back( -1 );
			}
			else
				bytes.push_back( strtoul( current, &current, 16 ) );
		}

		return bytes;
	}
	address_t find( std::string_view pattern ) const {
		if ( !m_ptr )
			return nullptr;
		PIMAGE_DOS_HEADER dos_header = reinterpret_cast< PIMAGE_DOS_HEADER >( m_ptr );
		PIMAGE_NT_HEADERS nt_headers = reinterpret_cast< PIMAGE_NT_HEADERS >( reinterpret_cast< uint8_t* >( m_ptr ) + dos_header->e_lfanew );

		auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
		auto pattern_bytes = ida_to_bytes( pattern );
		auto scan_bytes = reinterpret_cast< uint8_t* >( m_ptr );

		auto pattern_size = pattern_bytes.size( );
		auto pattern_data = pattern_bytes.data( );

		for ( int i = 0; i < size_of_image - pattern_size; i++ )
		{
			bool found = true;

			for ( int j = 0; j < pattern_size; ++j )
			{
				if ( pattern_data[ j ] == -1 )
					continue;

				if ( scan_bytes[ i + j ] != pattern_data[ j ] )
				{
					found = false;
					break;
				}
			}

			if ( found )
				return &scan_bytes[ i ];
		}

		return nullptr;
	}

	template <typename t = void*>
	t find( void* vtbl, int32_t index ) {
		return reinterpret_cast<t>( ( *static_cast<uintptr_t**>( vtbl ) )[ index ] );
	}
};

class c_modules {
public:
	module_t m_client{ };
	module_t m_server{ };
	module_t m_engine2{ };
	module_t m_schemasystem{ };
	module_t m_inputsystem{ };
	module_t m_sdl3{ };
	module_t m_tier0{ };
	module_t m_navsystem{ };
	module_t m_rendersystemdx11{ };
	module_t m_localize{ };
	module_t m_filesystem_stdio{ };
	module_t m_mesh_system{ };
	module_t m_particles{ };
	module_t m_panorama{ };
	module_t m_scenesystem{ };
	module_t m_materialsystem2{ };
	module_t m_matchmaking{ };
	module_t m_resourcesystem{ };
	module_t m_gameoverlayrenderer64{ };
	module_t m_animationsystem{ };
	module_t m_soundsystem{ };
	module_t m_worldrenderer{ };
	module_t m_vphysics{ };
	module_t m_steam_api{ };
public:
	void init( void ) {
		m_client = module_t( "client.dll"  );
		m_server = { xx( "server.dll" ) };
		m_engine2 = { xx( "engine2.dll" ) };
		m_schemasystem = { xx( "schemasystem.dll" ) };
		m_steam_api = { xx("steam_api64.dll") };
		m_inputsystem = { xx( "inputsystem.dll" ) };
		m_sdl3 = { xx( "SDL3.dll" ) };
		m_tier0 = { xx( "tier0.dll" ) };
		m_navsystem = { xx( "navsystem.dll" ) };
		m_rendersystemdx11 = { xx( "rendersystemdx11.dll" ) };
		m_localize = { xx( "localize.dll" ) };
		m_filesystem_stdio = { xx( "filesystem_stdio.dll" ) };
		m_mesh_system = { xx( "meshsystem.dll" ) };
		m_particles = { xx( "particles.dll" ) };
		m_panorama = { xx( "panorama.dll" ) };
		m_scenesystem = { xx( "scenesystem.dll" ) };
		m_materialsystem2 = { xx( "materialsystem2.dll" ) };
		m_matchmaking = { xx( "matchmaking.dll" ) };
		m_resourcesystem = { xx( "resourcesystem.dll" ) };
		m_gameoverlayrenderer64 = { xx( "gameoverlayrenderer64.dll" ) };
		m_animationsystem = { xx( "animationsystem.dll" ) };
		m_soundsystem = { xx( "soundsystem.dll" ) };
		m_worldrenderer = { xx( "worldrenderer.dll" ) };
		m_vphysics = { xx( "vphysics2.dll" ) };
	}
};

inline auto g_modules = std::make_unique<c_modules>( );
