#include "math.h"

bool c_math::screen_transform( const vec3_t& source, v_matrix& matrix, vec2_t& output )
{
	output.x = matrix.m[0][0] * source.x + matrix.m[0][1] * source.y + matrix.m[0][2] * source.z + matrix.m[0][3];
	output.y = matrix.m[1][0] * source.x + matrix.m[1][1] * source.y + matrix.m[1][2] * source.z + matrix.m[1][3];
	float w = matrix.m[3][0] * source.x + matrix.m[3][1] * source.y + matrix.m[3][2] * source.z + matrix.m[3][3];

	bool behind = false;
	if ( w < 0.001f )
	{
		behind = true;

		float invw = -1.0f / w;
		output.x *= invw;
		output.y *= invw;
	}
	else
	{
		behind = false;

		float invw = 1.0f / w;
		output.x *= invw;
		output.y *= invw;
	}
	return behind;
}

bool c_math::world_to_screen(const vec3_t& in, vec2_t& out)
{
	const auto& matrix = m_viewmatrix;

	const float flWidth = matrix[3][0] * in.x + matrix[3][1] * in.y + matrix[3][2] * in.z + matrix[3][3];

	if (flWidth < 0.001f)
		return false;

	const float flInverse = 1.0f / flWidth;
	out.x = (matrix[0][0] * in.x + matrix[0][1] * in.y + matrix[0][2] * in.z + matrix[0][3]) * flInverse;
	out.y = (matrix[1][0] * in.x + matrix[1][1] * in.y + matrix[1][2] * in.z + matrix[1][3]) * flInverse;

	const ImVec2 vecDisplaySize = ImGui::GetIO().DisplaySize;
	out.x = (vecDisplaySize.x * 0.5f) + (out.x * vecDisplaySize.x) * 0.5f;
	out.y = (vecDisplaySize.y * 0.5f) - (out.y * vecDisplaySize.y) * 0.5f;

	return true;
}

vec3_t c_math::calculate_angles(vec3_t view_pos, vec3_t pos) {
	vec3_t angle = { 0, 0, 0 };
	vec3_t delta = pos - view_pos;

	//angle.x = -asin(delta.z / delta.length()) * (180.0f / A_PI);
	//angle.y = atan2(delta.y, delta.x) * (180.0f / A_PI);
	vector_angles(delta, angle);
	angle.normalize_angle();

	return angle;
}

vec3_t c_math::calculate_angle( const vec3_t& origin, const vec3_t& destination ) {
	vec3_t delta = destination - origin;
	float hypotenuse = std::sqrt( delta.x * delta.x + delta.y * delta.y );

	vec3_t angles;
	angles.x = -std::atan2( delta.z, hypotenuse ) * A_RAD2DEG; // pitch
	angles.y = std::atan2( delta.y, delta.x ) * A_RAD2DEG;     // yaw
	angles.z = 0.0f;

	return angles;
}

vec3_t c_math::calculate_camera_position( vec3_t anchor, float distance, qangle_t view_angles ) {
	float yaw = view_angles.y * A_DEG2RAD;
	float pitch = view_angles.x * A_DEG2RAD;

	float x = anchor.x + distance * cosf( yaw ) * cosf( pitch );
	float y = anchor.y + distance * sinf( yaw ) * cosf( pitch );
	float z = anchor.z + distance * sinf( pitch );

	return { x, y, z };
}

float deg2rad(float deg)
{
	float result = deg * (A_PI / 180.0f);
	return result;
}

void sin_cos(float rad, float* sine, float* cosine)
{
	*sine = std::sinf(rad);
	*cosine = std::cosf(rad);
}

