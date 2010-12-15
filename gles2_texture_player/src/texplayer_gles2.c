/*!****************************************************************************
@File           texplayer_gles2.c

@Title          Texture Player

@Author         Texas Instruments

@Date           2010/08/19

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    Render an animated gles2 texture streaming demo

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
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if defined(SUPPORT_ANDROID_PLATFORM)
#include "eglhelper.h"
#endif

#include "texplayer.h"
/* EGL global variables */
EGLDisplay egldpy;
EGLSurface eglsurface;
EGLContext eglcontxt;

PFNGLTEXBINDSTREAMIMGPROC glTexBindStreamIMG;
PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC glGetTexAttrIMG;
PFNGLGETTEXSTREAMDEVICENAMEIMGPROC glGetTexDevIMG;

/* texture objects */
GLuint **pptex_objs;

/* Shader program */
static int program[2];

GLint model_view_idx[2], proj_idx[2];
float projection[16] = {
        4.0f,     0.0f,     0.0f,     0.0f,
        0.0f,     4.0f,     0.0f,     0.0f,
        0.0f,     0.0f,     -1.0f,     -1.0f,
        0.0f,     0.0f,     -1.0f,     0.0f
};
float modelview[16] = {
        1.0f,     0.0f,     0.0f,     0.0f,
        0.0f,     1.0f,     0.0f,     0.0f,
        0.0f,     0.0f,     1.0f,     0.0f,
        0.0f,     0.0f,    -9.0f,     1.0f
};

static GLfloat vertices[16][3] = {
        // x     y     z          id
        {-1.0, -1.0,  1.0}, // 1  0 left    First Strip
        {-1.0,  1.0,  1.0}, // 3  1
        {-1.0, -1.0, -1.0}, // 0  2
        {-1.0,  1.0, -1.0}, // 2  3
        { 1.0, -1.0, -1.0}, // 4  4  back
        { 1.0,  1.0, -1.0}, // 6  5
        { 1.0, -1.0,  1.0}, // 5  6 right
        { 1.0,  1.0,  1.0}, // 7  7

        { 1.0,  1.0, -1.0}, // 6  8 top     Second Strip
        {-1.0,  1.0, -1.0}, // 2  9
        { 1.0,  1.0,  1.0}, // 7  10
        {-1.0,  1.0,  1.0}, // 3  11
        { 1.0, -1.0,  1.0}, // 5  12 front
        {-1.0, -1.0,  1.0}, // 1  13
        { 1.0, -1.0, -1.0}, // 4  14 bottom
        {-1.0, -1.0, -1.0}  // 0  15
};
#define VER_POS_SIZE 3     /* x, y and z */

static GLfloat Normals[16][3] = {  // One normal per vertex.
        // x     y     z
        {-0.5, -0.5,  0.5}, // 1  left          First Strip
        {-0.5,  0.5,  0.5}, // 3
        {-0.5, -0.5, -0.5}, // 0
        {-0.5,  0.5, -0.5}, // 2
        { 0.5, -0.5, -0.5}, // 4  back
        { 0.5,  0.5, -0.5}, // 6
        { 0.5, -0.5,  0.5}, // 5  right
        { 0.5,  0.5,  0.5}, // 7

        { 0.5,  0.5, -0.5}, // 6  top           Second Strip
        {-0.5,  0.5, -0.5}, // 2
        { 0.5,  0.5,  0.5}, // 7
        {-0.5,  0.5,  0.5}, // 3
        { 0.5, -0.5,  0.5}, // 5  front
        {-0.5, -0.5,  0.5}, // 1
        { 0.5, -0.5, -0.5}, // 4  bottom
        {-0.5, -0.5, -0.5}  // 0
};

#define VER_NOR_SIZE 3     /* x, y and z */

