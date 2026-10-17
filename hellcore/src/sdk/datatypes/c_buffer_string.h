#pragma once
#include <cstdint>
#include <core/hooks/modules.h>
enum e_string_convert_error_policy {
    STRING_CONVERT_FLAG_SKIP = 1,
    STRING_CONVERT_FLAG_FAIL = 2,
    STRING_CONVERT_FLAG_ASSERT = 4,

    STRING_CONVERT_REPLACE = 0,
    STRING_CONVERT_SKIP = STRING_CONVERT_FLAG_SKIP,
    STRING_CONVERT_FAIL = STRING_CONVERT_FLAG_FAIL,

    STRING_CONVERT_ASSERT_REPLACE = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_REPLACE,
    STRING_CONVERT_ASSERT_SKIP = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_SKIP,
    STRING_CONVERT_ASSERT_FAIL = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_FAIL,
};

class c_buffer_string {
public:
    enum e_allocation_option_t {
        UNK1 = -1,
        UNK2 = 0,
        UNK3 = (1 << 1),
        UNK4 = (1 << 8),
        UNK5 = (1 << 9)
    };

    enum e_allocation_flags_t {
        LENGTH_MASK = (1 << 30) - 1,
        FLAGS_MASK = ~LENGTH_MASK,

        OVERFLOWED_MARKER = (1 << 30),
        FREE_HEAP_MARKER = (1 << 31),

        STACK_ALLOCATED_MARKER = (1 << 30),
        ALLOW_HEAP_ALLOCATION = (1 << 31)
    };

public:
    c_buffer_string( bool allow_heap_allocation = true ) :
        length_( 0 ),
        allocated_size_( (allow_heap_allocation* ALLOW_HEAP_ALLOCATION) | STACK_ALLOCATED_MARKER | sizeof( string_ ) ),
        string_ptr_( nullptr ) {
    }

    c_buffer_string( const char* string, bool allow_heap_allocation = true ) :
        c_buffer_string( allow_heap_allocation ) {
        insert( 0, string );
    }

public:
    c_buffer_string( const c_buffer_string& other ) : c_buffer_string( ) { *this = other; }

    c_buffer_string( const char* buf, uint64_t ex ) : c_buffer_string( ) { fixup_resource_name( buf, ex ); }
    c_buffer_string( const char* buf, int ex ) : c_buffer_string( ) { fixup_resource_name( buf, static_cast<uint64_t>( ex ) ); }

    void fixup_resource_name( const char* resource, uint64_t extension ) {
        resource_type_ = extension;
        using fn = void( __fastcall* )(c_buffer_string*, const char*, uint64_t);
        static auto fixup_fn = g_modules->m_resourcesystem.find( xx( "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 8B 41" ) ).as<fn>( );
        fixup_fn( this, resource, extension );
    }

    ~c_buffer_string( ) { purge( ); }

    void set_heap_allocation_state( bool state ) {
        if (state)
            allocated_size_ |= ALLOW_HEAP_ALLOCATION;
        else
            allocated_size_ &= ~ALLOW_HEAP_ALLOCATION;
    }

    int allocated_num( ) const { return allocated_size_ & LENGTH_MASK; }
    int length( ) const { return length_ & LENGTH_MASK; }

    bool can_heap_allocate( ) const { return (allocated_size_ & ALLOW_HEAP_ALLOCATION) != 0; }
    bool is_stack_allocated( ) const { return (allocated_size_ & STACK_ALLOCATED_MARKER) != 0; }
    bool should_free_memory( ) const { return (length_ & FREE_HEAP_MARKER) != 0; }
    bool is_overflowed( ) const { return (length_ & OVERFLOWED_MARKER) != 0; }

    bool is_input_string_unsafe( const char* data ) const {
        return ((void*)data >= this && (void*)data < &this[1]) ||
            (!is_allocation_empty( ) && data >= base( ) && data < (base( ) + allocated_num( )));
    }

    bool is_allocation_empty( ) const { return allocated_num( ) == 0; }

protected:
    char* base( ) { return is_stack_allocated( ) ? string_ : (!is_allocation_empty( ) ? string_ptr_ : nullptr); }
    const char* base( ) const { return const_cast<c_buffer_string*>( this )->base( ); }

public:
    void set_resource_type(uint64_t type) { resource_type_ = type; }

    void load_path(const char* path) { clear(); if (path) insert(0, path); }

    void clear( ) {
        if (!is_allocation_empty( ))
            base( )[0] = '\0';

        length_ &= ~LENGTH_MASK;
    }

public:
    const char* insert( int index, const char* buf, int count = -1, bool ignore_alignment = false ) {
        using fn = const char* (__fastcall*)(void*, int, const char*, int, bool);
        static auto insert_fn = g_modules->m_tier0.find( xx( "40 53 55 56 57 48 83 EC ? 48 63 EA" ) ).as<fn>( );
        return insert_fn(this, index, buf, count, ignore_alignment);
    }

    void purge( int allocated_bytes_to_preserve = 0 ) {
        using fn = void( __fastcall* )(void*, int);
        static auto purge_fn = g_modules->m_tier0.find( xx( "48 89 5C 24 ? 57 48 83 EC ? 8B 41 ? 8D 7A" ) ).as<fn>( );
        purge_fn(this, allocated_bytes_to_preserve);
    }

private:
    int length_;
    int allocated_size_;

    union {
        char* string_ptr_;
        char string_[0xC8];
    };
    uint64_t resource_unknown_ = 0;
    uint64_t resource_type_ = 0;
};
static_assert(sizeof(c_buffer_string) == 0xE0);