#include "renderer.hpp"

class ViewDelegate : public MTK::ViewDelegate
{
    public:
        ViewDelegate(MTL::Device* pDevice);
        ~ViewDelegate() override;
        void drawInMTKView(MTK::View* pView) override;

    private:
        Renderer* _pRenderer;
};
