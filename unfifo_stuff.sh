#!/bin/bash

# echo `date` > /tmp/unfifo_log
# echo $$ >> /tmp/unfifo_log

# we only change the scheduling policy of non kernel threads.
# which are those not having brackets in the "ps -eL -o pid,command" output
for i in $( ps -eLF | grep -v ' \[.*\]$' | grep -v PID | awk '{print $4}' );
do
	if [ "$i" != "$1" -a "$i" != "$2"  -a "$i" != "$$" ]
	then
		# echo $i >> /tmp/unfifo_log
		chrt -o -p 0 $i
	fi
done
