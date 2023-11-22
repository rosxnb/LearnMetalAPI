#include "renderer.hpp"
#include "Foundation/NSRange.hpp"
#include "Foundation/NSTypes.hpp"
#include "Metal/MTLCommandEncoder.hpp"
#include "utility.hpp"

#include <simd/simd.h>

Renderer::Renderer( MTL::Device* pDevice )
    : p_device( pDevice->retain() )
    , p_cmdQ( p_device->newCommandQueue() )
{ 
    m_frame = 0;
    m_angle = 0.f;
    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);

    build_shaders();
    build_buffers();
    build_frame_data();
}

Renderer::~Renderer()
{
    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        p_frameData[i]->release();

    p_argBuffer->release();
    p_shaderLibrary->release();

    p_vertexPositions->release();
    p_vertexColors->release();
    p_pipelineState->release();

    p_cmdQ->release();
    p_device->release();
}

void Renderer::build_shaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    m_shaderSrc = Utility::read_source("shader/program.metal");
    const char* shaderSrc = m_shaderSrc.data();

    NS::Error* pError {nullptr};
    p_shaderLibrary = p_device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
    if ( !p_shaderLibrary )
    {
        __builtin_printf("Library creation from shader source failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    MTL::Function* fnVertex             = p_shaderLibrary->newFunction( NS::String::string("main_vertex", UTF8StringEncoding) );
    MTL::Function* fnFragment           = p_shaderLibrary->newFunction( NS::String::string("main_fragment", UTF8StringEncoding) );
    MTL::RenderPipelineDescriptor* pRpd = MTL::RenderPipelineDescriptor::alloc()->init();

    pRpd->setVertexFunction( fnVertex );
    pRpd->setFragmentFunction( fnFragment );
    pRpd->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    pRpd->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    p_pipelineState = p_device->newRenderPipelineState( pRpd, &pError );
    if ( !p_pipelineState )
    {
        __builtin_printf("RenderPipelineState creation failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    fnVertex->release();
    fnFragment->release();
    pRpd->release();
}

void Renderer::build_buffers()
{
    const size_t NumVertices = 3;

    simd::float3 positions[NumVertices] =
    {
        { -0.8f,  0.8f, 0.0f },
        {  0.0f, -0.8f, 0.0f },
        { +0.8f,  0.8f, 0.0f }
    };

    simd::float3 colors[NumVertices] =
    {
        {  1.0, 0.3f, 0.2f },
        {  0.8f, 1.0, 0.0f },
        {  0.8f, 0.0f, 1.0 }
    };

    const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
    const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

    MTL::Buffer* pVertexPositionsBuffer = p_device->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pVertexColorsBuffer = p_device->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );

    p_vertexPositions = pVertexPositionsBuffer;
    p_vertexColors = pVertexColorsBuffer;

    memcpy( p_vertexPositions->contents(), positions, positionsDataSize );
    memcpy( p_vertexColors->contents(), colors, colorDataSize );

    p_vertexPositions->didModifyRange( NS::Range::Make( 0, p_vertexPositions->length() ) );
    p_vertexColors->didModifyRange( NS::Range::Make( 0, p_vertexColors->length() ) );

    // argument buffer encoding
    using NS::StringEncoding::UTF8StringEncoding;
    MTL::Function* fnVertex = p_shaderLibrary->newFunction( NS::String::string("main_vertex", UTF8StringEncoding) );
    MTL::ArgumentEncoder* argEnc = fnVertex->newArgumentEncoder(0);

    p_argBuffer = p_device->newBuffer( argEnc->encodedLength(), MTL::ResourceStorageModeManaged );
    
    argEnc->setArgumentBuffer( p_argBuffer, 0 );
    argEnc->setBuffer( p_vertexPositions, 0, 0 );
    argEnc->setBuffer( p_vertexColors, 0, 1 );

    p_argBuffer->didModifyRange(NS::Range::Make(0, p_argBuffer->length()));

    fnVertex->release();
    argEnc->release();
}

void Renderer::build_frame_data()
{
    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        p_frameData[i] = p_device->newBuffer( sizeof(FrameData), MTL::ResourceStorageModeManaged );
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame + 1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* pCurrentFrame = p_frameData[m_frame];

    MTL::CommandBuffer* pCmd = p_cmdQ->commandBuffer();

    dispatch_semaphore_wait( m_semaphore, DISPATCH_TIME_FOREVER );
    pCmd->addCompletedHandler( [this] (MTL::CommandBuffer* pCmd) {
        dispatch_semaphore_signal( this->m_semaphore );
    } );

    reinterpret_cast<FrameData*>( pCurrentFrame->contents() )->angle = (m_angle += 0.01f);
    pCurrentFrame->didModifyRange(NS::Range::Make(0, sizeof(FrameData)));

    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);

    pEnc->setRenderPipelineState(p_pipelineState);
    pEnc->setVertexBuffer( p_argBuffer, 0, 0 );
    pEnc->useResource( p_vertexPositions, MTL::ResourceUsageRead );
    pEnc->useResource( p_vertexColors, MTL::ResourceUsageRead );

    pEnc->setVertexBuffer( pCurrentFrame, 0, 1 );
    pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pool->release();
}
