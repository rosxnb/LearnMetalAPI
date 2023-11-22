#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
};

struct VertexData
{
    float3 positions;
};

struct InstanceData
{
    float4x4 instanceTransform;
    float4 instanceColor;
};

vertex 
V2F main_vertex( device const VertexData* vertexData [[ buffer(0) ]],
                 device const InstanceData* instanceData [[ buffer(1) ]],
                 uint instanceID [[ instance_id ]],
                 uint vertexId [[ vertex_id ]] )
{
    float4 pos = float4( vertexData[vertexId].positions, 1.f );

    V2F output;
    output.position = instanceData[instanceID].instanceTransform * pos;
    output.color = half3( instanceData[instanceID].instanceColor.rgb );
    return output;
}

fragment
half4 main_fragment( V2F data [[ stage_in ]] )
{
    return half4( data.color, 1.f );
}
