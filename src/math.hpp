#include <simd/simd.h>

namespace mth
{
    simd::float3 add( const simd::float3& a, const simd::float3& b );
    simd_float4x4 make_identity();
    simd::float4x4 make_perspective( float fovRadians, float aspect, float znear, float zfar );
    simd::float4x4 make_X_rotate( float rad );
    simd::float4x4 make_Y_rotate( float rad );
    simd::float4x4 make_Z_rotate( float rad );
    simd::float4x4 make_translate( const simd::float3& v );
    simd::float4x4 make_scale( const simd::float3& v );
}
