/*!****************************************************************************
@File           texplayer.h

@Title          Texture Player

@Author         Texas Instruments

@Date           2010/08/19

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    In coordination with another application streaming to the
                V4L2-GFX device we render the content on the screen

******************************************************************************/

#ifndef __TEXPLAYER_H__
#define __TEXPLAYER_H__

#if defined(SUPPORT_ANDROID_PLATFORM)
#include <nativehelper/jni.h>

#define LOG_TAG "GLESTEXPLAYER"
#include <cutils/log.h>
#define INFO  LOGI
#define ERROR LOGE
#else
#define INFO(fmt, arg...) \
	do {                                \
		printf(fmt "\n", ## arg);    \
	} while (0)
#define ERROR INFO

#endif	// SUPPORT_ANDROID_PLATFORM

#if defined(SUPPORT_ANDROID_PLATFORM)
JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_init(
	JNIEnv *env, jobject obj, jstring wrapLib);

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_eglMain(
	JNIEnv *env, jobject obj, jobject eglNativeWindow);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_resizeWindow(
	JNIEnv *env, jobject obj, jint width, jint height);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_requestQuit(
	JNIEnv *env, jobject obj);
#endif	// SUPPORT_ANDROID_PLATFORM

extern int v4l2_wait(int fd);
extern int acquire_v4l2_frame(int fd, struct v4l2_gfx_buf_params* p);
extern int release_v4l2_frame(int fd, int bufid);

/* texplayer_simple */
extern int init_glstate_simple(int fd);
extern void stream_texture_simple(int dev, void *dev2texturearg);
extern int draw_frame_simple(int fd);

#endif /* __TEXPLAYER_H__ */
