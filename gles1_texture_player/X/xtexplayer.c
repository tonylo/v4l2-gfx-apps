/**************************************************************************
 * Name         : xtexplayer.c
 *
 * Copyright    : 2006-2007 by Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any  human or computer language in any form
 *              : by any means, electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited,
 *              : Home Park Estate, Kings Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 * $Log: xtexplayer.c $
 **************************************************************************/
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "../src/texplayer.h"

#define	WINDOW_SIZE_PC	75
#define	MAX_SWAP_INTERVAL 3
#define	MIN(x,y)	((x) < (y) ? (x) : (y))

/* for unused parameters */
#define UNUSED(x) (void)(x)

static Window win;
static float view_rotx=5.0f;
static int paused = 0;
static int fullscreen = 0;
static unsigned int swapInterval =  0;
static const char defaultScreen[] = ":0";
static XRectangle winrect;
static XColor bgcolour;
static int mapped = 0;
static int aiX = 30;
static int aiY = 70;
static unsigned int auiW =  180;
static unsigned int auiH =  180;
static unsigned int auiMove = 0;
static unsigned int auiResize = 0;

/* Whether the below values have been set on the command line */
static int bExactPositionSpecified = 0;

static void setwinrect(Display *dpy, int init, int position)
{
        Screen *scr = DefaultScreenOfDisplay(dpy);

        if (fullscreen) {
                XMoveResizeWindow(dpy, win, 0, 0, WidthOfScreen(scr), HeightOfScreen(scr));

                if(init) {
                        winrect.x = 0;
                        winrect.y = 0;
                        winrect.width = WidthOfScreen(scr);
                        winrect.height = HeightOfScreen(scr);
                }
        } else {
#ifdef MAXSIZE
                int mindim = MIN(WidthOfScreen(scr), HeightOfScreen(scr));
                int size = (mindim * WINDOW_SIZE_PC ) / 100;
#else
                int size = HeightOfScreen(scr) / 2;
#endif
                if(position != -1) {
                        int posx = (position % 3) * WidthOfScreen(scr)/3;
                        int posy = (position / 3) * HeightOfScreen(scr)/4;

                        XMoveResizeWindow(dpy, win, posx, posy, WidthOfScreen(scr)/3, HeightOfScreen(scr)/4);
                } else {
                        XMoveResizeWindow(dpy, win,
                                          WidthOfScreen(scr) / 2 - size / 2,
                                          HeightOfScreen(scr) / 2 - size / 2,
                                          size, size);
                }
                if(init) {
                        winrect.x = WidthOfScreen(scr) / 2 - size / 2;
                        winrect.y = HeightOfScreen(scr) / 2 - size / 2;
                        winrect.width = size;
                        winrect.height = size;
                }
        }
}

static char * const error_strings [] = {
        "EGL_SUCCESS",
        "EGL_NOT_INITIALIZED",
        "EGL_BAD_ACCESS",
        "EGL_BAD_ALLOC",
        "EGL_BAD_ATTRIBUTE",
        "EGL_BAD_CONFIG",
        "EGL_BAD_CONTEXT",
        "EGL_BAD_CURRENT_SURFACE",
        "EGL_BAD_DISPLAY",
        "EGL_BAD_MATCH",
        "EGL_BAD_NATIVE_PIXMAP",
        "EGL_BAD_NATIVE_WINDOW",
        "EGL_BAD_PARAMETER",
        "EGL_BAD_SURFACE",
        "EGL_CONTEXT_LOST"
};

static void handle_egl_error (char *name)
{
        EGLint error_code = eglGetError ();
        printf("'%s' returned egl error '%s' (0x%x)\n",
               name, error_strings[error_code-EGL_SUCCESS], error_code);
        exit (1);
}

static void reshape (int w, int h)
{
        glViewport (0, 0, (GLsizei) w, (GLsizei) h);

        glScissor (0, 0, (GLsizei) w, (GLsizei) h);
}

