#pragma once

#include "controller.h"

#include <sdk/interfaces/econ_item_system.h>

struct c_fire_mode {
	float m_types[2];
};


class c_base_view_model : public c_base_entity {
public:
	//SCHEMA(m_weapon, c_base_handle, "C_BaseViewModel", "m_hWeapon");
};

class c_hud_model_weapon : public c_base_view_model
{
public:
};

class c_hud_model_arms : public c_base_view_model {
public:

};

class c_econ_item_view
{
public:
	SCHEMA( bool&, m_bInitialized, ( "C_EconItemView->m_bInitialized" ) );
	SCHEMA( int32_t&, m_iEntityQuality, ( "C_EconItemView->m_iEntityQuality" ) );

	SCHEMA( uint16_t&, m_iItemDefinitionIndex, ( "C_EconItemView->m_iItemDefinitionIndex" ) );
	SCHEMA( uint64_t&, m_iItemID, ( "C_EconItemView->m_iItemID" ) );
	SCHEMA( uint32_t&, m_iItemIDLow, ( "C_EconItemView->m_iItemIDLow" ) );
	SCHEMA( uint32_t&, m_iItemIDHigh, ( "C_EconItemView->m_iItemIDHigh" ) );
	SCHEMA( uint32_t&, m_iAccountID, ( "C_EconItemView->m_iAccountID" ) );

	SCHEMA( bool&, m_bDisallowSOC, ( "C_EconItemView->m_bDisallowSOC" ) );
	SCHEMA( bool&, m_bRestoreCustomMaterialAfterPrecache, ( "C_EconItemView->m_bRestoreCustomMaterialAfterPrecache" ) );

	SCHEMA_PTR( const char, m_szCustomName, ( "C_EconItemView->m_szCustomName" ) );
	SCHEMA_PTR( const char, m_szCustomNameOverride, ( "C_EconItemView->m_szCustomNameOverride" ) );

	int get_custom_paint_kit( );
	c_econ_item_definition* get_econ_item_definition( );

	c_econ_item_definition* get_static_data() {
		return g_memory->call_virtual<c_econ_item_definition*>(this, 16u);
	}
};

class c_attribute_container
{
public:
	SCHEMA_PTR( c_econ_item_view, m_Item, ( "C_AttributeContainer->m_Item" ) );
};

class c_econ_entity : public c_base_entity
{
public:
	SCHEMA( int&, m_nFallbackSeed, ( "C_EconEntity->m_nFallbackSeed" ) );
	SCHEMA( int&, m_nFallbackPaintKit, ( "C_EconEntity->m_nFallbackPaintKit" ) );
	SCHEMA( int&, m_nFallbackStatTrak, ( "C_EconEntity->m_nFallbackStatTrak" ) );

	SCHEMA( float&, m_flFallbackWear, ( "C_EconEntity->m_flFallbackWear" ) );

	SCHEMA( uint32_t&, m_OriginalOwnerXuidLow, ( "C_EconEntity->m_OriginalOwnerXuidLow" ) );
	SCHEMA( uint32_t&, m_OriginalOwnerXuidHigh, ( "C_EconEntity->m_OriginalOwnerXuidHigh" ) );

	SCHEMA_PTR( c_attribute_container, m_AttributeManager, ( "C_EconEntity->m_AttributeManager" ) );

	uint64_t m_OriginalOwnerXuid()
	{
		return ((uint64_t)(m_OriginalOwnerXuidHigh()) << 32) |
			m_OriginalOwnerXuidLow();
	}
};

class c_weapon_base_v_data
{
public:
	SCHEMA( int, m_nDamage, ( "CCSWeaponBaseVData->m_nDamage" ) );
	SCHEMA( int, m_nNumBullets, ( "CCSWeaponBaseVData->m_nNumBullets" ) );
	SCHEMA( int, m_WeaponType, ( "CCSWeaponBaseVData->m_WeaponType" ) );

