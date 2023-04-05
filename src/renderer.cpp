#include "renderer.hpp"

Renderer::Renderer(MTL::Device* pDevice)
    : _pDevice(pDevice)
    , _pCmdQ(_pDevice->newCommandQueue())
{
    buildShaders();
    buildBuffers();
}

Renderer::~Renderer()
{
    _pPositions->release();
    _pColors->release();
    _pPipeline->release();
    _pCmdQ->release();
    _pDevice->release();
}

void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            half3 color;
        };

        v2f vertex vertexMain( uint vertexId [[vertex_id]],
                               device const float3* positions [[buffer(0)]],
                               device const float3* colors [[buffer(1)]] )
        {
            v2f o;
            o.position = float4( positions[vertexId], 1.0);
            o.color = half3( colors[vertexId]);
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]] )
        {
            return half4( in.color, 1.0 );
        }
    )";

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError);

    if (!pLibrary)
    {
        __builtin_printf("Metal Library assignment failes: \n%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
    MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor *pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction( pVertexFn );
    pDesc->setFragmentFunction( pFragFn );
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

    _pPipeline = _pDevice->newRenderPipelineState( pDesc, &pError );
    if (!_pPipeline)
    {
        __builtin_printf("RenderPipeline assignment failed: \n%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    pLibrary->release();
}

void Renderer::buildBuffers()
{
    const size_t N = 3;

    simd::float3 positions[N] = 
    {
        {-0.8f, -0.8f, 0.0f},
        { 0.0f,  0.8f, 0.0f},
        { 0.8f, -0.8f, 0.0f}
    };

    simd::float3 colors[N] = 
    {
        {1.0f, 0.3f, 0.2f},
        {0.8f, 1.0f, 0.0f},
        {0.8f, 0.0f, 1.0f}
    };

    const size_t positionsDataSize = N * sizeof(simd::float3);
    const size_t colorsDataSize = N * sizeof(simd::float3);

    MTL::Buffer* pPositions = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pColors = _pDevice->newBuffer( colorsDataSize, MTL::ResourceStorageModeManaged );

    _pPositions = pPositions;
    _pColors = pColors;

    memcpy(_pPositions->contents(), positions, positionsDataSize);
    memcpy(_pColors->contents(), colors, colorsDataSize);

    _pPositions->didModifyRange( NS::Range::Make(0, _pPositions->length()) );
    _pColors->didModifyRange( NS::Range::Make(0, _pColors->length()) );
}

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmdBuff = _pCmdQ->commandBuffer();
    MTL::RenderPassDescriptor* pDesciptor = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEncoder = pCmdBuff->renderCommandEncoder(pDesciptor);

    pEncoder->setRenderPipelineState( _pPipeline );
    pEncoder->setVertexBuffer( _pPositions, 0, 0 );
    pEncoder->setVertexBuffer( _pColors, 0, 1 );
    pEncoder->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

    pEncoder->endEncoding();
    pCmdBuff->presentDrawable(pView->currentDrawable());
    pCmdBuff->commit();

    pPool->release();
}
