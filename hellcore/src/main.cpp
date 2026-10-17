#include <Includes.h>

#include "Entry.h"

BOOL APIENTRY DllMain( HMODULE hModule, DWORD uReasonForCall, LPVOID lpReserved ) {
    if ( uReasonForCall == DLL_PROCESS_ATTACH ) {
        CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)c_entry::entry, hModule, NULL, NULL );
    }
    else if ( uReasonForCall == DLL_PROCESS_DETACH ) {
        c_entry::cleanup();
    }

    return TRUE;
}