	SCHEMA( int32_t, m_iMaxClip1, ( "CBasePlayerWeaponVData->m_iMaxClip1" ) );
	SCHEMA( int32_t, m_iMaxClip2, ( "CBasePlayerWeaponVData->m_iMaxClip2" ) );
	SCHEMA( int32_t, m_iDefaultClip1, ( "CBasePlayerWeaponVData->m_iDefaultClip1" ) );
	SCHEMA( int32_t, m_iDefaultClip2, ( "CBasePlayerWeaponVData->m_iDefaultClip2" ) );
	SCHEMA( int32_t, m_iWeight, ( "CBasePlayerWeaponVData->m_iWeight" ) );

	SCHEMA( bool, m_bIsRevolver, ( "CCSWeaponBaseVData->m_bIsRevolver" ) );
	SCHEMA( float, m_flArmorRatio, ( "CCSWeaponBaseVData->m_flArmorRatio" ) );
	SCHEMA( float, m_flRange, ( "CCSWeaponBaseVData->m_flRange" ) );
	SCHEMA( float, m_flRangeModifier, ( "CCSWeaponBaseVData->m_flRangeModifier" ) );
	SCHEMA( float, m_flPenetration, ( "CCSWeaponBaseVData->m_flPenetration" ) );
	SCHEMA( float, m_flHeadshotMultiplier, ( "CCSWeaponBaseVData->m_flHeadshotMultiplier" ) );
	SCHEMA( float, m_flSpread, ( "CCSWeaponBaseVData->m_flSpread" ) );
	SCHEMA( float, m_flMaxSpeed, ( "CCSWeaponBaseVData->m_flMaxSpeed" ) );
	SCHEMA( float, m_flInaccuracyJumpInitial, ( "CCSWeaponBaseVData->m_flInaccuracyJumpInitial" ) );
	SCHEMA( float, m_flInaccuracyJumpApex, ( "CCSWeaponBaseVData->m_flInaccuracyJumpApex" ) );
	SCHEMA( c_fire_mode, m_flInaccuracyMove, ( "CCSWeaponBaseVData->m_flInaccuracyMove" ) );
	SCHEMA( float, m_flThrowVelocity, ( "CCSWeaponBaseVData->m_flThrowVelocity" ) );

	SCHEMA( const char*, m_szName, ( "CCSWeaponBaseVData->m_szName" ) );
	SCHEMA( c_fire_mode, m_flCycleTime, ( "CCSWeaponBaseVData->m_flCycleTime" ) );

	SCHEMA( int&, m_nZoomLevels, ( "CCSWeaponBaseVData->m_nZoomLevels" ) );
	SCHEMA( int&, m_nSpreadSeed, ( "CCSWeaponBaseVData->m_nSpreadSeed" ) );

	SCHEMA( c_fire_mode, m_flInaccuracyCrouch, ( "CCSWeaponBaseVData->m_flInaccuracyCrouch" ) );
	SCHEMA( c_fire_mode, m_flInaccuracyStand, ( "CCSWeaponBaseVData->m_flInaccuracyStand" ) );
	SCHEMA( float, m_flInaccuracyReload, ( "CCSWeaponBaseVData->m_flInaccuracyReload" ) );

};

class c_base_player_weapon : public c_econ_entity
{
public:
	SCHEMA( bool, m_bInReload, ( "C_CSWeaponBase->m_bInReload" ) );
	SCHEMA( bool, m_bBurstMode, ( "C_CSWeaponBase->m_bBurstMode" ) );
	SCHEMA( float, m_flWatTickOffset, ( "C_CSWeaponBase->m_flWatTickOffset" ) );

	SCHEMA( int, m_iBurstShotsRemaining, ( "C_CSWeaponBaseGun->m_iBurstShotsRemaining" ) );

	SCHEMA( int32_t, m_iClip1, ( "C_BasePlayerWeapon->m_iClip1" ) );
	SCHEMA( int32_t, m_iClip2, ( "C_BasePlayerWeapon->m_iClip2" ) );
	SCHEMA(int32_t, m_pReserveAmmo, ("C_BasePlayerWeapon->m_pReserveAmmo"));

	SCHEMA( int32_t&, m_nNextPrimaryAttackTick, ( "C_BasePlayerWeapon->m_nNextPrimaryAttackTick" ) );
	SCHEMA( int32_t&, m_nNextSecondaryAttackTick, ( "C_BasePlayerWeapon->m_nNextSecondaryAttackTick" ) );

