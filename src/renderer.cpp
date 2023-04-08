#include "renderer.hpp"

Renderer::Renderer(MTL::Device* pDevice)
    : _pDevice(pDevice)
    , _angle( 0.f )
    , _frame( 0 )
{
    _pCmdQ     = _pDevice->newCommandQueue();
    _semaphore = dispatch_semaphore_create( Renderer::kMaxFramesInFlight );
    
    buildShaders();
    buildBuffers();
    buildFrameData();
}

Renderer::~Renderer()
{
    _pLibrary->release();
    _pArgBuf->release();
    _pPositions->release();
    _pColors->release();

    for (int i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        _pFrameData[i]->release();

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

        struct VertexData
        {
            device float3* positions [[id(0)]];
            device float3* colors [[id(1)]];
        };

        struct FrameData 
        {
            float angle;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               constant FrameData* frameData [[buffer(1)]],
                               uint vertexId [[vertex_id]] )
        {
            float angle = frameData->angle;
            float3x3 rotMat = float3x3( sin(angle),  cos(angle), 0.f,
                                        cos(angle), -sin(angle), 0.f,
                                        0.f,     0.f,    1.f );

            v2f o;
            o.position = float4( rotMat * vertexData->positions[vertexId], 1.0);
            o.color = half3( vertexData->colors[vertexId]);
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]] )
        {
            return half4( in.color, 1.0 );
        }
    )";

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError);
    _pLibrary = pLibrary;

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

    _pPositions = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
    _pColors    = _pDevice->newBuffer( colorsDataSize, MTL::ResourceStorageModeManaged );

    memcpy(_pPositions->contents(), positions, positionsDataSize);
    memcpy(_pColors->contents(), colors, colorsDataSize);

    _pPositions->didModifyRange( NS::Range::Make(0, _pPositions->length()) );
    _pColors->didModifyRange( NS::Range::Make(0, _pColors->length()) );

    assert( _pLibrary );
    MTL::Function* pVertexFn = _pLibrary->newFunction( NS::String::string("vertexMain", NS::StringEncoding::UTF8StringEncoding) );
    MTL::ArgumentEncoder* pArgEncoder = pVertexFn->newArgumentEncoder( 0 );

    _pArgBuf= _pDevice->newBuffer( pArgEncoder->encodedLength(), MTL::ResourceStorageModeManaged);
    pArgEncoder->setArgumentBuffer( _pArgBuf, 0 );
    pArgEncoder->setBuffer( _pPositions, 0, 0);
    pArgEncoder->setBuffer( _pColors, 0, 1);

    _pArgBuf->didModifyRange( NS::Range::Make(0, _pArgBuf->length()) );

    pVertexFn->release();
    pArgEncoder->release();
}

struct FrameData
{
    float angle;
};

void Renderer::buildFrameData()
{
    for (int i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        _pFrameData[i] = _pDevice->newBuffer( sizeof(FrameData), MTL::ResourceStorageModeManaged );
}

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    _frame = (_frame + 1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* pFrameDataBuffer = _pFrameData[_frame];

    MTL::CommandBuffer* pCmdBuff = _pCmdQ->commandBuffer();

    dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
    pCmdBuff->addCompletedHandler( [this](MTL::CommandBuffer* pBuff) {
        dispatch_semaphore_signal(this->_semaphore);
    });

    reinterpret_cast< FrameData* >(pFrameDataBuffer->contents())->angle = (_angle += 0.01f);
    pFrameDataBuffer->didModifyRange( NS::Range::Make(0, sizeof(FrameData)) );

    MTL::RenderPassDescriptor* pDesciptor = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEncoder = pCmdBuff->renderCommandEncoder(pDesciptor);
    pEncoder->setRenderPipelineState( _pPipeline );

    pEncoder->setVertexBuffer( _pArgBuf, 0, 0 );
    pEncoder->useResource( _pPositions, MTL::ResourceUsageRead );
    pEncoder->useResource( _pColors, MTL::ResourceUsageRead );

    pEncoder->setVertexBuffer( pFrameDataBuffer, 0, 1 );
    pEncoder->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

    pEncoder->endEncoding();
    pCmdBuff->presentDrawable(pView->currentDrawable());
    pCmdBuff->commit();

    pPool->release();
}
