#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

#ifdef __NR_gettid
static pid_t gettid (void)
{
    return syscall(__NR_gettid);
}
#else
static pid_t gettid (void)
{
    return -ENOSYS;
}
#endif

//_syscall0(pid_t,gettid);

#include "ringbuffer.h"

/* how long to sleep between checks in the high prio thread */
#define SLEEPSECS     3

/* how long to sleep between writing "alive" messages to the ringbuffer 
   from the low prio thread */
#define LP_SLEEPSECS  1

/* the priority of the high prio thread */
#define PRIO         98

/* the priority of the low prio thread */
#define LP_PRIO       1

/* the ringbuffer used to transfer "alive" messages from low prio producer
   to high prio consumer */
jack_ringbuffer_t *rb;

pthread_t low_prio_thread;

/* the thread id's for the low prio and high prio threads.
   these get passed to the unfifo_stuff.sh script to make sure
   the watchdog doesn't repolicy itself to SCHED_OTHER */
pid_t lp_tid;
pid_t hp_tid;

void signalled(int signal) 
{

}

volatile int thread_finish;


/* this is the low prio thread. it simply writes to
   the ringbuffer to signal that it got to run, meaning it is still
   alive */
void *lp_thread_func(void *arg) {
	char data;
	struct timespec tv;

	lp_tid = gettid();
	
	/* syslog(LOG_INFO, "lp tid: %i", gettid()); */

	data = 0;
	while(!thread_finish) {
		/* we simply write stuff to the ringbuffer and go back to sleeping
		   we can ignore the return value, cause, when it's full, it's ok
		   the data doesn;t have any meaning. it just needs to be there 
		   running full shouldn't happen anyways */
		jack_ringbuffer_write(rb, &data, sizeof(data));

		/* then sleep a bit. but less than the watchdog high prio thread */
		tv.tv_sec = LP_SLEEPSECS;
		tv.tv_nsec = 0;
		// sleep(LP_SLEEPSECS);
		nanosleep(&tv, NULL);
	}
	return 0;
}

int main() 
{
	pid_t pid, sid;
	int done;
	struct sched_param params;
	char data;
	int err;
	int consumed;
	int count;
	struct timespec tv;
	char unfifo_cmd[1000];


	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then
	   we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);       

	/* Open any logs here */
	openlog("rt_watchdog", 0, LOG_DAEMON);
	syslog(LOG_INFO, "started");

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log any failure here */
		syslog(LOG_INFO, "setsid failed. exiting");
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
	if ((chdir("/")) < 0) {
		/* Log any failure here */
		syslog(LOG_INFO, "chdir failed. exiting");
		exit(EXIT_FAILURE);
	}


	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// syslog(LOG_INFO, "closed fd's");

	/* syslog(LOG_INFO, "hp tid: %i", gettid()); */
	hp_tid = gettid();

	/* not really nessecary, but wtf */
	mlockall(MCL_FUTURE);


	thread_finish = 0;
	done = 0;


	/* get ourself SCHED_FIFO with prio PRIO */
	params.sched_priority = PRIO;
	if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &params)) {
		syslog(LOG_INFO, "couldn't set realtime prio for main thread.. exiting..");
		exit(EXIT_FAILURE);	
	}
	// syslog(LOG_INFO, "prio set");
		
	/* create a ringbuffer for the low prio thread to signal it's alive */
	rb = jack_ringbuffer_create(1024);
	// syslog(LOG_INFO, "ringbuffer created");

	/* create low prio thread */
	err = pthread_create(&low_prio_thread, 0, lp_thread_func, 0);
	if(err) {
		syslog(LOG_INFO, "couldn't create low prio thread, exiting..");
		exit(EXIT_FAILURE);
	}	

	/* dirty. we really need to wait for the lp thread
	   to have written the lp_tid. (condition variable?) */
	sleep(1);

	// syslog(LOG_INFO, "created low prio thread");
	
	/* make the low prio thread sched_fifo */
	params.sched_priority = LP_PRIO;
	if (pthread_setschedparam(low_prio_thread, SCHED_FIFO, &params)) {
		syslog(LOG_INFO, "couldn't set realtime prio for lower prio thread.. exiting..");
		thread_finish = 1;
		pthread_join(low_prio_thread, NULL);

		exit(EXIT_FAILURE);	
	}
 

	/* this is the main loop. we somply check whether the low prio thread
	   got to run at all by looking into the ringbuffer. If it's empty,
	   the low prio thread is kinda dead */
	count = 0;
	while(!done) {
		tv.tv_sec = SLEEPSECS;
		tv.tv_nsec = 0;
		nanosleep(&tv, NULL);
		// sleep(SLEEPSECS); 
		count++;
		// syslog(LOG_INFO, "count %i", count);

		/* see if our little brother got to run */
		if (jack_ringbuffer_read_space(rb) == 0) {
			/* oh oh, it didn't */
			syslog(LOG_INFO, "low prio thread seems to be starved, taking measures...");

			/* we pass our own TID's to the script, so we are excluded from the 
			   scheduling policy change */
			sprintf(unfifo_cmd, "unfifo_stuff.sh %i %i", lp_tid, hp_tid);
			syslog(LOG_INFO, "unfifo command:");
			syslog(LOG_INFO, "%s", unfifo_cmd);

			if (system(unfifo_cmd) == -1) {
				syslog(LOG_INFO, "unfifo command failed");
			}
		} else {
			/* consume stuff from ringbuffer */
			consumed = 0;
			while (jack_ringbuffer_read_space(rb)) {
				consumed++;
				jack_ringbuffer_read(rb, &data, 1);
			}
			/* syslog(LOG_INFO, "lp alive: %i", consumed); */
		}
	}

	thread_finish = 1;	
	pthread_join(low_prio_thread, NULL);	

	syslog(LOG_INFO, "exiting");
	exit(EXIT_SUCCESS);
}
