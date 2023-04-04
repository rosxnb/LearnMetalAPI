#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "app_delegate.hpp"

int main()
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    AppDelegate appDelegate;
    NS::Application* pApp = NS::Application::sharedApplication();
    pApp->setDelegate( &appDelegate );
    pApp->run();

    pPool->release();

    return 0;
}
