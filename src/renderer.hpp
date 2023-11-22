#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();

        void draw( MTK::View* pView );
        void build_shaders();
        void build_buffers();
        void build_frame_data();

    private:
        MTL::Device* p_device;
        MTL::CommandQueue* p_cmdQ;

        MTL::RenderPipelineState* p_pipelineState;
        MTL::Buffer* p_vertexPositions;
        MTL::Buffer* p_vertexColors;

        MTL::Buffer* p_argBuffer;
        MTL::Library* p_shaderLibrary;

        int m_frame;
        float m_angle;
        MTL::Buffer* p_frameData[3];
        dispatch_semaphore_t m_semaphore;
        static constexpr int kMaxFramesInFlight = 3;

        std::string m_shaderSrc;
};

struct FrameData
{
    float angle;
};
