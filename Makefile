CFLAGS=-Wall -Wextra -MMD

SRC_DAEMON=main_daemon.c tcp.c uart.c
SRC_LEDS=ctrl_leds.c tcp.c

OBJ_DAEMON=$(SRC_DAEMON:.c=.o)
OBJ_LEDS=$(SRC_LEDS:.c=.o)

all: main_daemon ctrl_leds

main_daemon: $(OBJ_DAEMON)
	$(CC) -o$@ $^ $(LDFLAGS)

ctrl_leds: $(OBJ_LEDS)
	$(CC) -o$@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DAEMON) $(OBJ_LEDS) $(OBJ_DAEMON:.o=.d) $(OBJ_LEDS:.o=.d)

-include $(OBJ_DAEMON:.o=.d) $(OBJ_LEDS:.o=.d)
