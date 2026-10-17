#include "grenade.h"
#include "sdk/interfaces/csgo_input.h"
#include "cheat/features/entity cache/entity_cache.h"
#include <sdk/constants.h>
#include <context.h>
#include <sdk/interfaces/engine_cvar.h>
#include <core/interfaces/interfaces.h>
#include <sdk/interfaces/resource_system.h>
#include <sdk/datatypes/key_values.h>
#include <imgui/imgui_internal.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/menu/hell_gui/hell_gui.h>

void AddLineShadow(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, ImU32 shadow_col, float shadow_thickness, float line_thickness = 1.0f)
{
	if ((shadow_col & IM_COL32_A_MASK) == 0 || shadow_thickness <= 0.0f)
		return;

	ImVec2 dir = ImVec2(p2.x - p1.x, p2.y - p1.y);
	float len = ImSqrt(dir.x * dir.x + dir.y * dir.y);
	if (len == 0.0f)
		return;

	dir.x /= len;
	dir.y /= len;

	ImVec2 perp = ImVec2(-dir.y, dir.x);
	float half_thickness = (line_thickness + shadow_thickness) * 0.5f;

	ImVec2 points[4];
	points[0] = ImVec2(p1.x + perp.x * half_thickness, p1.y + perp.y * half_thickness);
	points[1] = ImVec2(p1.x - perp.x * half_thickness, p1.y - perp.y * half_thickness);
	points[2] = ImVec2(p2.x - perp.x * half_thickness, p2.y - perp.y * half_thickness);
	points[3] = ImVec2(p2.x + perp.x * half_thickness, p2.y + perp.y * half_thickness);

	draw_list->AddShadowConvexPoly(points, 4, shadow_col, shadow_thickness, ImVec2(0, 0));
}

struct panorama_image_data_t
{
	c_utl_buffer m_buffer;

	panorama_image_data_t(std::vector<uint8_t>& vBuffer)
		: m_buffer(vBuffer.data(), (int)vBuffer.size(), 0) {
	}

	bool load_svg(const char* svg_data, size_t data_size, uint32_t* out_width, uint32_t* out_height) const
	{
		static auto svg_parser_fn = g_modules->m_panorama.find(xx("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 4C 8B AD")).as<bool(__fastcall*)(
			const char* /* svg_buffer */,
			unsigned int /* buffer_size */,
			const c_utl_buffer* /* utl_buffer */,
			uint32_t* /* out_width */,
			uint32_t* /* out_height */,
			float /* scale */,
			void* /* unknown */
			)>();

		if (!svg_parser_fn)
			return false;

		return svg_parser_fn(svg_data, data_size, &m_buffer, out_width, out_height, -1, 0);
		//return false;
	}
};

icon_data_t get_panorama_texture(const std::string& path)
{
	const uint32_t hash_path = fnv1a::hash_32(path.c_str());
	auto icon = m_icons.find(hash_path);

	if (icon == m_icons.end())
	{

		static auto load_text_file = g_modules->m_client.find(xx("40 55 57 41 56 48 83 EC ? 4C 8B F1")).as<uint8_t * (__fastcall*)(const char*, void*)>();

		std::string full_path = std::format("panorama/images/{}.vsvg_c", path);
		std::string svg_data;

		const auto file_data = load_text_file(full_path.c_str(), nullptr);
		if (!file_data)
			return {};

		uint8_t* current_pos = file_data;
		while (!std::string(reinterpret_cast<const char*>(current_pos)).starts_with("<svg "))
			++current_pos;

		svg_data = reinterpret_cast<const char*>(current_pos);

		if (svg_data.empty())
			return {};

		std::vector<uint8_t> texture_data(0xFFFFFF);
		panorama_image_data_t image(texture_data);

		uint32_t width = 0, height = 0;
		if (!image.load_svg(svg_data.c_str(), svg_data.size(), &width, &height))
			return {};

		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* texture_view = nullptr;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA sub_resource = {};
		sub_resource.pSysMem = texture_data.data();
		sub_resource.SysMemPitch = desc.Width * 4;
		sub_resource.SysMemSlicePitch = 0;

		auto device = g_interfaces->m_device;
		device->CreateTexture2D(&desc, &sub_resource, &texture);

		if (texture)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = desc.MipLevels;
			srv_desc.Texture2D.MostDetailedMip = 0;

			device->CreateShaderResourceView(texture, &srv_desc, &texture_view);
			texture->Release();
		}

		if (!texture_view)
			return {};

		icon = m_icons.insert_or_assign(hash_path, icon_data_t(texture_view, width, height)).first;
		return icon->second;
	}

	return icon->second;
}

icon_data_t get_grenade_icon(int index)
{
	switch (index) {
	case -1:
		return get_panorama_texture(("icons/equipment/inferno"));
	case 0:
		return get_panorama_texture(("icons/equipment/hegrenade"));
	case 1:
		return get_panorama_texture(("icons/equipment/flashbang"));
	case 2:
		return get_panorama_texture(("icons/equipment/molotov"));
	case 3:
		return get_panorama_texture(("icons/equipment/decoy"));
	case 4:
		return get_panorama_texture(("icons/equipment/smokegrenade"));
	}

}

static bool CompareVectors(const vec3_t& a, const vec3_t& b)
{
	return (a.x < b.x) || (a.x == b.x && a.y < b.y);
}

std::vector<vec3_t> ConvexHull(std::vector<vec3_t> points)
{
	if (points.size() < 3)
		return points;

	std::sort(points.begin(), points.end(), CompareVectors);

	std::vector<vec3_t> hull;

	auto crossProduct = [](const vec3_t& O, const vec3_t& A, const vec3_t& B)
		{
			return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
		};

	for (const auto& p : points)
	{
		while (hull.size() >= 2 && crossProduct(hull[hull.size() - 2], hull.back(), p) <= 0)
		{
			hull.pop_back();
		}
		hull.push_back(p);
	}

	size_t lowerHullSize = hull.size();
	for (int i = points.size() - 2; i >= 0; i--)
	{
		while (hull.size() > lowerHullSize && crossProduct(hull[hull.size() - 2], hull.back(), points[i]) <= 0)
		{
			hull.pop_back();
		}
		hull.push_back(points[i]);
	}

	if (!hull.empty())
		hull.pop_back();

	return hull;
}

void PolygonGradient(ImDrawList* pDrawList, std::vector<ImVec2>& vecPoints, const hellcolor& colPolygon, int flEdgeAlpha, int flCenterAlpha)
{
	if (vecPoints.size() < 3)
		return;

	ImVec2 vecCenter(0.0f, 0.0f);
	for (const auto& point : vecPoints)
	{
		vecCenter.x += point.x;
		vecCenter.y += point.y;
	}
	vecCenter.x /= static_cast<float>(vecPoints.size());
	vecCenter.y /= static_cast<float>(vecPoints.size());

	auto DistanceToEdge = [](const ImVec2& point, const std::vector<ImVec2>& edges) -> float {
		float flMinDistance = FLT_MAX;
		int nCount = static_cast<int>(edges.size());

		for (int i = 0; i < nCount; i++)
		{
			const ImVec2& p1 = edges[i];
			const ImVec2& p2 = edges[(i + 1) % nCount];

			ImVec2 edge = ImVec2(p2.x - p1.x, p2.y - p1.y);
			float flEdgeLength = std::sqrt(edge.x * edge.x + edge.y * edge.y);

			if (flEdgeLength < 0.001f)
				continue;

			ImVec2 toPoint = ImVec2(point.x - p1.x, point.y - p1.y);

			float flT = std::clamp((toPoint.x * edge.x + toPoint.y * edge.y) / (flEdgeLength * flEdgeLength), 0.0f, 1.0f);

			ImVec2 closestPoint = ImVec2(p1.x + flT * edge.x, p1.y + flT * edge.y);

			float flDistance = std::sqrt((point.x - closestPoint.x) * (point.x - closestPoint.x) +
				(point.y - closestPoint.y) * (point.y - closestPoint.y));

			if (flDistance < flMinDistance)
				flMinDistance = flDistance;
		}

		return flMinDistance;
		};

	float flMaxDistance = DistanceToEdge(vecCenter, vecPoints);

	if (flMaxDistance < 0.1f)
		return;

	const int nCount = static_cast<int>(vecPoints.size());
	unsigned int vtx_base = pDrawList->_VtxCurrentIdx;
	pDrawList->PrimReserve(nCount * 3, nCount + 1);

	const ImVec2 uv = pDrawList->_Data->TexUvWhitePixel;

	hellcolor colCenter = hellcolor(
		colPolygon.Value.x,
		colPolygon.Value.y,
		colPolygon.Value.z,
		flCenterAlpha / 255.0f
	);
	pDrawList->PrimWriteVtx(vecCenter, uv, colCenter);

	for (int i = 0; i < nCount; i++)
	{
		const auto& point = vecPoints[i];
		float flDistance = DistanceToEdge(point, vecPoints);

		float flNormalizedDist = flDistance / flMaxDistance;
		float flAlphaFactor = flNormalizedDist;

		float flAlpha = flEdgeAlpha + (flCenterAlpha - flEdgeAlpha) * flAlphaFactor;
		flAlpha = std::clamp(flAlpha, 0.0f, 255.0f);

		hellcolor colPoint = hellcolor(colPolygon.Value.x, colPolygon.Value.y, colPolygon.Value.z, flAlpha / 255.0f);

		pDrawList->PrimWriteVtx(point, uv, colPoint);
	}

	for (int i = 0; i < nCount; i++)
	{
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base));
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + i));
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((i + 1) % nCount)));
	}
}


void CircleGradient(ImDrawList* pDrawList, const vec3_t& worldCenter, float worldRadius, const hellcolor& color, int edgeAlpha, int centerAlpha)
{
	if (worldRadius <= 0.0f)
		return;

	vec2_t vCenterScreen;
	if (!g_math->world_to_screen(worldCenter, vCenterScreen))
		return;

	const int numSegments = 64;
	const float angleStep = (2.0f * A_PI) / numSegments;

	unsigned int vtx_base = pDrawList->_VtxCurrentIdx;
	pDrawList->PrimReserve(numSegments * 3, numSegments + 1);

	const ImVec2 uv = pDrawList->_Data->TexUvWhitePixel;

	hellcolor colCenter = hellcolor(
		color.Value.x,
		color.Value.y,
		color.Value.z,
		centerAlpha / 255.0f
	);
	pDrawList->PrimWriteVtx(ImVec2(vCenterScreen.x, vCenterScreen.y), uv, colCenter);

	for (int i = 0; i < numSegments; i++)
	{
		float angle = i * angleStep;
		vec3_t worldPoint = vec3_t(
			worldCenter.x + std::cos(angle) * worldRadius,
			worldCenter.y + std::sin(angle) * worldRadius,
			worldCenter.z
		);

		vec2_t screenPoint;
		if (g_math->world_to_screen(worldPoint, screenPoint)) {
			hellcolor colPoint = hellcolor(color.Value.x, color.Value.y, color.Value.z, edgeAlpha / 255.0f);
			pDrawList->PrimWriteVtx(ImVec2(screenPoint.x, screenPoint.y), uv, colPoint);
		} else {
			pDrawList->PrimWriteVtx(ImVec2(vCenterScreen.x, vCenterScreen.y), uv, colCenter);
		}
	}

	for (int i = 0; i < numSegments; i++)
	{
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base));
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + i));
		pDrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((i + 1) % numSegments)));
	}
}

