#pragma once

#include <includes.h>

class c_entry {
public:
	static void entry( void* moduleptr );
	static void cleanup( void );
};
