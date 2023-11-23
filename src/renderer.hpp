#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();

        void draw( MTK::View* pView );
        void build_shaders();
        void build_buffers();
        void build_depth_stencil_states();

    private:
        MTL::Device* p_device;
        MTL::CommandQueue* p_cmdQ;

        MTL::RenderPipelineState* p_pipelineState;
        MTL::Buffer* p_vertexPositions;

        MTL::Library* p_shaderLibrary;

        int m_frame;
        float m_angle;
        dispatch_semaphore_t m_semaphore;

        static constexpr int kMaxFramesInFlight = 3;
        static constexpr int kNumInstances = 32;
        MTL::Buffer* p_indexBuffer;
        MTL::Buffer* p_instanceBuffer[kMaxFramesInFlight];

        MTL::DepthStencilState* p_depthStencilState;
        MTL::Buffer* p_cameraBuffer[kMaxFramesInFlight];

        std::string m_shaderSrc;
};

namespace shader_types
{

struct InstanceData
{
    simd::float4x4 instanceTransform;
    simd::float4 instanceColor;
};

struct CameraData
{
    simd::float4x4 perspectiveTransform;
    simd::float4x4 worldTransform;
};

}
