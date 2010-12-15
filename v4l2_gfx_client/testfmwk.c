/*!****************************************************************************
@File           testfmwk.c

@Title          V4L2 GFX client - test framework

@Author         Texas Instruments

@Date           2010/07/29

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    Test API code

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#include "testfmwk.h"

typedef struct impl_test_data {
        char tst_desc[256];
        int tst_no;
        int tst_cases;	// Number of test cases run
        int tst_passed;
        int tst_failed;
        int tst_have_result; // Reset to false by assert
        int tst_result;
        int tst_pause_on_failure;
} impl_test_data_t;

impl_test_data_t cur_impl_tdata;
struct termios g_test_termio, g_test_termio_orig;
int dev_fd;

int impl_test_platform_input_open(void)
{
#ifdef SUPPORT_ANDROID_PLATFORM
        /* open tty device for user control */
        if ((dev_fd = open("/dev/tty", O_RDWR|O_NDELAY)) == -1) {
                LOG("could not open tty device for user input\n");
                return ENODEV;
        }

        tcgetattr(dev_fd,&g_test_termio_orig);
        tcgetattr(dev_fd,&g_test_termio);
        cfmakeraw(&g_test_termio);
        g_test_termio.c_cc[VMIN] = 1;
        g_test_termio.c_cc[VTIME] = 0;

        if(tcsetattr(dev_fd,TCSANOW,&g_test_termio) == -1) {
                LOG("no user control\n");
        } else {
                LOG("tty connected....\n");
        }
#endif // SUPPORT_ANDROID_PLATFORM
        return 0;
}

void impl_test_pause(void)
{
        char c = 0;
        LOG("--- Type 'q' followed by <CR> to quit --- \n");
        LOG("--- Type 'c' followed by <CR> to continue --- \n");

        do {
                int ckb;
                if (read(dev_fd, &ckb, 1) == 1)
                        c = ckb;
                if (c == 'q')
                        exit(0);
        } while (c != 'c');
}

int impl_test_assert(int status, char *fname, int lineno)
{
        impl_test_data_t* tdata =  &cur_impl_tdata;
        char *fmt_failed = FAILKW " @(%s):%d\n";
        char *fmt_passed = PASSKW "\n";
        int passed = (status == 0) ? 0 : 1;

        tdata->tst_cases++;
        if (!passed)
                tdata->tst_failed++;
        else
                tdata->tst_passed++;

        if (!passed) {
                LOG(fmt_failed, fname, lineno);

                if (tdata->tst_have_result) {
                        char tra[4];
                        *((int*)tra)=tdata->tst_result;
                        LOG(":DUMP: result        = %d (0x%x) \"%c%c%c%c\"\n",
                            tdata->tst_result, tdata->tst_result, tra[0], tra[1], tra[2],
                            tra[3]);
                        LOG(":DUMP: current errno = %d \"%s\"\n", errno, strerror(errno));

                        if (tdata->tst_pause_on_failure)
                                impl_test_pause();
                }
        } else {
                LOG(fmt_passed, NULL);	// NULL added for ubuntu build...
        }
        tdata->tst_have_result=0;
        return passed;
}

void impl_test_setinfo(char *tst_desc, int tst_no)
{
        impl_test_data_t *tdata = &cur_impl_tdata;
        strncpy(tdata->tst_desc, tst_desc, 256);
        tdata->tst_no=tst_no;
        if (tst_no == -1) {
                LOG(TESTKW "%s\n", tdata->tst_desc);
        } else {
                LOG(TESTKW "%s [%d]\n", tdata->tst_desc, tdata->tst_no);
        }
}
void impl_test_casedata(int *testnump, int *testpassp, int *testfailp)
{
        impl_test_data_t *tdata = &cur_impl_tdata;
        *testnump = tdata->tst_cases;
        *testpassp = tdata->tst_passed;
        *testfailp = tdata->tst_failed;
}

void impl_test_result(int value)
{
        impl_test_data_t *tdata = &cur_impl_tdata;
        tdata->tst_result = value;
        tdata->tst_have_result = 1;
}

void impl_test_pause_on_failure(int val)
{
        impl_test_data_t *tdata = &cur_impl_tdata;
        tdata->tst_pause_on_failure = val ? 1 : 0;
}

