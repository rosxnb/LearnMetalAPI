#include "view_delegate.hpp"

ViewDelegate::ViewDelegate(MTL::Device* pDevice)
    : _pRenderer(new Renderer(pDevice))
{ }

ViewDelegate::~ViewDelegate()
{ delete _pRenderer; }

void ViewDelegate::drawInMTKView(MTK::View* pView)
{
    _pRenderer->draw(pView);
}