void DrawSmoke(C_SmokeGrenadeProjectile* pSmoke)
{
	if (!pSmoke || !GET_VAR(bool, VISUALS_PATH(m_enable_smoke_radius)))
		return;
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;
	if (!g_ctx->m_local_pawn)
		return;
	if (!g_ctx->m_local_controller)
		return;
	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	if (!pSmoke->m_bSmokeEffectSpawned())
		return;

	static std::unordered_map<C_SmokeGrenadeProjectile*, float> m_mapSmokeAlpha;
	static std::unordered_map<C_SmokeGrenadeProjectile*, float> m_mapSmokeStartTime;

	float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
	float flMaxTime = 18.5f;
	
	int nSmokeStartTick = pSmoke->m_nSmokeEffectTickBegin();
	float flSmokeStartTime = ticks_to_time(nSmokeStartTick);
	float flElapsedTime = flCurrentTime - flSmokeStartTime;
	float flRemainingTime = std::max(0.0f, flMaxTime - flElapsedTime);
	
	if (flRemainingTime <= 0.0f) {
		m_mapSmokeAlpha.erase(pSmoke);
		m_mapSmokeStartTime.erase(pSmoke);
		return;
	}

	vec3_t vPlayerPos = g_ctx->m_local_pawn->get_world_space_center();
	vec3_t vSmokePos = pSmoke->m_vSmokeDetonationPos();
	float flDistance = vPlayerPos.dist_to(vSmokePos);
	
	float flTargetAlpha = 1.0f;
	
	if (m_mapSmokeStartTime.find(pSmoke) == m_mapSmokeStartTime.end()) {
		m_mapSmokeStartTime[pSmoke] = flCurrentTime;
		m_mapSmokeAlpha[pSmoke] = 0.0f;
	}
	
	float flFadeInTime = 0.15f;
	float flFadeOutTime = 0.3f;
	float flTimeSinceStart = flCurrentTime - m_mapSmokeStartTime[pSmoke];
	
	if (flTimeSinceStart < flFadeInTime) {
		flTargetAlpha = flTimeSinceStart / flFadeInTime;
	} else if (flRemainingTime < flFadeOutTime) {
		flTargetAlpha = flRemainingTime / flFadeOutTime;
	}
	
	float flDistanceAlpha = 1.0f;
	if (flDistance > 200.0f) {
		flDistanceAlpha = std::max(0.0f, 1.0f - (flDistance - 200.0f) / 800.0f);
	}
	
	flTargetAlpha *= flDistanceAlpha;
	
	float flCurrentAlpha = m_mapSmokeAlpha[pSmoke];
	float flLerpSpeed = 12.0f;
	float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
	flCurrentAlpha = flCurrentAlpha + (flTargetAlpha - flCurrentAlpha) * (flLerpSpeed * flDeltaTime);
	m_mapSmokeAlpha[pSmoke] = flCurrentAlpha;
	
	if (flCurrentAlpha < 0.01f)
		return;

	constexpr float radius = 144.0f;
	constexpr int numSegments = 64;
	constexpr float angleStep = (2.0f * A_PI) / numSegments;

	std::vector<ImVec2> vec2DPoints;
	vec2DPoints.reserve(numSegments);

	for (int i = 0; i < numSegments; ++i)
	{
		float angle = i * angleStep;
		vec3_t circlePoint = vec3_t(
			vSmokePos.x + std::cos(angle) * radius,
			vSmokePos.y + std::sin(angle) * radius,
			vSmokePos.z
		);

		vec2_t vecScreenPosition;
		if (g_math->world_to_screen(circlePoint, vecScreenPosition)) {
			vec2DPoints.emplace_back(ImVec2(vecScreenPosition.x, vecScreenPosition.y));
		}
	}

	if (!vec2DPoints.empty())
	{
		hellcolor colSmoke = GET_VAR(hellcolor, VISUALS_PATH(m_smoke_radius_color));
		colSmoke.Value.w *= flCurrentAlpha;
		
		int nToMiddleAlpha = GET_VAR(int, VISUALS_PATH(m_smoke_to_middle_alpha));
		hellcolor colOutline = colSmoke;
		if (nToMiddleAlpha >= 15) {
			int nOutlineAlpha = std::min(255, nToMiddleAlpha + 15);
			colOutline.Value.w = (nOutlineAlpha / 255.0f) * flCurrentAlpha;
		} else {
			colOutline.Value.w = 0.0f;
		}
		
		ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
		
		if (vec2DPoints.size() >= 3) {
			pDrawList->AddPolyline(vec2DPoints.data(), vec2DPoints.size(), colOutline, ImDrawFlags_Closed, 1.0f);
		}
		
		CircleGradient(pDrawList, vSmokePos, radius, colSmoke, GET_VAR(int, VISUALS_PATH(m_smoke_to_middle_alpha)) * flCurrentAlpha, GET_VAR(int, VISUALS_PATH(m_smoke_from_middle_alpha)) * flCurrentAlpha);
	}
}

void DrawInferno(C_Inferno* pInferno)
{
	if (!pInferno || !GET_VAR(bool, VISUALS_PATH(m_enable_inferno_radius)))
		return;
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;
	if (!g_ctx->m_local_pawn)
		return;
	if (!g_ctx->m_local_controller)
		return;
	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	static std::unordered_map<C_Inferno*, float> m_mapInfernoAlpha;
	static std::unordered_map<C_Inferno*, float> m_mapInfernoStartTime;

	std::vector<vec3_t> vecPoints;
	vecPoints.reserve(pInferno->m_fireCount());

	for (int i = 0; i < pInferno->m_fireCount(); ++i)
	{
		if (pInferno->m_bFireIsBurning_at(i))
			vecPoints.emplace_back(pInferno->m_firePositions_at(i));
	}

	if (vecPoints.empty()) {
		m_mapInfernoAlpha.erase(pInferno);
		m_mapInfernoStartTime.erase(pInferno);
		return;
	}

	float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
	float flMaxTime = g_interfaces->m_engine_convar->find_by_name("inferno_flame_lifetime")->get_float();
	
	int nFireStartTick = pInferno->m_nFireEffectTickBegin();
	float flFireStartTime = ticks_to_time(nFireStartTick);
	float flElapsedTime = flCurrentTime - flFireStartTime;
	float flRemainingTime = std::max(0.0f, flMaxTime - flElapsedTime);
	
	vec3_t vPlayerPos = g_ctx->m_local_pawn->get_world_space_center();
	float flMinDistance = FLT_MAX;
	for (const auto& pos : vecPoints) {
		float flDistance = vPlayerPos.dist_to(pos);
		if (flDistance < flMinDistance) {
			flMinDistance = flDistance;
		}
	}
	
	float flTargetAlpha = 1.0f;
	
	if (m_mapInfernoStartTime.find(pInferno) == m_mapInfernoStartTime.end()) {
		m_mapInfernoStartTime[pInferno] = flCurrentTime;
		m_mapInfernoAlpha[pInferno] = 0.0f;
	}
	
	float flFadeInTime = 0.15f;
	float flFadeOutTime = 0.3f;
	float flTimeSinceStart = flCurrentTime - m_mapInfernoStartTime[pInferno];
	
	if (flTimeSinceStart < flFadeInTime) {
		flTargetAlpha = flTimeSinceStart / flFadeInTime;
	} else if (flRemainingTime < flFadeOutTime) {
		flTargetAlpha = flRemainingTime / flFadeOutTime;
	}
	
	float flDistanceAlpha = 1.0f;
	if (flMinDistance > 200.0f) {
		flDistanceAlpha = std::max(0.0f, 1.0f - (flMinDistance - 200.0f) / 800.0f);
	}
	
	flTargetAlpha *= flDistanceAlpha;
	
	float flCurrentAlpha = m_mapInfernoAlpha[pInferno];
	float flLerpSpeed = 12.0f;
	float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
	flCurrentAlpha = flCurrentAlpha + (flTargetAlpha - flCurrentAlpha) * (flLerpSpeed * flDeltaTime);
	m_mapInfernoAlpha[pInferno] = flCurrentAlpha;
	
	if (flCurrentAlpha < 0.01f)
		return;

	constexpr float radius = 60.0f;
	constexpr int numSegments = 90;
	constexpr float angleStep = (2.0f * A_PI) / numSegments;

	std::vector<vec3_t> vecNewPoints;
	vecNewPoints.reserve(vecPoints.size() * numSegments);

	for (const auto& pos : vecPoints)
	{
		for (int j = 0; j < numSegments; ++j)
		{
			float angle = j * angleStep;
			vecNewPoints.emplace_back(pos.x + std::cos(angle) * radius,
				pos.y + std::sin(angle) * radius,
				pos.z);
		}
	}

	std::vector<vec3_t> vecHullPoints = ConvexHull(vecNewPoints);

	if (vecHullPoints.empty())
		return;

	std::vector<ImVec2> vec2DPoints;
	vec2DPoints.reserve(vecHullPoints.size());

	for (const auto& vecPosition : vecHullPoints)
	{
		vec2_t vecScreenPosition;
		if (g_math->world_to_screen(vecPosition, vecScreenPosition)) {
			vec2DPoints.emplace_back(ImVec2(vecScreenPosition.x, vecScreenPosition.y));
		}
	}

	if (!vec2DPoints.empty())
	{
		hellcolor colInferno = GET_VAR(hellcolor, VISUALS_PATH(m_inferno_radius_color));
		colInferno.Value.w *= flCurrentAlpha;
		
		int nToMiddleAlpha = GET_VAR(int, VISUALS_PATH(m_to_middle_alpha));
		hellcolor colOutline = colInferno;
		if (nToMiddleAlpha >= 15) {
			int nOutlineAlpha = std::min(255, nToMiddleAlpha + 15);
			colOutline.Value.w = (nOutlineAlpha / 255.0f) * flCurrentAlpha;
		} else {
			colOutline.Value.w = 0.0f;
		}
		
		ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
		
		if (vec2DPoints.size() >= 3 && colOutline.Value.w > 0.0f) {
			pDrawList->AddPolyline(vec2DPoints.data(), vec2DPoints.size(), colOutline, ImDrawFlags_Closed, 1.0f);
		}
		
		PolygonGradient(pDrawList, vec2DPoints, colInferno, GET_VAR(int, VISUALS_PATH(m_to_middle_alpha)) * flCurrentAlpha, GET_VAR(int, VISUALS_PATH(m_from_middle_alpha)) * flCurrentAlpha);
	}
}