#if 0 /* original */
static GLfloat texcoord[16][2] = {
        // x   y
        {0.0, 0.0}, // 1  left                  First Strip
        {1.0, 0.0}, // 3
        {0.0, 1.0}, // 0
        {1.0, 1.0}, // 2
        {1.0, 0.0}, // 4  back
        {1.0, 1.0}, // 6
        {0.0, 0.0}, // 5  right
        {0.0, 1.0}, // 7
        {1.0, 0.0}, // 1  top                   Second Strip
        {1.0, 1.0}, // 3
        {0.0, 0.0}, // 0
        {0.0, 1.0}, // 2
        {0.0, 1.0}, // 4  front
        {0.0, 0.0}, // 6
        {1.0, 1.0}, // 5  bottom
        {1.0, 0.0}  // 7
};
#endif
static GLfloat texcoord[16][2] = {
        // x   y
        {0.1, 0.1}, // 1  left                  First Strip
        {0.9, 0.1}, // 3
        {0.1, 0.9}, // 0
        {0.9, 0.9}, // 2
        {0.9, 0.1}, // 4  back
        {1.0, 1.0}, // 6
        {0.0, 0.0}, // 5  right
        {0.0, 1.0}, // 7
        {1.0, 0.0}, // 1  top                   Second Strip
        {1.0, 1.0}, // 3
        {0.0, 0.0}, // 0
        {0.0, 1.0}, // 2
        {0.0, 1.0}, // 4  front
        {0.0, 0.0}, // 6
        {1.0, 1.0}, // 5  bottom
        {1.0, 0.0}  // 7
};
#define TEX_COORD_SIZE 2     /* s and t */

/* Shader programs */
/* Vertex shader */
const char *vshader_src =
        "uniform mat4 modelview;\n"
        "uniform mat4 projection;\n"
        "attribute vec4 vertex;\n"
        "attribute vec4 color;\n"
        "attribute vec3 normal;\n"
        "varying mediump vec4 v_color;\n"
        "attribute vec2 inputtexcoord;\n"
        "varying mediump vec2 texcoord;\n"
        "void main()\n"
        "{\n"
        "   vec4 eye_vertex = modelview*vertex;\n"
        "   gl_Position = projection*eye_vertex;\n"
        "   v_color = color;\n"
        "   texcoord = inputtexcoord;\n"
        "}";

/* Fragment shader */
const char *fshader_2 =
        "#ifdef GL_IMG_texture_stream2\n"
        "#extension GL_IMG_texture_stream2 : enable \n"
        " #endif \n"
        "varying mediump vec2 texcoord;\n"
        "uniform samplerStreamIMG streamtexture;\n"
        "varying mediump vec4 v_color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = textureStreamIMG(streamtexture, texcoord);\n"
//  "    gl_FragColor = v_color;\n"
        "}";
const char *fshader_1 =
        "void main(void)\n"
        "{\n"
        "   gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
        "}";
const char *fshader_3 =
        "varying mediump vec2 texcoord;\n"
        "uniform sampler2D basetexture;\n"
        "void main(void)\n"
        "{\n"
        "   gl_FragColor = texture2D(basetexture, texcoord);\n"
        "}";

/* shader object handles */
static int ver_shader, frag_shader[2];

static const GLenum dev2texture[] = {
        GL_TEXTURE0,
        GL_TEXTURE1,
        GL_TEXTURE2,
        GL_TEXTURE3
};
static int dev2texturecnt = sizeof(dev2texture)/sizeof(GLenum);

int gl_stream_texture(int dev)
{
//    const GLubyte *bc_dev_name;
        int no_buffers, width, height, format;
        int idx;
        INFO(">> stream_texture dev = %d", dev); // XXX
        if (dev >= dev2texturecnt) {
                ERROR("Invalid device number");
                return -1;
        }

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

        INFO("\ndevice: %d num: %d, width: %d, height: %d, format: \
            0x%x\n", dev, no_buffers, width, height, format);

        /* allocate texture objects */
        pptex_objs[dev] = malloc(sizeof(GLuint)*no_buffers);
        glGenTextures(no_buffers, pptex_objs[dev]);

        /* activate texture unit */
        glActiveTexture(dev2texture[dev]);

        /* associate buffers to textures */
        for (idx = 0; idx < no_buffers; idx++) {
                glBindTexture(GL_TEXTURE_STREAM_IMG, pptex_objs[dev][idx]);

                /* specify filters */
                glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR /*GL_NEAREST*/);
                glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR /*GL_NEAREST*/);

                // assign the buffer
                glTexBindStreamIMG(dev, idx);
        }
        INFO("texture binding is complete\n");
        return 0;
}

