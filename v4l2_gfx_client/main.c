/*!****************************************************************************
@File           main.c

@Title          V4L2 GFX client (test)

@Author         Texas Instruments

@Date           2010/07/29

@Copyright      Copyright (C) 2010 Texas Instruments.

				This file is licensed under the terms of the GNU General
				Public License version 2. This program is licensed "as is"
				without any warranty of any kind, whether express or implied.

@Platform       Android/Linux

@Description    Client of V4L2 APIs for the V4L2-GFX driver. Normally the user
				of these APIs would be some multimedia decoder.

				This program acts as a series of unit tests for the V4L2-GFX
				driver.

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/ioctl.h>

#include <linux/videodev.h>
#include <linux/omap_v4l2_gfx.h>

#include "testfmwk.h"

#define PARAM_FIXED_NAME 1
static int param_opts;
static char param_fixed_name[15];	/* WWWWxHHHH.nv12 */
static char* param_filename;
static int param_width;
static int param_height;
#if defined(SUPPORT_ANDROID_PLATFORM)
static char *fixed_name_path = "/system/bin/";
#else
static char *fixed_name_path = "/";
#endif

struct test_v4l2_session {
	int fd;
	int bcount;
	struct v4l2_pix_format pix;
	char **bufs;
	int *lens;
};

const char *g_devname = "/dev/video100";

static void tutil_report_results(void)
{
	int tests, passed, failed;
	TEST_CASEDATA(&tests, &passed, &failed);
	printf(" ========== ========== ========== ========== \n"); 
	printf(" Test results\n");
	printf(" ========== ========== ========== ========== \n"); 
	printf(" NUMBER OF TESTS     = %d\n", tests);
	printf(" PASSED              = %d\n", passed);
	printf(" FAILED              = %d\n", failed);
	printf(" ========== ========== ========== ========== \n"); 
}

static void tutil_print_fourcc(char *prefix, __u32 fourcc)
{
	char ca[4];
	*((__u32*)ca) = fourcc;
	TEST_PRINT("%s 0x%x \"%c%c%c%c\"",
			   prefix, fourcc, ca[0], ca[1], ca[2], ca[3]);
}

static void tutil_print_fourcc2(char *prefix, char *txt, __u32 fourcc)
{
	char ca[4];
	*((__u32*)ca) = fourcc;
	TEST_PRINT("%s %s0x%x \"%c%c%c%c\"",
			   prefix, txt, fourcc, ca[0], ca[1], ca[2], ca[3]);
}

static void tutil_print_v4l2_pix_format(
									char *prefix, struct v4l2_pix_format* pix)
{
	TEST_PRINT("%s pix width = %d", prefix, pix->width);
	TEST_PRINT("%s pix height = %d", prefix, pix->height);
	TEST_PRINT("%s pix bytesperline = %d", prefix, pix->bytesperline); 
	tutil_print_fourcc2(prefix, "pix pixelformat =", pix->pixelformat);
	TEST_PRINT("%s pix sizeimage = %d", prefix, pix->sizeimage);
	TEST_PRINT("%s pix colorspace = 0x%x", prefix, pix->colorspace);
	TEST_PRINT("%s pix priv = 0x%x", prefix, pix->priv); 
}

static void tutil_print_v4l2_buffer(char *prefix, struct v4l2_buffer* vbuf)
{
	TEST_PRINT("%s index = %d", prefix, vbuf->index);
	TEST_PRINT("%s type = %d", prefix, vbuf->type);
	TEST_PRINT("%s bytesused = %d", prefix, vbuf->bytesused);
	TEST_PRINT("%s flags = 0x%x", prefix, vbuf->flags);
	TEST_PRINT("%s sequence = 0x%x", prefix, vbuf->sequence);
	TEST_PRINT("%s memory = 0x%d", prefix, vbuf->memory);
	TEST_PRINT("%s m.offset = 0x%d", prefix, vbuf->m.offset);
	TEST_PRINT("%s m.userptr = 0x%ld", prefix, vbuf->m.userptr);
	TEST_PRINT("%s length = 0x%d", prefix, vbuf->length);
}

/* From:
   http://www.delorie.com/gnu/docs/glibc/libc_428.html
 */
static int timeval_subtract(struct timeval *result, struct timeval *x,
							struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}


static int tutil_driver_open(char* subtst_str)
{
	int fd;
	TEST_INFO(subtst_str);
	fd = open(g_devname, O_RDWR);
	TEST_RESULT(fd);
	TEST_ASSERT(fd >= 0);
	return fd;
}