void GrenadePredictionObject_t::Draw(hellcolor cColor, bool bOverlay, bool bEffects)
{
	if (bOverlay)
	{
		ImDrawList* pDrawList = ImGui::GetForegroundDrawList();
		if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
			return;
		if (!g_ctx->m_local_controller)
			return;
		if (!g_ctx->m_local_controller->m_bPawnIsAlive())
			return;

		if (m_GrenadePathPoint.empty()) return;

		int nCurrentTick = time_to_ticks(g_interfaces->m_global_vars->m_curtime);
		int nCurrentIndex = -1;

		for (size_t i = 0; i < m_GrenadePathPoint.size(); ++i)
		{
			if (m_GrenadePathPoint[i].m_nTick <= nCurrentTick)
			{
				nCurrentIndex = i;
			}
			else
			{
				break;
			}
		}

		if (nCurrentIndex == -1 || nCurrentIndex >= (int)m_GrenadePathPoint.size() - 1)
			return;

		vec3_t vPrev = m_GrenadePathPoint[nCurrentIndex].m_vPos;
		vec2_t vNadeStart_2D, vNadeEnd_2D;

		float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
		float flCycleTime = 2.f;
		float flPulseTime = 4.2f;
		float flCycleProgress = fmod(flCurrentTime, flCycleTime);
		float flWavePosition = -1.0f;
		float flWaveIntensity = 0.0f;
		
		bool bPulsatingEnabled = GET_VAR(bool, VISUALS_PATH(m_grenade_prediction_pulsating));
		if (bPulsatingEnabled && flCycleProgress <= flPulseTime) {
			flWavePosition = (flCycleProgress / flPulseTime) * 3.2f - 0.3f;
			flWaveIntensity = sin((flCycleProgress / flPulseTime) * IM_PI);
		}

		float flCurrentGrenadeProgress = (float)nCurrentIndex / (float)m_GrenadePathPoint.size();

		for (size_t i = nCurrentIndex + 1; i < m_GrenadePathPoint.size(); ++i)
		{
			auto& vNadePos = m_GrenadePathPoint[i];
			if (g_math->world_to_screen(vPrev, vNadeStart_2D) && g_math->world_to_screen(vNadePos.m_vPos, vNadeEnd_2D))
			{
				float flSegmentProgress = (float)i / (float)m_GrenadePathPoint.size();
				float flRelativeProgress = (flSegmentProgress - flCurrentGrenadeProgress) / (1.0f - flCurrentGrenadeProgress);
				
				hellcolor segmentColor = cColor;
				if (bPulsatingEnabled && flWavePosition >= -0.3f && flWaveIntensity > 0.0f) {
					float flWaveDistance = abs(flRelativeProgress - flWavePosition);
					float flWaveEffect = 1.0f - std::min(flWaveDistance * 2.8f, 1.0f);
					if (flWaveEffect > 0.0f) {
						float hue_shift = 0.15f;
						float r = cColor.Value.x;
						float g = cColor.Value.y;
						float b = cColor.Value.z;
						
						float max_val = std::max({r, g, b});
						float min_val = std::min({r, g, b});
						float delta = max_val - min_val;
						
						float h = 0.0f;
						if (delta > 0.0f) {
							if (max_val == r) h = fmod((g - b) / delta, 6.0f);
							else if (max_val == g) h = (b - r) / delta + 2.0f;
							else h = (r - g) / delta + 4.0f;
							h /= 6.0f;
						}
						
						h = fmod(h + hue_shift, 1.0f);
						if (h < 0.0f) h += 1.0f;
						
						float s = (max_val > 0.0f) ? delta / max_val : 0.0f;
						float v = max_val;
						
						float c = v * s;
						float x = c * (1.0f - abs(fmod(h * 6.0f, 2.0f) - 1.0f));
						float m = v - c;
						
						float r_new, g_new, b_new;
						if (h < 1.0f/6.0f) { r_new = c; g_new = x; b_new = 0; }
						else if (h < 2.0f/6.0f) { r_new = x; g_new = c; b_new = 0; }
						else if (h < 3.0f/6.0f) { r_new = 0; g_new = c; b_new = x; }
						else if (h < 4.0f/6.0f) { r_new = 0; g_new = x; b_new = c; }
						else if (h < 5.0f/6.0f) { r_new = x; g_new = 0; b_new = c; }
						else { r_new = c; g_new = 0; b_new = x; }
						
						segmentColor = hellcolor(
							r_new + m,
							g_new + m,
							b_new + m,
							cColor.Value.w
						);
						
						float flPulseFade = 1.0f;
						if (flWaveEffect < 0.98f) {
							flPulseFade = flWaveEffect / 0.98f;
							flPulseFade = flPulseFade * flPulseFade * flPulseFade * flPulseFade;
							segmentColor = hellcolor(
								cColor.Value.x + (segmentColor.Value.x - cColor.Value.x) * flPulseFade,
								cColor.Value.y + (segmentColor.Value.y - cColor.Value.y) * flPulseFade,
								cColor.Value.z + (segmentColor.Value.z - cColor.Value.z) * flPulseFade,
								cColor.Value.w
							);
						}
					}
				}

			float flAlpha = 1.0f;
			if (i == 1) {
				flAlpha = 0.0f;
			} else if (i <= 24) {
				flAlpha = (float)(i - 1) / 23.0f;
			}
			segmentColor.Value.w *= flAlpha;

			hellcolor glow_color = segmentColor;
			glow_color.Value.w *= 0.12f;
			if (GET_VAR(bool, VISUALS_PATH(m_grenade_prediction_glow))) {
				AddLineShadow(pDrawList, { vNadeStart_2D.x, vNadeStart_2D.y }, { vNadeEnd_2D.x, vNadeEnd_2D.y }, glow_color, 5.0f, 1.2f);
			}
			
			pDrawList->AddLine({ vNadeStart_2D.x,vNadeStart_2D.y }, { vNadeEnd_2D.x,vNadeEnd_2D.y }, segmentColor, 1.2f);
			}
			vPrev = vNadePos.m_vPos;
		}

		int nGrenadeType = -1;
		if (g_ctx->m_active_weapon) {
			fnv1a_t uHashedName = fnv1a::hash_32(g_ctx->m_active_weapon->get_class_name());
			nGrenadeType = g_GrenadePrediction->get_grenade_type(uHashedName);
		}

		bool bIsLastCollisionBeforeEnd = false;
		if (!m_GrenadePathPoint.empty() && nGrenadeType == 2) {
			for (size_t i = nCurrentIndex + 1; i < m_GrenadePathPoint.size(); ++i) {
				if (m_GrenadePathPoint[i].m_bIsCollision) {
					if (i == m_GrenadePathPoint.size() - 1 || 
						(i + 1 < m_GrenadePathPoint.size() && m_GrenadePathPoint[i + 1].m_vPos.dist_to(m_GrenadePathPoint.back().m_vPos) < 5.0f)) {
						bIsLastCollisionBeforeEnd = true;
						break;
					}
				}
			}
		}

		for (size_t i = nCurrentIndex + 1; i < m_GrenadePathPoint.size(); ++i)
		{
			auto& vNadePos = m_GrenadePathPoint[i];
			if (vNadePos.m_bIsCollision && g_math->world_to_screen(vNadePos.m_vPos, vNadeEnd_2D))
			{
				bool bSkipThisCircle = false;
				if (nGrenadeType == 2 && bIsLastCollisionBeforeEnd) {
					float flDistToEnd = vNadePos.m_vPos.dist_to(m_GrenadePathPoint.back().m_vPos);
					if (flDistToEnd < 5.0f) {
						bSkipThisCircle = true;
					}
				}
				
				if (!bSkipThisCircle) {
					hellcolor circle_color = hellcolor(255, 255, 255, 255);
					hellcolor circle_outline = hellcolor(0, 0, 0, 200);
					
					pDrawList->AddCircleFilled({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, circle_color, 12);
					pDrawList->AddCircle({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, circle_outline, 12, 0.5f);
				}
			}
		}

		if (nCurrentIndex < (int)m_GrenadePathPoint.size() && g_math->world_to_screen(m_GrenadePathPoint.back().m_vPos, vNadeEnd_2D))
		{
			pDrawList->AddCircleFilled({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, hellcolor(255, 148, 148, 255), 12);
			pDrawList->AddCircle({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, hellcolor(0, 0, 0, 200), 12, 0.5f);
		}
	}

	if (bEffects && !m_bDrewParticle)
	{
		/*std::vector<vec3_t> vecPoints;
		for (auto& GrenadePathPoint : m_GrenadePathPoint)
		{
			vecPoints.emplace_back(GrenadePathPoint.m_vPos);
		}

		g_ParticleMgr->AddParticlePoints("particles/entity/spectator_utility_trail.vpcf", vecPoints, Config::c(g_Variables.m_WorldVisuals.m_cGrenadePrediction2), 3.f, false);

		m_bDrewParticle = true;*/
	}
}

void CGrenadePrediction::SetGrenadeAct(int nButtons)
{
	bool bInAttack = nButtons & IN_ATTACK;
	bool bInAttack2 = nButtons & IN_ATTACK2;

	m_iGrenadeAct = (bInAttack && bInAttack2) ? ACT_LOB :
		(bInAttack2) ? ACT_DROP :
		(bInAttack) ? ACT_THROW :
		ACT_NONE;
}

bool CGrenadePrediction::IsHoldingGrenade()
{
	if (!g_ctx->m_local_pawn || !g_ctx->m_active_weapon)
		return false;
	
	int weapon_id = g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	return (weapon_id == WEAPON_HIGH_EXPLOSIVE_GRENADE || 
			weapon_id == WEAPON_FLASHBANG || 
			weapon_id == WEAPON_DECOY_GRENADE || 
			weapon_id == WEAPON_MOLOTOV || 
			weapon_id == WEAPON_INCENDIARY_GRENADE || 
			weapon_id == WEAPON_SMOKE_GRENADE);
}

void CGrenadePrediction::clear_static_maps()
{
	s_vecPredictedGrenades.clear();
}

void CGrenadePrediction::grenade_release()
{
	if (!GET_VAR(bool, MISC_PATH(m_enabled_grenade_release)))
		return;
	
	if (!g_ctx->m_local_pawn || !g_ctx->m_active_weapon)
		return;
	
	int weapon_id = g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	if (weapon_id != WEAPON_HIGH_EXPLOSIVE_GRENADE)
		return;
	
	c_base_cs_grenade* pGrenade = reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon);
	if (!pGrenade)
		return;
	
	if (!pGrenade->m_bPinPulled())
		return;
	
	if (!(g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK || g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK2))
		return;
	
	vec3_t vStartPos = g_ctx->m_local_pawn->get_eye_pos(), vVelocity;
	qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
	
	this->Setup(vStartPos, vVelocity, view_angles);
	std::vector<GrenadePathPoint_t> vecGrenadePathPoint = this->GetGrenadePathFromEntity(vStartPos, vVelocity, pGrenade, g_interfaces->m_global_vars->m_curtime);
	
	if (vecGrenadePathPoint.empty())
		return;
	
	GrenadePredictionObject_t tempNadeData;
	tempNadeData.m_GrenadePathPoint = vecGrenadePathPoint;
	
	int nMaxDamage = 0;
	
	for (auto& player : g_entity_cache->m_players) {
		c_cs_player_controller* pController = reinterpret_cast<c_cs_player_controller*>(player.m_controller);
		if (!pController)
			continue;

		c_cs_player_pawn* pPawn = pController->m_hPawn().get<c_cs_player_pawn>();
		if (!pPawn)
			continue;

		if (pPawn == g_ctx->m_local_pawn)
			continue;

		if (pController->m_iTeamNum() == g_ctx->m_local_controller->m_iTeamNum())
			continue;

		int nDamage = this->get_he_damage(tempNadeData, pPawn, 99.f, 350.f);
		if (nDamage > nMaxDamage) {
			nMaxDamage = nDamage;
		}
	}
	
	int nTargetDamage = GET_VAR(int, MISC_PATH(m_grenade_release_damage));
	
	if (nMaxDamage >= nTargetDamage) {
		g_ctx->m_cmd->m_buttons.m_value &= ~IN_ATTACK;
		g_ctx->m_cmd->m_buttons.m_value &= ~IN_ATTACK2;
	}
}

void CGrenadePrediction::TraceHull(vec3_t& vSrc, vec3_t& end, c_game_trace* pGameTrace) const
{
	c_ray ray{};
	c_trace_filter filter{ 0x1C3003, g_ctx->m_local_pawn, NULL, 4 };

	g_interfaces->m_phys2world->trace_shape(&ray, &vSrc, &end, &filter, pGameTrace);
}

void CGrenadePrediction::Setup(vec3_t& vSrc, vec3_t& vThrow, qangle_t aViewangles) const noexcept
{
	qangle_t aThrow = aViewangles;
	float flPitch = aThrow.x;

	if (flPitch <= 90.0f)
	{
		if (flPitch < -90.0f)
		{
			flPitch += 360.0f;
		}
	}
	else
	{
		flPitch -= 360.0f;
	}

	float a = flPitch - (90.0f - fabs(flPitch)) * 10.0f / 90.0f;
	aThrow.x = a;

	float flVel = 750.0f * 0.9f;

	static const float arrPower[] = { 1.0f, 1.0f, 0.5f, 0.0f };
	float b = arrPower[m_iGrenadeAct];
	b = b * 0.7f;
	b = b + 0.3f;
	flVel *= b;

	vec3_t vForward, vRight, vUp;
	aThrow.to_directions(&vForward, &vRight, &vUp);

	float off = (arrPower[m_iGrenadeAct] * 12.0f) - 12.0f;
	vSrc.z += off;

	c_game_trace GameTrace;
	vec3_t vecDest = vSrc;
	vecDest += vForward * 22.0f;

	TraceHull(vSrc, vecDest, &GameTrace);

	vec3_t vBack = vForward;
	vBack *= 6.0f;
	vSrc = GameTrace.m_end_pos;
	vSrc -= vBack;

	vThrow = g_ctx->m_local_pawn->m_vecAbsVelocity();
	vThrow *= 1.25f;
	vThrow += vForward * flVel;
}

int CGrenadePrediction::PhysicsClipVelocity(const vec3_t& vIn, const vec3_t& vNormal, vec3_t& vOut, float flOverBounce) const noexcept
{
	static const float STOP_EPSILON = 0.1f;

	float backoff;
	float change;
	float angle;
	int i, blocked;

	blocked = 0;

	angle = vNormal[2];

	if (angle > 0)
	{
		blocked |= 1;
	}
	if (!angle)
	{
		blocked |= 2;
	}

	backoff = vIn.dot(vNormal) * flOverBounce;

	for (i = 0; i < 3; i++)
	{
		change = vNormal[i] * backoff;
		vOut[i] = vIn[i] - change;
		if (vOut[i] > -STOP_EPSILON && vOut[i] < STOP_EPSILON)
		{
			vOut[i] = 0;
		}
	}

	return blocked;
}

void CGrenadePrediction::PushEntity(vec3_t& vSrc, const vec3_t& vMove, c_game_trace* pGameTrace) const noexcept
{
	vec3_t vecAbsEnd = vSrc;
	vecAbsEnd += vMove;
	this->TraceHull(vSrc, vecAbsEnd, pGameTrace);
}

void CGrenadePrediction::ResolveFlyCollisionCustom(c_game_trace* pGameTrace, vec3_t& vVelocity, float flInterval) const noexcept
{
	float surfaceElasticity = 1.0f;
	float grenadeElasticity = 0.45f;
	float totalElasticity = grenadeElasticity * surfaceElasticity;
	if (totalElasticity > 0.9f) totalElasticity = 0.9f;
	if (totalElasticity < 0.0f) totalElasticity = 0.0f;

	vec3_t vAbsVelocity;
	PhysicsClipVelocity(vVelocity, pGameTrace->m_hit_normal, vAbsVelocity, 2.0f);
	vAbsVelocity *= totalElasticity;

	float speedSqr = vAbsVelocity.length_sqr();
	static const float minSpeedSqr = 20.0f * 20.0f;

	if (speedSqr < minSpeedSqr)
	{
		vAbsVelocity.x = 0.0f;
		vAbsVelocity.y = 0.0f;
		vAbsVelocity.z = 0.0f;
	}

	if (pGameTrace->m_hit_normal.z > 0.7f)
	{
		float flSpeedSqr = vAbsVelocity.length_sqr();
		if (flSpeedSqr > 96000.0f)
		{
			float flLen = vAbsVelocity.normalized().dot(pGameTrace->m_hit_normal);
			if (flLen > 0.5f)
				vAbsVelocity *= (1.0f - flLen) + 0.5f;
		}

		if (flSpeedSqr < 400.0f)
		{
			vVelocity.x = 0.0f;
			vVelocity.y = 0.0f;
			vVelocity.z = 0.0f;
		}
		else
		{
			vVelocity = vAbsVelocity;
			vec3_t vPush = vAbsVelocity * ((1.0f - pGameTrace->m_fraction) * flInterval);
			PushEntity(pGameTrace->m_end_pos, vPush, pGameTrace);
		}
	}
	else
	{
		vVelocity = vAbsVelocity;
		vec3_t vPush = vAbsVelocity * ((1.0f - pGameTrace->m_fraction) * flInterval);
		PushEntity(pGameTrace->m_end_pos, vPush, pGameTrace);
	}
}
bool CGrenadePrediction::CheckDetonate(const vec3_t& vThrow, c_game_trace* pGameTrace, int iTick, float flInterval, c_weapon_base* pActiveWeapon) noexcept
{
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return false;
	if (!g_ctx->m_active_weapon)
		return false;
	
	int weapon_id = g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	static float flTpLen = 0.1f;
	switch (weapon_id)
	{
	case WEAPON_SMOKE_GRENADE:
	case WEAPON_DECOY_GRENADE:
		if (vThrow.length_2d() < flTpLen && (float)iTick * flInterval > 1.0f)
		{
			int iDetonateTickMod = (int)(0.2f / flInterval);
			return !(iTick % iDetonateTickMod);
		}
		return false;
	case WEAPON_MOLOTOV:
	case WEAPON_INCENDIARY_GRENADE:
		if (pGameTrace->m_fraction != 1.0f && pGameTrace->m_hit_normal.z > 0.7f)
			return true;
		return (float)iTick * flInterval > 2.0f;
	case WEAPON_FLASHBANG:
	case WEAPON_HIGH_EXPLOSIVE_GRENADE:
		return (float)iTick * flInterval > 1.5f && !(iTick % (int)(0.2f / flInterval));
	default:
		return false;
	}
}

void CGrenadePrediction::AddGravityMove(vec3_t& vMove, vec3_t& vVel, float flFrameTime, bool bOnGround) noexcept
{
	float flGravity = 800.0f * 0.4f;
	float flNewZ = vVel.z - (flGravity * flFrameTime);
	vMove.x = vVel.x * flFrameTime;
	vMove.y = vVel.y * flFrameTime;
	vMove.z = ((vVel.z + flNewZ) / 2.0f) * flFrameTime;
	vVel.z = flNewZ;
}

float CGrenadePrediction::CalculateArmor(float flDamage, int iArmorValue) noexcept
{
	if (iArmorValue > 0)
	{
		float newDamage = flDamage * 0.5f;
		float armor = (flDamage - newDamage) * 0.5f;

		if (armor > static_cast<float>(iArmorValue)) {
			armor = static_cast<float>(iArmorValue) * (1.f / 0.5f);
			newDamage = flDamage - armor;
		}

		flDamage = newDamage;
	}
	return flDamage;
}

void circle_progress(ImDrawList* pDrawList, const ImVec2& center, float radius,
	const hellcolor& color, float thickness, float progress, float start_angle) {
	if (progress <= 0.0f) return;

	const float bottom_angle = IM_PI * 0.5f;
	const float progress_angle = IM_PI * progress;
	const int num_segments = 32;

	float left_start = bottom_angle - progress_angle;
	float right_end = bottom_angle + progress_angle;

	pDrawList->PathClear();

	const float small_gap = 0.02f;
	if (progress >= 1.0f - small_gap * 2.0f) {
		pDrawList->PathArcTo(ImVec2(center.x, center.y), radius, left_start, right_end, num_segments * 2);
	}
	else {
		float left_end = bottom_angle - small_gap;
		float right_start = bottom_angle + small_gap;

		if (left_start <= left_end) {
			pDrawList->PathArcTo(ImVec2(center.x, center.y), radius, left_start, left_end, num_segments);
		}
		if (right_start <= right_end) {
			pDrawList->PathArcTo(ImVec2(center.x, center.y), radius, right_start, right_end, num_segments);
		}
	}
	pDrawList->PathStroke(color, ImDrawFlags_None, thickness);
}


void CGrenadePrediction::ProximityWarning()
{
	if (!GET_VAR(bool, VISUALS_PATH(m_enable_proximity_warnings)))
		return;
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;
	if (!g_ctx->m_local_pawn)
		return;
	if (g_ctx->m_local_pawn->m_iHealth() <= 0)
		return;
	if (!g_ctx->m_local_controller)
		return;
	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	static std::unordered_map<c_base_entity*, ImVec2> m_mapGrenadePrevPositions;
	static std::unordered_map<c_base_entity*, float> m_mapGrenadeLastUpdateTime;
	static std::unordered_map<c_base_entity*, float> m_mapGrenadeAlpha;
	static std::unordered_map<c_base_entity*, float> m_mapGrenadeStartTime;
	static std::unordered_map<c_base_entity*, float> m_mapDamageTextAlpha;
	static std::unordered_map<c_base_entity*, float> m_mapIconDamageAlpha;
	static std::unordered_map<c_base_entity*, int> m_mapLastDamage;

	std::vector<c_base_entity*> vecActiveGrenades;
	for (auto& rProjectile : s_vecPredictedGrenades) {
		if (rProjectile.m_pGrenadeEntity)
			vecActiveGrenades.push_back(rProjectile.m_pGrenadeEntity);
	}

	for (auto it = m_mapGrenadePrevPositions.begin(); it != m_mapGrenadePrevPositions.end();) {
		if (std::find(vecActiveGrenades.begin(), vecActiveGrenades.end(), it->first) == vecActiveGrenades.end()) {
			m_mapGrenadeLastUpdateTime.erase(it->first);
			m_mapGrenadeAlpha.erase(it->first);
			m_mapGrenadeStartTime.erase(it->first);
			m_mapDamageTextAlpha.erase(it->first);
			m_mapIconDamageAlpha.erase(it->first);
			m_mapLastDamage.erase(it->first);
			it = m_mapGrenadePrevPositions.erase(it);
		}
		else {
			++it;
		}
	}

	for (auto& rProjectile : s_vecPredictedGrenades) {
		/*if (rProjectile.m_bDrewParticle)*/ {

			rProjectile.m_flDistance = g_ctx->m_local_pawn->get_world_space_center().dist_to(rProjectile.m_GrenadePathPoint.back().m_vPos);

			if (rProjectile.m_GrenadePathPoint.empty() || rProjectile.m_flDistance == 0.f)
				continue;

			vec2_t vEndPosPoint{};
			bool bIsOnScreen = g_math->world_to_screen(rProjectile.m_GrenadePathPoint.back().m_vPos, vEndPosPoint);
			const ImVec2 vDisplaySize = ImGui::GetIO().DisplaySize;
			const ImVec2 vDisplayCenter = vDisplaySize / 2.f;
			float flPadding = vDisplaySize.x * -0.01f;

			constexpr auto RotatePoint = [](const ImVec2& vPoint, const ImVec2& vCenter, float flDeg) -> ImVec2 {
				flDeg = DEG2RAD(flDeg);

				const auto flCos = cosf(flDeg);
				const auto flSin = sinf(flDeg);

				ImVec2 vReturn = ImVec2();
				vReturn.x = flCos * (vPoint.x - vCenter.x) - flSin * (vPoint.y - vCenter.y);
				vReturn.y = flSin * (vPoint.x - vCenter.x) + flCos * (vPoint.y - vCenter.y);

				vReturn += vCenter;

				return vReturn;
				};

			auto flAlpha = std::clamp(exp(-(rProjectile.m_flDistance - 850.f) / 25.f), 0.f, 1.f);
			if (rProjectile.m_flDistance < 850.f)
				flAlpha = 1.f;

			if (flAlpha < 0.05f)
				continue;

			float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
			
			if (m_mapGrenadeStartTime.find(rProjectile.m_pGrenadeEntity) == m_mapGrenadeStartTime.end()) {
				m_mapGrenadeStartTime[rProjectile.m_pGrenadeEntity] = flCurrentTime;
				m_mapGrenadeAlpha[rProjectile.m_pGrenadeEntity] = 0.0f;
			}
			
			int nCurrentTick = time_to_ticks(g_interfaces->m_global_vars->m_curtime);
			int nStartTick = rProjectile.m_GrenadePathPoint.front().m_nTick;
			int nDetonationTick = rProjectile.m_nTickDetonation;
			int nTotalTicks = nDetonationTick - nStartTick;
			int nRemainingTicks = nDetonationTick - nCurrentTick;

			float flNormalTime = 1.0f;
			if (nTotalTicks > 0) {
				flNormalTime = (float)std::max(0, nRemainingTicks) / (float)nTotalTicks;
				flNormalTime = std::clamp(flNormalTime, 0.0f, 1.0f);
			}
			
			float flTargetAlpha = flAlpha;
			float flFadeInTime = 0.15f;
			float flTimeSinceStart = flCurrentTime - m_mapGrenadeStartTime[rProjectile.m_pGrenadeEntity];
			
			if (flTimeSinceStart < flFadeInTime) {
				flTargetAlpha *= (flTimeSinceStart / flFadeInTime);
			} else if (flNormalTime < 0.15f) {
				flTargetAlpha *= (flNormalTime / 0.15f);
			}
			
			float flCurrentAlpha = m_mapGrenadeAlpha[rProjectile.m_pGrenadeEntity];
			float flLerpSpeed = 12.0f;
			float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
			flCurrentAlpha = flCurrentAlpha + (flTargetAlpha - flCurrentAlpha) * (flLerpSpeed * flDeltaTime);
			m_mapGrenadeAlpha[rProjectile.m_pGrenadeEntity] = flCurrentAlpha;
			
			if (flCurrentAlpha < 0.01f)
				continue;

			ImVec2 vP1, vP2, vP3, vCircle;
			ImVec2 vArrowScreenPos(0.f, 0.f);
			float flAngle = 0.f;
			bool bIsOOF = false;
			if (!bIsOnScreen || vEndPosPoint.x < -flPadding || vEndPosPoint.x >(vDisplaySize.x + flPadding)
				|| vEndPosPoint.y < -flPadding || vEndPosPoint.y >(vDisplaySize.y + flPadding)) {
				bIsOOF = true;

				vec3_t vecViewOrigin = g_ctx->m_local_pawn->get_world_space_center();
				vec3_t vecTargetPos = rProjectile.m_GrenadePathPoint.back().m_vPos;
				vec3_t vecDelta = (vecTargetPos - vecViewOrigin).normalized();

				qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
				vec3_t fwd, right, up(0.f, 0.f, 1.f);

				g_math->angle_vectors(view_angles, fwd);
				fwd.z = 0.f;
				fwd.normalize();

				right = up.cross(fwd);
				float front = vecDelta.dot(fwd);
				float side = vecDelta.dot(right);

				const float flOffset = -(vDisplayCenter.x - 15.f) * (0.9f);
				float flRadiusX = flOffset;
				float flRadiusY = flOffset * (vDisplaySize.y / vDisplaySize.x);

				vec3_t vecOffScreenPos;
				vecOffScreenPos.x = flRadiusX * -side;
				vecOffScreenPos.y = flRadiusY * -front;

				float flOffScreenRotation = RAD2DEG(std::atan2(vecOffScreenPos.x, vecOffScreenPos.y) + A_PI);

				float yaw_rad = DEG2RAD(-flOffScreenRotation);
				float sa = std::sin(yaw_rad);
				float ca = std::cos(yaw_rad);

				vArrowScreenPos = ImVec2(
					vDisplayCenter.x + (flRadiusX * sa),
					vDisplayCenter.y - (flRadiusY * ca)
				);

				flOffScreenRotation = -flOffScreenRotation;
				flAngle = flOffScreenRotation;

				vP1 = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 18.f }, vArrowScreenPos, flAngle);
				vP2 = RotatePoint({ vArrowScreenPos.x - 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
				vP3 = RotatePoint({ vArrowScreenPos.x + 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
				vCircle = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y - 30.f }, vArrowScreenPos, flAngle);
			}
			else {
				vCircle = ImVec2(vEndPosPoint.x, vEndPosPoint.y);
			}

			ImDrawList* drawList = ImGui::GetForegroundDrawList();

			hellcolor vProxColor = GET_VAR(hellcolor, VISUALS_PATH(m_proximity_warnings_color));

			ImColor vProxColorAlpha25 = ImColor(
				vProxColor.Value.x,
				vProxColor.Value.y,
				vProxColor.Value.z,
				0.25f * flCurrentAlpha
			);

			ImColor vProxColorAlpha50 = ImColor(
				vProxColor.Value.x,
				vProxColor.Value.y,
				vProxColor.Value.z,
				0.3f * flCurrentAlpha
			);

			ImColor vProxColorFullAlpha = ImColor(
				vProxColor.Value.x,
				vProxColor.Value.y,
				vProxColor.Value.z,
				flCurrentAlpha
			);

			drawList->AddCircleFilled(ImVec2(vCircle.x, vCircle.y), 24.f, IM_COL32(30, 30, 30, (int)(180 * flCurrentAlpha)));

			if (bIsOOF) {
				ImVec2 vBackCenter = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 8.f }, vArrowScreenPos, flAngle);

				hellcolor glow_color = vProxColor;
				glow_color.Value.w *= 0.6f * flCurrentAlpha;
				ImU32 glow_color_u32 = ImGui::ColorConvertFloat4ToU32(glow_color.Value);
				
				ImVec2 glow_points[3] = { vP1, vP2, vP3 };
				drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 15.0f, ImVec2(0, 0));
				drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 10.0f, ImVec2(0, 0));
				drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 6.0f, ImVec2(0, 0));

				drawList->Flags |= ImDrawListFlags_AntiAliasedFill;
				drawList->PathClear();
				drawList->PathLineTo(ImVec2(vP1.x, vP1.y));
				drawList->PathLineTo(ImVec2(vP2.x, vP2.y));
				drawList->PathLineTo(vBackCenter);
				drawList->PathLineTo(ImVec2(vP3.x, vP3.y));
				drawList->PathFillConvex(vProxColorFullAlpha);
			}


			auto pIconData = get_grenade_icon(rProjectile.m_iNadeType);
			{
				int iTargetSize = 26;
				const auto flWtoHRatio = static_cast<float>(pIconData.width) / static_cast<float>(pIconData.height);
				auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
				auto Height = iTargetSize;
				ImVec2 vPos = vCircle;
				vPos.x -= Width * 0.5f;
				vPos.y -= Height * 0.5f;

				if (pIconData.texture_view) {
					bool bCanDamage = (rProjectile.m_iNadeType == 0 && rProjectile.m_flDistance <= rProjectile.m_flNadeRadius);
					int nDamage = 0;
					if (bCanDamage) {
						nDamage = get_he_damage(rProjectile, g_ctx->m_local_pawn, rProjectile.m_flNadeDamage, rProjectile.m_flNadeRadius);
					}

					float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
					float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
					
					if (m_mapDamageTextAlpha.find(rProjectile.m_pGrenadeEntity) == m_mapDamageTextAlpha.end()) {
						m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity] = 0.0f;
						m_mapIconDamageAlpha[rProjectile.m_pGrenadeEntity] = 1.0f;
						m_mapLastDamage[rProjectile.m_pGrenadeEntity] = 0;
					}
					
					float flTargetTextAlpha = (nDamage > 0) ? 1.0f : 0.0f;
					float flTargetIconAlpha = (nDamage > 0) ? 0.2f : 1.0f;
					
					float flLerpSpeed = 8.0f;
					m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity] = m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity] + (flTargetTextAlpha - m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity]) * (flLerpSpeed * flDeltaTime);
					m_mapIconDamageAlpha[rProjectile.m_pGrenadeEntity] = m_mapIconDamageAlpha[rProjectile.m_pGrenadeEntity] + (flTargetIconAlpha - m_mapIconDamageAlpha[rProjectile.m_pGrenadeEntity]) * (flLerpSpeed * flDeltaTime);
					
					if (nDamage > 0) {
						m_mapLastDamage[rProjectile.m_pGrenadeEntity] = nDamage;
					}

					hellcolor iconColor;
					float flIconAlpha = m_mapIconDamageAlpha[rProjectile.m_pGrenadeEntity] * flCurrentAlpha;
					iconColor = hellcolor(255, 255, 255, (int)(255 * flIconAlpha));

					drawList->AddImage((ImTextureID)pIconData.texture_view, ImVec2(vPos.x, vPos.y),
						ImVec2(vPos.x + Width, vPos.y + Height),
						ImVec2(0, 0), ImVec2(1, 1),
						iconColor);

					if (m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity] > 0.01f) {
						int nDisplayDamage = m_mapLastDamage[rProjectile.m_pGrenadeEntity];
						int nTextAlpha = (int)(255 * m_mapDamageTextAlpha[rProjectile.m_pGrenadeEntity] * flCurrentAlpha);
						
						std::string damageStr = "-" + std::to_string(nDisplayDamage) + " HP";
						const char* text = damageStr.c_str();
						ImVec2 textSize = g_font_manager->m_verdana_12_bold->CalcTextSizeA(12.f, FLT_MAX, 0.0f, text);

						ImVec2 textPos(
							vCircle.x - textSize.x * 0.5f,
							vCircle.y - textSize.y * 0.5f
						);

						float flColorRatio = std::min(1.0f, nDisplayDamage / 50.0f);
						int r = 255;
						int g = (int)(255 - (255 - 173) * flColorRatio);
						int b = (int)(255 - (255 - 173) * flColorRatio);
						ImU32 textColor = IM_COL32(r, g, b, nTextAlpha);

						drawList->AddText(g_font_manager->m_verdana_12_bold, 12.f, textPos, textColor, text);
					}
				}
			}

			ImColor vCircleProgressBg = ImColor(50, 50, 50, (int)(100 * flCurrentAlpha));
			circle_progress(drawList, vCircle, 23.f, vCircleProgressBg, 0.75f, 1.0f, 0.0f);
			circle_progress(drawList, vCircle, 23.f, vProxColorFullAlpha, 0.75f, flNormalTime, 1.5708f);
			if (!g_interfaces->m_global_vars->m_tick_count)
				continue;
			if (!g_interfaces->m_global_vars)
				continue;
			if (!rProjectile.m_iLastTick)
				continue;
			if (rProjectile.m_GrenadePathPoint_1.empty())
				continue;
			if (g_interfaces->m_global_vars->m_tick_count > rProjectile.m_iLastTick) {
				rProjectile.m_iLastTick = g_interfaces->m_global_vars->m_tick_count;
				rProjectile.m_GrenadePathPoint_1.pop_front();
			}

			if (rProjectile.m_iNadeType == 0) {
				if (rProjectile.m_flDistance <= rProjectile.m_flNadeRadius) {
					rProjectile.m_iDamage = get_he_damage(rProjectile, g_ctx->m_local_pawn, rProjectile.m_flNadeDamage, rProjectile.m_flNadeRadius);
				}
			}
		}
	}

	static std::unordered_map<int, vec3_t> m_mapInfernoPrevPositions;
	static std::unordered_map<int, float> m_mapInfernoLastUpdateTime;
	static std::unordered_map<int, float> m_mapInfernoProxAlpha;
	static std::unordered_map<int, float> m_mapInfernoProxStartTime;

	std::vector<int> vecInfernoIndices;
	for (auto& object : g_entity_cache->m_entity) {
		vecInfernoIndices.push_back(object.m_idx);

	}

	for (auto it = m_mapInfernoPrevPositions.begin(); it != m_mapInfernoPrevPositions.end();) {
		if (std::find(vecInfernoIndices.begin(), vecInfernoIndices.end(), it->first) == vecInfernoIndices.end()) {
			m_mapInfernoLastUpdateTime.erase(it->first);
			m_mapInfernoProxAlpha.erase(it->first);
			m_mapInfernoProxStartTime.erase(it->first);
			it = m_mapInfernoPrevPositions.erase(it);
		}
		else {
			++it;
		}
	}

	for (auto& object : g_entity_cache->m_entity) {
		C_Inferno* pInferno = reinterpret_cast<C_Inferno*>(object.m_pEntity);
		if (!pInferno)
			continue;

		vec3_t vPlayerPos = g_ctx->m_local_pawn->get_world_space_center();
		float flMinDistance = FLT_MAX;
		vec3_t vClosestFirePos{};
		bool bHasBurningFire = false;

		for (int i = 0; i < pInferno->m_fireCount(); ++i) {
			if (pInferno->m_bFireIsBurning_at(i)) {
				bHasBurningFire = true;
				vec3_t vFirePos = pInferno->m_firePositions_at(i);
				float flDistance = vPlayerPos.dist_to(vFirePos);
				if (flDistance < flMinDistance) {
					flMinDistance = flDistance;
					vClosestFirePos = vFirePos;
				}
			}
		}

		if (!bHasBurningFire || flMinDistance == FLT_MAX)
			continue;

		int nInfernoIndex = object.m_idx;
		float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
		vec3_t vTargetPos = vClosestFirePos;
		vec3_t vSmoothPos = vTargetPos;

		if (m_mapInfernoPrevPositions.find(nInfernoIndex) != m_mapInfernoPrevPositions.end()) {
			vec3_t vPrevPos = m_mapInfernoPrevPositions[nInfernoIndex];
			float flLastUpdateTime = m_mapInfernoLastUpdateTime[nInfernoIndex];
			float flDeltaTime = flCurrentTime - flLastUpdateTime;

			const float flLerpSpeed = 8.0f;
			float flLerpFactor = std::clamp(flDeltaTime * flLerpSpeed, 0.0f, 1.0f);

			vSmoothPos = vPrevPos + (vTargetPos - vPrevPos) * flLerpFactor;
		}
		else {
			vSmoothPos = vTargetPos;
		}

		m_mapInfernoPrevPositions[nInfernoIndex] = vSmoothPos;
		m_mapInfernoLastUpdateTime[nInfernoIndex] = flCurrentTime;

		vec2_t vEndPosPoint{};
		bool bIsOnScreen = g_math->world_to_screen(vSmoothPos, vEndPosPoint);
		const ImVec2 vDisplaySize = ImGui::GetIO().DisplaySize;
		const ImVec2 vDisplayCenter = vDisplaySize / 2.f;
		float flPadding = vDisplaySize.x * -0.01f;

		constexpr auto RotatePoint = [](const ImVec2& vPoint, const ImVec2& vCenter, float flDeg) -> ImVec2 {
			flDeg = DEG2RAD(flDeg);

			const auto flCos = cosf(flDeg);
			const auto flSin = sinf(flDeg);

			ImVec2 vReturn = ImVec2();
			vReturn.x = flCos * (vPoint.x - vCenter.x) - flSin * (vPoint.y - vCenter.y);
			vReturn.y = flSin * (vPoint.x - vCenter.x) + flCos * (vPoint.y - vCenter.y);

			vReturn += vCenter;

			return vReturn;
			};

		auto flAlpha = std::clamp(exp(-(flMinDistance - 850.f) / 25.f), 0.f, 1.f);
		if (flMinDistance < 850.f)
			flAlpha = 1.f;

		if (flAlpha < 0.05f)
			continue;

		if (m_mapInfernoProxStartTime.find(nInfernoIndex) == m_mapInfernoProxStartTime.end()) {
			m_mapInfernoProxStartTime[nInfernoIndex] = flCurrentTime;
			m_mapInfernoProxAlpha[nInfernoIndex] = 0.0f;
		}
		
		float flMaxTime = g_interfaces->m_engine_convar->find_by_name("inferno_flame_lifetime")->get_float();
		int nFireStartTick = pInferno->m_nFireEffectTickBegin();
		float flFireStartTime = ticks_to_time(nFireStartTick);
		float flElapsedTime = flCurrentTime - flFireStartTime;
		float flRemainingTime = std::max(0.0f, flMaxTime - flElapsedTime);
		
		float flTargetAlpha = flAlpha;
		float flFadeInTime = 0.15f;
		float flFadeOutTime = 0.4f;
		float flTimeSinceStart = flCurrentTime - m_mapInfernoProxStartTime[nInfernoIndex];
		
		if (flTimeSinceStart < flFadeInTime) {
			flTargetAlpha *= (flTimeSinceStart / flFadeInTime);
		} else if (flRemainingTime < flFadeOutTime) {
			flTargetAlpha *= (flRemainingTime / flFadeOutTime);
		}
		
		float flCurrentAlpha = m_mapInfernoProxAlpha[nInfernoIndex];
		float flLerpSpeed = 12.0f;
		float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
		flCurrentAlpha = flCurrentAlpha + (flTargetAlpha - flCurrentAlpha) * (flLerpSpeed * flDeltaTime);
		m_mapInfernoProxAlpha[nInfernoIndex] = flCurrentAlpha;
		
		if (flCurrentAlpha < 0.01f)
			continue;

		ImVec2 vP1, vP2, vP3, vCircle;
		ImVec2 vArrowScreenPos(0.f, 0.f);
		float flAngle = 0.f;
		bool bIsOOF = false;
		if (!bIsOnScreen || vEndPosPoint.x < -flPadding || vEndPosPoint.x >(vDisplaySize.x + flPadding)
			|| vEndPosPoint.y < -flPadding || vEndPosPoint.y >(vDisplaySize.y + flPadding)) {
			bIsOOF = true;

			vec3_t vecViewOrigin = g_ctx->m_local_pawn->get_world_space_center();
			vec3_t vecTargetPos = vSmoothPos;
			vec3_t vecDelta = (vecTargetPos - vecViewOrigin).normalized();

			qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
			vec3_t fwd, right, up(0.f, 0.f, 1.f);

			g_math->angle_vectors(view_angles, fwd);
			fwd.z = 0.f;
			fwd.normalize();

			right = up.cross(fwd);
			float front = vecDelta.dot(fwd);
			float side = vecDelta.dot(right);

			const float flOffset = -(vDisplayCenter.x - 15.f) * (0.9f);
			float flRadiusX = flOffset;
			float flRadiusY = flOffset * (vDisplaySize.y / vDisplaySize.x);

			vec3_t vecOffScreenPos;
			vecOffScreenPos.x = flRadiusX * -side;
			vecOffScreenPos.y = flRadiusY * -front;

			float flOffScreenRotation = RAD2DEG(std::atan2(vecOffScreenPos.x, vecOffScreenPos.y) + A_PI);

			float yaw_rad = DEG2RAD(-flOffScreenRotation);
			float sa = std::sin(yaw_rad);
			float ca = std::cos(yaw_rad);

			vArrowScreenPos = ImVec2(vDisplayCenter.x + (flRadiusX * sa), vDisplayCenter.y - (flRadiusY * ca));

			flOffScreenRotation = -flOffScreenRotation;
			flAngle = flOffScreenRotation;

			vP1 = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 18.f }, vArrowScreenPos, flAngle);
			vP2 = RotatePoint({ vArrowScreenPos.x - 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
			vP3 = RotatePoint({ vArrowScreenPos.x + 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
			vCircle = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y - 30.f }, vArrowScreenPos, flAngle);
		}
		else {
			vCircle = ImVec2(vEndPosPoint.x, vEndPosPoint.y);
		}

		float flNormalTime = 1.0f;
		if (flMaxTime > 0.f) {
			flNormalTime = std::clamp(flRemainingTime / flMaxTime, 0.0f, 1.0f);
		}

		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		hellcolor vProxColor = GET_VAR(hellcolor, VISUALS_PATH(m_proximity_warnings_color));

		ImColor vProxColorAlpha25 = ImColor(
			vProxColor.Value.x,
			vProxColor.Value.y,
			vProxColor.Value.z,
			0.25f * flCurrentAlpha
		);

		ImColor vProxColorAlpha50 = ImColor(
			vProxColor.Value.x,
			vProxColor.Value.y,
			vProxColor.Value.z,
			0.3f * flCurrentAlpha
		);

		ImColor vProxColorFullAlpha = ImColor(
			vProxColor.Value.x,
			vProxColor.Value.y,
			vProxColor.Value.z,
			flCurrentAlpha
		);

		drawList->AddCircleFilled(ImVec2(vCircle.x, vCircle.y), 24.f, IM_COL32(30, 30, 30, (int)(180 * flCurrentAlpha)));

		if (bIsOOF) {
			ImVec2 vBackCenter = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 8.f }, vArrowScreenPos, flAngle);

			hellcolor glow_color = vProxColor;
			glow_color.Value.w *= 0.6f * flCurrentAlpha;
			ImU32 glow_color_u32 = ImGui::ColorConvertFloat4ToU32(glow_color.Value);
			
			ImVec2 glow_points[3] = { vP1, vP2, vP3 };
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 15.0f, ImVec2(0, 0));
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 10.0f, ImVec2(0, 0));
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 6.0f, ImVec2(0, 0));

			drawList->Flags |= ImDrawListFlags_AntiAliasedFill;
			drawList->PathClear();
			drawList->PathLineTo(ImVec2(vP1.x, vP1.y));
			drawList->PathLineTo(ImVec2(vP2.x, vP2.y));
			drawList->PathLineTo(vBackCenter);
			drawList->PathLineTo(ImVec2(vP3.x, vP3.y));
			drawList->PathFillConvex(vProxColorFullAlpha);
		}

		auto pIconData = get_grenade_icon(-1);
		{
			int iTargetSize = 22;
			const auto flWtoHRatio = static_cast<float>(pIconData.width) / static_cast<float>(pIconData.height);
			auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
			auto Height = iTargetSize;
			ImVec2 vPos = vCircle;
			vPos.x -= Width * 0.5f;
			vPos.y -= Height * 0.5f;

			if (pIconData.texture_view) {
				hellcolor iconColor = hellcolor(255, 255, 255, (int)(255 * flCurrentAlpha));

				drawList->AddImage((ImTextureID)pIconData.texture_view, ImVec2(vPos.x, vPos.y),
					ImVec2(vPos.x + Width, vPos.y + Height),
					ImVec2(0, 0), ImVec2(1, 1),
					iconColor);
			}
		}

		ImColor vCircleProgressBg = ImColor(50, 50, 50, (int)(100 * flCurrentAlpha));
		circle_progress(drawList, vCircle, 23.f, vCircleProgressBg, 0.75f, 1.0f, 0.0f);
		circle_progress(drawList, vCircle, 23.f, vProxColorFullAlpha, 0.75f, flNormalTime, 1.5708f);
	}

	static std::unordered_map<int, vec3_t> m_mapSmokePrevPositions;
	static std::unordered_map<int, float> m_mapSmokeLastUpdateTime;
	static std::unordered_map<int, float> m_mapSmokeProxAlpha;
	static std::unordered_map<int, float> m_mapSmokeProxStartTime;

	std::vector<int> vecSmokeIndices;
	for (auto& object : g_entity_cache->m_grenade_entity) {
		vecSmokeIndices.push_back(object.m_idx);
	}

	for (auto it = m_mapSmokePrevPositions.begin(); it != m_mapSmokePrevPositions.end();) {
		if (std::find(vecSmokeIndices.begin(), vecSmokeIndices.end(), it->first) == vecSmokeIndices.end()) {
			m_mapSmokeLastUpdateTime.erase(it->first);
			m_mapSmokeProxAlpha.erase(it->first);
			m_mapSmokeProxStartTime.erase(it->first);
			it = m_mapSmokePrevPositions.erase(it);
		}
		else {
			++it;
		}
	}

	for (auto& object : g_entity_cache->m_grenade_entity) {
		C_SmokeGrenadeProjectile* pSmoke = reinterpret_cast<C_SmokeGrenadeProjectile*>(object.m_pEntity);
		if (!pSmoke)
			continue;

		if (!pSmoke->m_bSmokeEffectSpawned())
			continue;

		vec3_t vPlayerPos = g_ctx->m_local_pawn->get_world_space_center();
		vec3_t vSmokePos = pSmoke->m_vSmokeDetonationPos();
		float flDistance = vPlayerPos.dist_to(vSmokePos);

		int nSmokeIndex = object.m_idx;
		float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
		vec3_t vTargetPos = vSmokePos;
		vec3_t vSmoothPos = vTargetPos;

		if (m_mapSmokePrevPositions.find(nSmokeIndex) != m_mapSmokePrevPositions.end()) {
			vec3_t vPrevPos = m_mapSmokePrevPositions[nSmokeIndex];
			float flLastUpdateTime = m_mapSmokeLastUpdateTime[nSmokeIndex];
			float flDeltaTime = flCurrentTime - flLastUpdateTime;

			const float flLerpSpeed = 8.0f;
			float flLerpFactor = std::clamp(flDeltaTime * flLerpSpeed, 0.0f, 1.0f);

			vSmoothPos = vPrevPos + (vTargetPos - vPrevPos) * flLerpFactor;
		}
		else {
			vSmoothPos = vTargetPos;
		}

		m_mapSmokePrevPositions[nSmokeIndex] = vSmoothPos;
		m_mapSmokeLastUpdateTime[nSmokeIndex] = flCurrentTime;

		vec2_t vEndPosPoint{};
		bool bIsOnScreen = g_math->world_to_screen(vSmoothPos, vEndPosPoint);
		const ImVec2 vDisplaySize = ImGui::GetIO().DisplaySize;
		const ImVec2 vDisplayCenter = vDisplaySize / 2.f;
		float flPadding = vDisplaySize.x * -0.01f;

		constexpr auto RotatePoint = [](const ImVec2& vPoint, const ImVec2& vCenter, float flDeg) -> ImVec2 {
			flDeg = DEG2RAD(flDeg);

			const auto flCos = cosf(flDeg);
			const auto flSin = sinf(flDeg);

			ImVec2 vReturn = ImVec2();
			vReturn.x = flCos * (vPoint.x - vCenter.x) - flSin * (vPoint.y - vCenter.y);
			vReturn.y = flSin * (vPoint.x - vCenter.x) + flCos * (vPoint.y - vCenter.y);

			vReturn += vCenter;

			return vReturn;
			};

		auto flAlpha = std::clamp(exp(-(flDistance - 850.f) / 25.f), 0.f, 1.f);
		if (flDistance < 850.f)
			flAlpha = 1.f;

		if (flAlpha < 0.05f)
			continue;

		if (m_mapSmokeProxStartTime.find(nSmokeIndex) == m_mapSmokeProxStartTime.end()) {
			m_mapSmokeProxStartTime[nSmokeIndex] = flCurrentTime;
			m_mapSmokeProxAlpha[nSmokeIndex] = 0.0f;
		}
		
		float flMaxTime = 18.5f;
		int nSmokeStartTick = pSmoke->m_nSmokeEffectTickBegin();
		float flSmokeStartTime = ticks_to_time(nSmokeStartTick);
		float flElapsedTime = flCurrentTime - flSmokeStartTime;
		float flRemainingTime = std::max(0.0f, flMaxTime - flElapsedTime);
		
		if (flRemainingTime <= 0.0f)
			continue;
		
		float flTargetAlpha = flAlpha;
		float flFadeInTime = 0.15f;
		float flFadeOutTime = 0.4f;
		float flTimeSinceStart = flCurrentTime - m_mapSmokeProxStartTime[nSmokeIndex];
		
		if (flTimeSinceStart < flFadeInTime) {
			flTargetAlpha *= (flTimeSinceStart / flFadeInTime);
		} else if (flRemainingTime < flFadeOutTime) {
			flTargetAlpha *= (flRemainingTime / flFadeOutTime);
		}
		
		float flCurrentAlpha = m_mapSmokeProxAlpha[nSmokeIndex];
		float flLerpSpeed = 12.0f;
		float flDeltaTime = g_interfaces->m_global_vars->m_frame_time;
		flCurrentAlpha = flCurrentAlpha + (flTargetAlpha - flCurrentAlpha) * (flLerpSpeed * flDeltaTime);
		m_mapSmokeProxAlpha[nSmokeIndex] = flCurrentAlpha;
		
		if (flCurrentAlpha < 0.01f)
			continue;

		ImVec2 vP1, vP2, vP3, vCircle;
		ImVec2 vArrowScreenPos(0.f, 0.f);
		float flAngle = 0.f;
		bool bIsOOF = false;
		if (!bIsOnScreen || vEndPosPoint.x < -flPadding || vEndPosPoint.x >(vDisplaySize.x + flPadding)
			|| vEndPosPoint.y < -flPadding || vEndPosPoint.y >(vDisplaySize.y + flPadding)) {
			bIsOOF = true;

			vec3_t vecViewOrigin = g_ctx->m_local_pawn->get_world_space_center();
			vec3_t vecTargetPos = vSmoothPos;
			vec3_t vecDelta = (vecTargetPos - vecViewOrigin).normalized();

			qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
			vec3_t fwd, right, up(0.f, 0.f, 1.f);

			g_math->angle_vectors(view_angles, fwd);
			fwd.z = 0.f;
			fwd.normalize();

			right = up.cross(fwd);
			float front = vecDelta.dot(fwd);
			float side = vecDelta.dot(right);

			const float flOffset = -(vDisplayCenter.x - 15.f) * (0.9f);
			float flRadiusX = flOffset;
			float flRadiusY = flOffset * (vDisplaySize.y / vDisplaySize.x);

			vec3_t vecOffScreenPos;
			vecOffScreenPos.x = flRadiusX * -side;
			vecOffScreenPos.y = flRadiusY * -front;

			float flOffScreenRotation = RAD2DEG(std::atan2(vecOffScreenPos.x, vecOffScreenPos.y) + A_PI);

			float yaw_rad = DEG2RAD(-flOffScreenRotation);
			float sa = std::sin(yaw_rad);
			float ca = std::cos(yaw_rad);

			vArrowScreenPos = ImVec2(vDisplayCenter.x + (flRadiusX * sa), vDisplayCenter.y - (flRadiusY * ca));

			flOffScreenRotation = -flOffScreenRotation;
			flAngle = flOffScreenRotation;

			vP1 = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 18.f }, vArrowScreenPos, flAngle);
			vP2 = RotatePoint({ vArrowScreenPos.x - 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
			vP3 = RotatePoint({ vArrowScreenPos.x + 4.f, vArrowScreenPos.y + 6.f }, vArrowScreenPos, flAngle);
			vCircle = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y - 30.f }, vArrowScreenPos, flAngle);
		}
		else {
			vCircle = ImVec2(vEndPosPoint.x, vEndPosPoint.y);
		}

		float flNormalTime = 1.0f;
		if (flMaxTime > 0.f) {
			flNormalTime = std::clamp(flRemainingTime / flMaxTime, 0.0f, 1.0f);
		}

		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		hellcolor vProxColor = GET_VAR(hellcolor, VISUALS_PATH(m_proximity_warnings_color));

		ImColor vProxColorFullAlpha = ImColor(
			vProxColor.Value.x,
			vProxColor.Value.y,
			vProxColor.Value.z,
			flCurrentAlpha
		);

		drawList->AddCircleFilled(ImVec2(vCircle.x, vCircle.y), 24.f, IM_COL32(30, 30, 30, (int)(180 * flCurrentAlpha)));

		if (bIsOOF) {
			ImVec2 vBackCenter = RotatePoint({ vArrowScreenPos.x, vArrowScreenPos.y + 8.f }, vArrowScreenPos, flAngle);

			hellcolor glow_color = vProxColor;
			glow_color.Value.w *= 0.6f * flCurrentAlpha;
			ImU32 glow_color_u32 = ImGui::ColorConvertFloat4ToU32(glow_color.Value);
			
			ImVec2 glow_points[3] = { vP1, vP2, vP3 };
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 15.0f, ImVec2(0, 0));
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 10.0f, ImVec2(0, 0));
			drawList->AddShadowConvexPoly(glow_points, 3, glow_color_u32, 6.0f, ImVec2(0, 0));

			drawList->Flags |= ImDrawListFlags_AntiAliasedFill;
			drawList->PathClear();
			drawList->PathLineTo(ImVec2(vP1.x, vP1.y));
			drawList->PathLineTo(ImVec2(vP2.x, vP2.y));
			drawList->PathLineTo(vBackCenter);
			drawList->PathLineTo(ImVec2(vP3.x, vP3.y));
			drawList->PathFillConvex(vProxColorFullAlpha);
		}

		auto pIconData = get_grenade_icon(4);
		{
			int iTargetSize = 22;
			const auto flWtoHRatio = static_cast<float>(pIconData.width) / static_cast<float>(pIconData.height);
			auto Width = static_cast<uint32_t>(flWtoHRatio * iTargetSize);
			auto Height = iTargetSize;
			ImVec2 vPos = vCircle;
			vPos.x -= Width * 0.5f;
			vPos.y -= Height * 0.5f;

			if (pIconData.texture_view) {
				hellcolor iconColor = hellcolor(255, 255, 255, (int)(255 * flCurrentAlpha));

				drawList->AddImage((ImTextureID)pIconData.texture_view, ImVec2(vPos.x, vPos.y),
					ImVec2(vPos.x + Width, vPos.y + Height),
					ImVec2(0, 0), ImVec2(1, 1),
					iconColor);
			}
		}

		ImColor vCircleProgressBg = ImColor(50, 50, 50, (int)(100 * flCurrentAlpha));
		circle_progress(drawList, vCircle, 23.f, vCircleProgressBg, 0.75f, 1.0f, 0.0f);
		circle_progress(drawList, vCircle, 23.f, vProxColorFullAlpha, 0.75f, flNormalTime, 1.5708f);
	}
}

