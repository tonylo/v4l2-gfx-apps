/*!****************************************************************************
@File           v4l2_gfx.h

@Title          Local v4l2 help functions

@Author         Texas Instruments

@Date           2010/12/15

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description

******************************************************************************/

#ifndef __V4L2_GFX_LCL_H__
#define __V4L2_GFX_LCL_H__

#include <linux/omap_v4l2_gfx.h>

extern int v4l2_open(void);
extern int v4l2_wait(int fd);
extern int acquire_v4l2_frame(int fd, struct v4l2_gfx_buf_params* p);
extern int release_v4l2_frame(int fd, int bufid);

#endif /* __V4L2_GFX_LCL_H__ */
