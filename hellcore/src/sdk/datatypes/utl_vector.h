#pragma once

template<class T>
struct c_utl_vector
{
    int size;
    char pad_002[4];
    T* elements;
    char pad_003[8];
};

template <class T>
class c_utl_vector2
{
public:
    c_utl_vector2( )
    {
        size = 0;
        elements = nullptr;
    }

    T& operator[]( int i )
    {
        return elements[i];
    }

    int count( ) const
    {
        return size;
    }

    int size;
    T* elements;
};