static void
event_loop(Display *dpy, Window win, EGLDisplay display, EGLSurface surface, int frameStop, int dev_fd)
{
        int i = 0;

        while (1) {
                if(auiMove == 1) {
                        if(i == 1)
                                XMoveWindow(dpy, win, winrect.x, winrect.y + 100);
                        else if(i == 2)
                                XMoveWindow(dpy, win, winrect.x, winrect.y - 200);
                        else if(i == 3)
                                XMoveWindow(dpy, win, winrect.x + 100, winrect.y + 100);
                        else if(i == 4)
                                XMoveWindow(dpy, win, winrect.x -200, winrect.y);
                }
                if(auiMove == 2) {
                        if(i % 30 == 29) {
                                if(i % 600 < 150 || 450 <= i % 600) {
                                        XMoveWindow(dpy, win, winrect.x, winrect.y + 10);
                                } else {
                                        XMoveWindow(dpy, win, winrect.x, winrect.y - 10);
                                }
                        }
                }
                if(auiMove == 3) {
                        if(i % 30 == 29) {
                                if(i % 600 < 150 || 450 <= i % 600) {
                                        XMoveWindow(dpy, win, winrect.x + 10, winrect.y);
                                } else {
                                        XMoveWindow(dpy, win, winrect.x - 10, winrect.y);
                                }
                        }
                }
                if(auiMove == 4) {
                        if(i % 200 == 99) {
                                if(i % 6000 < 1500 || 4500 <= i % 6000) {
                                        XMoveWindow(dpy, win, winrect.x + 10, winrect.y + 20);
                                } else {
                                        XMoveWindow(dpy, win, winrect.x - 10, winrect.y - 20);
                                }
                        }
                }
                if(auiMove == 5) {
                        if(i % 200 == 99) {
                                if(i % 6000 < 1500 || 4500 <= i % 6000) {
                                        XMoveWindow(dpy, win, winrect.x + 20, winrect.y + 10);
                                } else {
                                        XMoveWindow(dpy, win, winrect.x - 20, winrect.y - 10);
                                }
                        }
                }
                if(auiResize == 1) {
                        if(i % 8 == 7) {
                                if(i % 120 < 30 || 90 <= i % 120) {
                                        XResizeWindow(dpy, win, winrect.width + 20, winrect.height);
                                } else {
                                        XResizeWindow(dpy, win, winrect.width - 20, winrect.height);
                                }
                        }
                }
                if(auiResize == 2) {
                        if(i % 8 == 7) {
                                if(i % 120 < 30 || 90 <= i % 120) {
                                        XResizeWindow(dpy, win, winrect.width, winrect.height + 20);
                                } else {
                                        XResizeWindow(dpy, win, winrect.width, winrect.height - 20);
                                }
                        }
                }
                if(auiResize == 3) {
                        if(i % 8 == 7) {
                                if(i % 120 < 30 || 90 <= i % 120) {
                                        XResizeWindow(dpy, win, winrect.width + 20, winrect.height + 20);
                                } else {
                                        XResizeWindow(dpy, win, winrect.width - 20, winrect.height - 20);
                                }
                        }
                }
                if(auiResize == 4) {
                        if(i % 300 == 299) {
                                printf("Window dimensions have changed to %d, %d\n", winrect.height, winrect.width);
                                XResizeWindow(dpy, win, winrect.height, winrect.width);
                        }
                }

                i++;

                while (XPending(dpy) > 0) {
                        XEvent event;

                        XNextEvent(dpy, &event);

                        switch (event.type) {
                        case Expose: {
                                /*
                                 * Record that we're now mapped.  Because we're
                                 * rendering every frame anyway, we don't bother
                                 * blitting to handle the exposure.
                                 */
                                mapped = 1;

                                break;
                        }
                        case ConfigureNotify: {
                                /*
                                 * If the window's width or height change then do a
                                 * reshape.  All changes of the window's size come
                                 * through this code-path, whether they were user
                                 * (e.g. window manager) or internally generated.
                                 */
                                if ((event.xconfigure.width != winrect.width) ||
                                    (event.xconfigure.height != winrect.height)) {
                                        reshape(event.xconfigure.width, event.xconfigure.height);
                                }

                                /*
                                 * Copy new window configuration in from event structure.
                                 */
                                winrect.width = event.xconfigure.width;
                                winrect.height = event.xconfigure.height;
                                winrect.x = event.xconfigure.x;
                                winrect.y = event.xconfigure.y;

                                break;
                        }
                        case ButtonMotionMask: {
                                printf("mouse!\n");

                                break;
                        }
                        case UnmapNotify: {
                                /*
                                 * Record that we're no longer mapped.  This stops
                                 * us rendering every frame.
                                 */
                                mapped = 0;

                                break;
                        }
                        case KeyPress: {
                                int code = XLookupKeysym(&event.xkey, 0);

                                switch (code) {
                                        /*
                                         * Key code events to move/resize the window.  We
                                         * simply use Xlib primitives to move/resize the
                                         * window and let our event loop catch up by
                                         * handling the ConfigureNotify events.  This gives
                                         * us a single, consistent way of dealing with all
                                         * window position/size related happenings, whether
                                         * they be caused by an external agent (user, WM) or
                                         * by ourselves.
                                         */
                                case XK_Up: {
                                        if (!fullscreen) {
                                                XMoveWindow(dpy, win, winrect.x, winrect.y - 3);
                                        }

                                        break;
                                }
                                case XK_Down: {
                                        if (!fullscreen) {
                                                XMoveWindow(dpy, win, winrect.x, winrect.y + 3);
                                        }

                                        break;
                                }
                                case XK_Left: {
                                        if (!fullscreen) {
                                                XMoveWindow(dpy, win, winrect.x - 3, winrect.y);
                                        }

                                        break;
                                }
                                case XK_Right: {
                                        if (!fullscreen) {
                                                XMoveWindow(dpy, win, winrect.x + 3, winrect.y);
                                        }

                                        break;
                                }
                                case XK_Page_Up: {
                                        if (!fullscreen) {
                                                XResizeWindow(dpy, win, winrect.width + 10, winrect.height + 10);
                                        }

                                        break;
                                }
                                case XK_Page_Down: {
                                        if (!fullscreen) {
                                                XResizeWindow(dpy, win, winrect.width - 10, winrect.height - 10);
                                        }

                                        break;
                                }
                                case 'r': {
                                        XRaiseWindow(dpy, win);

                                        break;
                                }
                                case 'l': {
                                        XLowerWindow(dpy, win);

                                        break;
                                }
                                case 'f': {
                                        fullscreen = 1 - fullscreen;

                                        setwinrect(dpy, 0, -1);

                                        break;
                                }
                                case 'i': {
                                        swapInterval++;
                                        if(swapInterval > MAX_SWAP_INTERVAL)
                                                swapInterval = 0;

                                        eglSwapInterval(display,swapInterval);
                                        break;
                                }
                                case 27: /* escape */
                                case 'q':
                                case 'Q': {
                                        return;
                                }

                                case 'a': {
                                        view_rotx += 1.0;

                                        break;
                                }
                                case 'z': {
                                        view_rotx -= 1.0;

                                        break;
                                }
                                case ' ': { /* space */
                                        paused = 1 - paused;

                                        break;
                                }

                                default: {
#ifdef XKB
                                        XkbStdBell(XtDisplay(dpy, win, 0, XkbBI_MinorError));
#else
                                        XBell(dpy, 0);
#endif
                                }
                                }
                        }
                        }
                }

                /*
                 * Only rendering if we're actually mapped onto the display.
                 * The window system module won't display anything anyway
                 * but doing this saves CPU time for other processes if
                 * that's important to anyone.
                 */
                if (mapped) {
                        gl_draw_frame(dev_fd);

                        eglSwapBuffers (display, surface);

                        if (--frameStop == 0) {
                                break;
                        }
                }
        }
}



