#pragma once

#include "entity.h"
#include "../datatypes/user_cmd.h"

class c_in_game_money_services
{
public:
	SCHEMA(int32_t, m_iAccount, ("CCSPlayerController_InGameMoneyServices->m_iAccount"));
	SCHEMA(int32_t, m_iTotalCashSpent, ("CCSPlayerController_InGameMoneyServices->m_iTotalCashSpent"));
};

class c_user_cmd_manager
{
public:
    c_user_cmd m_commands[150];
    int32_t m_sequence;
    char pad_5914[4];
    double m_real_time;
    int32_t m_last_command;
    char pad_5924[4];
    c_csgo_user_cmd_pb m_pending_command;

    c_user_cmd* get_cmd() {
        return get_cmd_by_sequence(m_sequence);
    }

    c_user_cmd* get_cmd_by_sequence(int sequence) {
        if (sequence <= 0)
            return nullptr;
        auto* command = &m_commands[sequence % 150];
        return command->m_sequence_number == sequence ? command : nullptr;
    }

};

static_assert(offsetof(c_user_cmd_manager, m_sequence) == 0x5910);
static_assert(sizeof(c_user_cmd_manager) == 0x5970);

class c_base_player_weapon;
class c_cs_player_controller : public c_base_entity
{
public:
	SCHEMA( c_in_game_money_services*, m_pInGameMoneyServices, ( "CCSPlayerController->m_pInGameMoneyServices" ) );
	SCHEMA( const char*, m_sSanitizedPlayerName, ( "CCSPlayerController->m_sSanitizedPlayerName" ) );
	SCHEMA( int32_t&, m_nTickBase, ( "CBasePlayerController->m_nTickBase" ) );

	SCHEMA( bool, m_bIsLocalPlayerController, ( "CBasePlayerController->m_bIsLocalPlayerController" ) );
	SCHEMA( c_base_handle, m_hPawn, ( "CBasePlayerController->m_hPawn" ) );
	SCHEMA( c_base_handle, m_hObserverPawn, ( "CCSPlayerController->m_hObserverPawn" ) );
	SCHEMA( int, m_iPing, ( "CCSPlayerController->m_iPing" ) );
	SCHEMA( bool, m_bPawnIsAlive, ( "CCSPlayerController->m_bPawnIsAlive" ) );
	SCHEMA( uint64_t, m_steamID, ( "CBasePlayerController->m_steamID" ) );

	bool can_shoot(c_base_player_weapon* weapon);
	bool is_firing(c_base_player_weapon* weapon);

	bool is_bot() {
		uint64_t steam_id = m_steamID();
		if (steam_id == 0) return true;
		
		uint32_t account_type = (steam_id >> 52) & 0xF;
		if (account_type == 10) return true;
		
		uint32_t account_id = steam_id & 0xFFFFFFFF;
		if (account_id <= 100) return true;
		
		return false;
	}

	c_user_cmd_manager* get_cmd_manager( ) {
		static auto fn = g_modules->m_client.find( xx( "41 56 41 57 48 83 EC ? 48 8D 54 24" ) ).as<c_user_cmd_manager * (__fastcall*)(c_cs_player_controller*)>( );		
		return fn( this );
	};

};