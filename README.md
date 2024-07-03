A very simple watchdog that consists of two threads:

- A high priority "waiter" thread that periodically waits a certain time (default: 5 seconds) for being woken up by the waker
- A low priority "waker" thread that periodically (default: once per second) wakes the waiter

If the waiter is not woken up in due time the command specified as the `--command` parameter is run. Per default this command changes the scheduling class of all threads in the system with priorities between 55 and 85 to SCHED_OTHER

