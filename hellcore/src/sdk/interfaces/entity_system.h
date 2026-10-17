#pragma once
#include <core/hooks/modules.h>

#include <sdk/entity/pawn.h>
#include <sdk/entity/controller.h>

class c_entity_system
{
public:

    address_t get_base_entity( int index )
    {

        //__int64 v2; // rcx
        //__int64 v3; // rcx

        //if ( (unsigned int)index <= 0x7FFE
        //    && (unsigned int)(index >> 9) <= 0x3F
        //    && (v2 = *(std::uintptr_t*)(std::uintptr_t( this ) + 8 * (index >> 9) + 16)) != 0
        //    && (v3 = 120 * (index & 0x1FF), v3 + v2)
        //    && (*(std::uintptr_t*)(v3 + v2 + 16) & 0x7FFF) == index )
        //{
        //    return { reinterpret_cast<void*>(*(std::uintptr_t*)(v3 + v2)) };
        //}
        using function_t = void* (__fastcall*)(void*, int);
        static function_t get_akll = g_modules->m_client.find(xx("4C 8D 49 ? 81 FA")).as<function_t>();
        return get_akll(this,index);
    }

    int get_highest_entity_index() {
 auto** chunks=reinterpret_cast<const std::byte**>(reinterpret_cast<std::byte*>(this)+0x10);
 for(int chunk=63;chunk>=0;--chunk) {
  if(!chunks[chunk]) continue;
  for(int slot=511;slot>=0;--slot) {
   const int index=chunk*512+slot;
   if(index>0x7FFE) continue;
   const auto* identity=chunks[chunk]+slot*0x70;
   const auto handle=*reinterpret_cast<const std::uint32_t*>(identity+0x10);
   if((handle&0x7FFF)==index && *reinterpret_cast<void* const*>(identity)) return index;
  }
 }
 return -1;
}

    template <class T>
    T* create_entity_by_class_name(const char* name) 
    {
        using function_t = void* (__fastcall*)(void*, int, const char*, int, int, int, char);
        static function_t get_akll = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 49 8B F8 44 8B F2")).as<function_t>();
        return reinterpret_cast<T*>(get_akll(this, -1, name, 0, -1, -1, 0));
    }

    c_cs_player_pawn* get_player_pawn(int split_screen_slot)
    {
        static auto fn = g_modules->m_client.find(xx("E8 ? ? ? ? 4C 8B E0 48 85 C0 0F 84 ? ? ? ? ? ? ? 48 8B C8 FF 92 ? ? ? ? 84 C0 0F 84 ? ? ? ? ? ? ? ? 49 8B CC")).relative(1, 5).as<c_cs_player_pawn * (__fastcall*)(int)>();
        return fn(split_screen_slot);
    }

    c_cs_player_controller* get_player_controller(int split_screen_slot)
    {
        static auto fn = g_modules->m_client.find(xx("E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 0F B6 0D")).relative(1, 5).as<c_cs_player_controller * (__fastcall*)(int)>();
        return fn(split_screen_slot);
    }
};