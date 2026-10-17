#pragma once
#include <vector>
#include <utils/memory.h>
#include <sdk/datatypes/fnv1a.h>


enum cvar_flags {
	FCVAR_NONE = 0,
	FCVAR_UNREGISTERED = ( 1 << 0 ),
	FCVAR_DEVELOPMENTONLY = ( 1 << 1 ),
	FCVAR_GAMEDLL = ( 1 << 2 ),
	FCVAR_CLIENTDLL = ( 1 << 3 ),
	FCVAR_HIDDEN = ( 1 << 4 ),
	FCVAR_PROTECTED = ( 1 << 5 ),
	FCVAR_SPONLY = ( 1 << 6 ),
	FCVAR_ARCHIVE = ( 1 << 7 ),
	FCVAR_NOTIFY = ( 1 << 8 ),
	FCVAR_USERINFO = ( 1 << 9 ),
	FCVAR_CHEAT = ( 1 << 14 ),
	FCVAR_PRINTABLEONLY = ( 1 << 10 ),
	FCVAR_UNLOGGED = ( 1 << 11 ),
	FCVAR_NEVER_AS_STRING = ( 1 << 12 ),
	FCVAR_REPLICATED = ( 1 << 13 ),
	FCVAR_DEMO = ( 1 << 16 ),
	FCVAR_DONTRECORD = ( 1 << 17 ),
	FCVAR_RELOAD_MATERIALS = ( 1 << 20 ),
	FCVAR_RELOAD_TEXTURES = ( 1 << 21 ),
	FCVAR_NOT_CONNECTED = ( 1 << 22 ),
	FCVAR_MATERIAL_SYSTEM_THREAD = ( 1 << 23 ),
	FCVAR_ARCHIVE_XBOX = ( 1 << 24 ),
	FCVAR_ACCESSIBLE_FROM_THREADS = ( 1 << 25 ),
	FCVAR_SERVER_CAN_EXECUTE = ( 1 << 28 ),
	FCVAR_SERVER_CANNOT_QUERY = ( 1 << 29 ),
	FCVAR_CLIENTCMD_CAN_EXECUTE = ( 1 << 30 ),
	FCVAR_MATERIAL_THREAD_MASK = ( FCVAR_RELOAD_MATERIALS | FCVAR_RELOAD_TEXTURES | FCVAR_MATERIAL_SYSTEM_THREAD )
};

using cvar_iterator = unsigned long long;

union convar_value {
	bool i1;
	short i16;
	unsigned short u16;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
	float fl;
	double db;
	const char* sz;
};

class c_convar {
public:
    const char* m_name;
    const void* m_default_value;
    const void* m_min_value;
    const void* m_max_value;
    const char* m_description;
    std::uint16_t m_type;
    std::uint16_t m_reserved;
    std::uint32_t m_registered;
    std::uint64_t m_flags;
    char padding[0x20];
    convar_value m_value;
    bool get_bool() const {return m_value.i1;}
    int get_int() const {return m_value.i32;}
    float get_float() const {return m_value.fl;}
};
static_assert(offsetof(c_convar,m_value)==0x58);
struct convar_entry_t {
    c_convar* m_convar;
    std::uint16_t m_generation;
    std::uint16_t m_next;
    std::uint32_t m_links;
};
static_assert(sizeof(convar_entry_t)==0x10);
class c_engine_cvar {
public:
    PAD(0x4A);
    std::uint16_t m_allocation;
    PAD(4);
    convar_entry_t* m_entries;
    std::uint16_t m_head;
    c_convar* find_by_name(const char* name) const {
        return name ? find_by_hash(fnv1a::hash_64(name)) : nullptr;
    }
    c_convar* find_by_hash(std::uint64_t hash) const {
        const unsigned limit=m_allocation&0x7FFF;
        for(unsigned index=m_head,visited=0;m_entries && index<limit && visited<limit;
            index=m_entries[index].m_next,++visited) {
            auto* var=m_entries[index].m_convar;
            if(var && var->m_name && fnv1a::hash_64(var->m_name)==hash) return var;
        }
        return nullptr;
    }
    void unlock_hidden_vars() const {
        const unsigned limit=m_allocation&0x7FFF;
        auto& saved=hidden_vars();
        for(unsigned index=m_head,visited=0;m_entries && index<limit && visited<limit;
            index=m_entries[index].m_next,++visited) {
            auto* var=m_entries[index].m_convar;
            if(!var || !(var->m_flags&(FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY))) continue;
            bool exists=false;
            for(const auto& entry:saved) if(entry.first==var) {exists=true;break;}
            if(!exists) saved.emplace_back(var,var->m_flags);
            var->m_flags&=~static_cast<std::uint64_t>(FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY);
        }
    }
    void lock_hidden_vars() {
        auto& saved=hidden_vars();
        for(const auto& entry:saved) entry.first->m_flags=entry.second;
        saved.clear();
    }
private:
    static std::vector<std::pair<c_convar*,std::uint64_t>>& hidden_vars() {
        static std::vector<std::pair<c_convar*,std::uint64_t>> saved;
        return saved;
    }
};
static_assert(offsetof(c_engine_cvar,m_entries)==0x50);
static_assert(offsetof(c_engine_cvar,m_head)==0x58);
