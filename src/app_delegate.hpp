#include "view_delegate.hpp"

class AppDelegate : public NS::ApplicationDelegate
{
    public:
        ~AppDelegate();

        NS::Menu* createMenuBar();

        void applicationWillFinishLaunching(NS::Notification* pNotification) override;
        void applicationDidFinishLaunching(NS::Notification* pNotification) override;
        bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender) override;

    private:
        NS::Window* _pWindow;
        MTK::View* _pView;
        MTL::Device* _pDevice;
        ViewDelegate* _pViewDelegate;
};
