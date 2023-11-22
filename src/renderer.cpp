#include "renderer.hpp"
#include "Foundation/NSRange.hpp"
#include "Foundation/NSTypes.hpp"
#include "Metal/MTLCommandEncoder.hpp"
#include "utility.hpp"

Renderer::Renderer( MTL::Device* pDevice )
    : p_device( pDevice->retain() )
    , p_cmdQ( p_device->newCommandQueue() )
{ 
    m_frame = 0;
    m_angle = 0.f;
    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);

    build_shaders();
    build_buffers();
    build_instance_data();
}

Renderer::~Renderer()
{
    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        p_instanceBuffer[i]->release();

    p_indexBuffer->release();

    p_shaderLibrary->release();

    p_vertexPositions->release();
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
    constexpr float s = 0.5f;

    constexpr simd::float3 verts[] = {
        { -s, -s, +s },
        { +s, -s, +s },
        { +s, +s, +s },
        { -s, +s, +s }
    };

    constexpr uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };

    constexpr size_t vertexDataSize = sizeof( verts );
    constexpr size_t indexDataSize = sizeof( indices );

    p_vertexPositions = p_device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    p_indexBuffer = p_device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    memcpy( p_vertexPositions->contents(), verts, vertexDataSize );
    memcpy( p_indexBuffer->contents(), indices, indexDataSize );

    p_vertexPositions->didModifyRange( NS::Range::Make( 0, p_vertexPositions->length() ) );
    p_indexBuffer->didModifyRange( NS::Range::Make( 0, p_indexBuffer->length() ) );
}

void Renderer::build_instance_data()
{
    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
        p_instanceBuffer[i] = p_device->newBuffer( kNumInstances * sizeof(shader_types::InstanceData), MTL::ResourceStorageModeManaged );
}

void Renderer::draw( MTK::View* pView )
{
    using simd::float4x4, simd::float4;

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame + 1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* pCurrentFrame = p_instanceBuffer[m_frame];

    MTL::CommandBuffer* pCmd = p_cmdQ->commandBuffer();

    dispatch_semaphore_wait( m_semaphore, DISPATCH_TIME_FOREVER );
    pCmd->addCompletedHandler( [this] (MTL::CommandBuffer* pCmd) {
        dispatch_semaphore_signal( this->m_semaphore );
    } );

    m_angle += 0.003f;
    constexpr float scl = 0.1f;

    auto pInstanceData = reinterpret_cast<shader_types::InstanceData *>(pCurrentFrame->contents());
    for (size_t i = 0; i < Renderer::kNumInstances; ++i)
    {
        float iDivNumInstances = i / (float)kNumInstances;
        float xoff = (iDivNumInstances * 2.0f - 1.0f) + (1.f/kNumInstances);
        float yoff = sin( ( iDivNumInstances + m_angle ) * 2.0f * M_PI);
        pInstanceData[ i ].instanceTransform = (float4x4){ (float4){ scl * sinf(m_angle), scl * cosf(m_angle), 0.f, 0.f },
                                                           (float4){ scl * cosf(m_angle), scl * -sinf(m_angle), 0.f, 0.f },
                                                           (float4){ 0.f, 0.f, scl, 0.f },
                                                           (float4){ xoff, yoff, 0.f, 1.f } };

        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf( M_PI * 2.0f * iDivNumInstances );
        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };
    }
    pCurrentFrame->didModifyRange(NS::Range::Make(0, pCurrentFrame->length()));

    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);

    pEnc->setRenderPipelineState(p_pipelineState);
    pEnc->setVertexBuffer( p_vertexPositions, 0, 0 );
    pEnc->setVertexBuffer( pCurrentFrame, 0, 1);

    pEnc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                 6, MTL::IndexType::IndexTypeUInt16,
                                 p_indexBuffer,
                                 0,
                                 kNumInstances );

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pool->release();
}