static void tutil_driver_close(char* subtst_str, int fd)
{
	int rv;
	TEST_INFO(subtst_str);
	rv = close(fd);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);
}

static int tutil_vidioc_querybuf(int fd, int index, struct v4l2_buffer* vbp)
{
	int request = VIDIOC_QUERYBUF;
	int rv;
	TEST_INFO2("VIDIOC_QUERYBUF ioctl", index);
	vbp->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	vbp->index = index;
	rv = ioctl(fd, request, vbp);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);
	return rv;
}

static void tutil_vidioc_try_fmt(int fd,  struct v4l2_format *fmt)
{
	int request = VIDIOC_TRY_FMT;
	int rv;

	TEST_INFO("Issue VIDIOC_TRY_FMT ioctl");
	rv = ioctl(fd, request, fmt);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	TEST_PRINT("Dumping rcvd v4l2_pix_format");
	tutil_print_v4l2_pix_format("VIDIOC_TRY_FMT fmt", &fmt->fmt.pix);

}

/* Quick and messy function to open a video */
static void *tutil_open_video(
				unsigned long *framecnt, int height, int width,
				const char* vid_name)
{
	int fd;
	unsigned char *yuv_buf;
	off64_t vidsz;
	int framesz;
#if 0
	char vid_name[15];	/* WWWWxHHHH.nv12 */
	char wt[5], ht[5];
	snprintf(wt, 5, "%d", width);
	snprintf(ht, 5, "%d", height);
	snprintf(vid_name, 15, "%sx%s.nv12", wt, ht);
#endif
	printf("Expecting to find file '%s'\n", vid_name);

	framesz = (width * height) * 3 / 2; /* NV12 */

	fd = open(vid_name, O_RDWR|O_LARGEFILE);
	if (fd < 0) {
		printf("ERROR: could not open YUV file\n");
		exit(-1);
	}

	vidsz = lseek64(fd, 0, SEEK_END);
	if (vidsz <= 0) {
		printf("ERROR: bad sized file: %llu\n", vidsz);
		exit(-1);
	}

	printf("File bytes %llu\n", vidsz);
	lseek(fd, 0, SEEK_SET);

	yuv_buf = mmap(NULL, vidsz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (yuv_buf == MAP_FAILED) {
		int maxframes = 1073741824 / framesz;

		printf("First mmap failed, try to map 1GB of the file\n");
		vidsz = maxframes * framesz;
		yuv_buf = mmap(NULL, vidsz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (yuv_buf == MAP_FAILED) {
			printf("ERROR: mmap %s\n", vid_name);
			exit(-1);
		}
	}

	*framecnt = vidsz / framesz;
	printf("frame count = %ld\n", *framecnt);

	return ((void *)yuv_buf);
}

static char* tutil_copy_nv12frame_to_tiler(char *dest, char *src, int h, int w)
{
	int i;
	for (i=0; i<h; i++) {
//		printf("+%x %x\n", dest, src);
		memcpy(dest, src, w);
		dest+=4096;
		src+=w;
	}
	for (i=0; i< (h / 2); i++) {
//		printf("-%x %x\n", dest, src);
		memcpy(dest, src, w);
		dest+=4096;
		src+=w;
	}
	return src;
}

static void testcluster_vidioc_querycap(int fd)
{
	__u32 expectedcaps;
	int request = VIDIOC_QUERYCAP;
	int rv;
	struct v4l2_capability cap;
	__u32 *ap;

	TEST_INFO("Issue VIDIOC_QUERYCAP ioctl");
	rv = ioctl(fd, request, &cap);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	TEST_PRINT("QUERYCAP.capabilities = 0x%x", cap.capabilities);
	TEST_PRINT("QUERYCAP.driver = %s", cap.driver); 
	TEST_PRINT("QUERYCAP.card = %s", cap.card); 
	TEST_PRINT("QUERYCAP.bus_info = %s", cap.bus_info);
	TEST_PRINT("QUERYCAP.version = 0x%x", cap.version);
	ap = cap.reserved;
	TEST_PRINT("QUERYCAP.reserved = %1x %1x %1x %1x",
			   ap[0], ap[1], ap[2], ap[3]);

	TEST_INFO("Validate QUERYCAP.capabilities");
	expectedcaps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT;
	TEST_PRINT("capabilities, expect 0x%x", expectedcaps); 
	TEST_ASSERT(expectedcaps == cap.capabilities);
}

static void tutil_free_session(struct test_v4l2_session *sesh)
{
	if (!sesh)
		return;
	if (sesh->lens)
		free(sesh->lens);
	if (sesh->bufs)
		free(sesh->bufs);
	free(sesh);
}

/*
 * Note the signature of the function takes a 32bit color so we can use function
 * pointer for different color spaces
 */
static void tutil_clear_mmaped_region_rgb16(
					void *pa, unsigned int colorin, struct v4l2_pix_format* pix)
{
	__u16 color = (__u16)colorin;
	int w=pix->width, h=pix->height, stride=pix->bytesperline;
	__u8 *bstart = (__u8*)pa;
	__u16 *ptr;
	
	int r, c;
	for (r=0; r<h; r++) {
		ptr = (__u16*)(bstart + (r * stride));
		for (c=0; c<w; c++)
			ptr[c]=color;
	}
}

typedef void (*pfnClearFn)(void*, unsigned int, struct v4l2_pix_format*);

static pfnClearFn tutil_get_bpp_clear_fn(__u32 pixelformat)
{
	switch(pixelformat) {
	case V4L2_PIX_FMT_RGB565:
		return &tutil_clear_mmaped_region_rgb16;
	default:
		return NULL;
	}
}

static void tutil_print_timestamp(
	struct timeval *deltatv,
	struct timeval *curtv,
	struct timeval *oldtv,
	int *avg_cnt,
	double *deltatv_avg,
	double *peaktv,
	double *worsttv
)
{
	timeval_subtract(deltatv, curtv, oldtv);
	if ((oldtv->tv_sec == 0) && (oldtv->tv_usec == 0))
		*deltatv_avg = 0;
	else  {
		double dur = (double)deltatv->tv_sec +
			((double)deltatv->tv_usec / 1000000);
		*deltatv_avg += dur;
		*avg_cnt = *avg_cnt + 1;
		if ((dur != 0) && (dur < *peaktv))
			*peaktv = dur;
		if (dur > *worsttv)
			*worsttv = dur;
	}

	TEST_PRINT("TIMESTAMP %ld.%ld (delta %06f, avg = %f, "
			"peak = %f, worst = %f) \n",
			curtv->tv_sec, curtv->tv_usec, 
			(float)deltatv->tv_sec + ((float)deltatv->tv_usec / 1000000),
			*avg_cnt ? *deltatv_avg / *avg_cnt : 0, *peaktv, *worsttv);

	if (*avg_cnt > 100) {
		*avg_cnt = 0;
		*deltatv_avg = 0;
	}
	*oldtv = *curtv;
}


static void testcluster_vidioc_log_status(int fd)
{
	int request = VIDIOC_LOG_STATUS;
	int rv;

	TEST_INFO("Issue VIDIOC_STATUS ioctl");
	rv = ioctl(fd, request);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);
}

static void testcluster_vidioc_enum_fmt_vid_out(int fd)
{
	int request = VIDIOC_ENUM_FMT;
	struct v4l2_fmtdesc fmtdesc;
	int rv;
	int idx, maxidx;
	__u32 supported_formats[] = {
		V4L2_PIX_FMT_RGB565,
		V4L2_PIX_FMT_RGB32,
		V4L2_PIX_FMT_YUYV,
		V4L2_PIX_FMT_UYVY,
		V4L2_PIX_FMT_NV12,
		0 };

	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	TEST_INFO("Issue VIDIOC_ENUM_FMT ioctl");
	rv = ioctl(fd, request, &fmtdesc);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	TEST_PRINT("List all formats from VIDIOC_ENUM_FMT ioctl");
	for (idx=0; rv == 0; idx++) {
		fmtdesc.index = idx;
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		rv = ioctl(fd, request, &fmtdesc);
		TEST_PRINT("rv = %d", rv); 
		if (rv == 0) {
			TEST_PRINT("ENUM_FMT flags 0x%x", fmtdesc.flags);
			TEST_PRINT("ENUM_FMT description %s", fmtdesc.description);
			tutil_print_fourcc("ENUM_FMT pixelformat", fmtdesc.pixelformat);
		} else {
			TEST_INFO("validate out of bounds enum for ENUM_FMT");
			TEST_RESULT(errno);
			TEST_ASSERT(errno == EINVAL);
		}
	}

	// Validation
	maxidx = idx - 1;
	for (idx=0; idx < maxidx; idx++) {
		__u32* sfp = supported_formats;
		TEST_INFO2("Validate supported pixelformats VIDIOC_ENUM_FMT ioctl",
				  idx);
		fmtdesc.index = idx;
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		rv = ioctl(fd, request, &fmtdesc);
		if (rv != 0) {
			// Unexpected failure
			TEST_ASSERT(rv == 0);
			continue;
		}
		tutil_print_fourcc("Attempt to validate pixelformat",
							fmtdesc.pixelformat);

		// Check supported list
		while (*sfp != 0) {
			if (fmtdesc.pixelformat == *sfp)
				break;
			sfp++;
		}
		TEST_RESULT(*sfp);
		if (!(TEST_ASSERT(fmtdesc.pixelformat == *sfp))) {
			tutil_print_fourcc("This pixelformat is not expected by driver:",
								fmtdesc.pixelformat);
		}
	}
}

static void testcluster_vidioc_g_fmt_vid_out(int fd)
{
	int request = VIDIOC_G_FMT;
	int rv;
	struct v4l2_format fmt;

	TEST_INFO("Issue VIDIOC_G_FMT ioctl");
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rv = ioctl(fd, request, &fmt);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);
	
	TEST_PRINT("Dumping rcvd v4l2_pix_format");
	tutil_print_v4l2_pix_format("VIDIOC_G_FMT fmt", &fmt.fmt.pix);
}

