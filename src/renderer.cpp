#include "renderer.hpp"
#include "Foundation/NSTypes.hpp"
#include "Metal/MTLCommandEncoder.hpp"
#include "Metal/MTLResource.hpp"
#include "utility.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_img.hpp"

#include <simd/simd.h>

Renderer::Renderer( MTL::Device* pDevice )
    : p_device( pDevice->retain() )
    , p_cmdQ( p_device->newCommandQueue() )
{ 
    build_shaders();
    build_buffers();
    build_textures();
}

Renderer::~Renderer()
{
    p_texture->release();
    p_texCoords->release();

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
    const size_t NumVertices = 6;

    simd::float3 positions[NumVertices] =
    {
        { -0.8f,  0.8f, 0.0f }, // left top
        { +0.8f,  0.8f, 0.0f }, // right top
        { +0.8f, -0.8f, 0.0f }, // right down
        { +0.8f, -0.8f, 0.0f }, // right down
        { -0.8f, -0.8f, 0.0f }, // left down
        { -0.8f,  0.8f, 0.0f }, // left top
    };

    simd::float3 colors[NumVertices] =
    {
        {  1.0, 0.3f, 0.2f },
        {  0.8f, 1.0, 0.0f },
        {  0.8f, 0.0f, 1.0 },
        {  1.0, 0.3f, 0.2f },
        {  0.8f, 1.0, 0.0f },
        {  0.8f, 0.0f, 1.0 },
    };

    simd::float2 textCoords[NumVertices] =
    {
        {0.f, 0.f}, // left top
        {1.f, 0.f}, // right top
        {1.f, 1.f}, // right down
        {1.f, 1.f}, // right down
        {0.f, 1.f}, // left down
        {0.f, 0.f}, // left top
    };

    const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
    const size_t colorDataSize = NumVertices * sizeof( simd::float3 );
    const size_t textCoordsSize = NumVertices * sizeof( simd::float2 );

    MTL::Buffer* pVertexPositionsBuffer = p_device->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pVertexColorsBuffer = p_device->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );

    p_vertexPositions = pVertexPositionsBuffer;
    p_vertexColors = pVertexColorsBuffer;
    p_texCoords = p_device->newBuffer( textCoordsSize, MTL::ResourceStorageModeManaged );

    memcpy( p_vertexPositions->contents(), positions, positionsDataSize );
    memcpy( p_vertexColors->contents(), colors, colorDataSize );
    memcpy( p_texCoords->contents(), textCoords, textCoordsSize );

    p_vertexPositions->didModifyRange( NS::Range::Make( 0, p_vertexPositions->length() ) );
    p_vertexColors->didModifyRange( NS::Range::Make( 0, p_vertexColors->length() ) );
    p_texCoords->didModifyRange( NS::Range::Make( 0, p_texCoords->length() ) );

    // argument buffer encoding
    using NS::StringEncoding::UTF8StringEncoding;
    MTL::Function* fnVertex = p_shaderLibrary->newFunction( NS::String::string("main_vertex", UTF8StringEncoding) );
    MTL::ArgumentEncoder* argEnc = fnVertex->newArgumentEncoder(0);

    p_argBuffer = p_device->newBuffer( argEnc->encodedLength(), MTL::ResourceStorageModeManaged );
    
    argEnc->setArgumentBuffer( p_argBuffer, 0 );
    argEnc->setBuffer( p_vertexPositions, 0, 0 );
    argEnc->setBuffer( p_vertexColors, 0, 1 );
    argEnc->setBuffer( p_texCoords, 0, 2 );

    p_argBuffer->didModifyRange(NS::Range::Make(0, p_argBuffer->length()));

    fnVertex->release();
    argEnc->release();
}

void Renderer::build_textures()
{
    // stbi_set_flip_vertically_on_load(true);
    m_imgData = stbi_load(m_texPath, &m_imgWidth, &m_imgHeight, &m_imgChannels, STBI_rgb_alpha);

    if (!m_imgData)
    {
        __builtin_printf("Couldn't load the image for texture \n");
        return;
    }

    MTL::TextureDescriptor* pTexDesc = MTL::TextureDescriptor::alloc()->init();
    pTexDesc->setWidth(m_imgWidth);
    pTexDesc->setHeight(m_imgHeight);
    pTexDesc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    pTexDesc->setTextureType(MTL::TextureType2D);
    pTexDesc->setStorageMode(MTL::StorageModeManaged);
    pTexDesc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead);

    p_texture = p_device->newTexture(pTexDesc);

    size_t bytesPerPixel = m_imgChannels; // Assuming 1 byte per channel
    size_t bytesPerRow = (bytesPerPixel * m_imgWidth + 3) & ~3; // Align to a 4-byte boundary
    p_texture->replaceRegion( MTL::Region(0, 0, 0, m_imgWidth, m_imgHeight, 1), 0, m_imgData, bytesPerRow );

    pTexDesc->release();
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = p_cmdQ->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);

    pEnc->setRenderPipelineState(p_pipelineState);
    pEnc->setVertexBuffer( p_argBuffer, 0, 0 );
    pEnc->useResource( p_vertexPositions, MTL::ResourceUsageRead );
    pEnc->useResource( p_vertexColors, MTL::ResourceUsageRead );
    pEnc->useResource( p_texCoords, MTL::ResourceUsageRead );

    pEnc->setFragmentTexture( p_texture, 0 );

    pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6) );

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pool->release();
}
