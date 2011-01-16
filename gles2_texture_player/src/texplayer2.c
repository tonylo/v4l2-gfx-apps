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
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#include <linux/videodev.h>

#if defined(SUPPORT_ANDROID_PLATFORM)
#include <private/ui/android_natives_priv.h>
#endif
#include <EGL/egl.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "texplayer.h"

#define __unref __attribute__((unused))

#if defined(SUPPORT_ANDROID_PLATFORM)
#include "eglhelper.h"

static jfieldID	gSurfaceFieldID;
#endif

/* EGL global variables */
EGLDisplay egldpy;
EGLSurface eglsurface;
EGLContext eglcontxt;
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
                                EGL_NONE
                               };
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

        eglsurface = eglCreateWindowSurface(egldpy, configs[0], eglwindow, NULL);
        if (eglsurface == EGL_NO_SURFACE)
                egl_error("eglCreateWindowSurface");

        eglcontxt = eglCreateContext(egldpy, configs[0], EGL_NO_CONTEXT,
                                     contxt_attribs);
        if (eglcontxt == EGL_NO_CONTEXT)
                egl_error("eglCreateContext");

        if ((egl_err = eglMakeCurrent(egldpy, eglsurface, eglsurface,
                                      eglcontxt)) != EGL_TRUE)
                egl_error("eglMakeCurrent");

        //printf("swap interval = 0\n");
        //eglSwapInterval(egldpy, (EGLint)0);

        return 1;
}

void deinit_display(void)
{
//XXX    /* unbind the context with the current thread */
//XXX    eglMakeCurrent(egldpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
//XXX                EGL_NO_CONTEXT);

//XXX    /* destroy context and surface */
//XXX    eglDestroyContext(egldpy, eglcontxt);
//XXX    eglDestroySurface(egldpy, eglsurface);
//XXX    eglTerminate(egldpy);
}

static void egl_error(char *error)
{
        int err = eglGetError();
        ERROR("%s: %s\n", error, egl_strerror(err));

}

const char *egl_strerror(int code)
{
        switch (code) {
        case EGL_SUCCESS:
                return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
                return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
                return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
                return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
                return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG:
                return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT:
                return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE:
                return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
                return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH:
                return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP:
                return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
                return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER:
                return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE:
                return "EGL_BAD_SURFACE";
        default:
                return "Unknown error";
        }
}

static void deinit_glstate(void)
{
        ERROR("Flushing the GL state\n");

        gl_deinit_state();

        /* un bind the context */
        eglMakeCurrent(egldpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        /* destroy surfaces */
        eglDestroyContext(egldpy, eglcontxt);
        eglDestroySurface(egldpy, eglsurface);
        eglTerminate(egldpy);
}

int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglwindow)
{
        int dev_fd;
        int num_devices = 1;
        int rv=0;
        int glesversion = 2;

        INFO("======================= --- --- ======================");
        INFO("======================- STARTED -=====================");
        INFO("======================= ------- ======================");

        dev_fd = v4l2_open();
        if (dev_fd < 0) {
                rv = errno;
                goto end;
        }

        /* setup the GL state */
        init_display(eglDisplay, eglwindow, glesversion);

        if ((rv = gl_init_state(dev_fd, egldpy, eglsurface, eglcontxt, num_devices)) != 0) {
                goto end;
        }

        INFO("gl state is initialized");

        glGetIntegerv(GL_TEXTURE_NUM_STREAM_DEVICES_IMG, &num_devices);
        if (num_devices == 0) {
                ERROR("No stream devices available\n");
                goto deinit;
        }
        INFO("%d stream devices are available\n", num_devices);

        g_num_devices = num_devices;

        rv = gl_stream_texture(0);
        if (rv != 0)
                goto deinit;

        while (1 && handle_events() == 0) {
                gl_draw_frame(dev_fd);
        }

deinit:
        deinit_glstate();
        deinit_display();

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
        if (giQuit) {
                giQuit = 0;
                return -1;
        }

        return 0;
}

/*****************************************************************************/
/* JNI Entry Points                                                          */
/*****************************************************************************/

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_init(
        JNIEnv *env, jobject __unref obj, jstring wrapLib)
{
        const char *str;
        jclass c;

        LOGI("init");

        c = (*env)->FindClass(env, "android/view/Surface");
        if (!c)
                return -EFAULT;

        gSurfaceFieldID = (*env)->GetFieldID(env, c, "mSurface", "I");
        if (!gSurfaceFieldID)
                return -EFAULT;

        str = (*env)->GetStringUTFChars(env, wrapLib, NULL);
        if (!str)
                return -EFAULT;

        (*env)->ReleaseStringUTFChars(env, wrapLib, str);
        return 0;
}

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_eglMain(
        JNIEnv *env, jobject __unref obj, jobject eglNativeWindow)
{
        EGLNativeWindowType window = (EGLNativeWindowType)(
                                             (*env)->GetIntField(env, eglNativeWindow, gSurfaceFieldID) + 8);

        return EglMain(EGL_DEFAULT_DISPLAY, window);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_resizeWindow(
        JNIEnv __unref *env, jobject __unref obj, jint width, jint height)
{
        ResizeWindow((unsigned int)width, (unsigned int)height);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_requestQuit(
        JNIEnv __unref *env, jobject __unref obj)
{
        RequestQuit();
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_setParams(
        JNIEnv __unref *env, jobject __unref obj, jint param, jint value)
{
        gl_set_app_params(param, value);
}


static int myjniRegisterNativeMethods(JNIEnv* env, const char* className,
                                      const JNINativeMethod* gMethods,
                                      int numMethods)
{
        jclass clazz;

        LOGE("Registering %s natives\n", className);
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

        {"init", "(Ljava/lang/String;)I", (void*)Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_init},
        {"eglMain", "(Ljava/lang/Object;)I", (void*)Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_eglMain},
        {"resizeWindow", "(II)V", (void*)Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_resizeWindow},
        {"requestQuit", "()V", (void*)Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_requestQuit},
        {"setParams", "(II)V", (void*)Java_com_imgtec_powervr_ddk_Gles2TexturePlayer_setParams},
};

extern jint JNI_OnLoad(JavaVM* vm, void __unref *reserved)
{
        JNIEnv* env = NULL;
        jint result = -1;

        if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
                return result;
        }

        myjniRegisterNativeMethods(env, "com/imgtec/powervr/ddk/Gles2TexturePlayer/Gles2TexturePlayer", sMethods, 5);
        return JNI_VERSION_1_4;
}

#else // SUPPORT_ANDROID_PLATFORM

static void usage(char *argv0)
{
        fprintf(stderr, "usage: %s [-o animation]\n", argv0);
        fprintf(stderr, "       Where animation is 0 (flat texture stream)\n");
        fprintf(stderr, "       Where animation is 1 (3D animated texture stream)\n");
}

int main(int argc, char *argv[])
{
        int anim_opt;
        int c;
        while ((c = getopt(argc, argv, "o:")) != EOF) {
                switch (c) {
                case 'o':
                        anim_opt = atoi(optarg);
                        gl_set_app_params(GLAPP_PARM_ANIMATED, anim_opt);
                        break;
                default:
                        usage(argv[0]);
                        break;
                }
        }
        if(optind < argc) {
                usage(argv[0]);
        }
        return EglMain(EGL_DEFAULT_DISPLAY, 0);
}

int handle_events(void)
{
        return 0;
}
#endif // SUPPORT_ANDROID_PLATFORM