/*
 * Test the default properties of the V4L2 GFX driver by setting
 * some image dimensions which are always expected and nothing
 * else
 */
static void testcluster_vidioc_try_fmt_vid_out_default(int fd)
{
	int request = VIDIOC_TRY_FMT;
	int rv, defidx;
	struct v4l2_format fmt;
	bzero(&fmt, sizeof (struct v4l2_format));

	TEST_INFO("Issue VIDIOC_TRY_FMT ioctl");
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = 256;
	fmt.fmt.pix.height = 256;
	rv = ioctl(fd, request, &fmt);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	TEST_PRINT("Dumping rcvd v4l2_pix_format");
	tutil_print_v4l2_pix_format("VIDIOC_TRY_FMT fmt", &fmt.fmt.pix);

	// Validation
	defidx = 0;	// set subtest index
	TEST_INFO2("Validate VIDIOC_TRY_FMT ioctl defaults", defidx);
	TEST_RESULT(fmt.fmt.pix.pixelformat);
	tutil_print_fourcc("default pixelformat, expect ", V4L2_PIX_FMT_RGB565);
	TEST_ASSERT(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565);

	defidx++;
	TEST_INFO2("Validate VIDIOC_TRY_FMT ioctl defaults", defidx);
	TEST_RESULT(fmt.fmt.pix.colorspace);
	TEST_PRINT("default colorspace, expect 0x%x", V4L2_COLORSPACE_SRGB);
	TEST_ASSERT(fmt.fmt.pix.colorspace == V4L2_COLORSPACE_SRGB);

	// XXX TODO validate stride and image size calculations
	// XXX TODO height / width obtained - could be modified
}

