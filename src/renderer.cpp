#include "renderer.hpp"

Renderer::Renderer(MTL::Device* pDevice)
    : _pDevice(pDevice)
    , _pCmdQ(_pDevice->newCommandQueue())
{ }

Renderer::~Renderer()
{
    _pDevice->release();
    _pCmdQ->release();
}

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmdBuff = _pCmdQ->commandBuffer();
    MTL::RenderPassDescriptor* pDesciptor = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEncoder = pCmdBuff->renderCommandEncoder(pDesciptor);

    pEncoder->endEncoding();
    pCmdBuff->presentDrawable(pView->currentDrawable());
    pCmdBuff->commit();

    pPool->release();
}