int CGrenadePrediction::get_he_damage(GrenadePredictionObject_t& rNadeData, c_cs_player_pawn* pPawn, float flNadeDMG, float flNadeRAD) {
	vec3_t vStart = rNadeData.m_GrenadePathPoint.back().m_vPos;
	vec3_t vEnd = pPawn->get_world_space_center();
	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return 0;
	c_ray traceRay;
	c_game_trace gameTrace;
	c_trace_filter traceFilter(0x1C1003, pPawn, nullptr, 4);

	g_interfaces->m_phys2world->trace_shape(&traceRay, &vStart, &vEnd, &traceFilter, &gameTrace);
	if (gameTrace.m_fraction > 0.97f) {
		float flDamage = flNadeDMG * exp(-powf(vEnd.dist_to(vStart), 2.f) / (2.f * powf((flNadeRAD / 3.f), 2.f)));
		if (pPawn->m_ArmorValue() > 0) {
			float flArmorRatioMult = flDamage * 0.5f;
			float flActual = (flDamage - flArmorRatioMult) * 0.5f;

			if (flActual > static_cast<float>(pPawn->m_ArmorValue())) {
				flActual = static_cast<float>(pPawn->m_ArmorValue()) * (1.f / 0.5f);
				flArmorRatioMult = flDamage - flActual;
			}
			flDamage = flArmorRatioMult;
		}
		return static_cast<int>(roundf(flDamage));
	}
	return 0;
}

