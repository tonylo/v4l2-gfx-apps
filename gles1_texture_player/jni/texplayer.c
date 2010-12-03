/*!****************************************************************************
@File           texplayer.c

@Title          Texture Player

@Author         Texas Instruments

@Date           2010/08/19

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    In coordination with another application streaming to the
                buffer class device we render the content on the screen

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <dlfcn.h>

#include <linux/videodev.h>
#include <linux/omap_v4l2_gfx.h>

#if defined(SUPPORT_ANDROID_PLATFORM)
#include <private/ui/android_natives_priv.h>
#endif
#include <EGL/egl.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "texplayer.h"

#define __unref __attribute__((unused))

#if defined(SUPPORT_ANDROID_PLATFORM)
#include "eglhelper.h"

static jfieldID	gSurfaceFieldID;
typedef int  (*EglMain_t)(EGLNativeDisplayType display,
						  EGLNativeWindowType window);
typedef void (*ResizeWindow_t)(unsigned int width, unsigned int height);
typedef void (*RequestQuit_t)(void);
#endif

/* EGL global variables */
EGLDisplay egldpy;
EGLSurface eglsurface;
EGLContext eglcontxt;

/* texture objects */
GLuint **pptex_objs;
GLuint logotex;

/* Function declarations */
PFNGLTEXBINDSTREAMIMGPROC glTexBindStreamIMG = NULL;
PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC glGetTexAttrIMG = NULL;
PFNGLGETTEXSTREAMDEVICENAMEIMGPROC glGetTexDevIMG = NULL;

int g_num_devices;

/* Function declarations */
static void egl_error(char *name);
const char *egl_strerror(int code);
void deinit_display(void);
int handle_events(void);
int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglwindow);

/*****************************************************************************/
/*****************************************************************************/

static int init_display(
			EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglwindow,
			int glesversion)
{
    EGLConfig configs[2];
    EGLBoolean egl_err;

#if !defined(SUPPORT_ANDROID_PLATFORM)
    int config_attribs[] = {EGL_BUFFER_SIZE,    EGL_DONT_CARE,
                            EGL_RED_SIZE,       5,
                            EGL_GREEN_SIZE,     6,
                            EGL_BLUE_SIZE,      5,
                            EGL_DEPTH_SIZE,     8,
                            EGL_RENDERABLE_TYPE,
							glesversion == 1 ? EGL_OPENGL_ES_BIT :
											   EGL_OPENGL_ES2_BIT,
                            EGL_NONE};
	int no_configs;
#endif 
    int contxt_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, glesversion, EGL_NONE};
    int major, minor;

    INFO("glesversion = %d\n", glesversion);
    INFO("%d %d %d\n", contxt_attribs[0], contxt_attribs[1], contxt_attribs[2]);

    /* get display */
    egldpy = eglGetDisplay(eglDisplay);


    /* initialize display */
    if ((egl_err = eglInitialize(egldpy, &major, &minor)) != EGL_TRUE)
        egl_error("eglInitialize");

    INFO("major = %d minor = %d\n", major, minor);

#if defined(SUPPORT_ANDROID_PLATFORM)
	configs[0] = findMatchingWindowConfig(egldpy,
		glesversion == 1 ? EGL_OPENGL_ES_BIT : EGL_OPENGL_ES2_BIT,
		eglwindow);
	configs[1] = (EGLConfig)0;
#else

    if ((egl_err = eglGetConfigs(egldpy, configs, 2, &no_configs))
            != EGL_TRUE) {
        egl_error("eglGetConfigs");
	}

    if ((egl_err = eglChooseConfig(egldpy, config_attribs, configs, 2,
					&no_configs)) != EGL_TRUE) {
        egl_error("eglChooseConfig");
	}
