#include "renderer.hpp"
#include "utility.hpp"
#include "math.hpp"

Renderer::Renderer( MTL::Device* pDevice )
    : p_device( pDevice->retain() )
    , p_cmdQ( p_device->newCommandQueue() )
{ 
    m_frame = 0;
    m_angle = 0.f;
    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);

    build_shaders();
    build_buffers();
    build_depth_stencil_states();
}

Renderer::~Renderer()
{
    p_depthStencilState->release();

    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
    {
        p_instanceBuffer[i]->release();
        p_cameraBuffer[i]->release();
    }

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
        { -s, +s, +s },

        { -s, -s, -s },
        { -s, +s, -s },
        { +s, +s, -s },
        { +s, -s, -s }
    };

    constexpr uint16_t indices[] = {
        0, 1, 2, /* front */
        2, 3, 0,

        1, 7, 6, /* right */
        6, 2, 1,

        7, 4, 5, /* back */
        5, 6, 7,

        4, 0, 3, /* left */
        3, 5, 4,

        3, 2, 6, /* top */
        6, 5, 3,

        4, 7, 1, /* bottom */
        1, 0, 4
    };

    constexpr size_t vertexDataSize = sizeof( verts );
    constexpr size_t indexDataSize = sizeof( indices );

    p_vertexPositions = p_device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    p_indexBuffer = p_device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    memcpy( p_vertexPositions->contents(), verts, vertexDataSize );
    memcpy( p_indexBuffer->contents(), indices, indexDataSize );

    p_vertexPositions->didModifyRange( NS::Range::Make( 0, p_vertexPositions->length() ) );
    p_indexBuffer->didModifyRange( NS::Range::Make( 0, p_indexBuffer->length() ) );

    for (size_t i = 0; i < Renderer::kMaxFramesInFlight; ++i)
    {
        p_instanceBuffer[i] = p_device->newBuffer( kNumInstances * sizeof(shader_types::InstanceData), MTL::ResourceStorageModeManaged );
        p_cameraBuffer[i] = p_device->newBuffer( sizeof(shader_types::CameraData), MTL::ResourceStorageModeManaged );
    }
}

void Renderer::build_depth_stencil_states()
{
    MTL::DepthStencilDescriptor* pDepthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDepthDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDepthDesc->setDepthWriteEnabled(true);

    p_depthStencilState = p_device->newDepthStencilState( pDepthDesc );
    pDepthDesc->release();
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

    simd::float3 objectPosition = { 0.f, 0.f, -5.f };
    simd::float4x4 rt           = math::make_translate(objectPosition);
    simd::float4x4 rr           = math::make_Y_rotate(-m_angle);
    simd::float4x4 rtInv        = math::make_translate( {-objectPosition.x, -objectPosition.y, -objectPosition.z} );
    simd::float4x4 fullRotation = rt * rr * rtInv;

    for (size_t i = 0; i < Renderer::kNumInstances; ++i)
    {
        float iDivNumInstances = i / (float)kNumInstances;
        float xoff = (iDivNumInstances * 2.0f - 1.0f) + (1.f/kNumInstances);
        float yoff = sin( ( iDivNumInstances + m_angle ) * 2.0f * M_PI);

        simd::float4x4 scale = math::make_scale( (simd::float3){ scl, scl, scl } );
        simd::float4x4 zrot = math::make_Z_rotate( m_angle );
        simd::float4x4 yrot = math::make_Y_rotate( m_angle );
        simd::float4x4 translate = math::make_translate( math::add( objectPosition, { xoff, yoff, 0.f } ) );

        pInstanceData[ i ].instanceTransform = fullRotation * translate * yrot * zrot * scale;

        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf( M_PI * 2.0f * iDivNumInstances );
        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };
    }
    pCurrentFrame->didModifyRange(NS::Range::Make(0, pCurrentFrame->length()));

    MTL::Buffer* pCurrentCamera = p_cameraBuffer[ m_frame ];
    auto pCameraData = reinterpret_cast<shader_types::CameraData*>( pCurrentCamera->contents() );
    pCameraData->perspectiveTransform = math::make_perspective( 45.f * M_PI / 180.f, 1.f, 0.03f, 500.f );
    pCameraData->worldTransform = math::make_identity();
    pCurrentCamera->didModifyRange(NS::Range::Make(0, pCurrentCamera->length()));


    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);

    pEnc->setRenderPipelineState(p_pipelineState);
    pEnc->setDepthStencilState(p_depthStencilState);

    pEnc->setVertexBuffer( p_vertexPositions, 0, 0 );
    pEnc->setVertexBuffer( pCurrentFrame, 0, 1);
    pEnc->setVertexBuffer( pCurrentCamera, 0, 2 );

    pEnc->setCullMode( MTL::CullModeBack );
    pEnc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

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
