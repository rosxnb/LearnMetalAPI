#include "app_delegate.hpp"

AppDelegate::~AppDelegate()
{
    _pView->release();
    _pWindow->release();
    _pDevice->release();
    delete _pViewDelegate;
}

NS::Menu* AppDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Menu* pAppMenu = NS::Menu::alloc()->init();
    NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
    {
        NS::String* pAppName = NS::RunningApplication::currentApplication()->localizedName();
        NS::String* pQuitName = NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(pAppName);
        SEL quitCb = NS::MenuItem::registerActionCallback("quitapp:", [](void *, SEL, const NS::Object* pSender){
            auto pApp = NS::Application::sharedApplication();
            pApp->terminate( pSender );
        });

        NS::MenuItem* pQuitItem = pAppMenu->addItem(pQuitName, quitCb, NS::String::string("q", UTF8StringEncoding));
        pQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
        pAppMenuItem->setSubmenu( pAppMenu );
    }

    NS::Menu* pWindowMenu = NS::Menu::alloc()->init( NS::String::string("Window", UTF8StringEncoding) );
    NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
    {
        NS::String* pCloseName = NS::String::string("Close Window", UTF8StringEncoding);
        SEL closeCb = NS::MenuItem::registerActionCallback("closewindow:", [](void *, SEL, const NS::Object* pSender){
            auto pApp = NS::Application::sharedApplication();
            pApp->windows()->object< NS::Window >(0)->close();
        });

        NS::MenuItem* pCloseItem = pWindowMenu->addItem(pCloseName, closeCb, NS::String::string("w", UTF8StringEncoding));
        pCloseItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
        pWindowMenuItem->setSubmenu( pWindowMenu );
    }

    NS::Menu* pMainMenu = NS::Menu::alloc()->init();
    pMainMenu->addItem(pAppMenuItem);
    pMainMenu->addItem(pWindowMenuItem);

    pAppMenu->release();
    pWindowMenu->release();
    pAppMenuItem->release();
    pWindowMenuItem->release();

    return pMainMenu;
}

void AppDelegate::applicationWillFinishLaunching( NS::Notification* pNotification )
{
    NS::Menu* pMenu = createMenuBar();
    NS::Application* pApp = reinterpret_cast< NS::Application* >(pNotification->object());
    pApp->setMainMenu(pMenu);
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

void AppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
{
    _pDevice = MTL::CreateSystemDefaultDevice();
    _pViewDelegate = new ViewDelegate( _pDevice );

    CGRect frame = (CGRect){
        {260.f, 180.f},
        {1200.f, 750.f}
    };

    _pView = MTK::View::alloc()->init(frame, _pDevice);
    _pView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    _pView->setClearColor(MTL::ClearColor::Make(0.01, 0.01, 0.01, 1.0));
    _pView->setDelegate( _pViewDelegate );

    _pWindow = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled | NS::WindowStyleMaskResizable,
            NS::BackingStoreBuffered,
            false );

    _pWindow->setContentView( _pView );
    _pWindow->setTitle( NS::String::string("MyMetalAppDude!", NS::StringEncoding::UTF8StringEncoding) );
    _pWindow->makeKeyAndOrderFront( nullptr );

    NS::Application* pApp = reinterpret_cast< NS::Application* >(pNotification->object());
    pApp->activateIgnoringOtherApps( true );
}

bool AppDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender )
{
    return true;
}
