#pragma once

#include <context.h>

class c_game_rules
{
public:
	SCHEMA(bool, m_bIsValveDS, ("C_CSGameRules->m_bIsValveDS"));
	SCHEMA(bool, m_bTeamIntroPeriod, ("C_CSGameRules->m_bTeamIntroPeriod"));
};