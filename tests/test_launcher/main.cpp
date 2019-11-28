// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/App/Android/AndroidApplication.h"
using namespace AE::App;

extern int Test_STL ();
extern int Test_Platform (IApplication &app, IWindow &wnd);
extern int Test_Networking ();
extern int Test_Scripting ();
extern int Test_ECSst ();


class WndListener final : public IWindow::IWndListener
{
private:
    IApplication&	_app;
    uint			_counter	= 0;

public:
    WndListener (IApplication &app) : _app{app} {}
    ~WndListener () override {}

    void OnCreate (IWindow &) override
    {
        AE_LOGI( "OnCreate" );
    }

    void OnDestroy (IWindow &) override
    {
        AE_LOGI( "OnDestroy" );
    }

    void OnStart (IWindow &) override
    {
        AE_LOGI( "OnStart" );
    }

    void OnStop (IWindow &) override
    {
        AE_LOGI( "OnStop" );
    }

    void OnEnterForeground (IWindow &) override
    {
        AE_LOGI( "OnEnterForeground" );
    }

    void OnEnterBackground (IWindow &) override
    {
        AE_LOGI( "OnEnterBackground" );
    }

    void OnUpdate (IWindow &wnd) override
    {
        if ( ++_counter == 100 )
        {
            wnd.Close();
        }
    }

    void OnResize (IWindow &, const uint2 &) override
    {
        AE_LOGI( "OnResize" );
    }

    void OnSurfaceCreated (IWindow &wnd) override
    {
        AE_LOGI( "OnSurfaceCreated" );
        Test_STL();
        Test_Networking();
        Test_Scripting();
        Test_Platform( _app, wnd );
        Test_ECSst();
    }

    void OnSurfaceDestroyed (IWindow &) override
    {
        AE_LOGI( "OnSurfaceDestroyed" );
    }
};


class AppListener final : public IApplication::IAppListener
{
private:
    WindowPtr  _window;

public:
    AppListener () {}
    ~AppListener () override {}

    void OnStart (IApplication &app) override
    {
        _window = app.CreateWindow( MakeUnique<WndListener>( app ), Default );
        CHECK_FATAL( _window );
    }

    void OnUpdate (IApplication &a) override {}
    void OnStop (IApplication &) override {}
};


UniquePtr<IApplication::IAppListener>  AE_OnAppCreated ()
{
    return MakeUnique<AppListener>();
}

extern "C" JNIEXPORT jint  JNI_OnLoad (JavaVM* vm, void*)
{
return AndroidApplication::OnJniLoad( vm );
}

extern "C" void JNI_OnUnload (JavaVM *vm, void *)
{
    return AndroidApplication::OnJniUnload( vm );
}
