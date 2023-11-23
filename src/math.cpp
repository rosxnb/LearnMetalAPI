#include "math.hpp"

simd::float4x4 math::make_perspective( float fovRad, float aspect, float znear, float zfar )
{
    using simd::float4;
    float ys = 1.f / tanf(fovRad * 0.5f);
    float xs = ys / aspect;
    float zs = zfar / ( znear - zfar );
    return simd_matrix_from_rows((float4){ xs, 0.0f, 0.0f, 0.0f },
                                 (float4){ 0.0f, ys, 0.0f, 0.0f },
                                 (float4){ 0.0f, 0.0f, zs, znear * zs },
                                 (float4){ 0, 0, -1, 0 });
}

simd::float4x4 math::make_X_rotate( float rad )
{
    using simd::float4;
    const float a = rad;
    return simd_matrix_from_rows((float4){ 1.0f, 0.0f, 0.0f, 0.0f },
                                 (float4){ 0.0f, cosf( a ), sinf( a ), 0.0f },
                                 (float4){ 0.0f, -sinf( a ), cosf( a ), 0.0f },
                                 (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
}

simd::float4x4 math::make_Y_rotate( float rad )
{
    using simd::float4;
    const float a = rad;
    return simd_matrix_from_rows((float4){ cosf( a ), 0.0f, sinf( a ), 0.0f },
                                 (float4){ 0.0f, 1.0f, 0.0f, 0.0f },
                                 (float4){ -sinf( a ), 0.0f, cosf( a ), 0.0f },
                                 (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
}

simd::float4x4 math::make_Z_rotate( float rad )
{
    using simd::float4;
    const float a = rad;
    return simd_matrix_from_rows((float4){ cosf( a ), sinf( a ), 0.0f, 0.0f },
                                 (float4){ -sinf( a ), cosf( a ), 0.0f, 0.0f },
                                 (float4){ 0.0f, 0.0f, 1.0f, 0.0f },
                                 (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
}

simd::float4x4 math::make_translate( const simd::float3& vec )
{
    using simd::float4;
    const float4 col0 = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float4 col1 = { 0.0f, 1.0f, 0.0f, 0.0f };
    const float4 col2 = { 0.0f, 0.0f, 1.0f, 0.0f };
    const float4 col3 = { vec.x, vec.y, vec.z, 1.0f };
    return simd_matrix( col0, col1, col2, col3 );
}

simd::float4x4 math::make_scale( const simd::float3& vec )
{
    using simd::float4;
    return simd_matrix((float4){ vec.x, 0, 0, 0 },
                       (float4){ 0, vec.y, 0, 0 },
                       (float4){ 0, 0, vec.z, 0 },
                       (float4){ 0, 0, 0, 1.0 });
}

