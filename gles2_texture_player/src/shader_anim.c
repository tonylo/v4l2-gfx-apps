/*!****************************************************************************
@File           shader_anim.c

@Author         Texas Instruments

@Date           2011/01/11

@Copyright      Copyright (C) 2011 Texas Instruments.

@Platform       Android/Linux

@Description    Vertex shader program for animated render

******************************************************************************/

#include <GLES2/gl2.h>

GLfloat g_vertices_anim[16][3] = {
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

const char *g_vshader_anim =
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

