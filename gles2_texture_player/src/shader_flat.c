/*!****************************************************************************
@File           shader_flat.c

@Author         Texas Instruments

@Date           2011/01/11

@Copyright      Copyright (C) 2011 Texas Instruments.

@Platform       Android/Linux

@Description    Vertex shader program for flat render

******************************************************************************/

#include <GLES2/gl2.h>

GLfloat g_vertices_flat[16][3] = {
        /* x     y     z */
        {-1.0,  1.0,  0.0},
        { 1.0,  1.0,  0.0},
        {-1.0, -1.0,  0.0},
        { 1.0, -1.0,  0.0},
};

const char *g_vshader_flat =
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
        "   gl_Position = vertex;\n"
        "   v_color = color;\n"
        "   texcoord = inputtexcoord;\n"
        "}";

