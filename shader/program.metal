#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
    float2 texcord;
};

struct VertexData
{
    device float3* positions [[id(0)]];
    device float3* colors [[id(1)]];
    device float2* texcords [[id(2)]];
};

vertex 
V2F main_vertex( uint const vertexId [[ vertex_id ]],
                 device const VertexData* vertexData [[ buffer(0) ]],
                 device const float4x4& proj [[ buffer(1) ]] )
{
    V2F output;
    output.position = proj * float4( vertexData->positions[vertexId], 1.f );
    output.color = half3( vertexData->colors[vertexId] );
    output.texcord = vertexData->texcords[vertexId].xy;
    return output;
}

fragment
half4 main_fragment( V2F data [[ stage_in ]],
                     texture2d< half, access::sample > tex [[texture(0)]] )
{
    constexpr sampler s( address::repeat, filter::linear );
    half3 texel = tex.sample( s, data.texcord ).rgb;
    return half4( texel, 1.f ) * half4( data.color, 1.f );
}