	SCHEMA( float&, m_flNextSecondaryAttackTickRatio, ( "C_BasePlayerWeapon->m_flNextSecondaryAttackTickRatio" ) );
	SCHEMA( float&, m_flNextPrimaryAttackTickRatio, ( "C_BasePlayerWeapon->m_flNextPrimaryAttackTickRatio" ) );

	SCHEMA( int32_t&, m_nPostponeFireReadyTicks, ( "C_CSWeaponBase->m_nPostponeFireReadyTicks" ) );
	SCHEMA( float&, m_nPostponeFireReadyFrac, ( "C_CSWeaponBase->m_flPostponeFireReadyFrac" ) );

	c_weapon_base_v_data* weapon_data( )
	{
		if (!this)
			return nullptr;

		return *reinterpret_cast<c_weapon_base_v_data**>(reinterpret_cast<uintptr_t>(this) + 0x380 + 0x8);
	}
};

class c_weapon_base : public c_base_player_weapon
{
public:
	SCHEMA( bool, m_bBurstMode, ( "C_CSWeaponBase->m_bBurstMode" ) );
	SCHEMA( int, m_zoomLevel, ( "C_CSWeaponBaseGun->m_zoomLevel" ) );
	SCHEMA( float&, m_flRecoilIndex, ( "C_CSWeaponBase->m_flRecoilIndex" ) );
	SCHEMA( float&, m_flLastAccuracyUpdateTime, ( "C_CSWeaponBase->m_flLastAccuracyUpdateTime" ) );
	SCHEMA( int, m_weaponMode, ( "C_CSWeaponBase->m_weaponMode" ) );
	SCHEMA( float&, m_fAccuracyPenalty, ( "C_CSWeaponBase->m_fAccuracyPenalty" ) );
	SCHEMA( float, m_fLastShotTime, ( "C_CSWeaponBase->m_fLastShotTime" ) );
	SCHEMA( float, m_flTurningInaccuracy, ( "C_CSWeaponBase->m_flTurningInaccuracy" ) );

	bool can_shoot( c_cs_player_controller* local_controller )
	{
		return ( m_iClip1( ) > 0 ) && ( m_nNextPrimaryAttackTick( ) <= local_controller->m_nTickBase( ) );
	}


	float get_inaccuracy( );
	float get_spread( );
	float get_max_speed( );

	void  update_accuracy_penalty( );

	void try_update_accuracy( );
	bool is_at_max_accuracy( );

	void regen_weapon_skin( );

	void update_subclass();

	struct c_base_gunfire_data {
		char m_pad00[0x14];
		vec3_t m_shoot_position;
		char m_pad01[52];
	};

};

class c_base_cs_grenade : public c_weapon_base {
public:
	SCHEMA( float, m_fThrowTime, ( "C_BaseCSGrenade->m_fThrowTime" ) );

	SCHEMA( bool, m_bPinPulled, ( "C_BaseCSGrenade->m_bPinPulled" ) );
	SCHEMA( bool, m_bJumpThrow, ( "C_BaseCSGrenade->m_bJumpThrow" ) );
	SCHEMA( float, m_flThrowStrength, ( "C_BaseCSGrenade->m_flThrowStrength" ) );
	SCHEMA(float, m_DmgRadius, ("C_BaseGrenade->m_DmgRadius"));
	SCHEMA(float, m_flDamage, ("C_BaseGrenade->m_flDamage"));
};

class c_base_cs_grenade_projectile : public c_base_cs_grenade {
public:
	SCHEMA(int32_t, m_nExplodeEffectTickBegin, ("C_BaseCSGrenadeProjectile->m_nExplodeEffectTickBegin"));
	SCHEMA(vec3_t, m_vInitialPosition, ("C_BaseCSGrenadeProjectile->m_vInitialPosition"));
	SCHEMA(vec3_t, m_vInitialVelocity, ("C_BaseCSGrenadeProjectile->m_vInitialVelocity"));
	SCHEMA(bool, m_bExplode_effect_began, ("C_BaseCSGrenadeProjectile->m_bExplodeEffectBegan"));


};