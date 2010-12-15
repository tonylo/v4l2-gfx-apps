
#include <stdio.h>
#include <time.h>

#include "misc.h"

#ifdef SUPPORT_ANDROID_PLATFORM
void timestamp(void)
{
        /* Not needed on Android this can be obtained from logcat */
}
#else
void timestamp(void)
{
        time_t ltime;
        struct tm *t;

        ltime=time(NULL);
        t=localtime(&ltime);
        printf("%02d:%02d:%02d: ", t->tm_hour, t->tm_min, t->tm_sec);
}
#endif

