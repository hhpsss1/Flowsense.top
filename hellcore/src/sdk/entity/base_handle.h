#pragma once
#include <cstdint>

#define INVALID_EHANDLE_INDEX 0xFFFFFFFF
#define ENT_ENTRY_MASK 0x7FFF
#define NUM_SERIAL_NUM_SHIFT_BITS 15
#define ENT_MAX_NETWORKED_ENTRY 16384

struct resource_binding_t {
    void* data;
};

struct resource_name_info_t
{
    const char* resource_name_symbol;
};

using resource_name_handle_t = const resource_name_info_t*;

template <class T>
class c_strong_handle {
public:
    operator T* () const
    {
        if ( binding == nullptr )
            return nullptr;

        return static_cast<T*>(binding->data);
    }

    T* operator->( ) const
    {
        if ( binding == nullptr )
            return nullptr;

        return static_cast<T*>(binding->data);
    }

    const resource_binding_t* binding;
    resource_name_handle_t name;
};

class c_base_handle
{
public:
    c_base_handle( ) noexcept;
    c_base_handle( const int entry, const int serial ) noexcept;
    bool operator!=( const c_base_handle& other ) const noexcept;
    bool operator==( const c_base_handle& other ) const noexcept;
    bool operator<( const c_base_handle& other ) const noexcept;
    bool is_valid( ) const noexcept;
    int get_entry_index( ) const noexcept;
    int get_serial_number( ) const noexcept;
    int to_int( ) const;
    void* get_base_entity( );

    template <typename T = void>
    T* get( ) {
        return reinterpret_cast<T*>(get_base_entity( ));
    };

    std::uint32_t index;
};


template <typename T>
class c_handle : public c_base_handle {
public:
    auto Get( ) {
        return reinterpret_cast<T*>( c_base_handle::get<T>( ) );
    }
};