/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window(Display *dpy,
            Screen *scr,
            EGLDisplay display,
            EGLConfig Config,
            const char *name,
            EGLSurface *winRet,
            EGLContext *ctxRet,
            int position)
{
        EGLSurface surface;
        EGLContext context;
        GLint max[2] = { 0, 0 };
        EGLBoolean eRetStatus;
        XWindowAttributes WinAttr;
        int XResult = BadImplementation;
        int blackColour = BlackPixel(dpy, DefaultScreen(dpy));

		UNUSED(scr);

        win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1,
                                  0, blackColour, blackColour);
        if(bExactPositionSpecified) {
                XMoveResizeWindow(dpy, win, aiX, aiY, auiW, auiH);
                winrect.x = aiX;
                winrect.y = aiY;
                winrect.width = auiW;
                winrect.height = auiH;
        } else {
                setwinrect(dpy, 1, position);
        }

        XSelectInput(dpy, win, KeyPressMask|ButtonMotionMask|ExposureMask|StructureNotifyMask);

        XResult = XGetWindowAttributes(dpy, win, &WinAttr);
        if(!XResult) {
                handle_egl_error ("eglCreateWindowSurface");
        }

        XMapWindow(dpy, win);
        {
                XSizeHints sizehints;
                sizehints.x = winrect.x;
                sizehints.y = winrect.y;
                sizehints.width  = winrect.width;
                sizehints.height = winrect.height;
                sizehints.flags = USSize | USPosition;
                XSetNormalHints(dpy, win, &sizehints);
                XSetStandardProperties(dpy, win, name, name,
                                       None, (char **)NULL, 0, &sizehints);
        }

        surface = eglCreateWindowSurface (display, Config, (NativeWindowType)win, NULL);
        if (surface == EGL_NO_SURFACE) {
                handle_egl_error ("eglCreateWindowSurface");
        }

        context = eglCreateContext (display, Config, EGL_NO_CONTEXT, NULL);
        if (context == EGL_NO_CONTEXT) {
                handle_egl_error ("eglCreateContext");
        }

        eRetStatus = eglMakeCurrent (display, surface, surface, context);
        if( eRetStatus != EGL_TRUE ) {
                handle_egl_error ("eglMakeCurrent");
        }

        /* Check for maximum size supported by the GL rasterizer */
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max);

        if (winrect.width > max[0] || winrect.height > max[1]) {
                printf("Error: Requested window size (%d/%d) larger than "
                       "maximum supported by GL engine (%d/%d).\n",
                       winrect.width, winrect.height, (int)max[0], (int)max[1]);
                exit(EXIT_FAILURE);
        }

        if(fullscreen) {
                eglSwapInterval(display, swapInterval);
        }

        *winRet = surface;
        *ctxRet = context;
}

