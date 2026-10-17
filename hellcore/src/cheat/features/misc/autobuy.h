#pragma once
#include <memory>

class c_autobuy {
public:
    void update();
    void on_round_start();
    
private:
    bool can_buy();
    void execute_buy_command(const char* command);
    void buy_primary();
    void buy_secondary();
    void buy_additional();
    void on_player_spawn();
    
    bool m_was_alive = false;
    bool m_has_bought_this_life = false;
    int m_last_team = 0;
    bool m_pending_buy = false;
    bool m_waiting_for_round_start_buy = false;
};

inline std::unique_ptr<c_autobuy> g_autobuy = std::make_unique<c_autobuy>();
