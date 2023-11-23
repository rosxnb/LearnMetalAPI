#ifndef MATH_HPP
#define MATH_HPP

#include <simd/simd.h>

namespace math
{

constexpr simd::float3 add( const simd::float3& a, const simd::float3& b );
constexpr simd_float4x4 make_identity();
simd::float4x4 make_perspective( float fovRad, float aspect, float znear, float zfar );
simd::float4x4 make_X_rotate( float rad );
simd::float4x4 make_Y_rotate( float rad );
simd::float4x4 make_Z_rotate( float rad );
simd::float4x4 make_translate( const simd::float3& vec );
simd::float4x4 make_scale( const simd::float3& vec );
simd::float3x3 discard_translation( const simd::float4x4& mat );

}


constexpr simd::float3 math::add( const simd::float3& a, const simd::float3& b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

constexpr simd_float4x4 math::make_identity()
{
    return (simd_float4x4) {
        (simd::float4) { 1.f, 0.f, 0.f, 0.f },
        (simd::float4) { 0.f, 1.f, 0.f, 0.f },
        (simd::float4) { 0.f, 0.f, 1.f, 0.f },
        (simd::float4) { 0.f, 0.f, 0.f, 1.f }
    };
}

#endif // !MATH_HPP
