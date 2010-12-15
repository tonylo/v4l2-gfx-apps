/*!****************************************************************************
@File           texplayer_simple.c

@Title          Texture Player

@Author         Texas Instruments

@Date           2010/08/19

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    Render a simple texture streaming demo

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

#include "texplayer.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
#include <private/ui/android_natives_priv.h>
#endif
#include <EGL/egl.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
//#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include "texplayer.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
#include "eglhelper.h"
#endif

/* EGL global variables */
extern EGLDisplay egldpy;
extern EGLSurface eglsurface;

/* texture objects */
extern GLuint **pptex_objs;

//#define FILTERING_TYPE GL_LINEAR
#define FILTERING_TYPE GL_NEAREST

static GLfloat vertices[] = {0.0,0.0,  0.0,0.0,  0.0,0.0, 0.0,0.0};
static GLfloat texcoord[] = {0,1,  0,0,  1,1, 1,0};
static GLfloat texcoord_orig[] = {0,1,  0,0,  1,1, 1,0};

extern PFNGLTEXBINDSTREAMIMGPROC glTexBindStreamIMG;
extern PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC glGetTexAttrIMG;
extern PFNGLGETTEXSTREAMDEVICENAMEIMGPROC glGetTexDevIMG;

static int g_image_width;
static int g_image_height;

static void draw_setcrop(struct v4l2_gfx_buf_params *p, int iw, int ih)
{
	float top=p->crop_top, left=p->crop_left;
	float cwidth = p->crop_width, cheight = p->crop_height;
	//float width=2048, height=912;
	float width=iw, height=ih;

	if (top == 0 && left == 0 && cwidth == 0 && cheight == 0) {
		glTexCoordPointer (2, GL_FLOAT, 0, texcoord_orig);
		return;		// No cropping information
	}

	texcoord[0] = left / width;
	texcoord[1] = 1.0f - ((height - cheight - top) / height);
	texcoord[2] = left / width;
	texcoord[3] = (top / height);
	texcoord[4] = 1.0f - ((width - left - cwidth) / width);
	texcoord[5] = 1.0f - ((height - cheight - top) / height);
	texcoord[6] = 1.0f - ((width - left - cwidth) / width);
	texcoord[7] = (top / height);
	glTexCoordPointer (2, GL_FLOAT, 0, texcoord);
}


void stream_texture_simple(int dev, void* dev2texturearg)
{
//    const GLubyte *bc_dev_name;
    int no_buffers, width, height, format;
    int idx;
	//int doadjust = 1;
	GLenum *dev2texture = (GLenum*)dev2texturearg;

    INFO(">> stream_texture dev = %d", dev); // XXX

#if 0
    /* get the device id */
    bc_dev_name = glGetTexDevIMG(dev);
    if (!bc_dev_name)
        return -4;
#endif

    glGetTexAttrIMG(dev, GL_TEXTURE_STREAM_DEVICE_NUM_BUFFERS_IMG,
            &no_buffers);
    glGetTexAttrIMG(dev, GL_TEXTURE_STREAM_DEVICE_WIDTH_IMG, &width);
    glGetTexAttrIMG(dev, GL_TEXTURE_STREAM_DEVICE_HEIGHT_IMG, &height);
    glGetTexAttrIMG(dev, GL_TEXTURE_STREAM_DEVICE_FORMAT_IMG, &format);

	g_image_width = width;
	g_image_height = height;

	// scaling... 
	//width = 640; height = 480; doadjust = 0;
	//width = 864; height = 480; doadjust = 0;
	//width = 320; height = 240; doadjust = 0;

	vertices[0] = 0.0;
	vertices[1] = height;
	vertices[2] = 0.0;
	vertices[3] = 0.0;
	vertices[4] = width;
	vertices[5] = height;
	vertices[6] = width;
	vertices[7] = 0.0;

    INFO("\ndevice: %d num: %d, width: %d, height: %d, format: \
            0x%x\n", dev, no_buffers, width, height, format);

#if 0 // XXX switch to image width/height
	if (doadjust) {
		if (height < g_ortho_height) {
			INFO("Adjusting height from %d to %d\n", g_ortho_height, height);
			g_ortho_height = height;
		}
		if (width < g_ortho_width) {
			INFO("Adjusting width from %d to %d\n", g_ortho_width, width);
			g_ortho_width = width;
		}
	}
#endif

    /* allocate texture objects */
    pptex_objs[dev] = malloc(sizeof(GLuint)*no_buffers);
    glGenTextures(no_buffers, pptex_objs[dev]);

	INFO("%d", __LINE__);
    /* activate texture unit */
    glActiveTexture(dev2texture[dev]);

	INFO("%d", __LINE__);
    /* associate buffers to textures */
    for (idx = 0; idx < no_buffers; idx++)
    {
        glBindTexture(GL_TEXTURE_STREAM_IMG, pptex_objs[dev][idx]);

        /* specify filters */
        glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER,
						FILTERING_TYPE);
        glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER,
						FILTERING_TYPE);

        // assign the buffer
        glTexBindStreamIMG(dev, idx);
    }
    ERROR("texture binding is complete\n");
}

