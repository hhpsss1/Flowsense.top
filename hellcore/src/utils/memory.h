#pragma once
#include <memory>

class c_memory {
public:

    template< typename T >
    __forceinline T get_method( void* thisptr, uintptr_t idx )
    {
        return reinterpret_cast<T>((*static_cast<uintptr_t**>(thisptr))[idx]);
    }

    template <typename T, typename... Args>
    __forceinline T call_virtual( void* class_, unsigned int index, Args... args )
    {
        auto func = get_method<T( __thiscall* )(void*, Args...)>( class_, index );
        return func( class_, args... );
    }
};

inline auto g_memory = std::make_unique<c_memory>( );