/*
 * Pre-req: fmt argument must have been filled out by TRY_FMT
 */
static void testcluster_vidioc_s_fmt_vid_out(
										int fd, struct v4l2_format* fmt)
										//int fd, struct v4l2_pix_format* pix)
{
	int request;
	int rv;
//	struct v4l2_format fmt;
//	bzero(&fmt, sizeof (struct v4l2_format));
//	bzero(pix, sizeof (struct v4l2_pix_format));

//	TEST_INFO("Prepare future VIDIOC_S_FMT with VIDIOC_TRY_FMT ioctl");
//	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
//	fmt.fmt.pix.width = 256;
//	fmt.fmt.pix.height = 256;
//	fmt.fmt.pix.pixel - XXX TODO - see try etc
//	rv = ioctl(fd, request, &fmt);
//	TEST_RESULT(rv);
//	TEST_ASSERT(rv == 0);

	TEST_INFO("Issue VIDIOC_S_FMT ioctl");
	request = VIDIOC_S_FMT;
	rv = ioctl(fd, request, fmt);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

//	*pix = fmt.fmt.pix;
}

static int testcluster_vidioc_reqbufs_vid_out(int fd)
{
	int request = VIDIOC_REQBUFS;
	int rv;
	struct v4l2_requestbuffers rbs;

	TEST_INFO("Issue VIDIOC_REQBUFS ioctl");
	rbs.memory = V4L2_MEMORY_MMAP;
	rbs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rbs.count = 3;
//	rbs.count = 10;
	rv = ioctl(fd, request, &rbs);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);
	TEST_PRINT("buffers returned = %d", rbs.count); 
	return rv == 0 ? rbs.count : 0;
}

