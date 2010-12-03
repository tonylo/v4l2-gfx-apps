/*!****************************************************************************
@File           texplayer.h

@Title          Texture Player

@Author         Texas Instruments

@Date           2010/08/19

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    In coordination with another application streaming to the
                buffer class device we render the content on the screen

******************************************************************************/

#ifndef __TEXPLAYER_H__
#define __TEXPLAYER_H__

#if defined(SUPPORT_ANDROID_PLATFORM)
#include <nativehelper/jni.h>

#define LOG_TAG "GLES2TEXPLAYER"
#include <cutils/log.h>
#define INFO  LOGI
#define ERROR LOGE
#else
extern void timestamp(void);
#define INFO(fmt, arg...) \
	do {                                \
		timestamp(); printf(fmt "\n", ## arg);    \
	} while (0)
#define ERROR INFO

#endif	// SUPPORT_ANDROID_PLATFORM

#if defined(SUPPORT_ANDROID_PLATFORM)
JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_init(
	JNIEnv *env, jobject obj, jstring wrapLib);

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_eglMain(
	JNIEnv *env, jobject obj, jobject eglNativeWindow);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_resizeWindow(
	JNIEnv *env, jobject obj, jint width, jint height);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_requestQuit(
	JNIEnv *env, jobject obj);
#endif	// SUPPORT_ANDROID_PLATFORM

extern int v4l2_wait(int fd);
extern int acquire_v4l2_frame(int fd, struct v4l2_gfx_buf_params* p);
extern int release_v4l2_frame(int fd, int bufid);

/* texplayer_gles2 */
extern int init_glstate(int fd);
extern void gles2_deinit_glstate(void);
extern void stream_texture(int dev, void *dev2texturearg);
extern void draw_frame(int fd);

/* texplayer_simple */
extern int init_glstate_simple(int fd);
extern void stream_texture_simple(int dev, void *dev2texturearg);
extern void draw_frame_simple(int fd);

#endif /* __TEXPLAYER_H__ */
