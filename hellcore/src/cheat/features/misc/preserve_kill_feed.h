#pragma once

#include <context.h>
#include <sdk/constants.h>

#include <core/interfaces/interfaces.h>
#include "../../features/skins/skins.h"
#include <cfloat>

class hud_death_notice {
private:
    char _pad_0[0x20];

public:
    static hud_death_notice* get() {
        auto ptr = g_skins->find_hud_element("CCSGO_HudDeathNotice");
        return reinterpret_cast<hud_death_notice*>(ptr);
    }

    void clear() {
        static auto clear_fn = g_modules->m_client.find("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 71 ? 33 DB").as<int(__fastcall*)(hud_death_notice*)>();

        if (clear_fn) 
            clear_fn(reinterpret_cast<hud_death_notice*>(reinterpret_cast<uintptr_t>(this) - 0x20));
    }

    float& expire_time() {
        return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x50);
    }
};

class c_kill_feed
{
public:
	void preserve_kill_feed();
	bool b_new_round = false;
};
inline auto g_kill_feed = std::make_unique<c_kill_feed>();