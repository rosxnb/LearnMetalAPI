#include "renderer.hpp"
#include <simd/simd.h>
#include <fstream>
#include <sstream>
#include <memory>

Renderer::Renderer( MTL::Device* pDevice )
    : _pDevice( pDevice->retain() )
    , _pCmdQ( _pDevice->newCommandQueue() )
    , _angle( 0.f )
    , _frame( 0 )
    , _semaphore( dispatch_semaphore_create( kMaxFrames ) )
{ 
    build_shaders();
    build_buffers();
}

Renderer::~Renderer()
{
    delete _pShaderSrc; 

    for ( int i = 0; i < kMaxFrames; ++i )
        _pInstanceData[i]->release();
    
    _pIndexData->release();
    _pVertexData->release();
    _pLib->release();
    _pRps->release();
    _pCmdQ->release();
    _pDevice->release();
}

std::string* Renderer::read_source(const char* filepath) const
{
    std::string* buffer = new std::string;

    std::ifstream fileHandler;
    fileHandler.exceptions( std::ifstream::badbit | std::ifstream::failbit );

    try 
    {
        fileHandler.open(filepath);
        std::stringstream sstream;
        sstream << fileHandler.rdbuf();
        buffer->assign( sstream.str() );
    }
    catch (std::ifstream::failure err)
    {
        __builtin_printf("Failed to load file: %s \n", filepath);
        __builtin_printf("%s \n\n", err.what());
    }

    return buffer;
}

void Renderer::build_shaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Error* pError {nullptr};
    const char* shaderSrc = read_source("shader/program.metal")->data();
    _pLib = _pDevice->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
    if ( !_pLib )
    {
        __builtin_printf("Library creation from shader source failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    MTL::Function* fnVertex     = _pLib->newFunction( NS::String::string("main_vertex", UTF8StringEncoding) );
    MTL::Function* fnFragment   = _pLib->newFunction( NS::String::string("main_fragment", UTF8StringEncoding) );
    MTL::RenderPipelineDescriptor* pRpd = MTL::RenderPipelineDescriptor::alloc()->init();

    pRpd->setVertexFunction( fnVertex );
    pRpd->setFragmentFunction( fnFragment );
    pRpd->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

    _pRps = _pDevice->newRenderPipelineState( pRpd, &pError );
    if ( !_pRps )
    {
        __builtin_printf("RenderPipelineState creation failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    fnVertex->release();
    fnFragment->release();
    pRpd->release();
}

namespace ShaderType
{
    struct InstanceData 
    {
        simd::float4x4 instanceTransform;
        simd::float4 instanceColor;
    };
}

void Renderer::build_buffers()
{
    using simd::float3;

    float s = 0.5f;
    float3 vertices[] = 
    {
        { -s, -s, +s },     // 3rd quadrant
        { +s, -s, +s },     // 4th quadrant
        { +s, +s, +s },     // 1st quadrant
        { -s, +s, +s }      // 2nd quadrant
    };
    uint16_t indices[] = 
    {
        0, 1, 2,
        2, 3, 0
    };

    constexpr size_t size_vertices = sizeof( vertices );
    constexpr size_t size_indices = sizeof( indices );
    _pVertexData   = _pDevice->newBuffer( size_vertices, MTL::ResourceStorageModeManaged );
    _pIndexData    = _pDevice->newBuffer( size_indices, MTL::ResourceStorageModeManaged );

    memcpy( _pVertexData->contents(), vertices, size_vertices );
    memcpy( _pIndexData->contents(), indices, size_indices );

    _pVertexData->didModifyRange( NS::Range::Make(0 , _pVertexData->length()) );
    _pIndexData->didModifyRange( NS::Range::Make(0, _pIndexData->length()) );

    constexpr size_t size_instance = kNumInstances * sizeof( ShaderType::InstanceData );
    for ( int i = 0; i < kMaxFrames; ++i )
        _pInstanceData[i] = _pDevice->newBuffer( size_instance, MTL::ResourceStorageModeManaged );
}

void Renderer::draw( MTK::View* pView )
{
    using simd::float4x4, simd::float4;
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    _angle += 0.001f;
    _frame = (_frame + 1) % kMaxFrames;
    MTL::Buffer* pInstanceDataBuffer = _pInstanceData[ _frame ];

    MTL::CommandBuffer* pCmdBuff = _pCmdQ->commandBuffer();
    dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
    pCmdBuff->addCompletedHandler( [this]( MTL::CommandBuffer* )
            {
                dispatch_semaphore_signal(this->_semaphore);
            } );


    constexpr float scl = 0.05f;
    ShaderType::InstanceData* pInstanceData = reinterpret_cast< ShaderType::InstanceData* >(pInstanceDataBuffer->contents());
    for ( size_t i = 0; i < kNumInstances; ++i )
    {
        float iDivNumInstances  = (float) i / kNumInstances;
        float r                 = iDivNumInstances;
        float g                 = 1.f - r;
        float b                 = iDivNumInstances * M_PI * 2.f;
        float xoff              = (iDivNumInstances * 2.f - 1.f) + (1.f / kNumInstances);
        float yoff              = sin( (iDivNumInstances + _angle) * M_PI * 2.f );

        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.f };
        pInstanceData[ i ].instanceTransform = (float4x4)
        {
            (float4) { scl * sinf(_angle), scl *  cosf(_angle),  0.f,  0.f },
            (float4) { scl * cosf(_angle), scl * -sinf(_angle),  0.f,  0.f },
            (float4) {  0.f,  0.f, scl, 0.f },
            (float4) { xoff, yoff, 0.f, 1.f }
        };
    }
    pInstanceDataBuffer->didModifyRange( NS::Range::Make(0, pInstanceDataBuffer->length()) );

    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmdBuff->renderCommandEncoder( pRpd );

    pEnc->setRenderPipelineState( _pRps );
    pEnc->setVertexBuffer( _pVertexData, 0, 0);
    pEnc->setVertexBuffer( pInstanceDataBuffer, 0, 1);

    // void drawIndexedPrimitives( PrimitiveType primitiveType, NS::UInteger indexCount, IndexType indexType,
    //                             const class Buffer* pIndexBuffer, NS::UInteger indexBufferOffset, 
    //                             NS::UInteger instanceCount );
    pEnc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, 
                                 6,
                                 MTL::IndexType::IndexTypeUInt16,
                                 _pIndexData,
                                 0,
                                 kNumInstances );

    pEnc->endEncoding();
    pCmdBuff->presentDrawable( pView->currentDrawable() );
    pCmdBuff->commit();

    pPool->release();
}
