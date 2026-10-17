#include "preserve_kill_feed.h"

void c_kill_feed::preserve_kill_feed() {
    static bool was_enabled = false;

    auto notice = hud_death_notice::get();

    if (!notice || !g_ctx->m_local_controller)
        return;

    bool is_enabled = GET_VAR(bool, MISC_PATH(m_preserve_kill_feed));

    if (!is_enabled) {
        if (was_enabled) {
            notice->clear();
            was_enabled = false;
        }

        notice->expire_time() = 1.5f;
        return;
    }

    was_enabled = true;

    if (b_new_round) {
        notice->clear();
        b_new_round = false;
    }

    notice->expire_time() = FLT_MAX;
}