static void usage(char *argv0)
{
        fprintf(stderr, "usage: %s [-d display] [-b colourname] [-f #num_frames] [-s #fullscreen] [-p position] [-x left] [-y top] [-w width] [-h height] [-m move] [-r resize] [-a rotate] [-i swap_interval(0-3)] \n", argv0);
        fprintf(stderr, "       Where position is between 0 to 11.\n");

        exit(1);
}

int main(int argc, char** argv)
{
        Screen *screen;
        Display* dpy;
        Visual *vis;
        char *displayname = (char *)defaultScreen;
        char *colourname = NULL;
        int c;
        int position = -1;
        XColor exact;
        EGLDisplay display;
        EGLSurface surface;
        EGLContext context;
        EGLConfig configs[2];
        EGLBoolean eRetStatus;
        EGLint config_count;
        EGLint major, minor;
        EGLint cfg_attribs[] = {
                EGL_NATIVE_VISUAL_TYPE, 0,
                EGL_BUFFER_SIZE,    EGL_DONT_CARE,
                EGL_RED_SIZE,       5,
                EGL_GREEN_SIZE,     6,
                EGL_BLUE_SIZE,      5,
                EGL_DEPTH_SIZE,     8,
                EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
                EGL_NONE
        };

        int frameStop = 0;
        int dev_fd;

        while ((c = getopt(argc, argv, "d:b:f:s:p:x:y:w:h:m:r:a:i:")) != EOF) {
                switch (c) {
                case 'b':
                        colourname = optarg;
                        break;
                case 'd':
                        displayname = optarg;
                        break;
                case 'f':
                        frameStop = atol(optarg);
                        break;
                case 's':
                        fullscreen = atol(optarg);
                        break;
                case 'p':
                        position = atol(optarg);
                        break;
                case 'x':
                        aiX = atol(optarg);
                        bExactPositionSpecified = 1;
                        break;
                case 'y':
                        aiY = atol(optarg);
                        bExactPositionSpecified = 1;
                        break;
                case 'i':
                        swapInterval = atol(optarg);
                        break;
                case 'w':
                        auiW = atol(optarg);
                        bExactPositionSpecified = 1;
                        break;
                case 'h':
                        auiH = atol(optarg);
                        bExactPositionSpecified = 1;
                        break;
                case 'm':
                        auiMove = atol(optarg);
                        break;
                case 'r':
                        auiResize = atol(optarg);
                        break;
                case 'a':
                        view_rotx = atol(optarg);
                        break;
                default:
                        usage(argv[0]);
                }
        }

        if(optind < argc) {
                usage(argv[0]);
        }

        /*
        * Initialise X related stuff.
        */
        {
                dpy = XOpenDisplay(displayname);

                if (!dpy) {
                        printf("Error: couldn't open display \n");
                        return EXIT_FAILURE;
                }

                screen = XDefaultScreenOfDisplay(dpy);

                bgcolour.red = 0;
                bgcolour.green = 0x8000;
                bgcolour.blue = 0;

                if (colourname &&
                    XLookupColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), colourname, &exact, &bgcolour) == 0) {
                        fprintf(stderr, "%s: can't find colour %s - using green.\n", argv[0], colourname);
                }

                vis = DefaultVisual(dpy, DefaultScreen(dpy));
        }

        printf ("--------------------- started ---------------------\n");

        display = eglGetDisplay ((NativeDisplayType)dpy);

        eRetStatus = eglInitialize(display, &major, &minor);
        if( eRetStatus != EGL_TRUE )
                handle_egl_error("eglInitialize");

        cfg_attribs[1] = (EGLint)vis;

        eRetStatus = eglChooseConfig (display, cfg_attribs, configs, 1, &config_count);
        if( eRetStatus != EGL_TRUE || !config_count)
                handle_egl_error ("eglChooseConfig");

        make_window(dpy, screen, display, configs[0], "xgles1_texture_player", &surface, &context, position);

        dev_fd = v4l2_open();
        if (dev_fd < 0)
                goto term;

        if(gl_init_state(dev_fd, display, surface, context, 1) != 0)
                goto term;

        if (gl_stream_texture(0) != 0)
                goto term;

        event_loop(dpy, win, display, surface, frameStop, dev_fd);

term:
        eglDestroyContext (display, context);
        eglDestroySurface (display, surface);
        eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate (display);

        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);

        fprintf (stdout, "--------------------- finished ---------------------\n");
        return 0;
}