void c_math::angle_vectors(const vec3_t& angles, vec3_t& forward)
{
	float sp, sy, cp, cy;

	sin_cos(deg2rad(angles[1]), &sy, &cy);
	sin_cos(deg2rad(angles[0]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

void c_math::angle_vectors_new(const vec3_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up) {
	float sp, sy, sr, cp, cy, cr;

	sy = sin(DEG2RAD(angles.y));
	cy = cos(DEG2RAD(angles.y));
	sp = sin(DEG2RAD(angles.x));
	cp = cos(DEG2RAD(angles.x));
	sr = sin(DEG2RAD(angles.z));
	cr = cos(DEG2RAD(angles.z));

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = -1 * sr * sp * cy + -1 * cr * -sy;
		right->y = -1 * sr * sp * sy + -1 * cr * cy;
		right->z = -1 * sr * cp;
	}

	if (up)
	{
		up->x = cr * sp * cy + -sr * -sy;
		up->y = cr * sp * sy + -sr * cy;
		up->z = cr * cp;
	}
}

void c_math::angle_vectors( const vec3_t& angles, vec3_t* forward, vec3_t* right , vec3_t* up  ) {
	float pitch = angles.x * A_DEG2RAD;
	float yaw = angles.y * A_DEG2RAD;
	float roll = angles.z * A_DEG2RAD;

	float sp = std::sin( pitch ), cp = std::cos( pitch );
	float sy = std::sin( yaw ), cy = std::cos( yaw );
	float sr = std::sin( roll ), cr = std::cos( roll );

	if ( forward )
		*forward = vec3_t( cp * cy, cp * sy, -sp );

	if ( right )
		*right = vec3_t( -1 * sr * sp * cy + -1 * cr * -sy,
			-1 * sr * sp * sy + -1 * cr * cy,
			-1 * sr * cp );

	if ( up )
		*up = vec3_t( cr * sp * cy + -sr * -sy,
			cr * sp * sy + -sr * cy,
			cr * cp );
}

float c_math::normalize_float( float value )
{
	while ( value > 180.f )
		value -= 360.f;

	while ( value < -180.f )
		value += 360.f;

	return value;
}

void c_math::vector_angles( const vec3_t& forward, qangle_t& angles ) {
	if ( forward.x == 0.0f && forward.y == 0.0f ) {
		angles.x = (forward.z > 0.0f) ? -90.0f : 90.0f;
		angles.y = 0.0f;
	}
	else {
		angles.x = -std::atan2( forward.z, std::sqrt( forward.x * forward.x + forward.y * forward.y ) ) * A_RAD2DEG;
		angles.y = std::atan2( forward.y, forward.x ) * A_RAD2DEG;
	}

	angles.z = 0.0f;
}
__forceinline void sqrt_fast(float* __restrict pOut, float* __restrict pIn) {
	_mm_store_ss(pOut, _mm_sqrt_ss(_mm_load_ss(pIn)));
	/*
	movss   xmm0, DWORD PTR [rdi]   ;pIn into xmm0 register
	sqrtss  xmm0, xmm0              ;square root of xmm0 register
	movss   DWORD PTR [rsi], xmm0   ;pOut
	*/
}

qangle_t c_math::vector_to_angle(const vec3_t& vForward) {
	if (vForward.x == 0.f && vForward.y == 0.f)
		return qangle_t(0.f, vForward.z > 0.f ? 270.f : 90.f, 0.f);

	float flYaw = std::atan2(vForward.y, vForward.x) * 180.f / A_PI;
	if (flYaw < 0.f)
		flYaw += 360.f;

	float flHypoLen = vForward.x * vForward.x + vForward.y * vForward.y;
	float flHypo{};
	sqrt_fast(&flHypo, &flHypoLen);
	float flPitch = std::atan2(-vForward.z, flHypo) * 180.f / A_PI;
	if (flPitch < 0.f)
		flPitch += 360.f;
	return qangle_t(flPitch, flYaw, 0.f);
}

float c_math::angle_diff( float a, float b ) {
	float diff = fmodf( a - b + 180.0f, 360.0f );
	if ( diff < 0 )
		diff += 360.0f;
	return diff - 180.0f;
}


 float c_math::rand_float( float min, float max ) {
	return min + static_cast< float >( rand( ) ) / ( static_cast< float >( RAND_MAX / ( max - min ) ) );
}

vec3_t c_math::extrapolate_tick(const vec3_t& p0, const vec3_t& v0, const int ticks) {
	return p0 + (v0 * (0.015625f * ticks));
 }
