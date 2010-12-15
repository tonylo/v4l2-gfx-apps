/*!****************************************************************************
@File           misc.h

@Title

@Author         Texas Instruments

@Date           2010/12/15

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description

******************************************************************************/

#ifndef __MISC_H__
#define __MISC_H__

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

#endif /* __MISC_H__ */
