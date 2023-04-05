#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

class Renderer
{
    public:
        Renderer(MTL::Device* pDevice);
        ~Renderer();

        void buildShaders();
        void buildBuffers();
        void draw(MTK::View* pView);

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCmdQ;
        MTL::RenderPipelineState* _pPipeline;
        MTL::Buffer* _pPositions;
        MTL::Buffer* _pColors;
};
