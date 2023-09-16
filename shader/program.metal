#include <metal_stdlib>
using namespace metal;

struct V2F
{
    float4 position [[ position ]];
    half3 color;
};

vertex 
V2F main_vertex( uint const vertexId [[ vertex_id ]] )
{
}

fragment
half4 main_fragment( V2F data [[ stage_in ]] )
{
}
