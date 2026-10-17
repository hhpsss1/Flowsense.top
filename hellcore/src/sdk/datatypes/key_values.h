#pragma once

#include <context.h>

#pragma once

enum e_kv3_hash_keys : uint64_t {
    kv3_unknown_key_hash = 0x31415926,
    kv3_id_unknown_hash1 = 0x469806E97412167C,
    kv3_id_unknown_hash2 = 0xE73790B53EE6F2AF
};

struct MaterialKeyVar_t {
    uint64_t m_key;
    const char* m_name;

    MaterialKeyVar_t( uint64_t key, const char* name ) :
        m_key( key ), m_name( name ) {
    }

    MaterialKeyVar_t( const char* name, bool should_find_key = false ) :
        m_name( name ) {
        m_key = should_find_key ? find_key( name ) : 0x0;
    }

    // helper ida: CBodyGameSystem::NotifyResourcePreReload
    uint64_t find_key(const char* name) {
        using fn_find_key_var = uint64_t(__fastcall*)(const char*, unsigned int, int);
        static fn_find_key_var find_key_var = g_modules->m_particles.find(xx("48 89 5C 24 ? 57 48 81 EC ? ? ? ? 33 C0 8B DA")).as<fn_find_key_var>();

        return find_key_var(name, 0x12U, 0x31415926);
    }
};

class c_utl_buffer {
public:
    PAD( 0x80 );

    c_utl_buffer( int a1, int size, int a3 ) {
        using fn_utl_buffer_init = void( __stdcall* )( c_utl_buffer*, int, int, int );
        static fn_utl_buffer_init utl_buffer_init = g_modules->m_tier0.find( xx( "40 55 56 41 57 48 83 EC ? 33 ED" ) ).as<fn_utl_buffer_init>( );

        utl_buffer_init( this, a1, size, a3 );
    }

    c_utl_buffer(void* a1, int size, int a3 ) {
        using fn_utl_buffer_init = void( __stdcall* )(c_utl_buffer*, void*, int, int);
        static fn_utl_buffer_init utl_buffer_init = g_modules->m_tier0.find( xx( "48 89 5C 24 ? 57 48 83 EC ? 33 FF 41 8B C0" ) ).as<fn_utl_buffer_init>( );

        utl_buffer_init( this, a1, size, a3);
    }

    void put_string(const char* string) {
        using fn_utl_buffer_put_string = void(__stdcall*)(c_utl_buffer*, const char*);
        static fn_utl_buffer_put_string utl_buffer_put_string = reinterpret_cast<fn_utl_buffer_put_string>(GetProcAddress(static_cast<HMODULE>(g_modules->m_tier0.get()),"?PutString@CUtlBuffer@@QEAAXPEBD@Z"));

        utl_buffer_put_string(this, string);
    }

    void ensure_capacity( int size ) {
        using fn_utl_buffer_ensure_capacity = void( __stdcall* )( c_utl_buffer*, int );
        static fn_utl_buffer_ensure_capacity utl_buffer_ensure_capacity = g_modules->m_tier0.find( xx( "48 89 74 24 ? 57 48 83 EC ? 8B 41 ? 8D 72" ) ).as<fn_utl_buffer_ensure_capacity>( );

        utl_buffer_ensure_capacity(this, size );
    }
};

struct kv3_id_t {
    const char* m_name;
    uint64_t m_hash1;
    uint64_t m_hash2;
};

class c_key_values_3 {
public:
    uint64_t m_key{};
    void* m_value{};

    void load_from_buffer(const char* text) {
 using fn_t=bool(__fastcall*)(c_key_values_3*,void*,const char*,const kv3_id_t&,const char*,unsigned);
 static auto fn=reinterpret_cast<fn_t>(GetProcAddress(static_cast<HMODULE>(g_modules->m_tier0.get()),"?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z"));
 const kv3_id_t id{"generic",kv3_id_unknown_hash1,kv3_id_unknown_hash2};
 if(fn && text) fn(this,nullptr,text,id,"",0);
}

    static c_key_values_3* create_material_resource() {
 using fn_t=c_key_values_3*(__fastcall*)(c_key_values_3*,unsigned char,unsigned char);
 static auto fn=g_modules->m_tier0.find(xx("40 53 48 83 EC 30 80 FA 06 0F B6 C2 41 B9 16")).as<fn_t>();
 auto* value=new c_key_values_3{};
 return fn ? fn(value,6,1) : nullptr;
}

    bool load_kv3(c_utl_buffer* buffer) {
 using fn_t=bool(__fastcall*)(c_key_values_3*,void*,c_utl_buffer*,const kv3_id_t&,const char*,unsigned);
 static auto fn=reinterpret_cast<fn_t>(GetProcAddress(static_cast<HMODULE>(g_modules->m_tier0.get()),"?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEAVCUtlBuffer@@AEBUKV3ID_t@@PEBDI@Z"));
 const kv3_id_t id{"generic",kv3_id_unknown_hash1,kv3_id_unknown_hash2};
 return fn && buffer && fn(this,nullptr,buffer,id,"",0);
}
};
    