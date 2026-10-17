#include "controller.h"
#include "weapon.h"
#include <context.h>

bool c_cs_player_controller::can_shoot(c_base_player_weapon* weapon) {
	return (weapon->m_iClip1() > 0) && (weapon->m_nNextPrimaryAttackTick() <= m_nTickBase());
}

bool c_cs_player_controller::is_firing(c_base_player_weapon* weapon) {
	if (!weapon || !g_ctx->m_cmd)
		return false;

	const float server_time = ticks_to_time(m_nTickBase());
	c_weapon_base_v_data* data = weapon->weapon_data();
	if (!data)
		return false;

	if (c_base_cs_grenade* grenade = (c_base_cs_grenade*)(weapon); grenade != nullptr && data->m_WeaponType() == weapontype_grenade)
		return !grenade->m_bPinPulled() && grenade->m_fThrowTime() > 0.f && grenade->m_fThrowTime() < server_time;
	else if (data->m_WeaponType() == weapontype_knife)
		return (g_ctx->m_cmd->m_buttons.m_value & (IN_ATTACK) || g_ctx->m_cmd->m_buttons.m_value & (IN_ATTACK2)) && can_shoot(weapon);
	else if (weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() == weapon_revolver)
		return (g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK) && g_interfaces->m_global_vars->m_curtime >= ticks_to_time(weapon->m_nPostponeFireReadyTicks());
	else
		return g_ctx->m_cmd->m_buttons.m_value & (IN_ATTACK) && can_shoot(weapon);
}