static void testcluster_vidioc_querybuf_vid_out(int fd, int bcount)
{
	struct v4l2_buffer vbuf;
	int idx;

	if (bcount <= 0) {
		TEST_PRINT("Skipping VIDIOC_QUERYBUF test, buffer count is invalid");
		return;
	}

	for (idx = 0; idx < bcount; idx++) {
		(void)tutil_vidioc_querybuf(fd, idx, &vbuf);
		TEST_PRINT("Dumping rcvd v4l2_buffer"); 
		tutil_print_v4l2_buffer("VIDIOC_QUERYBUF", &vbuf);
	}
	return;
}

/*
 * @pre VIDIOC_S_FMT with usuable parameters
 * @pre VIDIOC_REQBUFS 
 */
static struct test_v4l2_session* testcluster_vidioc_mmap(
							int fd, int bcount, struct v4l2_pix_format* pix)
{
	struct test_v4l2_session *sesh;
	struct v4l2_buffer vbuf;
	char **bufs;
	int *lens;
	int idx, rv, failures=0;
	int w=pix->width, h=pix->height, stride=pix->bytesperline;
	pfnClearFn cleanfn;

	TEST_PRINT("Starting mmap usage testing");

	if (bcount == 0) {
		TEST_PRINT("No buffers to memory map - skip");
		return NULL;
	}

	bufs = calloc(bcount, sizeof(char*));
	lens = calloc(bcount, sizeof(int));
	if (!bufs || !lens) {
		TEST_PRINT("Memory allocation unexpectedly failed @line %d", __LINE__);
		goto end;
	}

	for (idx = 0; idx < bcount; idx++) {

		rv  = tutil_vidioc_querybuf(fd, idx, &vbuf);
		if (rv) {
			TEST_PRINT("Error from querybuf on buf idx %d", idx);
			break;
		}

		if (vbuf.flags == V4L2_BUF_FLAG_MAPPED) {
			TEST_PRINT("Buffer already mapped %d", idx);
			break;
		}

		TEST_INFO2("MMAP v4l2 buffer", idx);
		lens[idx] = vbuf.length;
		bufs[idx] = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
				fd, vbuf.m.offset);
		TEST_RESULT((int)(bufs[idx]));
		if (!TEST_ASSERT(bufs[idx] != MAP_FAILED)) {
			failures++;
		}
	}

	if (failures)
		goto end;

	/* XXX Note that this 'touching' is ignorant of bpp */
	for (idx = 0; idx < bcount; idx++) {
		char *ptr;
		int j;

		TEST_PRINT("buf addr 0x%x len %d", (int)bufs[idx], lens[idx]);
		TEST_PRINT("image dims (w = %d, h = %d, stride = %d)", w, h, stride);

		TEST_PRINT("Next test location %s:@line %d", __FILE__, __LINE__);
		TEST_INFO2("Touch MMAP'd memory 'in the right place'", idx);
		// A data abort could occur in this tests so flush output
		fflush(stdout);

		// Write to the buffers last row
		ptr = bufs[idx];
		ptr += (h-1) * stride;
		for (j=0; j<w; j++)
			ptr[j]='@';

		fflush(stdout);
		TEST_ASSERT(1);
	}

	// Read the last row just to make sure
	for (idx = 0; idx < bcount; idx++) {
		char *ptr;
		int j;
		char *readbuf;

		readbuf = calloc(w+1, sizeof(char));
		ptr = bufs[idx];
		ptr += (h-1) * stride;
		for (j=0; j<w; j++)
			readbuf[j] = ptr[j];
		readbuf[w] = '\0';
		// OPTIONAL printf("%s\n", readbuf); 
	}

	// We may get a data abort so print location details here
	TEST_PRINT("Next test location %s:@line %d", __FILE__, __LINE__);

	/*
	 * Write zero to the image memory making sure that we observe any
	 * stride limitations
	 */
	TEST_INFO("Zero image memory regions");

	cleanfn = tutil_get_bpp_clear_fn(pix->pixelformat);
	if (cleanfn) {
		fflush(stdout);

		for (idx = 0; idx < bcount; idx++) {
			cleanfn(bufs[idx], 0xffffffff, pix);

		}
	} else {
		TEST_PRINT("Skipping clearing the memory region");
	}
	TEST_ASSERT(1);
	// TODO - could also check the maths on the size of the memory mapped
	//		  region vs the stride / height inputs

