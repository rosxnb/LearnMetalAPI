#include "renderer.hpp"
#include "utility.hpp"

Renderer::Renderer( MTL::Device* pDevice )
    : p_device( pDevice->retain() )
    , p_cmdQ( p_device->newCommandQueue() )
{ 
    build_shaders();
    build_buffers();
}

Renderer::~Renderer()
{
    p_cmdQ->release();
    p_device->release();
    delete p_shaderSrc; 
}

void Renderer::build_shaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Error* pError {nullptr};
    const char* shaderSrc = Utility::read_source("shader/program.metal")->data();
    MTL::Library* pShaderLib = p_device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
    if ( !pShaderLib )
    {
        __builtin_printf("Library creation from shader source failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    MTL::Function* fnVertex     = pShaderLib->newFunction( NS::String::string("main_vertex", UTF8StringEncoding) );
    MTL::Function* fnFragment   = pShaderLib->newFunction( NS::String::string("main_fragment", UTF8StringEncoding) );
    MTL::RenderPipelineDescriptor* pRpd = MTL::RenderPipelineDescriptor::alloc()->init();

    pRpd->setVertexFunction( fnVertex );
    pRpd->setFragmentFunction( fnFragment );
    pRpd->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    pRpd->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    MTL::RenderPipelineState* pRPS = p_device->newRenderPipelineState( pRpd, &pError );
    if ( !pRPS )
    {
        __builtin_printf("RenderPipelineState creation failed. \n");
        __builtin_printf("%s \n\n", pError->localizedDescription()->utf8String());
        assert( false );
    }

    fnVertex->release();
    fnFragment->release();
    pRpd->release();
    pShaderLib->release();
    pRPS->release();
}

void Renderer::build_buffers()
{
}

void Renderer::draw( MTK::View* pView )
{
}
