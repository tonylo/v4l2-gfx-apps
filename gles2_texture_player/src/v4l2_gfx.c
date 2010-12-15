
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stropts.h>

#include <linux/omap_v4l2_gfx.h>
#include "v4l2_gfx.h"
#include "misc.h"

int v4l2_open(void)
{
        int dev_fd;
        dev_fd = open("/dev/video100", O_RDWR);
        if (dev_fd < 0) {
                ERROR("Cannot open V4L2 device : %s", strerror(errno));
        }
        return dev_fd;
}

int v4l2_wait(int fd)
{
        int request = V4L2_GFX_IOC_CONSUMER;
        int rv;
        struct v4l2_gfx_consumer_params p;
        p.type = V4L2_GFX_CONSUMER_WAITSTREAM;
        p.timeout_ms = 0;
        p.acquire_timeout_ms = 0;
        rv = ioctl(fd, request, &p);
        if (rv == 0) {
                INFO("consumer woke\n");
        }
        return rv;
}

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