end:
	/*
	 * After mmaping the buffers we have set up everything we need to
	 * start streaming to the V4L2 device.
	 *
	 * We will return the collated information via a test_v4l2_session
	 * object
	 */
	sesh = calloc(1, sizeof(struct test_v4l2_session)); assert(sesh);
	sesh->fd = fd;
	sesh->bcount = bcount;
	sesh->pix = *pix;
	sesh->bufs = calloc(bcount, sizeof(char*)); assert(sesh->bufs);
	sesh->lens = calloc(bcount, sizeof(int)); assert(sesh->lens);
	for (idx=0; idx<bcount; idx++) {
		sesh->bufs[idx] = bufs[idx];
		sesh->lens[idx] = lens[idx];
	}

	if (bufs)
		free(bufs);
	if (lens)
		free(lens);
	return sesh;
}

static void testcluster_v4l2_gfx_private_ioctls(int fd)
{
    int request = V4L2_GFX_IOC_ACQ;
    int rv;
	struct v4l2_gfx_buf_params parms;

    TEST_INFO("Issue V4L2_GFX_IOC_ACQ ioctl");
    rv = ioctl(fd, request, &parms);
    TEST_RESULT(rv);
    TEST_ASSERT(rv == 0);

	// XXX TODO REL ioctl
}

static void testcluster_stream(struct test_v4l2_session *sesh)
{
	int idx, rv;
	struct v4l2_buffer vbuf;
	enum v4l2_buf_type type;
	int fd = sesh->fd, bcount = sesh->bcount;
	bzero(&vbuf, sizeof(struct v4l2_buffer));
	
	/*
	 * Stream on should only work if we have queued some buffers
	 */
	TEST_INFO("VIDIOC_STREAMON ioctl before QBUF (should fail)");
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rv = ioctl(fd, VIDIOC_STREAMON, &type);
	TEST_RESULT(rv);
	TEST_ASSERT(rv != 0);

	for (idx = 0; idx < bcount; idx++) {
		TEST_INFO2("Issue VIDIOC_QBUF ioctl", idx);
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vbuf.index = idx;
		rv = ioctl(fd, VIDIOC_QBUF, &vbuf);
		TEST_RESULT(rv);
		TEST_ASSERT(rv == 0);
	}

	/*
	 * Streaming will now work, we hope
	 */
	TEST_INFO("Issue VIDIOC_STREAMON ioctl");
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rv = ioctl(fd, VIDIOC_STREAMON, &type);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	for (idx = 0; idx < bcount; idx++) {
		TEST_INFO2("Issue VIDIOC_DQBUF ioctl", idx);
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vbuf.index = idx;
		rv = ioctl(fd, VIDIOC_DQBUF, &vbuf);
		TEST_RESULT(rv);
		TEST_ASSERT(rv == 0);

		TEST_PRINT("TIMESTAMP %ld.%06ld\n",
			vbuf.timestamp.tv_sec, vbuf.timestamp.tv_usec);
	}

}

static void testcluster_stream2(
				struct test_v4l2_session *sesh, struct v4l2_pix_format *pix)
{
	int idx, rv;
	struct v4l2_buffer vbuf;
	__u16 color; // XXX bpp fixed
	enum v4l2_buf_type type;
	int fd = sesh->fd, bcount = sesh->bcount;
	bzero(&vbuf, sizeof(struct v4l2_buffer));
	
//	color = 0xf800;	// red
	color = 0x07c0;		// green
	for (idx = 0; idx < bcount; idx++) {
		tutil_clear_mmaped_region_rgb16(sesh->bufs[idx], color, pix); // XXX bpp fixed...
		if (color == 0xf800)
			color = 0x07c0;		// green
		else if (color == 0x07c0)
			color = 0xffc0;		// yellow
		else if (color == 0xffc0)
			color = 0xf800;
		
		TEST_INFO2("Issue VIDIOC_QBUF ioctl", idx);
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vbuf.index = idx;
		rv = ioctl(fd, VIDIOC_QBUF, &vbuf);
		TEST_RESULT(rv);
		TEST_ASSERT(rv == 0);
	}