int gl_init_state(int fd, EGLDisplay a_egldpy, EGLSurface a_eglsurface, EGLContext a_eglcontxt, int num_devices)
{
        int rv;
        const GLubyte *glext;
        int shader_status;
        int i;

        egldpy = a_egldpy;
        eglsurface = a_eglsurface;
        eglcontxt = a_eglcontxt;

        INFO("gl_init_state\n");
        pptex_objs = malloc(sizeof(int*)*num_devices);
        if (!pptex_objs) {
                ERROR("pptex_objs");
                rv = ENOMEM;
                return -1;
        }

        if (!(glext = glGetString(GL_EXTENSIONS))) {
                ERROR("GL_EXTENSIONS fails %x\n", (int)glext);
                return -1;
        }
        INFO ("GL extension supported: %s\n", (char *)glext);

        if (!strstr((char *)glext, "GL_IMG_texture_stream2")) {
                ERROR ("GL_IMG_texture_stream2 : %s\n", (char*)glext);
                return -2;
        }

        glTexBindStreamIMG = (PFNGLTEXBINDSTREAMIMGPROC)eglGetProcAddress("glTexBindStreamIMG");
        glGetTexAttrIMG = (PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC)eglGetProcAddress
                          ("glGetTexStreamDeviceAttributeivIMG");
        glGetTexDevIMG = (PFNGLGETTEXSTREAMDEVICENAMEIMGPROC)eglGetProcAddress
                         ("glGetTexStreamDeviceNameIMG");

        INFO("glTexBindStreamIMG: %p\n"
             "glGetTexAttrIMG: %p\n"
             "glGetTexDevIMG: %p\n",
             glTexBindStreamIMG, glGetTexAttrIMG, glGetTexDevIMG);
        if (!glTexBindStreamIMG || !glGetTexAttrIMG || !glGetTexDevIMG) {
                ERROR("Could not access the Texture Streaming APIs");
                return -3;
        }


        /* Initialize shaders */
        ver_shader = glCreateShader(GL_VERTEX_SHADER);
        frag_shader[0] = glCreateShader(GL_FRAGMENT_SHADER);
        frag_shader[1] = glCreateShader(GL_FRAGMENT_SHADER);

        ERROR("created shaders\n");

        /* Attach and compile shaders */
        /* shader source is null terminated */
        glShaderSource(ver_shader, 1, (const char **)&vshader_src, NULL);

        ERROR("shader source attached\n");

        glCompileShader(ver_shader);

        /* compile status check */
        glGetShaderiv(ver_shader, GL_COMPILE_STATUS, &shader_status);
        if (shader_status != GL_TRUE) {
                char buf[1024];
                glGetShaderInfoLog(ver_shader, sizeof(buf), NULL, buf);
                ERROR("Vertex shader compilation failed, info log:\n%s\n", buf);
                return 1;
        }

        ERROR("vertex shader compiled\n");

        for (i = 0; i < 2; i++) {
                if (i == 0)
                        glShaderSource(frag_shader[i], 1, (const char **)&fshader_3, NULL);
                else
                        glShaderSource(frag_shader[i], 1, (const char **)&fshader_2, NULL);

                ERROR("frag source attached \n");
                glCompileShader(frag_shader[i]);

                /* compile status check */
                glGetShaderiv(frag_shader[i], GL_COMPILE_STATUS, &shader_status);
                if (shader_status != GL_TRUE) {
                        char buf[1024];
                        glGetShaderInfoLog(frag_shader[i], sizeof(buf), NULL, buf);
                        ERROR("Fragment shader compilation failed, info log:\n%s\n", buf);
                        return -1;
                }

                program[i] = glCreateProgram();

                /* Attach shader to the program */
                glAttachShader(program[i], ver_shader);
                glAttachShader(program[i], frag_shader[i]);

                /* associating the program attributes */
                glBindAttribLocation(program[i], 0, "vertex");
                glBindAttribLocation(program[i], 1, "normal");
                glBindAttribLocation(program[i], 2, "inputtexcoord");

                /* link the program */
                glLinkProgram(program[i]);

                glGetProgramiv(program[i], GL_LINK_STATUS, &shader_status);
                if (shader_status != GL_TRUE) {
                        char buf[1024];
                        glGetProgramInfoLog(program[i], sizeof(buf), NULL, buf);
                        ERROR("Program linking failed, info log:\n%s\n", buf);
                        return -1;
                }

                glValidateProgram(program[i]);

                glGetProgramiv(program[i], GL_VALIDATE_STATUS, &shader_status);
                if (shader_status != GL_TRUE) {
                        char buf[1024];
                        glGetProgramInfoLog(program[i], sizeof(buf), NULL, buf);
                        ERROR("Program validation failed, info log:\n%s\n", buf);
                        return -1;
                }

                /* pass the model view and projection matrix to shader */
                model_view_idx[i] = glGetUniformLocation(program[i], "modelview");
                proj_idx[i] = glGetUniformLocation(program[i], "projection");

                ERROR("vertex shader setting complete\n");

                if (i == 0) {
                        int sampler2d;
                        sampler2d = glGetUniformLocation(program[0], "basetexture");

                        glUseProgram(program[0]);
                        glUniform1i(sampler2d, 7);
                }
                ERROR("fragment shader setting is complete\n");
        }

        /* assign the vertices and their texture co-ords */

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, VER_POS_SIZE, GL_FLOAT,
                              GL_FALSE, 0, vertices);
        glVertexAttribPointer(1, VER_NOR_SIZE, GL_FLOAT,
                              GL_FALSE, 0, Normals);
        glVertexAttribPointer(2, TEX_COORD_SIZE, GL_FLOAT,
                              GL_FALSE, 0, texcoord);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        /* texture binding and shader setup complete */
        /* everything good :)*/

        INFO("Wait for MM device");
        rv = v4l2_wait(fd);

        return rv;
}