#endif

    eglsurface = eglCreateWindowSurface(egldpy, configs[0], eglwindow,
            NULL);
    if (eglsurface == EGL_NO_SURFACE)
        egl_error("eglCreateWindowSurface");

    eglcontxt = eglCreateContext(egldpy, configs[0], EGL_NO_CONTEXT,
                contxt_attribs);
    if (eglcontxt == EGL_NO_CONTEXT)
        egl_error("eglCreateContext");

    if ((egl_err = eglMakeCurrent(egldpy, eglsurface, eglsurface,
            eglcontxt)) != EGL_TRUE)
        egl_error("eglMakeCurrent");

	printf("swap interval = 0\n");
	eglSwapInterval(egldpy, (EGLint)0);

    return 1;
}

void deinit_display(void)
{
	free(pptex_objs);
	/* unbind the context with the current thread */
	eglMakeCurrent(egldpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);

	/* destroy context and surface */
	eglDestroyContext(egldpy, eglcontxt);
	eglDestroySurface(egldpy, eglsurface);
	eglTerminate(egldpy);
}

static void egl_error(char *error)
{
    int err = eglGetError();
    ERROR("%s: %s\n", error, egl_strerror(err));

}

const char *egl_strerror(int code)
{
    switch(code)
    {
    case EGL_SUCCESS: return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
    case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
    case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
    case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
    case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
    case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
    case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
    default: return "Unknown error";
    }
}

static const GLenum dev2texture[] = {
    GL_TEXTURE0,
    GL_TEXTURE1,
    GL_TEXTURE2,
    GL_TEXTURE3
};

static void deinit_glstate(void)
{
    ERROR("Flushing the GL state\n");

    /* un bind the context */
    eglMakeCurrent(egldpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    /* destroy surfaces */
    eglDestroyContext(egldpy, eglcontxt);
    eglDestroySurface(egldpy, eglsurface);
    eglTerminate(egldpy);
}

int v4l2_wait(int fd)
{
	int request = V4L2_GFX_IOC_CONSUMER;
	int rv;
	struct v4l2_gfx_consumer_params p;
	p.type = V4L2_GFX_CONSUMER_WAITSTREAM;
	p.timeout_ms = 0;
	p.acquire_timeout_ms = 2000;
	rv = ioctl(fd, request, &p);
	if (rv == 0) {
		INFO("stream ready\n");
	} else {
		/*
		 * Seems to be able to occur when this client generates a signal
		 * which interrupts the ioctl
		 */
		INFO("Could not get stream rv:%d errno:%d", rv, errno);
	}
	return rv;
}

//int acquire_v4l2_frame(int fd, int *bufid)
int acquire_v4l2_frame(int fd, struct v4l2_gfx_buf_params* p)
{
	int request = V4L2_GFX_IOC_ACQ;
	int rv;
	rv = ioctl(fd, request, p);
//	INFO("acquire_v4l2_frame = %d, bufid = %d", rv, p->bufid);
	return rv;
}

int release_v4l2_frame(int fd, int bufid)
{
	int request = V4L2_GFX_IOC_REL;
	int rv;
	struct v4l2_gfx_buf_params parms;

//	INFO("release %d", bufid);
	parms.bufid = bufid;
	rv = ioctl(fd, request, &parms);
	return rv;
}

int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglwindow)
{
	int dev_fd;
	int num_devices;
	int rv=0;
	int glesversion;

	glesversion = 1;

	INFO("======================= --- --- ======================");
	INFO("======================- STARTED -=====================");
	INFO("======================= ------- ======================");

	pptex_objs = malloc(sizeof(int*)*num_devices);
	if (!pptex_objs) {
		ERROR("pptex_objs");
		rv = ENOMEM;
		goto end;
	}

	dev_fd = open("/dev/video100", O_RDWR);	// XXX cleaner interface needed..
	if (dev_fd < 0) {
		ERROR("Cannot open V4L2 device : %s", strerror(errno));
		rv = errno;
		goto end;
	}

	/* setup the GL state */
	init_display(eglDisplay, eglwindow, glesversion);

	if ((rv = init_glstate_simple(dev_fd)) != 0) {
		ERROR("init_glstate_simple failed\n");
		goto end;
	}

	INFO("gl state is initialized");

	glGetIntegerv(GL_TEXTURE_NUM_STREAM_DEVICES_IMG, &num_devices);
	if (num_devices == 0) {
		ERROR("No stream devices available\n");
		deinit_glstate();
		deinit_display();
		return 0;
	}
	INFO("%d stream devices are available\n", num_devices);

	g_num_devices = num_devices;

	stream_texture_simple(0, (void*)dev2texture);

	while (1 && handle_events() == 0) {
		rv = draw_frame_simple(dev_fd);
		if (rv != 0) {
			INFO("End of stream detected");
			break;
		}
	}

	deinit_display();
	deinit_glstate();
	free(pptex_objs);

	INFO("exiting\n");

end:

	INFO("======================= ------- ======================");
	INFO("=======================  ENDED  ======================");
	INFO("======================= --   -- ======================");

	return rv;
}

