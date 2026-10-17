#pragma once
#include <sdk/entity/weapon.h>
#include <sdk/interfaces/physics.h>
#include <deque>
#include <unordered_map>

class ID3D11ShaderResourceView;
struct icon_data_t
{
	ID3D11ShaderResourceView* texture_view;
	uint32_t width;
	uint32_t height;

	icon_data_t() : texture_view(nullptr), width(0), height(0) {}
	icon_data_t(ID3D11ShaderResourceView* view, uint32_t w, uint32_t h) : texture_view(view), width(w), height(h) {}
};

inline icon_data_t get_panorama_texture(const std::string& path);
inline std::unordered_map<uint32_t, icon_data_t> m_icons;

struct GrenadePathPoint_t;
struct GrenadePredictionObject_t
{
	vec3_t m_vInitialPosition;
	vec3_t m_vInitialVelocity;
	float m_flDistance;
	int m_iTotalPoints = 0;
	c_base_entity* m_pGrenadeEntity;
	int m_iLastTick;
	int m_nTickDetonation;
	//Precache Values
	float m_flNadeDamage = 0.f;
	float m_flNadeRadius = 0.f;
	int m_iNadeType = 0; //Avoid redundant hashing
	int    m_iDamage = 0;
	bool  m_bLethal = false;

	std::vector<GrenadePathPoint_t> m_GrenadePathPoint;

	std::deque<vec3_t> m_GrenadePathPoint_1;

	bool m_bDrewParticle = false;
	bool m_bIsThrown = false;
	c_base_handle m_hGrenadeProjectile;
	float m_fThrowTime;

	void Draw(hellcolor cColor, bool bOverlay, bool bEffects);
};
inline std::vector < GrenadePredictionObject_t > s_vecPredictedGrenades;

struct GrenadePathPoint_t
{
	vec3_t m_vPos;
	int m_nTick;
	bool m_bIsCollision;

	GrenadePathPoint_t(vec3_t pos, int tick) : m_vPos(pos), m_nTick(tick), m_bIsCollision(false) {}

	GrenadePathPoint_t(vec3_t pos, int tick, bool isCollision) : m_vPos(pos), m_nTick(tick), m_bIsCollision(isCollision) {}

	bool HasPassed(int currentTick) const
	{
		return currentTick > m_nTick;
	}
};

class CGrenadePrediction
{
private:
	std::vector<std::pair<ImVec2, ImVec2>> m_vecScreenPoints;
	std::vector<std::pair<ImVec2, ImVec2>> m_vecEndPoints;
	std::vector<std::pair<ImVec2, std::string>> m_vecDmgPoints;

	int m_iGrenadeAct = 1;
private:
	enum EAct
	{
		ACT_NONE,
		ACT_THROW,
		ACT_LOB,
		ACT_DROP,
	};
public:
	bool IsHoldingGrenade();
	void clear_static_maps();
	void grenade_release();
	void SetGrenadeAct(int nButtons);
	void TraceHull(vec3_t& vSrc, vec3_t& end, c_game_trace* pGameTrace) const;
	void Setup(vec3_t& vSrc, vec3_t& vThrow, qangle_t aViewangles) const noexcept;
	int PhysicsClipVelocity(const vec3_t& vIn, const vec3_t& vNormal, vec3_t& vOut, float flOverBounce) const noexcept;
	void PushEntity(vec3_t& vSrc, const vec3_t& vMove, c_game_trace* pGameTrace) const noexcept;
	void ResolveFlyCollisionCustom(c_game_trace* pGameTrace, vec3_t& vVelocity, float flInterval) const noexcept;
	bool CheckDetonate(const vec3_t& vThrow, c_game_trace* pGameTrace, int iTick, float flInterval, c_weapon_base* pActiveWeapon) noexcept;
	void AddGravityMove(vec3_t& vMove, vec3_t& vVel, float flFrameTime, bool bOnGround) noexcept;
	float CalculateArmor(float flDamage, int iArmorValue) noexcept;
	int get_grenade_type(uint64_t uHashedName);
	const char* get_grenade_name(int nade_type);
	int get_he_damage(GrenadePredictionObject_t& rNadeData, c_cs_player_pawn* pPawn, float flNadeDMG, float flNadeRAD);
	void ProximityWarning();
	void DrawGrenadeNames();
	std::vector<GrenadePathPoint_t> GetGrenadePathFromEntity(vec3_t vSrc, vec3_t vVelocity, c_base_cs_grenade* pGrenade, float flSpawnTime);
	void Run();
};
inline static const auto g_GrenadePrediction = std::make_unique<CGrenadePrediction>();