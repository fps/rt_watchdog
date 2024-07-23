A very simple watchdog that consists of two threads:

- A high priority "waiter" thread that periodically waits a certain time (default: 5 seconds) for being woken up by the waker
- A low priority "waker" thread that periodically (default: once per second) wakes the waiter

If the waiter is not woken up in due time the command specified as the `--command` parameter is run. Per default this command changes the scheduling class of all threads in the system with priorities between 55 and 85 to SCHED_OTHER

So as long as you put your potentially locking up `SCHED_FIFO` programs to a priority in between these two the `rt_watchdog` might save your day

# Requirements

Just some core utilities like `chrt`, `awk`, `grep` and `bash`, and additionally a C++ compiler, `gnumak` and `boost-program-options`

# Interface:

```console
Allowed options:
  -h [ --help ]                         Show this help text
  --command arg (=bash -c $'echo rt_watchdog timed out. Changing thread scheduling classes to SCHED_OTHER | wall; for n in $(ps -eL -o pid=,rtprio= | grep -v - | awk \'$2 >= 55\' | awk \'$2 <= 85\' | awk \'{print $1}\'); do chrt -o -p 0 $n; done')
                                        The command to run in case of a timeout
  --waker-period arg (=1)               The waker period (seconds)
  --waiter-timeout arg (=5)             The waiter timeout (seconds)
  --waker-priority arg (=0)             The waker priority (SCHED_FIFO). If set
                                        to 0 SCHED_OTHER is used
  --waiter-priority arg (=95)           The waiter priority (SCHED_FIFO)
```
