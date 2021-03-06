// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

package AE.engine;

import android.app.Activity;
import android.content.res.Configuration;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.os.Handler;
import android.view.WindowManager;

//
// Base Activity
//
public class BaseActivity
		extends		Activity
		implements	SurfaceHolder.Callback,
					View.OnKeyListener,
					View.OnTouchListener
{
	public static final String	TAG = "AE";

	private int 				_wndID		= 0;
	
	private Handler				_handler	= new Handler();
	private static final long	_tickDelay	= 1000 / 30;
	private final Runnable		_nativeTick	= new Runnable()
	{
		@Override public final void run ()
		{
			try {
				native_Update( _wndID );
				_handler.postDelayed(_nativeTick, _tickDelay);
			}
			catch (Exception e) {
				Log.i( TAG,"Exception: " + e.toString() );
			}
		}
	};


//-----------------------------------------------------------------------------
// Activity

	@Override protected void onCreate (android.os.Bundle savedInstance)
	{
		super.onCreate( savedInstance );
		_wndID = native_OnCreate( this );
		_CreateSurface();
	}

	@Override protected void onDestroy ()
	{
		super.onDestroy();
		native_OnDestroy( _wndID );
	}

	@Override protected void onPause ()
	{
		super.onPause();
		native_OnEnterBackground( _wndID );

		// update before stop
		_nativeTick.run();
		_handler.removeCallbacks( _nativeTick );
	}

	@Override protected void onResume ()
	{
		super.onResume();
		native_OnEnterForeground( _wndID );
		_nativeTick.run();
	}

	@Override protected void onStart ()
	{
		super.onStart();
		native_OnStart( _wndID );
	}

	@Override protected void onStop ()
	{
		super.onStop();
		native_OnStop( _wndID );
		_handler.removeCallbacks( _nativeTick );
	}
	
	@Override public void onConfigurationChanged (Configuration newConfig)
	{
		super.onConfigurationChanged( newConfig );
		Display display = ((WindowManager)getSystemService( WINDOW_SERVICE )).getDefaultDisplay();
		native_OnOrientationChanged( _wndID, display.getRotation() );
	}
	
//-----------------------------------------------------------------------------
 // SurfaceHolder.Callback
 
	private SurfaceView		_surfaceView;

	private final void  _CreateSurface ()
	{
		_surfaceView = new SurfaceView( this );
		setContentView( _surfaceView );
		
		SurfaceHolder holder = _surfaceView.getHolder();
		holder.addCallback( this );

		_surfaceView.setFocusable( true );
		_surfaceView.setFocusableInTouchMode( true );
		_surfaceView.requestFocus();
		_surfaceView.setOnKeyListener( this );
		_surfaceView.setOnTouchListener( this );
	}

	@Override public final void surfaceChanged (SurfaceHolder holder, int format, int w, int h) {
		native_SurfaceChanged( _wndID, holder.getSurface() );
	}

	@Override public final void surfaceCreated (SurfaceHolder holder) {
	}

	@Override public final void surfaceDestroyed (SurfaceHolder holder) {
		native_SurfaceDestroyed( _wndID );
	}
	

//-----------------------------------------------------------------------------
// View.OnKeyListener

	@Override public final boolean onKey (View v, int keyCode, KeyEvent ev)
	{
		if ( keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
			 keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
			 keyCode == KeyEvent.KEYCODE_VOLUME_MUTE ) {
			// android will change volume
			return false;
		}
		if ( keyCode == KeyEvent.KEYCODE_BACK ) {
			return false;
		}
		native_OnKey( _wndID, keyCode, ev.getAction() );
		return true;
	}
	

//-----------------------------------------------------------------------------
// View.OnTouchListener

	private static final int _maxTouches	= 8;
	private float[]			_touchData		= new float[_maxTouches * 4]; // packed: {id, x, y, pressure}

	@Override public final boolean onTouch (View vw, MotionEvent ev)
	{
		int num_pointers = Math.min( ev.getPointerCount(), _maxTouches );
		int action = ev.getActionMasked();
		
		if ( action == MotionEvent.ACTION_MOVE && num_pointers > 1 )
		{
			for (int i = 0, j = 0; i < num_pointers; ++i)
			{
				_touchData[j++] = (float)ev.getPointerId( i );
				_touchData[j++] = ev.getX( i );
				_touchData[j++] = ev.getY( i );
				_touchData[j++] = ev.getPressure( i );
			}
		}
		else
		{
			int index = ev.getActionIndex();
			_touchData[0] = (float)ev.getPointerId( index );
			_touchData[1] = ev.getX( index );
			_touchData[2] = ev.getY( index );
			_touchData[3] = ev.getPressure( index );
		}

		native_OnTouch( _wndID, action, num_pointers, _touchData );
		return true;
	}

//-----------------------------------------------------------------------------
// called from native

	public void Close ()
	{
		this.finish();
	}

//-----------------------------------------------------------------------------
// native

	private static native int  native_OnCreate (Object wnd);
	private static native void  native_OnDestroy (int id);
	private static native void  native_OnStart (int id);
	private static native void  native_OnStop (int id);
	private static native void  native_OnEnterForeground (int id);
	private static native void  native_OnEnterBackground (int id);
	private static native void  native_SurfaceChanged (int id, Object surface);
	private static native void  native_SurfaceDestroyed (int id);
	private static native void  native_Update (int id);
	private static native void  native_OnKey (int id, int keycode, int action);
	private static native void  native_OnTouch (int id, int action, int count, float[] data);
	private static native void  native_OnOrientationChanged (int id, int newOrientation);
}