#ifdef SUPPORT_ANDROID_PLATFORM

static int          giQuit;
static unsigned int guiWidth;
static unsigned int guiHeight;

/*
 * This function is expected by the SGX DDK Test JNI wrapper for Android
 */
void ResizeWindow(unsigned int width, unsigned int height)
{
    guiWidth = width;
    guiHeight = height;
}

/*
 * This function is expected by the SGX DDK Test JNI wrapper for Android
 */
void RequestQuit(void)
{
    giQuit = 1;
}


int handle_events(void)
{
    if(giQuit)
    {
        giQuit = 0;
        return -1;
    }

	return 0;
}

/*****************************************************************************/
/* JNI Entry Points                                                          */
/*****************************************************************************/

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_init(
	JNIEnv *env, jobject __unref obj, jstring wrapLib)
{
	const char *str;
    jclass c;

	LOGI("init");

	c = (*env)->FindClass(env, "android/view/Surface");
	if(!c)
		return -EFAULT;

	gSurfaceFieldID = (*env)->GetFieldID(env, c, "mSurface", "I");
	if(!gSurfaceFieldID)
		return -EFAULT;

	str = (*env)->GetStringUTFChars(env, wrapLib, NULL);
	if(!str)
		return -EFAULT;

	(*env)->ReleaseStringUTFChars(env, wrapLib, str);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_eglMain(
	JNIEnv *env, jobject __unref obj, jobject eglNativeWindow)
{
	EGLNativeWindowType window = (EGLNativeWindowType)(
		(*env)->GetIntField(env, eglNativeWindow, gSurfaceFieldID) + 8);

	return EglMain(EGL_DEFAULT_DISPLAY, window);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_resizeWindow(
	JNIEnv __unref *env, jobject __unref obj, jint width, jint height)
{
	ResizeWindow((unsigned int)width, (unsigned int)height);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_requestQuit(
	JNIEnv __unref *env, jobject __unref obj)
{
	RequestQuit();
}

static int myjniRegisterNativeMethods(JNIEnv* env, const char* className,
										const JNINativeMethod* gMethods,
										int numMethods)
{
    jclass clazz;

    LOGV("Registering %s natives\n", className);
    clazz = (*env)->FindClass(env, className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}

static JNINativeMethod sMethods[] = {
         /* name, signature, funcPtr */

        {"init", "(Ljava/lang/String;)I", (void*)Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_init},
        {"eglMain", "(Ljava/lang/Object;)I", (void*)Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_eglMain},
        {"resizeWindow", "(II)V", (void*)Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_resizeWindow},
        {"requestQuit", "()V", (void*)Java_com_imgtec_powervr_ddk_Gles1TexturePlayer_requestQuit},
};

extern jint JNI_OnLoad(JavaVM* vm, void __unref *reserved) {
    JNIEnv* env = NULL;
    jint result = -1;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }

    myjniRegisterNativeMethods(env, "com/imgtec/powervr/ddk/Gles1TexturePlayer/Gles1TexturePlayer", sMethods, 4);
    return JNI_VERSION_1_4;
}

#else // SUPPORT_ANDROID_PLATFORM

int main(int argc, char *argv[])
{
	return EglMain(EGL_DEFAULT_DISPLAY, 0);
}

int handle_events(void)
{
	return 0;
}
#endif // SUPPORT_ANDROID_PLATFORM