int CGrenadePrediction::get_grenade_type(uint64_t uHashedName) {
	switch (uHashedName) {
	case fnv1a::hash_32("C_HEGrenadeProjectile"): return 0;
	case fnv1a::hash_32("C_FlashbangProjectile"): return 1;
	case fnv1a::hash_32("C_MolotovProjectile"): return 2;
	case fnv1a::hash_32("C_DecoyProjectile"): return 3;
	case fnv1a::hash_32("C_SmokeGrenadeProjectile"): return 4;
		//Local Nades
	case fnv1a::hash_32("C_HEGrenade"): return 0;
	case fnv1a::hash_32("C_Flashbang"): return 1;
	case fnv1a::hash_32("C_MolotovGrenade"):
	case fnv1a::hash_32("C_IncendiaryGrenade"): return 2;
	case fnv1a::hash_32("C_DecoyGrenade"): return 3;
	case fnv1a::hash_32("C_SmokeGrenade"): return 4;
	}
	return -1;
}

const char* CGrenadePrediction::get_grenade_name(int nade_type) {
	switch (nade_type) {
	case 0: return "Grenade";
	case 1: return "Flashbang";
	case 2: return "Molotov";
	case 3: return "Decoy";
	case 4: return "Smoke";
	}
	return "";
}

std::vector<GrenadePathPoint_t> CGrenadePrediction::GetGrenadePathFromEntity(vec3_t vSrc, vec3_t vVelocity, c_base_cs_grenade* pGrenade, float flSpawnTime)
{
	std::vector<GrenadePathPoint_t> points{};
	int spawnTick = time_to_ticks(flSpawnTime);
	vec3_t velocity = vVelocity;
	vec3_t position = vSrc;

	float flInterval = interval_per_tick;
	int nLogStep = static_cast<int>(0.05f / flInterval);
	int nLogTimer = 0;

	points.emplace_back(GrenadePathPoint_t(position, spawnTick));

	for (unsigned int simTick = 0; simTick < 4096; ++simTick) {
		if (!nLogTimer)
			points.emplace_back(GrenadePathPoint_t(position, spawnTick + simTick));

		vec3_t move;
		this->AddGravityMove(move, velocity, flInterval, false);

		c_game_trace tr;
		PushEntity(position, move, &tr);

		bool bShouldDetonate = false;
		
		if (tr.m_fraction != 1.0f)
		{
			points.emplace_back(GrenadePathPoint_t(tr.m_end_pos, spawnTick + simTick, true));
			
			if (CheckDetonate(velocity, &tr, simTick, flInterval, pGrenade))
				bShouldDetonate = true;
			else
				this->ResolveFlyCollisionCustom(&tr, velocity, flInterval);
			
			nLogTimer = 0;
		}
		else
		{
			c_game_trace dummyTrace;
			dummyTrace.m_fraction = 1.0f;
			if (CheckDetonate(velocity, &dummyTrace, simTick, flInterval, pGrenade))
				bShouldDetonate = true;
		}

		position = tr.m_end_pos;

		if (bShouldDetonate)
			break;

		if (nLogTimer >= nLogStep)
			nLogTimer = 0;
		else
			++nLogTimer;
	}

	return points;
}

