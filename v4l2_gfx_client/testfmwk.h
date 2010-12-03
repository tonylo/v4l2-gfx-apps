/*!****************************************************************************
@File           testfmwk.h

@Title          V4L2 GFX client - test framework header

@Author         Texas Instruments

@Date           2010/07/29

@Copyright      Copyright (C) 2010 Texas Instruments.

@Platform       Android/Linux

@Description    Test API code 

******************************************************************************/

/******************************************************************************/
/* Private functions / implementations details                                */
/******************************************************************************/
extern int impl_test_platform_input_open(void);
extern void impl_test_pause(void);
extern int impl_test_assert(int status, char *fname, int lineno);
extern void impl_test_setinfo(char *tst_desc, int tst_no);
extern void impl_test_casedata(int *testnump, int *testpassp, int *testfailp);
extern void impl_test_result(int value);
extern void impl_test_pause_on_failure(int val);

#define TESTKW ":TEST:  "
#define FAILKW ":FAILED:"
#define PASSKW ":PASS:  "
#define DUMPKW ":DUMP:  "
#define DATAKW ":DATA:  "

#define LOG(fmt, arg...) \
    printf(fmt, ## arg)

#define IMPL_TEST_ASSERT(expression, fname, lineno) \
	impl_test_assert((expression), fname, lineno)

/******************************************************************************/
/* Public functions / Test APIs                                               */
/******************************************************************************/
#define TEST_INFO(desc) \
	impl_test_setinfo(desc, -1)
#define TEST_INFO2(desc, idx) \
	impl_test_setinfo(desc, idx)
#define TEST_ASSERT(expression) \
	IMPL_TEST_ASSERT((expression), __FILE__, __LINE__)
#define TEST_CASEDATA(testnump, testpassp, testfailedp) \
	impl_test_casedata(testnump, testpassp, testfailedp)
#define TEST_RESULT(value) \
	impl_test_result(value)
#define TEST_PRINT(fmt, arg...) LOG("%s" fmt "\n", DATAKW, ## arg)

// Arguments to TEST_INIT()
#define TEST_FLAG_DEFAULT	0
#define TEST_FLAG_PAUSE_ON_ERROR	1

#define TEST_INIT(val) { \
	impl_test_platform_input_open(); \
	impl_test_pause_on_failure(val); \
	}

