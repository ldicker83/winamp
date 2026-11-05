#ifndef BENCHMARKH
#define BENCHMARKH


#if defined(_WIN32)
static uint64_t Benchmark()
{
	return GetTickCount64();
}

#elif defined(__ANDROID__)
#include <time.h>

// Make sure to divide by 1000000 (1 million) to get results in Milliseconds, as they are returned in nanoseconds
static uint64_t Benchmark()
{
	struct timespec ts;
	uint64_t count;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
	count=(uint64_t)ts.tv_sec*1000000000ULL + (uint64_t)ts.tv_nsec;
	return count;
}

#endif

#endif