#pragma once

#include <sdk/entity/pawn.h>
#include <sdk/interfaces/trace.h>
#include <cheat/features/aimbot shared/hitbox.h>

class c_trace_result
{
public:
	vec3_t m_end;      //0x0000
	char pad_000C[28]; //0x000C
};

class c_handle_bullet_penetration_data
{
public:
	float m_damage;
	float m_penetration;
	float m_range_modifier;
	float m_range;
	int m_pen_count;
	bool m_failed;
	bool m_can_penetrate;

	c_handle_bullet_penetration_data() : m_damage(0.f), m_penetration(0.f), m_range_modifier(1.f), m_range(8192.f), m_pen_count(4), m_failed(false), m_can_penetrate(false) { }
};

struct penetration_data_t {
	float m_damage{ };
	uint8_t m_hitgroup{ };
	int m_hitbox{ };
	bool m_penetrated{ true };
};

struct local_context_t {
	// weapon data
	float m_headshot_multiplier;
	float m_armor_ratio;
	float m_damage;
	float m_penetration;
	float m_range_mod;
	int   m_local_team;
};
static local_context_t m_local_context;

class c_penetration
{
public:
	struct player_context_t {
		c_cs_player_pawn* m_pawn = nullptr;
		c_skeleton_instance* m_skeleton = nullptr;
		bool m_has_helmet{};
		bool m_has_heavy_armor{};
		float m_head_scale{};
		float m_body_scale{};
		float m_stomach_scale{};
		float m_legs_scale{};
		float m_armor_value{};
		float m_armor_ratio{};
		float m_armor_bonus{};
		float m_heavy_armor_bonus{};
		float m_hitgroup_scale[8]{};
		bool  m_hitgroup_has_armor[8]{};

		int scale_damage(const int& hitgroup, float damage);
		void fill(c_cs_player_pawn* pawn);

		bool fire_bullet(vec3_t start, vec3_t& end, c_cs_player_pawn* target, c_handle_bullet_penetration_data& data);
	} m_player_context;

	struct trace_calls_t {
		void create_trace(c_trace_data* trace_data, vec3_t start, vec3_t delta, c_trace_filter* filter);
		uint64_t damage_to_point(c_trace_data* trace_data, float damage, float penetration, float range_modifier, int team_num);
	} m_trace_calls;

	void update_local_ctx();
};
inline auto g_penetration = std::make_unique<c_penetration>();