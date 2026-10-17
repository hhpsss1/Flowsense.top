#include "weapon.h"

#include <utils/memory.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/global_variables.h>
#include <sdk/constants.h>
#include <context.h>

int c_econ_item_view::get_custom_paint_kit()
{
	return g_memory->call_virtual<int>(this, 3);
}

c_econ_item_definition* c_econ_item_view::get_econ_item_definition()
{
	return g_memory->call_virtual<c_econ_item_definition*>(this, 16);
}

void c_weapon_base::regen_weapon_skin() {
 using fn_t=void(__fastcall*)();
 static auto fn=g_modules->m_client.find(xx("48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10")).as<fn_t>();
 if(fn) fn();
}



void c_weapon_base::update_subclass()
{
	using fn_update_subclass = void(__fastcall*)(void*);
	static fn_update_subclass update_class = g_modules->m_client.find(xx("4C 8B DC 53 48 81 EC ? ? ? ? 48 8B 41 10 48 8B D9 8B 50 30 C1 EA 04")).as<fn_update_subclass>();
	update_class(this);
}









float c_weapon_base::get_inaccuracy() {
 using fn_t=float(__fastcall*)(void*,float*,float*);
 static auto fn=g_modules->m_client.find(xx("48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24")).as<fn_t>();
 float x{},y{};
 return fn ? fn(this,&x,&y) : 0.f;
}

float c_weapon_base::get_spread() {
	static auto fn = g_modules->m_client.find(xx("48 63 91 ?? ?? ?? ?? 48 8B 81 ?? ?? ?? ?? 85 D2 78 ?? 48 83 FA 02 73 ?? F3 0F 10 84 90 50 07 00 00 C3 F3 0F 10 80 50 07 00 00 C3")).as<float(__fastcall*)(c_weapon_base*)>();
	return fn(this);
}

float c_weapon_base::get_max_speed() {
	return g_memory->call_virtual<float>(this, 366, m_weaponMode());
}

void c_weapon_base::update_accuracy_penalty() {
 static auto fn=g_modules->m_client.find(xx("40 57 41 56 48 83 EC 68 48 8B F9 E8 ? ? ? ? 4C 8B F0 48 85 C0")).as<void(__fastcall*)(c_weapon_base*)>();
 if(fn) fn(this);
}

void c_weapon_base::try_update_accuracy() {
    auto* globals = g_interfaces->m_global_vars;
    if (!globals)
        return;
    static c_weapon_base* last_weapon = nullptr;
    static c_global_variables* last_globals = nullptr;
    static int last_frame = -1;
    if (last_weapon != this || last_globals != globals || last_frame != globals->m_frame_count) {
        update_accuracy_penalty();
        last_weapon = this;
        last_globals = globals;
        last_frame = globals->m_frame_count;
    }
}

bool c_weapon_base::is_at_max_accuracy() {
	return false;
}


