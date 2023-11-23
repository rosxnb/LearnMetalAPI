#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
    float3 normal;
};

struct VertexData
{
    float3 position;
    float3 normal;
};

struct InstanceData
{
    float4x4 instanceTransform;
    float4 instanceColor;
    float3x3 instanceNormalTransform;
};

struct CameraData
{
    float4x4 perspectiveTransform;
    float4x4 worldTransform;
    float3x3 worldNormalTransform;
};

vertex 
V2F main_vertex( device const VertexData* vertexData [[ buffer(0) ]],
                 device const InstanceData* instanceData [[ buffer(1) ]],
                 device const CameraData& cameraData [[ buffer(2) ]],
                 uint instanceId [[ instance_id ]],
                 uint vertexId [[ vertex_id ]] )
{
    V2F o;

    const device VertexData& vd = vertexData[ vertexId ];
    float4 pos = float4( vd.position, 1.0 );
    pos = instanceData[ instanceId ].instanceTransform * pos;
    pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
    o.position = pos;

    float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
    normal = cameraData.worldNormalTransform * normal;
    o.normal = normal;

    o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
    return o;

}

fragment
half4 main_fragment( V2F in [[ stage_in ]] )
{
    // return half4( data.color, 1.f );

    float3 l = normalize(float3( 1.0, 1.0, 0.8 ));
    float3 n = normalize( in.normal );

    float ndotl = saturate( dot( n, l ) );
    return half4( in.color * 0.1 + in.color * ndotl, 1.0 );
}
