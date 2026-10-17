#pragma once

/*
This file is intended only for header files that are rarely modified
*/

#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_ENABLE_FREETYPE

#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <iostream>
#include <d3d11.h>
#include <dxgi.h>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <any>
#include <random>
#include <bitset>
#include <variant>

#include <math/math types/view_matrix.h>
#include <math/math types/vector.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <freetype/imgui_freetype.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

typedef ImVec4 hellvec4;
typedef ImVec2 hellvec2;
typedef ImColor hellcolor;

#include <sdk/steam_types.h>
#include <ida_defs.h>

#include <minhook/minhook.h>
#include <utils/encryption/xor.h>
#include <utils/encryption/encrypted_ptr.h>
#include <utils/logging.h>
#include <core/hooks/modules.h>


#define _CS_INTERNAL_CONCATENATE(LEFT, RIGHT) LEFT##RIGHT
#define CS_CONCATENATE(LEFT, RIGHT) _CS_INTERNAL_CONCATENATE(LEFT, RIGHT)

#define PAD(SIZE)										\
private:												\
	char CS_CONCATENATE(pad_0, __COUNTER__)[SIZE]; \
public:

#define offset( type, name, offset )                                                                                                                 \
    type& name ( ) {                                                                                                                                 \
        return *( type* )( std::uintptr_t ( this ) + offset );                                                                                       \
    }

#undef min
#undef max
#undef snprintf