void CGrenadePrediction::DrawGrenadeNames()
{
	if (!GET_VAR(bool, VISUALS_PATH(m_enable_grenade_names)))
		return;

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;
	if (!g_ctx->m_local_pawn)
		return;
	if (!g_ctx->m_local_controller)
		return;
	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
		return;

	ImDrawList* pDrawList = ImGui::GetForegroundDrawList();
	if (!g_interfaces->m_global_vars)
		return;
	float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
	int nCurrentTick = time_to_ticks(flCurrentTime);

	for (auto& rProjectile : s_vecPredictedGrenades)
	{
		if (rProjectile.m_GrenadePathPoint.empty())
			continue;

		if (rProjectile.m_iNadeType < 0 || rProjectile.m_iNadeType > 4)
			continue;

		int nStartTick = rProjectile.m_GrenadePathPoint.front().m_nTick;
		int nDetonationTick = rProjectile.m_GrenadePathPoint.back().m_nTick;

		if (nCurrentTick < nStartTick || nCurrentTick >= nDetonationTick)
			continue;

		vec3_t vGrenadePos;
		bool bFoundPos = false;

		for (size_t i = 0; i < rProjectile.m_GrenadePathPoint.size() - 1; ++i)
		{
			const auto& point1 = rProjectile.m_GrenadePathPoint[i];
			const auto& point2 = rProjectile.m_GrenadePathPoint[i + 1];

			if (nCurrentTick >= point1.m_nTick && nCurrentTick <= point2.m_nTick)
			{
				float flDeltaTick = static_cast<float>(point2.m_nTick - point1.m_nTick);
				if (flDeltaTick > 0.0f)
				{
					float flLerpFactor = static_cast<float>(nCurrentTick - point1.m_nTick) / flDeltaTick;
					vGrenadePos = point1.m_vPos + (point2.m_vPos - point1.m_vPos) * flLerpFactor;
				}
				else
				{
					vGrenadePos = point1.m_vPos;
				}
				bFoundPos = true;
				break;
			}
		}

		if (!bFoundPos)
			vGrenadePos = rProjectile.m_GrenadePathPoint.back().m_vPos;

		vec2_t vScreenPos;
		if (!g_math->world_to_screen(vGrenadePos, vScreenPos))
			continue;

		const char* grenade_name = get_grenade_name(rProjectile.m_iNadeType);
		if (!grenade_name || !*grenade_name)
			continue;

		c_visuals::e_font_type font_type = (c_visuals::e_font_type)(GET_VAR(int, VISUALS_PATH(m_grenade_names_font_type)));
		c_visuals::e_text_shadow_type shadow_type = (c_visuals::e_text_shadow_type)(GET_VAR(int, VISUALS_PATH(m_grenade_names_shadow_type)));

		ImFont* font = g_visuals->get_font(font_type);
		if (!font)
			continue;

		hellvec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, grenade_name);
		hellvec2 text_pos = { vScreenPos.x - text_size.x * 0.5f, vScreenPos.y - text_size.y * 0.5f };

		hellcolor fg_color = GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color));
		hellcolor bg_color = GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color_bg));

		if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_drop)
		{
			hellvec2 outline_pos = { text_pos.x + 1.f, text_pos.y + 1.f };
			pDrawList->AddText(font, font->LegacySize, outline_pos, bg_color, grenade_name);
		}
		else if (shadow_type == c_visuals::e_text_shadow_type::text_shadow_full)
		{
			static const hellvec2 outline_offsets[8] = {
				{ -1, -1 },
				{ -1, 1 },
				{ 1, -1 },
				{ 1, 1 },
				{ 0, 1 },
				{ 1, 0 },
				{ 0, -1 },
				{ -1, 0 },
			};

			for (const hellvec2& off : outline_offsets)
			{
				pDrawList->AddText(font, font->LegacySize, text_pos + off, bg_color, grenade_name);
			}
		}

		pDrawList->AddText(font, font->LegacySize, text_pos, fg_color, grenade_name);

		if (GET_VAR(bool, VISUALS_PATH(m_grenade_names_show_icon)))
		{
			auto icon_data = get_grenade_icon(rProjectile.m_iNadeType);
			if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
			{
				int icon_size = 15;
				const float flWtoHRatio = static_cast<float>(icon_data.width) / static_cast<float>(icon_data.height);
				auto icon_width = static_cast<uint32_t>(flWtoHRatio * icon_size);
				auto icon_height = icon_size;

				ImVec2 icon_pos = { vScreenPos.x - icon_width * 0.5f, text_pos.y - icon_height - 4.f };

				pDrawList->AddImage((ImTextureID)icon_data.texture_view,
					ImVec2(icon_pos.x, icon_pos.y),
					ImVec2(icon_pos.x + icon_width, icon_pos.y + icon_height),
					ImVec2(0, 0), ImVec2(1, 1),
					fg_color);
			}
		}
	}
}

