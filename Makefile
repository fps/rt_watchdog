PREFIX = /usr/local


COMPILE_FLAGS = -Wall -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS

LINK_FLAGS    = -lpthread -lm

TARGET  = rt_watchdog

STUFF   = rt_watchdog ringbuffer
GLADE   = $(TARGET).glade
OBJECTS = $(STUFF:%=%.o)
SOURCES = $(STUFF:%=%.c)
HEADERS = $(STUFF:%=%.h)




all: $(TARGET) test_rt

test_rt: test.c
	$(CC) -o test_rt test.c -lpthread -Wall -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS


$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LINK_FLAGS)

$(OBJECTS): %.o: %.c
	$(CC) -c $< $(COMPILE_FLAGS)


.PHONY: clean
clean:
	rm -f $(TARGET) *~ $(PLUGIN_OBJECTS) $(OBJECTS) core* *.lst test_rt

.PHONY: install
install: rt_watchdog unfifo_stuff.sh
	install -D rt_watchdog $(PREFIX)/bin/rt_watchdog
	install -D unfifo_stuff.sh $(PREFIX)/bin/unfifo_stuff.sh