	/*
	 * Streaming will now work, we hope
	 */
	TEST_INFO("Issue VIDIOC_STREAMON ioctl");
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rv = ioctl(fd, VIDIOC_STREAMON, &type);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	color = 0xf;	// XXX bpp fixed...
	while(1) {
		for (idx = 0; idx < bcount; idx++) {

			TEST_INFO2("Issue VIDIOC_DQBUF ioctl", idx);
			vbuf.memory = V4L2_MEMORY_MMAP;
			vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			vbuf.index = idx;
			rv = ioctl(fd, VIDIOC_DQBUF, &vbuf);
			TEST_RESULT(rv);
			TEST_ASSERT(rv == 0);

			TEST_PRINT("TIMESTAMP %ld.%06ld\n",
					vbuf.timestamp.tv_sec, vbuf.timestamp.tv_usec);

			if (idx == 0) {
				tutil_clear_mmaped_region_rgb16(sesh->bufs[idx], color, pix);
				color = color << 2;
				if (color == 0)
					color = 0xf;
			}

			TEST_INFO2("Issue VIDIOC_QBUF ioctl", idx);
			vbuf.memory = V4L2_MEMORY_MMAP;
			vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			vbuf.index = idx;
			rv = ioctl(fd, VIDIOC_QBUF, &vbuf);
			TEST_RESULT(rv);
			TEST_ASSERT(rv == 0);

		}
	}
}

static void testcluster_stream_file(
				struct test_v4l2_session *sesh, int docopy, const char *fname)
{
	int idx, rv;
	struct v4l2_buffer vbuf;
	enum v4l2_buf_type type;
	int fd = sesh->fd, bcount = sesh->bcount;
	char *fptr, *fptr_orig;
	unsigned long framecnt, curframe;
	struct timeval oldtv;
	double deltatv_avg = 0;
	double peaktv = 999;
	double worsttv = 0;
	int avg_cnt=0;
	int h=sesh->pix.height, w=sesh->pix.width;
	int framesz;

	bzero(&vbuf, sizeof(struct v4l2_buffer));
	bzero(&oldtv, sizeof(oldtv));
	
	TEST_PRINT("Opening file............\n");
	fptr_orig = tutil_open_video(&framecnt, h, w, fname);

	framesz = h * w * 3 / 2;	// nv12 ...
	curframe=0;
//	curframe=3000;
	
	for (idx = 0; idx < bcount; idx++) {

		fptr = fptr_orig + (curframe * framesz);
		tutil_copy_nv12frame_to_tiler(sesh->bufs[idx], fptr, h, w);

		TEST_INFO2("Issue VIDIOC_QBUF ioctl", idx);
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vbuf.index = idx;
		rv = ioctl(fd, VIDIOC_QBUF, &vbuf);
		TEST_RESULT(rv);
		TEST_ASSERT(rv == 0);

		curframe = (curframe == (framecnt-1)) ? 0 : curframe + 1;
	}

	/*
	 * Streaming will now work, we hope
	 */
	TEST_INFO("Issue VIDIOC_STREAMON ioctl");
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rv = ioctl(fd, VIDIOC_STREAMON, &type);
	TEST_RESULT(rv);
	TEST_ASSERT(rv == 0);

	while(1) {

		for (idx = 0; idx < bcount; idx++) {
			struct timeval deltatv;

			TEST_INFO2("Issue VIDIOC_DQBUF ioctl", idx);
			vbuf.memory = V4L2_MEMORY_MMAP;
			vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			vbuf.index = idx;
			rv = ioctl(fd, VIDIOC_DQBUF, &vbuf);
			TEST_RESULT(rv);
			TEST_ASSERT(rv == 0);

			tutil_print_timestamp(
								&deltatv, &vbuf.timestamp, &oldtv,
								&avg_cnt, &deltatv_avg, &peaktv, &worsttv);

			fptr = fptr_orig + (curframe * framesz);
			if (docopy) 
				tutil_copy_nv12frame_to_tiler(sesh->bufs[idx], fptr, h, w);

			TEST_INFO2("Issue VIDIOC_QBUF ioctl", idx);
			vbuf.memory = V4L2_MEMORY_MMAP;
			vbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			vbuf.index = idx;
			rv = ioctl(fd, VIDIOC_QBUF, &vbuf);
			TEST_RESULT(rv);
			TEST_ASSERT(rv == 0);

			curframe = (curframe == (framecnt-1)) ? 0 : curframe + 1;
		}
	}
}

static void set_param_fixed_name(void)
{
	char wt[5], ht[5];
	snprintf(wt, 5, "%d", param_width);
	snprintf(ht, 5, "%d", param_height);
	snprintf(param_fixed_name, 15, "%sx%s.nv12", wt, ht);
}