int init_glstate_simple(int fd)
{
    const GLubyte *glext;
	int rv;

    if (!(glext = glGetString(GL_EXTENSIONS))) {
		ERROR("Couldn't get GL EXTENSIONS");
        rv = ENOENT; goto end;
	}
	ERROR ("GL extension supported: %s\n", (char *)glext);

    if (!strstr((char *)glext, "GL_IMG_texture_stream")) {
		ERROR("Can't find GL_IMG_texture_stream");
        rv = ENOENT; goto end;
	}

    glTexBindStreamIMG = (PFNGLTEXBINDSTREAMIMGPROC)
						eglGetProcAddress("glTexBindStreamIMG");
    glGetTexAttrIMG = (PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC)
						eglGetProcAddress("glGetTexStreamDeviceAttributeivIMG");
    glGetTexDevIMG = (PFNGLGETTEXSTREAMDEVICENAMEIMGPROC)
						eglGetProcAddress("glGetTexStreamDeviceNameIMG");

	INFO(	"glTexBindStreamIMG: %s\n"
			"glGetTexAttrIMG: %s\n"
			"glGetTexDevIMG: %s\n",
			(char *)glTexBindStreamIMG,
			(char *)glGetTexAttrIMG,
			(char *)glGetTexDevIMG);

    if (!glTexBindStreamIMG || !glGetTexAttrIMG || !glGetTexDevIMG) {
		ERROR("Missing a required texture streaming API");
        rv = ENOENT; goto end;
	}

    glShadeModel (GL_SMOOTH);

    glEnableClientState (GL_VERTEX_ARRAY);

    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    glVertexPointer   (2, GL_FLOAT, 0, vertices);

    glTexCoordPointer (2, GL_FLOAT, 0, texcoord);

	INFO("Wait for MM device");
	rv = v4l2_wait(fd);

end:
    return rv;
}

int draw_frame_simple(int fd)
{
	int rv, bufid;
    struct v4l2_gfx_buf_params p;

	rv = acquire_v4l2_frame(fd, &p);
	if (rv != 0) {
		INFO("Condition on acquire rv:%d errno:%d", rv, errno);
		goto end;
	}
	bufid = p.bufid;

	glClearColor(0.0, 0.0, 1.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrthof(0, g_image_width, g_image_height, 0, 0, 1);

	/* This would scale the image if we needed to */
//    glScalef(1.8f, 1.8f, 1);
//    glScalef(0.5f, 0.5f, 1);

	draw_setcrop(&p, g_image_width, g_image_height);

	glEnable(GL_TEXTURE_STREAM_IMG);
	glBindTexture(GL_TEXTURE_STREAM_IMG,  pptex_objs[0][bufid]); /* dev 0 */

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_TEXTURE_STREAM_IMG);

	/* TODO something with error code */
	eglSwapBuffers(egldpy, eglsurface);

	rv = release_v4l2_frame(fd, bufid);
	if (rv != 0) {
		ERROR("error on release... rv:%d errno:%d", rv, errno);
		goto end;
	}
end:
	return rv;
}

