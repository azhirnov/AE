// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

package AE.engine;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Environment;
import android.widget.Toast;

import java.io.File;

//
// Base Application
//
public class BaseApplication extends Application
{

	public void  onCreate ()
	{
		super.onCreate();
		native_OnCreate( this, getResources().getAssets() );

		_SendDirectories();
		_SendDeviceInfo();
		_SendDisplayInfo();
	}
	
	

//-----------------------------------------------------------------------------
// utils

	private void _SendDirectories ()
	{
		File 	internalPath	= getFilesDir();
		File	internalCache	= getCacheDir();
		File	externalPath	= getExternalFilesDir(null);
		File	externalCache	= getExternalCacheDir();
		File	externalStorage	= Environment.getExternalStorageDirectory();

		native_SetDirectories(
				( internalPath    == null ? "" : internalPath.getAbsolutePath()    ),
				( internalCache   == null ? "" : internalCache.getAbsolutePath()   ),
				( externalPath    == null ? "" : externalPath.getAbsolutePath()    ),
				( externalCache   == null ? "" : externalCache.getAbsolutePath()   ),
				( externalStorage == null ? "" : externalStorage.getAbsolutePath() ));
	}

	private void  _SendDeviceInfo ()
	{
		// TODO
	}
	
	private void  _SendDisplayInfo ()
	{
		// TODO
	}

	
//-----------------------------------------------------------------------------
// called from native

	@SuppressWarnings("unused")
	public final void  ShowToast (String msg, boolean longTime)
	{
		int duration = longTime ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT;
		Toast toast = Toast.makeText( this, msg, duration );
		toast.show();
	}
	
	@SuppressWarnings("unused")
	public final boolean  IsNetworkConnected ()
	{
		try {
			ConnectivityManager cm = (ConnectivityManager)getSystemService( Context.CONNECTIVITY_SERVICE );
			NetworkInfo ni = cm.getActiveNetworkInfo();
			if ( ni != null ) {
				NetworkInfo.DetailedState state = ni.getDetailedState();
				if ( state == NetworkInfo.DetailedState.CONNECTED )
					return true;
			}
		} catch (Exception e) {
			//Log.e( TAG, "exception: " + e.toString() );
		}
		return false;
	}

	@SuppressWarnings("unused")
	public void  CreateWindow ()
	{
		// TODO
		//Intent intent = new Intent(this, AE.engine.BaseActivity.class );
		//intent.setFlags( Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK );
		//startActivity( intent );
	}

//-----------------------------------------------------------------------------
// native

	private static native void  native_OnCreate (Object app, Object assetMngr);
	private static native void  native_SetDirectories (String internal, String internalCache, String external, String externalCache, String externalStorage);
}
