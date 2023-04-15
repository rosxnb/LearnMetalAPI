#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
};

struct VertexData
{
    float3 position;
};

struct InstanceData
{
    float4x4 instanceTransform;
    float4   instanceColor;
};

struct CameraData
{
    float4x4 perspectiveTransform;
    float4x4 worldTransform;
};

vertex 
V2F main_vertex( device const VertexData* vertexData [[ buffer(0) ]],
                 device const InstanceData* instanceData [[ buffer(1) ]],
                 device const CameraData& cameraData [[ buffer(2) ]],
                 uint const instanceId [[ instance_id ]],
                 uint const vertexId [[ vertex_id ]] )
{
    float4 pos;
    pos = float4( vertexData[ vertexId ].position, 1.f );
    pos = instanceData[ instanceId ].instanceTransform * pos;
    pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;

    V2F out;
    out.position = pos;
    out.color = half3( instanceData[ instanceId ].instanceColor.rbg );
    return out;
}

fragment
half4 main_fragment( V2F data [[ stage_in ]] )
{
    return half4( data.color, 1.f );
}
