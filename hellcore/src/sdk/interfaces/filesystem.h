#pragma once

#include <utils/memory.h>

class c_file_system
{
public:
    bool exists( const char* file_name, const char* format )
    {
        return g_memory->call_virtual<bool>( this, 21, file_name, format );
    }

    void* open( const char* file_name, const char* options, const char* path_id = nullptr )
    {
        return g_memory->call_virtual<void*>( this, 13, file_name, options, path_id );
    }

    void close( void* file )
    {
        g_memory->call_virtual<void>( this, 14, file );
    }

    unsigned int size( void* file )
    {
        return g_memory->call_virtual<unsigned int>( this, 18, file );
    }

    int read( void* output, int size, void* file )
    {
        return g_memory->call_virtual<int>( this, 11, output, size, file );
    }
};