void gl_deinit_state(void)
{
        ERROR("Flushing the GL state\n");

        /* clean up shaders */
        glDeleteProgram(program[0]);
        glDeleteProgram(program[1]);
        glDeleteShader(ver_shader);
        glDeleteShader(frag_shader[0]);
        glDeleteShader(frag_shader[1]);
        free(pptex_objs);
}

int gl_draw_frame(int fd)
{
        static float rot_x = 0.0;
        static float rot_y = 0.0;
        float sx, cx, sy, cy;
        int tex_sampler, i=0;
        int rv=0, bufid;
        struct v4l2_gfx_buf_params p;

        /* rotate the cube */
        sx = (float)sin(rot_x);
        cx = (float)cos(rot_x);
        sy = (float)sin(rot_y);
        cy = (float)cos(rot_y);

        modelview[0] = cy;
        modelview[1] = 0;
        modelview[2] = -sy;
        modelview[4] = sy*sy;
        modelview[5] = cx;
        modelview[6] = cy*sx;
        modelview[8] = sy*cx;
        modelview[9] = -sx;
        modelview[10] = cx*cy;

//	glClearColor(0.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(program[0]);

        glUniformMatrix4fv(model_view_idx[0], 1, GL_FALSE, modelview);
        glUniformMatrix4fv(proj_idx[0], 1, GL_FALSE, projection);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        rv = acquire_v4l2_frame(fd, &p);
        bufid = p.bufid;
        if (rv != 0) {
                INFO("Condition on acquire rv:%d errno:%d", rv, errno);
                if (errno != ENODEV)
                        rv = 0;
                goto end;
        }

        glBindTexture(GL_TEXTURE_STREAM_IMG,
                      pptex_objs[i][bufid]);    // first buffer

        /* associate the stream texture */
        tex_sampler = glGetUniformLocation(program[1], "streamtexture");

        glUseProgram(program[1]);

        /* associate the sampler to a texture unit */
        glUniform1i(tex_sampler, i);

        glUniformMatrix4fv(model_view_idx[1], 1, GL_FALSE, modelview);
        glUniformMatrix4fv(proj_idx[1], 1, GL_FALSE, projection);

        glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);
        eglSwapBuffers(egldpy, eglsurface);

        rv = release_v4l2_frame(fd, bufid);
        if (rv != 0) {
                ERROR("error on release... rv:%d errno:%d", rv, errno);
                goto end;
        }

        /* TODO something with error code */

        rot_x += 0.01;
        rot_y += 0.01;

//	INFO("Sleeping");
//	usleep(100000);
end:
        return rv;
}


