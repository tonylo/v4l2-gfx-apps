/*!****************************************************************************
@File           Gles2TexturePlayer.java

@Title          Test app for Android (Java component)

@Author         Texas Instruments

@Date           2010/10/22

@Copyright      Copyright (C) Texas Instruments - http://www.ti.com/

@Platform       android

@Description    Play a video clip and coordinate with an appropriately
                modified multimedia backend to stream textures to a GL scene.


Modifications :-
$Log: Gles2TexturePlayer.java $
Revision 1.1  2010/10/22 11:04:00  tony.lofthouse
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk.Gles2TexturePlayer;

import java.lang.Runnable;
import java.lang.Thread;
import java.util.Random;

import android.view.SurfaceHolder;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.Surface;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.media.MediaPlayer;
import java.io.IOException;
import android.os.PowerManager;

import android.database.Cursor;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.provider.MediaStore;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class Gles2TexturePlayer extends Activity
            implements SurfaceHolder.Callback, OnItemSelectedListener
{
    /** Load wrapped library and find key symbols */
    private static native int init(String wrapLib);

    /** Wrapper for app's EglMain(); unmarshals eglNativeWindow */
    private static native int eglMain(Object surface);

    /** Wrapper for app's ResizeWindow() */
    private static native void resizeWindow(int width, int height);

    /** Wrapper for app's RequestQuit() */
    private static native void requestQuit();

    /** Parameters, this needs to align with C source data structure */
    private static int GLAPP_PARM_NONE = 0;
    private static int GLAPP_PARM_ANIMATED = 1;

    /** Pass parameters to C implementation */
    private static native void setParams(int param, int value);

    // This points to the location of the JNI library i.e. our C code
    private static final String mJni_library_locn =
        "/lib/libgles2_texture_player.so";

    private Thread mAppThread;
    private Surface mSurface;
    private MediaPlayer mMediaPlayer;
    private PowerManager.WakeLock mWl;
    private boolean mLaunchVideo;

    /* Support for listing video files */
    private Cursor mVideoCursor;
    private int mCount;
    private int mVideoColumnIndex;
    private String mFilename;
    private ButtonClickhandler mButtonClickhandler;
    private OnCheckedChangeListener mCheckBoxAnimatedhandler;

    /* Entry point for Activity */
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initialize_nativecode();

        setParamsUi(GLAPP_PARM_ANIMATED, 0);

        setContentView(R.layout.main);	/* Read layout xml */

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "DoNotDimScreen");

        Spinner mediafilelist = (Spinner)findViewById(R.id.SpinnerMediaFileList);
        Button videolaunchbtn = (Button)findViewById(R.id.ButtonLaunchVideo);
        Button otherlaunchbtn = (Button)findViewById(R.id.ButtonLaunchStream);
        CheckBox cb_animated = (CheckBox)findViewById(R.id.cb_animated);

        String[] proj = { MediaStore.Video.Media._ID,
                          MediaStore.Video.Media.DATA,
                          MediaStore.Video.Media.DISPLAY_NAME,
                          MediaStore.Video.Media.SIZE
                        };
        mVideoCursor = managedQuery(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, proj, null, null, null);

        mCount = mVideoCursor.getCount();

        Log.i("APP:", "video count = " + mCount);

        mediafilelist.setAdapter(new VideoAdapter(getApplicationContext()));
        mediafilelist.setPrompt("Located media files:");
        mediafilelist.setOnItemSelectedListener(this);
        mButtonClickhandler = new ButtonClickhandler(this, true);
        videolaunchbtn.setOnClickListener(mButtonClickhandler);
        mButtonClickhandler = new ButtonClickhandler(this, false);
        otherlaunchbtn.setOnClickListener(mButtonClickhandler);
        mCheckBoxAnimatedhandler = new AnimCheckChangehandler(this);
        cb_animated.setOnCheckedChangeListener(mCheckBoxAnimatedhandler);
    }

    //@Override
    protected void onPause() {
        super.onPause();
        mWl.release();
    }
    //@Override
    protected void onResume() {
        super.onResume();
        mWl.acquire();
    }

    /* do something when the spinner list (i.e. filenames) is activated */
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        Log.i("APP:", "Got an item selected");

        mVideoColumnIndex = mVideoCursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA);
        mVideoCursor.moveToPosition(position);
        mFilename = mVideoCursor.getString(mVideoColumnIndex);
        Log.i("APP:", "Filename is:" + mFilename);
    }

    public void onNothingSelected(AdapterView<?> arg0) {

    }

    /* This handles obtaining a list of media files */
    public class VideoAdapter extends BaseAdapter {
        private Context vContext;

        public VideoAdapter(Context c) {
            vContext = c;
        }

        public int getCount() {
            return mCount;
        }

        public Object getItem(int position) {
            return position;
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            System.gc();
            TextView tv = new TextView(vContext.getApplicationContext());
            String id = null;
            if (convertView == null) {
                mVideoColumnIndex = mVideoCursor.getColumnIndexOrThrow(MediaStore.Video.Media.DISPLAY_NAME);
                mVideoCursor.moveToPosition(position);
                id = mVideoCursor.getString(mVideoColumnIndex);
                mVideoColumnIndex = mVideoCursor.getColumnIndexOrThrow(MediaStore.Video.Media.SIZE);
                mVideoCursor.moveToPosition(position);
                id += " Size(KB):" + mVideoCursor.getString(mVideoColumnIndex);
                tv.setText(id);
            } else
                tv = (TextView) convertView;
            return tv;
        }
    }

    /* Handle touch events on the surface created by the playvideo button */
    private class ViewTouchHandler implements OnTouchListener {

        private Gles2TexturePlayer mActivity;
        public ViewTouchHandler(Gles2TexturePlayer mainactivity) {
            mActivity = mainactivity;
        }

        public boolean onTouch(View arg0, MotionEvent arg1) {
            Log.i("APP:", "onTouch");
            /* TODO: we should be able to send this event to the native gl code */
            return false;
        }

    }

    /*
     * Handle clicking the playback button clicks, this will
     * create a surface for the GL native code to use
     */
    private class ButtonClickhandler implements OnClickListener {
        private SurfaceView mSurfaceView;
        private Gles2TexturePlayer mActivity;
        private boolean mLaunchVideo;

        /*
         * @param  launchvideo  If true we will attempt to use the native video
         *                      playback APIs to render video.
         *
         *                      If false, this app assumes some other process is
         *                      going to stream content to the underlying buffer
         *                      class device.
         */
        public ButtonClickhandler(Gles2TexturePlayer activity, boolean launchvideo) {
            mActivity = activity;
            mLaunchVideo = launchvideo;
        }

        public void onClick(View v) {
            Log.i("APP:", "Button clicked, selected file:" + mFilename);
            if (mLaunchVideo)
                mActivity.doLaunchVideo();
            else
                mActivity.doNotLaunchVideo();

//			mActivity.initialize_nativecode();

            mSurfaceView = new SurfaceView(getApplication());
            SurfaceHolder holder = mSurfaceView.getHolder();
            holder.addCallback(mActivity);

            mSurfaceView.setOnTouchListener(new ViewTouchHandler(mActivity));
            setContentView(mSurfaceView);
        }
    }

    private class AnimCheckChangehandler implements OnCheckedChangeListener {
        private Gles2TexturePlayer mActivity;

        public AnimCheckChangehandler(Gles2TexturePlayer activity) {
            mActivity = activity;
        }

        public void onCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
            if (isChecked) {
                setParamsUi(GLAPP_PARM_ANIMATED, 1); /* set */
            } else {
                setParamsUi(GLAPP_PARM_ANIMATED, 0); /* clear */
            }
        }
    }

    /* See: SurfaceHolder.Callback */
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        resizeWindow(w, h);
    }

    /* See: SurfaceHolder.Callback */
    public void surfaceCreated(SurfaceHolder holder) {

        Log.i("APP:", "Surface for native GL code created");
        mAppThread = new Thread(new AppRunnable());
        mSurface = holder.getSurface();

        /* Write a message on the new surface */
        Canvas canvas = holder.lockCanvas();
        Paint p = new Paint();
        p.setStyle(Paint.Style.FILL);
        p.setColor(Color.GREEN);
        p.setTextSize(40);
        p.setAntiAlias(true);
        canvas.drawText("V4L2-GFX needs a data stream, waiting", 75, 75, p);
        holder.unlockCanvasAndPost(canvas);

        /*
         * Note: we must start the GL render thread before launching a video with MediaPlayer.
         * This is because a tweaked version of libstagefrighthw.so knows how to
         * render to texture buffers when an application opens the V4L2-GFX driver first
         * (as a video consumer) rather than routing to overlays.
         */
        mAppThread.start();

        if (mLaunchVideo && (mFilename != null)) {
            mMediaPlayer = new MediaPlayer();
            launchVideo(holder);
        }
    }

    /* See: SurfaceHolder.Callback */
    public void surfaceDestroyed(SurfaceHolder holder) {
        requestQuit();
        try {
            mMediaPlayer.release();
            mAppThread.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    public void doLaunchVideo() {
        mLaunchVideo = true;
    }
    public void doNotLaunchVideo() {
        mLaunchVideo = false;
    }

    public void initialize_nativecode()
    {
        String mTestShellLib = "/data/data/" +
                               getClass().getPackage().getName() +
                               mJni_library_locn;
        System.load(mTestShellLib);

        String mWrapLib = "";
        if (init(mWrapLib) < 0)	/* Call the JNI init entry point */
            throw new RuntimeException("Failed to initialize " + mWrapLib);
    }

    public void setParamsUi(int param, int value) {
        /* Call native interface */
        setParams(param, value);
    }

    private class AppRunnable implements Runnable {
        public void run() {
            eglMain(mSurface);
        }
    }

    protected void launchVideo(SurfaceHolder holder) {
        Log.i("APP:", "Setting up media player=");

        try {
            mMediaPlayer.setDataSource(mFilename);
            mMediaPlayer.setDisplay(holder);
            mMediaPlayer.setLooping(true);
            mMediaPlayer.prepare();
        }
        catch (IOException ex){
            Log.i("APP:", "Exception during media player setup");
            return;

        }
        mMediaPlayer.setLooping(true);
        mMediaPlayer.start();
    }


}