static int parse_args(int argc, char **argv)
{
	int c;

	if (argc == 1)
		return 0;

	while ((c = getopt(argc, argv, "f:w:h:")) != -1) {

		switch (c)
		{
		case 'f':
			param_filename = optarg;
			break;
		case 'w':
			param_width = atoi(optarg);
			break;
		case 'h':
			param_height = atoi(optarg);
			break;
		default:
			printf("Unknown arg: %s\n", optarg);
			break;
		}
	}
	if (param_height == 0 || param_width == 0) {
		fprintf(stderr, "Both width (-w) and height (-h) parameters must be given\n");
		return -1;
	}

	if (!param_filename) {
		if (param_height && param_width) {
			set_param_fixed_name();
			param_opts |= PARAM_FIXED_NAME;
		} else {
			return -1;
		}
	}
	return 1;
}

#if defined(SUPPORT_ANDROID_PLATFORM)
int main(void)
#else
int main(int argc, char **argv)
#endif
{
	int fd;
	int bcount;
//	struct v4l2_pix_format pix;
	struct test_v4l2_session *sesh;
	struct v4l2_format fmt;
	int interactive;

#if defined(SUPPORT_ANDROID_PLATFORM)
	/* For some reason when built for Android, optarg will blow up. Using
	   readelf it seems that something odd is happening with the optarg
	   symbol - perhaps we have some compiler flags causing us issues.
	
 	   For now, force interactive with fixed parameters
	   This predicates a raw nv12 file being present.
	 */
	interactive = 1;
	param_width = 320;
	param_height = 240;
	set_param_fixed_name();
	param_opts |= PARAM_FIXED_NAME;
#else
	interactive = parse_args(argc, argv);
	if (interactive < 0) {
		fprintf(stderr, "Exiting, can't understand cmd line arguments\n");
		return EINVAL;
	}
#endif

	if (!interactive) {
		/*
		 * If not in interactive mode run just the unit tests, we need to set
		 * some defaults
		 */
		param_width = 1920;
		param_height = 1080;
	}

	//TEST_INIT(TEST_FLAG_DEFAULT);
	TEST_INIT(TEST_FLAG_PAUSE_ON_ERROR);// Will stall test if an error occurs

	fd = tutil_driver_open("Initial driver open");

	tutil_driver_close("Test driver close", fd);

	fd = tutil_driver_open("Driver re-open");

	testcluster_vidioc_querycap(fd);

	testcluster_vidioc_log_status(fd);

	testcluster_vidioc_enum_fmt_vid_out(fd);

	/*
	 * Note that when we get a format at this point no valid format data
	 * will be present. Only after we issue the VIDIOC_S_FMT will we get
	 * something
	 */
	testcluster_vidioc_g_fmt_vid_out(fd);

	testcluster_vidioc_try_fmt_vid_out_default(fd);

	/* Try V4L2 with RGB16 */
	bzero(&fmt, sizeof (struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
//  fmt.fmt.pix.width = 256; fmt.fmt.pix.height = 256;
//	fmt.fmt.pix.width = 320; fmt.fmt.pix.height = 240;
//	fmt.fmt.pix.width = 1920; fmt.fmt.pix.height = 816;
//	fmt.fmt.pix.width = 852; fmt.fmt.pix.height = 362;
	fmt.fmt.pix.width = param_width; fmt.fmt.pix.height = param_height;

	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

	tutil_vidioc_try_fmt(fd, &fmt);
	
	testcluster_vidioc_s_fmt_vid_out(fd, &fmt);

	testcluster_vidioc_g_fmt_vid_out(fd);

	bcount = testcluster_vidioc_reqbufs_vid_out(fd);

	testcluster_vidioc_querybuf_vid_out(fd, bcount);

	sesh = testcluster_vidioc_mmap(fd, bcount, &fmt.fmt.pix);
	if (!sesh)
		goto end;
	
	//testcluster_stream(sesh);

	//testcluster_v4l2_gfx_private_ioctls(fd);

	//testcluster_stream2(sesh, &fmt.fmt.pix);

	if (interactive) {
		char fname[255];
		char *fp;
		fp = fname;
		if (param_opts & PARAM_FIXED_NAME) {
			int l = strlen(fixed_name_path) + strlen(param_fixed_name);
			if (l < 255) {
				strcpy(fname, fixed_name_path);
				strcat(fname, param_fixed_name);
			} else {
				fprintf(stderr,
					"Abend: Invalid path: line %s:%d\n", __FILE__, __LINE__);
				exit(-99);
			}
		}
		else {
			strncpy(fname, param_filename, 255);
		}
		//	testcluster_stream_file(sesh, 0, fp);
		testcluster_stream_file(sesh, 1, fp);
	}

	tutil_free_session(sesh);
end:
	tutil_driver_close("Final driver close", fd);
	tutil_report_results();
	return 0;
}