void CGrenadePrediction::Run()
{
	m_vecScreenPoints.clear();
	m_vecEndPoints.clear();
	m_vecDmgPoints.clear();

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game())
		return;
	if (!g_ctx->m_local_pawn)
		return;
	if (g_ctx->m_local_pawn->m_iHealth() <= 0)
	{
		s_vecPredictedGrenades.clear();
		return;
	}
	if (!g_ctx->m_local_controller)
		return;
	if (!g_ctx->m_local_controller->m_bPawnIsAlive())
	{
		s_vecPredictedGrenades.clear();
		return;
	}

	this->DrawGrenadeNames();

	ImDrawList* pDrawList = ImGui::GetForegroundDrawList();

	for (const auto& object : g_entity_cache->m_entity) {
		C_Inferno* pInferno = reinterpret_cast<C_Inferno*>(object.m_pEntity);
		if (!pInferno)
			continue;

		DrawInferno(pInferno);
	}

	for (const auto& object : g_entity_cache->m_grenade_entity) {
		C_SmokeGrenadeProjectile* pSmoke = reinterpret_cast<C_SmokeGrenadeProjectile*>(object.m_pEntity);
		if (!pSmoke)
			continue;

		DrawSmoke(pSmoke);
	}

	if (!GET_VAR(bool, VISUALS_PATH(m_enable_grenade_prediction)))
		return;
	
	for (int i = 0; i < s_vecPredictedGrenades.size(); i++)
	{
		GrenadePredictionObject_t& GrenadePredictionObject = s_vecPredictedGrenades[i];
		if (ticks_to_time(GrenadePredictionObject.m_GrenadePathPoint.back().m_nTick) <= g_interfaces->m_global_vars->m_curtime)
		{
			s_vecPredictedGrenades.erase(s_vecPredictedGrenades.begin() + i);
			--i;
			continue;
		}

		GrenadePredictionObject.Draw(GET_VAR(hellcolor, VISUALS_PATH(m_grenade_prediction_color)), true, 0);
	}
	
	if (!g_ctx->m_active_weapon)
		return;
	
	int weapon_id = g_ctx->m_active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	if (weapon_id != WEAPON_HIGH_EXPLOSIVE_GRENADE && 
		weapon_id != WEAPON_FLASHBANG && 
		weapon_id != WEAPON_DECOY_GRENADE && 
		weapon_id != WEAPON_MOLOTOV && 
		weapon_id != WEAPON_INCENDIARY_GRENADE && 
		weapon_id != WEAPON_SMOKE_GRENADE)
		return;
	if (weapon_id == WEAPON_C4_EXPLOSIVE)
		return;
	for (int i = 0; i < s_vecPredictedGrenades.size(); i++)
	{
		GrenadePredictionObject_t& GrenadePredictionObject = s_vecPredictedGrenades[i];
		if (ticks_to_time(GrenadePredictionObject.m_GrenadePathPoint.back().m_nTick) <= g_interfaces->m_global_vars->m_curtime)
		{
			s_vecPredictedGrenades.erase(s_vecPredictedGrenades.begin() + i);
			--i;
			continue;
		}

		GrenadePredictionObject.Draw(GET_VAR(hellcolor, VISUALS_PATH(m_grenade_prediction_color)), true, 0);
	}

	if (!g_interfaces->m_engine->is_connected() || !g_interfaces->m_engine->in_game() || !g_ctx->m_local_controller)
		return;

	c_cs_player_pawn* pPawn = g_ctx->m_local_pawn;
	if (!pPawn || !pPawn->is_alive())
		return;

	if (pPawn->m_nActualMoveType() == MOVETYPE_NOCLIP)
		return;

	auto* pActiveWeapon = pPawn->m_pWeaponServices()->m_hActiveWeapon().get<c_weapon_base>();
	if (!pActiveWeapon)
		return;

	this->SetGrenadeAct(g_ctx->m_cmd->m_buttons.m_value);

	if (!g_ctx->m_active_weapon_data || g_ctx->m_active_weapon_data->m_WeaponType() != WEAPONTYPE_GRENADE)
		return;

	c_base_cs_grenade* pGrenade = reinterpret_cast<c_base_cs_grenade*>(g_ctx->m_active_weapon);
	if (!pGrenade)
		return;

	vec3_t vStartPos = g_ctx->m_local_pawn->get_eye_pos(), vVelocity;
	
	qangle_t view_angles = g_interfaces->m_csgo_input->get_view_angle();
	
	if (GET_VAR(bool, MISC_PATH(m_enabled_straight_throw))) {
		vec3_t vThrowDirection{};
		g_math->angle_vectors(view_angles, &vThrowDirection);

		static const float arrPower[] = { 1.0f, 1.0f, 0.5f, 0.0f };
		float flThrowStrength = arrPower[m_iGrenadeAct];
		vec3_t vNewDirection = vThrowDirection * (std::clamp(g_ctx->m_active_weapon_data->m_flThrowVelocity() * 0.9f, 15.f, 750.f) * (flThrowStrength * 0.7f + 0.3f)) + g_ctx->m_local_pawn->m_vecAbsVelocity() * 1.25f;
		vNewDirection.normalize();

		vec3_t aNewViewAngle;
		g_math->vector_angles(vNewDirection, aNewViewAngle);

		view_angles.y += (view_angles.y - aNewViewAngle.y);
	}
	
	this->Setup(vStartPos, vVelocity, view_angles);

	std::vector<GrenadePathPoint_t> vecGrenadePathPoint = this->GetGrenadePathFromEntity(vStartPos, vVelocity, pGrenade, g_interfaces->m_global_vars->m_curtime);

	vec3_t vPrev = vecGrenadePathPoint[0].m_vPos;
	vec2_t vNadeStart_2D, vNadeEnd_2D;
	hellcolor cGrenadePredictionColor = GET_VAR(hellcolor, VISUALS_PATH(m_grenade_prediction_color));

		float flCurrentTime = g_interfaces->m_global_vars->m_curtime;
		float flCycleTime = 2.f;
		float flPulseTime = 4.2f;
		float flCycleProgress = fmod(flCurrentTime, flCycleTime);
		float flWavePosition = -1.0f;
		float flWaveIntensity = 0.0f;
	
		bool bPulsatingEnabled = GET_VAR(bool, VISUALS_PATH(m_grenade_prediction_pulsating));
		if (bPulsatingEnabled && flCycleProgress <= flPulseTime) {
			flWavePosition = (flCycleProgress / flPulseTime) * 3.2f - 0.3f;
			flWaveIntensity = sin((flCycleProgress / flPulseTime) * IM_PI);
		}

		for (size_t i = 1; i < vecGrenadePathPoint.size(); ++i)
		{
			auto& vNade = vecGrenadePathPoint[i];
			if (g_math->world_to_screen(vPrev, vNadeStart_2D) && g_math->world_to_screen(vNade.m_vPos, vNadeEnd_2D))
			{
				float flSegmentProgress = (float)i / (float)vecGrenadePathPoint.size();
				
				hellcolor segmentColor = cGrenadePredictionColor;
				if (bPulsatingEnabled && flWavePosition >= -0.3f && flWaveIntensity > 0.0f) {
					float flWaveDistance = abs(flSegmentProgress - flWavePosition);
					float flWaveEffect = 1.0f - std::min(flWaveDistance * 2.8f, 1.0f);
				if (flWaveEffect > 0.0f) {
					float hue_shift = 0.15f;
					float r = cGrenadePredictionColor.Value.x;
					float g = cGrenadePredictionColor.Value.y;
					float b = cGrenadePredictionColor.Value.z;
					
					float max_val = std::max({r, g, b});
					float min_val = std::min({r, g, b});
					float delta = max_val - min_val;
					
					float h = 0.0f;
					if (delta > 0.0f) {
						if (max_val == r) h = fmod((g - b) / delta, 6.0f);
						else if (max_val == g) h = (b - r) / delta + 2.0f;
						else h = (r - g) / delta + 4.0f;
						h /= 6.0f;
					}
					
					h = fmod(h + hue_shift, 1.0f);
					if (h < 0.0f) h += 1.0f;
					
					float s = (max_val > 0.0f) ? delta / max_val : 0.0f;
					float v = max_val;
					
					float c = v * s;
					float x = c * (1.0f - abs(fmod(h * 6.0f, 2.0f) - 1.0f));
					float m = v - c;
					
					float r_new, g_new, b_new;
					if (h < 1.0f/6.0f) { r_new = c; g_new = x; b_new = 0; }
					else if (h < 2.0f/6.0f) { r_new = x; g_new = c; b_new = 0; }
					else if (h < 3.0f/6.0f) { r_new = 0; g_new = c; b_new = x; }
					else if (h < 4.0f/6.0f) { r_new = 0; g_new = x; b_new = c; }
					else if (h < 5.0f/6.0f) { r_new = x; g_new = 0; b_new = c; }
					else { r_new = c; g_new = 0; b_new = x; }
					
					segmentColor = hellcolor(
						r_new + m,
						g_new + m,
						b_new + m,
						cGrenadePredictionColor.Value.w
					);
					
					float flPulseFade = 1.0f;
					if (flWaveEffect < 0.98f) {
						flPulseFade = flWaveEffect / 0.98f;
						flPulseFade = flPulseFade * flPulseFade * flPulseFade * flPulseFade;
						segmentColor = hellcolor(
							cGrenadePredictionColor.Value.x + (segmentColor.Value.x - cGrenadePredictionColor.Value.x) * flPulseFade,
							cGrenadePredictionColor.Value.y + (segmentColor.Value.y - cGrenadePredictionColor.Value.y) * flPulseFade,
							cGrenadePredictionColor.Value.z + (segmentColor.Value.z - cGrenadePredictionColor.Value.z) * flPulseFade,
							cGrenadePredictionColor.Value.w
						);
					}
				}
			}

			float flAlpha = 1.0f;
			if (i == 1) {
				flAlpha = 0.0f;
			} else if (i <= 24) {
				flAlpha = (float)(i - 1) / 23.0f;
			}
			segmentColor.Value.w *= flAlpha;

			hellcolor glow_color = segmentColor;
			glow_color.Value.w *= 0.12f;
			if (GET_VAR(bool, VISUALS_PATH(m_grenade_prediction_glow))) {
				AddLineShadow(pDrawList, { vNadeStart_2D.x, vNadeStart_2D.y }, { vNadeEnd_2D.x, vNadeEnd_2D.y }, glow_color, 5.0f, 1.2f);
			}

			pDrawList->AddLine({ vNadeStart_2D.x,vNadeStart_2D.y }, { vNadeEnd_2D.x,vNadeEnd_2D.y }, segmentColor, 1.2f);
		}
		vPrev = vNade.m_vPos;
	}

	int nGrenadeTypeRun = -1;
	if (pGrenade) {
		fnv1a_t uHashedName = fnv1a::hash_32(pGrenade->get_class_name());
		nGrenadeTypeRun = this->get_grenade_type(uHashedName);
	}

	bool bIsLastCollisionBeforeEndRun = false;
	if (!vecGrenadePathPoint.empty() && nGrenadeTypeRun == 2) {
		for (size_t i = 1; i < vecGrenadePathPoint.size(); ++i) {
			if (vecGrenadePathPoint[i].m_bIsCollision) {
				if (i == vecGrenadePathPoint.size() - 1 || 
					(i + 1 < vecGrenadePathPoint.size() && vecGrenadePathPoint[i + 1].m_vPos.dist_to(vecGrenadePathPoint.back().m_vPos) < 5.0f)) {
					bIsLastCollisionBeforeEndRun = true;
					break;
				}
			}
		}
	}

	for (size_t i = 1; i < vecGrenadePathPoint.size(); ++i)
	{
		auto& vNade = vecGrenadePathPoint[i];
		if (vNade.m_bIsCollision && g_math->world_to_screen(vNade.m_vPos, vNadeEnd_2D))
		{
			bool bSkipThisCircle = false;
			if (nGrenadeTypeRun == 2 && bIsLastCollisionBeforeEndRun) {
				float flDistToEnd = vNade.m_vPos.dist_to(vecGrenadePathPoint.back().m_vPos);
				if (flDistToEnd < 5.0f) {
					bSkipThisCircle = true;
				}
			}
			
			if (!bSkipThisCircle) {
				hellcolor circle_color = hellcolor(255, 255, 255, 255);
				hellcolor circle_outline = hellcolor(0, 0, 0, 200);
				
				pDrawList->AddCircleFilled({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, circle_color, 12);
				pDrawList->AddCircle({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, circle_outline, 12, 0.5f);
			}
		}
	}

	if (!vecGrenadePathPoint.empty() && g_math->world_to_screen(vecGrenadePathPoint.back().m_vPos, vNadeEnd_2D))
	{
		pDrawList->AddCircleFilled({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, hellcolor(255, 148, 148, 255), 12);
		pDrawList->AddCircle({ vNadeEnd_2D.x,vNadeEnd_2D.y }, 2.0f, hellcolor(0, 0, 0, 200), 12, 0.5f);
	}

	if (!vecGrenadePathPoint.empty() && (g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK || g_ctx->m_cmd->m_buttons.m_value & IN_ATTACK2))
	{
		int nGrenadeType = -1;
		if (pGrenade) {
			{
				fnv1a_t uHashedName = fnv1a::hash_32(pGrenade->get_class_name());
				nGrenadeType = this->get_grenade_type(uHashedName);
			}
		}

		if (nGrenadeType == 0) {
			GrenadePredictionObject_t tempNadeData;
			tempNadeData.m_GrenadePathPoint = vecGrenadePathPoint;

			float flNadeDamage = 99.0f;
			float flNadeRadius = 350.0f;

			if (pGrenade) {
				c_base_cs_grenade* pBaseGrenade = reinterpret_cast<c_base_cs_grenade*>(pGrenade);
				if (pBaseGrenade) {
					flNadeDamage = pBaseGrenade->m_flDamage();
					flNadeRadius = pBaseGrenade->m_DmgRadius();
				}
			}

			int nDamage = 0;

			bool bIsLethal;

			for (auto& player : g_entity_cache->m_players) {
				c_cs_player_controller* pController = reinterpret_cast<c_cs_player_controller*>(player.m_controller);
				if (!pController)
					continue;

				c_cs_player_pawn* pPawn = pController->m_hPawn().get< c_cs_player_pawn>();
				if (!pPawn)
					continue;

				if (pPawn == g_ctx->m_local_pawn)
					continue;

				if (pController->m_iTeamNum() == g_ctx->m_local_controller->m_iTeamNum())
					continue;

				nDamage = this->get_he_damage(tempNadeData, pPawn, 99.f, 350.f);

				bIsLethal = nDamage >= pPawn->m_iHealth();
			}

			if (nDamage > 0) {
				const ImVec2 vDisplaySize = ImGui::GetIO().DisplaySize;
				const ImVec2 vDisplayCenter = vDisplaySize / 2.f;

				std::string damageText = "DMG: " + std::to_string(nDamage);
				const char* text = damageText.c_str();
				ImVec2 textSize = g_font_manager->m_verdana_14->CalcTextSizeA(14.f, FLT_MAX, 0.0f, text);

				ImVec2 textPos(vDisplayCenter.x - textSize.x * 0.5f, vDisplayCenter.y + 30.f);

				pDrawList->AddText(g_font_manager->m_verdana_14, 14.f, ImVec2(textPos.x + 1, textPos.y + 1), hellcolor(0, 0, 0, 200), std::string(text).c_str());

				hellcolor textColor = bIsLethal ? hellcolor(255, 148, 148, 255) : hellcolor(255, 255, 255, 255);

				pDrawList->AddText(g_font_manager->m_verdana_14, 14.f, textPos, textColor, std::string(text).c_str());

			}
		}
	}
}