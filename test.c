#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

int main() {
	struct sched_param params;
	int done = 0;
	struct timeval tv;
	time_t startsec;
	unsigned long int loops;
	int outer_loops;

	params.sched_priority = 1;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);

	gettimeofday(&tv,0);
	startsec = tv.tv_sec;
#if 0
	while(!done) {
		gettimeofday(&tv, 0);
		if(tv.tv_sec - startsec > 10)
			done = 1;		
	}
#endif
	for (outer_loops = 0; outer_loops < 333; ++outer_loops) {
		for (loops = 0; loops < 1000000000; loops++) {done += 1;}
	}
	exit (EXIT_SUCCESS);
}
