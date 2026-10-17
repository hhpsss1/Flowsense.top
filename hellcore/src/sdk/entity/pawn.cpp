#include "pawn.h"
#include <utils/memory.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/engine_cvar.h>
#include <sdk/interfaces/global_variables.h>
#include <sdk/constants.h>
#include <context.h>
#include <cheat/features/engine_prediction/engine_prediction.h>

constexpr int COLLISION_GROUP_PLAYER_MOVEMENT = 8;

vec3_t c_player_weapon_services::weapon_shoot_position() {
	vec3_t ret = {};
	g_memory->call_virtual<void>(this, 29, &ret);
	return ret;
}

c_contents* c_cs_player_pawn::get_contents() {
	return g_memory->call_virtual<c_contents*>(this, 63);
}

vec3_t c_cs_player_pawn::get_eye_pos() {
 auto* node=m_pGameSceneNode();
 return node ? node->m_vecAbsOrigin()+m_vecViewOffset() : vec3_t{};
}

vec3_t c_cs_player_pawn::get_shoot_pos() {
	return get_eye_pos();
}

bool c_cs_player_pawn::is_alive() {
	return m_iHealth() > 0 && m_lifeState() == LIFE_ALIVE;
}

bool c_cs_player_pawn::is_enemy() {
	c_cs_player_pawn* local_pawn = g_ctx->m_local_pawn;

	if (!local_pawn)
		return true;

	if (this == local_pawn)
		return false;

	return this->m_iTeamNum() != local_pawn->m_iTeamNum();
}

c_weapon_base* c_cs_player_pawn::get_active_weapon()
{
	const auto player_weapon_services = m_pWeaponServices();
	if (!player_weapon_services)
		return nullptr;

	auto entity = player_weapon_services->m_hActiveWeapon().get<c_weapon_base >();
	if (!entity || !entity->is_weapon())
		return nullptr;

	return entity;
}

uint32_t c_cs_player_pawn::get_collision_owner_index()
{
	const auto player_collision = m_pCollision();
	if (player_collision && !(player_collision->m_usSolidFlags() & 4))
		return m_hOwnerEntity().get_entry_index();

	return -1;
}
bool c_cs_player_pawn::has_armor(int hitgroup)
{
	if (hitgroup == HITGROUP_HEAD)
		return m_pItemServices()->m_bHasHelmet();

	return m_ArmorValue();
}

bool c_cs_player_pawn::IsArmored(int hitgroup) {
	if (hitgroup == HITGROUP_HEAD) {
		if (!m_pItemServices())
			return false;

		return m_pItemServices()->m_bHasHelmet();
	}
	else
		return m_ArmorValue() > 0;
}

void c_cs_player_pawn::set_body_group(int first, int second)
{
	using fn_set_body_group = void(__fastcall*)(void*, int, unsigned int);
	static fn_set_body_group set_body_group_ = g_modules->m_client.find(xx("85 D2 0F 88 ? ? ? ? 53 55")).as<fn_set_body_group>();
	set_body_group_(this, first, second);
}

void c_cs_player_pawn::run_pre_think()
{
	g_memory->call_virtual<void>(this, 322);
}



void c_cs_player_pawn::run_post_think()
{
	g_memory->call_virtual<void>(this, 333);
}

















//call that before walk move


















































float c_cs_player_pawn::get_some_timing(int idx0, int idx1) {
	static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 63 D8 48 8B F1")).as<float(__fastcall*)(c_cs_player_pawn*, int, int)>();
	return fn(this, idx0, idx1);
}

void c_cs_player_pawn::set_velocity(vec3_t* velocity) {
 if(!velocity) return;
 m_vecVelocity()=*velocity;
 m_vecAbsVelocity()=*velocity;
}

vec3_t c_cs_player_pawn::get_weapon_recoil() {
	vec3_t recoil = {};
	//static auto fn = g_modules->m_client.find(xx("40 53 56 57 48 83 EC ? 80 B9 ? ? ? ? ? 41 0F B6 F0")).as<void(__fastcall*)(void*, vec3_t*, bool)>();
	//fn(this, &recoil, true);
	return recoil;
}


c_hud_model_arms* c_cs_player_pawn::get_view_arms()
{

	c_hud_model_arms* m_hud_model = (c_hud_model_arms*)this->m_hud_model_arms().get();
	if (!m_hud_model)
		return nullptr;

	return m_hud_model;
}

std::vector<c_hud_model_weapon*> c_cs_player_pawn::get_view_models()
{
	c_hud_model_arms* pViewArms = get_view_arms(); // m_hHudModelArms
	if (!pViewArms)
		return {};

	c_game_scene_node* pGameSceneNode = pViewArms->m_pGameSceneNode();
	if (!pGameSceneNode) {
		return {};
	}

	std::vector<c_hud_model_weapon*> vecViewModels = {};
	for (c_game_scene_node* pChild = pGameSceneNode->m_child(); pChild; pChild = pChild->m_sibling()) {
		c_base_entity* pOwner = reinterpret_cast<c_base_entity*>(pChild->m_owner());
		if (!pOwner) {
			continue;
		}

		if (const auto* name=pOwner->get_class_name(); name && std::strcmp(name,"C_CS2HudModelWeapon")==0) { // pOwner->GetClassName() == "C_CS2HudModelWeapon"
			vecViewModels.push_back(reinterpret_cast<c_hud_model_weapon*>(pOwner));
		}
	}

	return vecViewModels;
}

c_hud_model_weapon* c_cs_player_pawn::get_view_model_brutforce() {

	c_base_player_weapon* pWeapon = this->get_active_weapon(); // CPlayer_WeaponServices -> m_hActiveWeapon
	if (!pWeapon) {
		return nullptr;
	}

	std::vector<c_hud_model_weapon*> vecViewModels = get_view_models();
	if (vecViewModels.empty()) {
		return nullptr;
	}

	for (c_hud_model_weapon* pViewModel : vecViewModels) {
		c_base_entity* pOwner = (c_base_entity*)pViewModel->m_hOwnerEntity().get(); // m_hOwnerEntity
		if (!pOwner) {
			continue;
		}

		if (pOwner == pWeapon) {
			return pViewModel;
		}
	}

	return nullptr;
}
