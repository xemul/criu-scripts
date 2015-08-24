OBJS=stopexec

all: $(OBJS)

stopexec: stopexec.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

.PHONY: clean

clean:
	rm -f $(OBJS)
