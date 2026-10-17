#include "autobuy.h"
#include <cheat/config/vars.h>
#include <context.h>
#include <core/interfaces/interfaces.h>
#include <core/hooks/modules.h>
#include <sdk/schema/schema.h>

bool c_autobuy::can_buy() {
    if (!g_ctx->m_local_pawn)
        return false;

    if (!g_ctx->m_local_pawn->m_bInBuyZone())
        return false;

    if (!g_interfaces->m_game_rules)
        return false;

    if (g_interfaces->m_game_rules->m_bTeamIntroPeriod())
        return false;

    return true;
}

void c_autobuy::execute_buy_command(const char* command) {
    if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;
        
    char full_command[256];
    snprintf(full_command, sizeof(full_command), "buy %s", command);
    g_interfaces->m_engine->exec_client_cmd_unrestricted(full_command);
}

void c_autobuy::buy_primary() {
    int primary_selection = GET_VAR(int, MISC_PATH(m_autobuy_primary));
    
    switch (primary_selection) {
        case 1:
            execute_buy_command("ssg08");
            break;
        case 2:
            execute_buy_command("g3sg1");
            execute_buy_command("scar20");
            break;
        case 3:
            execute_buy_command("awp");
            break;
    }
}

void c_autobuy::buy_secondary() {
    int secondary_selection = GET_VAR(int, MISC_PATH(m_autobuy_secondary));
    
    switch (secondary_selection) {
        case 1:
            execute_buy_command("deagle");
            break;
        case 2:
            execute_buy_command("p250");
            break;
        case 3:
            execute_buy_command("revolver");
            break;
        case 4:
            execute_buy_command("elite");
            break;
    }
}

void c_autobuy::buy_additional() {
    std::vector<bool> additional_items = GET_VAR(std::vector<bool>, MISC_PATH(m_autobuy_additional));
    
    if (additional_items.size() > 0 && additional_items[0]) {
        execute_buy_command("hegrenade");
    }
    
    if (additional_items.size() > 1 && additional_items[1]) {
        if (g_ctx->m_local_controller && g_ctx->m_local_controller->m_iTeamNum() == 2) {
            execute_buy_command("incgrenade");
        } else {
            execute_buy_command("molotov");
        }
    }
    
    if (additional_items.size() > 2 && additional_items[2]) {
        execute_buy_command("smokegrenade");
    }
    
    if (additional_items.size() > 3 && additional_items[3]) {
        execute_buy_command("taser");
    }

    if (additional_items.size() > 4 && additional_items[4]) {
        execute_buy_command("vesthelm");
    }

    if (additional_items.size() > 5 && additional_items[5]) {
        execute_buy_command("defuser");
    }
}

void c_autobuy::on_player_spawn() {
    if (!GET_VAR(bool, MISC_PATH(m_enabled_autobuy)))
        return;
        
    if (m_has_bought_this_life)
        return;
        
    if (!can_buy())
        return;
        
    buy_primary();
    buy_secondary();
    buy_additional();
    
    m_has_bought_this_life = true;
}

void c_autobuy::update() {
    if (!g_interfaces->m_engine || !g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
        return;
        
    bool is_alive = g_ctx->m_local_pawn && g_ctx->m_local_pawn->is_alive();
    int current_team = g_ctx->m_local_pawn ? g_ctx->m_local_pawn->m_iTeamNum() : 0;
    
    if (m_waiting_for_round_start_buy && can_buy()) {
        if (is_alive && !m_has_bought_this_life) {
            on_player_spawn();
        }
        m_waiting_for_round_start_buy = false;
    }
    
    if (!m_waiting_for_round_start_buy) {
        if (!m_was_alive && is_alive) {
            m_has_bought_this_life = false;
            m_pending_buy = true;
        }
        
        if (m_was_alive && !is_alive) {
            m_has_bought_this_life = false;
        }
        
        if (m_last_team != current_team && current_team != 0) {
            m_has_bought_this_life = false;
            if (is_alive) {
                m_pending_buy = true;
            }
        }
        
        if (m_pending_buy && is_alive && !m_has_bought_this_life && can_buy()) {
            on_player_spawn();
            m_pending_buy = false;
        }
    }
    
    m_was_alive = is_alive;
    m_last_team = current_team;
}

void c_autobuy::on_round_start() {
    m_has_bought_this_life = false;
    m_pending_buy = false;
    m_waiting_for_round_start_buy = true;
}