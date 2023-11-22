#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
};

struct VertexData
{
    device float3* positions [[id(0)]];
    device float3* colors [[id(1)]];
};

struct FrameData
{
    float angle;
};

vertex 
V2F main_vertex( uint const vertexId [[ vertex_id ]],
                 device const VertexData* vertexData [[ buffer(0) ]],
                 constant FrameData* frameData [[ buffer(1) ]] )
{
    float a = frameData->angle;
    float3x3 rotationMatrix = float3x3(
        sin(a), cos(a), 0.f,
        cos(a), -sin(a), 0.f,
        0.f, 0.f, 1.f
    );

    V2F output;
    output.position = float4( rotationMatrix * vertexData->positions[vertexId], 1.f );
    output.color = half3( vertexData->colors[vertexId] );
    return output;
}

fragment
half4 main_fragment( V2F data [[ stage_in ]] )
{
    return half4( data.color, 1.f );
}
