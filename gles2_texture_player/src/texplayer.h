/*!****************************************************************************
@File           texplayer.h

@Title

@Author         Texas Instruments

@Date           2010/12/15

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description

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

/* texplayer_gles2 */
extern int gl_init_state(int fd, EGLDisplay a_egldpy, EGLSurface a_eglsurface, EGLContext a_eglcontxt, int num_devices);
extern void gl_deinit_state(void);
extern int gl_stream_texture(int dev);
extern int gl_draw_frame(int fd);

/* N.B. These values must sync with the java sources */
#define GLAPP_PARM_NONE 0
#define GLAPP_PARM_ANIMATED 1

extern void gl_set_app_params(int a_parm, int a_value);

#include "v4l2_gfx.h"
#include "misc.h"

#endif /* __TEXPLAYER_H__ */
