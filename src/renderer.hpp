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
        void build_depth_stencil_states();

        std::string* read_source( const char* filepath ) const;

    private:
        static constexpr int kMaxFrames = 3;
        static constexpr int kNumInstances = 32;

        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCmdQ;
        MTL::RenderPipelineState* _pRps;
        MTL::DepthStencilState* _pDss;
        MTL::Library* _pLib;
        MTL::Buffer* _pIndexData;
        MTL::Buffer* _pVertexData;
        MTL::Buffer* _pInstanceData[ kMaxFrames ];
        MTL::Buffer* _pCameraData[ kMaxFrames ];

        float _angle;
        int _frame;
        dispatch_semaphore_t _semaphore;
        std::string* _pShaderSrc;
};
